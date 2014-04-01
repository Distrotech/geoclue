/* vim: set et ts=8 sw=8: */
/*
 * Copyright (C) 2014 Red Hat, Inc.
 *
 * Geoclue is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Geoclue is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Geoclue; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#include <stdlib.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <string.h>
#include "gclue-web-source.h"
#include "gclue-error.h"
#include "geocode-glib/geocode-location.h"

/**
 * SECTION:gclue-web-source
 * @short_description: Web-based geolocation
 * @include: gclue-glib/gclue-web-source.h
 *
 * Baseclass for all sources that solely use a web resource for geolocation.
 **/

struct _GClueWebSourcePrivate {
        SoupSession *soup_session;

        SoupMessage *query;

        gulong network_changed_id;

        guint64 last_submitted;

        gboolean network_available;
};

G_DEFINE_ABSTRACT_TYPE (GClueWebSource, gclue_web_source, GCLUE_TYPE_LOCATION_SOURCE)

static void
query_callback (SoupSession *session,
                SoupMessage *query,
                gpointer     user_data)
{
        GClueWebSource *web;
        GError *error = NULL;
        char *contents;
        char *str;
        GeocodeLocation *location;
        SoupURI *uri;

        if (query->status_code == SOUP_STATUS_CANCELLED)
                return;

        web = GCLUE_WEB_SOURCE (user_data);
        web->priv->query = NULL;

        if (query->status_code != SOUP_STATUS_OK) {
                g_warning ("Failed to query location: %s", query->reason_phrase);
		return;
	}

        contents = g_strndup (query->response_body->data, query->response_body->length);
        uri = soup_message_get_uri (query);
        str = soup_uri_to_string (uri, FALSE);
        g_debug ("Got following response from '%s':\n%s",
                 str,
                 contents);
        g_free (str);
        location = GCLUE_WEB_SOURCE_GET_CLASS (web)->parse_response (web,
                                                                     contents,
                                                                     &error);
        g_free (contents);
        if (error != NULL) {
                g_warning ("Failed to query location: %s", error->message);
                return;
        }

        gclue_location_source_set_location (GCLUE_LOCATION_SOURCE (web),
                                            location);
        g_object_unref (location);
}

static void
on_network_changed (GNetworkMonitor *monitor,
                    gboolean         available,
                    gpointer         user_data)
{
        GClueWebSource *web = GCLUE_WEB_SOURCE (user_data);
        GError *error = NULL;
        gboolean last_available = web->priv->network_available;

        web->priv->network_available = available;
        if (last_available == available)
                return; /* We already reacted to netork change */
        if (!available) {
                g_debug ("Network unavailable");
                return;
        }
        g_debug ("Network available");

        if (web->priv->query != NULL)
                return;

        web->priv->query = GCLUE_WEB_SOURCE_GET_CLASS (web)->create_query
                                        (web,
                                         &error);

        if (web->priv->query == NULL) {
                g_warning ("Failed to create query: %s", error->message);
                g_error_free (error);
                return;
        }

        soup_session_queue_message (web->priv->soup_session,
                                    web->priv->query,
                                    query_callback,
                                    web);
}

static void
gclue_web_source_finalize (GObject *gsource)
{
        GClueWebSourcePrivate *priv = GCLUE_WEB_SOURCE (gsource)->priv;

        if (priv->network_changed_id) {
                g_signal_handler_disconnect (g_network_monitor_get_default (),
                                             priv->network_changed_id);
                priv->network_changed_id = 0;
        }

        if (priv->query != NULL) {
                g_debug ("Cancelling query");
                soup_session_cancel_message (priv->soup_session,
                                             priv->query,
                                             SOUP_STATUS_CANCELLED);
                priv->query = NULL;
        }

        g_clear_object (&priv->soup_session);

        G_OBJECT_CLASS (gclue_web_source_parent_class)->finalize (gsource);
}

static void
gclue_web_source_constructed (GObject *object)
{
        GNetworkMonitor *monitor;
        GClueWebSourcePrivate *priv = GCLUE_WEB_SOURCE (object)->priv;

        priv->soup_session = soup_session_new_with_options
                        (SOUP_SESSION_REMOVE_FEATURE_BY_TYPE,
                         SOUP_TYPE_PROXY_RESOLVER_DEFAULT,
                         NULL);

        monitor = g_network_monitor_get_default ();
        priv->network_changed_id =
                g_signal_connect (monitor,
                                  "network-changed",
                                  G_CALLBACK (on_network_changed),
                                  object);

        if (g_network_monitor_get_network_available (monitor))
                on_network_changed (monitor, TRUE, object);
}

static void
gclue_web_source_class_init (GClueWebSourceClass *klass)
{
        GObjectClass *gsource_class = G_OBJECT_CLASS (klass);

        gsource_class->finalize = gclue_web_source_finalize;
        gsource_class->constructed = gclue_web_source_constructed;

        g_type_class_add_private (klass, sizeof (GClueWebSourcePrivate));
}

static void
gclue_web_source_init (GClueWebSource *web)
{
        web->priv = G_TYPE_INSTANCE_GET_PRIVATE ((web), GCLUE_TYPE_WEB_SOURCE, GClueWebSourcePrivate);
}

/**
 * gclue_web_source_refresh:
 * @source: a #GClueWebSource
 *
 * Causes @source to refresh location. Its meant to be used by subclasses if
 * they have reason to suspect location might have changed.
 **/
void
gclue_web_source_refresh (GClueWebSource *source)
{
        GNetworkMonitor *monitor;
        GClueWebSourcePrivate *priv;

        g_return_if_fail (GCLUE_IS_WEB_SOURCE (source));

        priv = source->priv;
        if (priv->query != NULL)
                return;

        monitor = g_network_monitor_get_default ();
        if (g_network_monitor_get_network_available (monitor))
                on_network_changed (monitor, TRUE, source);
}

static void
submit_query_callback (SoupSession *session,
                       SoupMessage *query,
                       gpointer     user_data)
{
        SoupURI *uri;

        uri = soup_message_get_uri (query);
        if (query->status_code != SOUP_STATUS_OK &&
            query->status_code != SOUP_STATUS_NO_CONTENT) {
                g_warning ("Failed to submit location data to '%s': %s",
                           soup_uri_to_string (uri, FALSE),
                           query->reason_phrase);
		return;
	}

        g_debug ("Successfully submitted location data to '%s'",
                 soup_uri_to_string (uri, FALSE));
}

#define SUBMISSION_ACCURACY_THRESHOLD 100
#define SUBMISSION_TIME_THRESHOLD     60  /* seconds */

static void
on_submit_source_location_notify (GObject    *source_object,
                                  GParamSpec *pspec,
                                  gpointer    user_data)
{
        GClueLocationSource *source = GCLUE_LOCATION_SOURCE (source_object);
        GClueWebSource *web = GCLUE_WEB_SOURCE (user_data);
        GNetworkMonitor *monitor;
        GeocodeLocation *location;
        SoupMessage *query;
        GError *error = NULL;

        location = gclue_location_source_get_location (source);
        if (location == NULL ||
            geocode_location_get_accuracy (location) >
            SUBMISSION_ACCURACY_THRESHOLD ||
            geocode_location_get_timestamp (location) <
            web->priv->last_submitted + SUBMISSION_TIME_THRESHOLD)
                return;

        web->priv->last_submitted = geocode_location_get_timestamp (location);

        monitor = g_network_monitor_get_default ();
        if (!g_network_monitor_get_network_available (monitor))
                return;

        query = GCLUE_WEB_SOURCE_GET_CLASS (web)->create_submit_query
                                        (web,
                                         location,
                                         &error);
        if (query == NULL) {
                if (error != NULL) {
                        g_warning ("Failed to create submission query: %s",
                                   error->message);
                        g_error_free (error);
                }

                return;
        }

        soup_session_queue_message (web->priv->soup_session,
                                    query,
                                    submit_query_callback,
                                    web);
}

/**
 * gclue_web_source_set_submit_source:
 * @source: a #GClueWebSource
 *
 * Use this function to provide a location source to @source that is used
 * for submitting location data to resource being used by @source. This will be
 * a #GClueModemGPS but we don't assume that here, in case we later add a
 * non-modem GPS source and would like to pass that instead.
 **/
void
gclue_web_source_set_submit_source (GClueWebSource      *web,
                                    GClueLocationSource *submit_source)
{
        /* Not implemented by subclass */
        if (GCLUE_WEB_SOURCE_GET_CLASS (web)->create_submit_query == NULL)
                return;

        g_signal_connect (G_OBJECT (submit_source),
                          "notify::location", 
                          G_CALLBACK (on_submit_source_location_notify),
                          web);

        on_submit_source_location_notify (G_OBJECT (submit_source), NULL, web);
}
