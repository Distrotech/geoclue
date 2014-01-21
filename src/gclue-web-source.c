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
#include "geocode-location.h"

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
};

static void
gclue_web_source_start (GClueLocationSource *source);
static void
gclue_web_source_stop (GClueLocationSource *source);

G_DEFINE_ABSTRACT_TYPE (GClueWebSource, gclue_web_source, GCLUE_TYPE_LOCATION_SOURCE)

static void
gclue_web_source_finalize (GObject *gsource)
{
        GClueWebSource *web = (GClueWebSource *) gsource;

        g_clear_object (&web->priv->soup_session);

        G_OBJECT_CLASS (gclue_web_source_parent_class)->finalize (gsource);
}

static void
gclue_web_source_class_init (GClueWebSourceClass *klass)
{
        GClueLocationSourceClass *source_class = GCLUE_LOCATION_SOURCE_CLASS (klass);
        GObjectClass *gsource_class = G_OBJECT_CLASS (klass);

        source_class->start = gclue_web_source_start;
        source_class->stop = gclue_web_source_stop;
        gsource_class->finalize = gclue_web_source_finalize;

        g_type_class_add_private (klass, sizeof (GClueWebSourcePrivate));
}

static void
gclue_web_source_init (GClueWebSource *web)
{
        web->priv = G_TYPE_INSTANCE_GET_PRIVATE ((web), GCLUE_TYPE_WEB_SOURCE, GClueWebSourcePrivate);

        web->priv->soup_session = soup_session_new_with_options
                        (SOUP_SESSION_REMOVE_FEATURE_BY_TYPE,
                         SOUP_TYPE_PROXY_RESOLVER_DEFAULT,
                         NULL);
}

static void
query_callback (SoupSession *session,
                SoupMessage *query,
                gpointer     user_data)
{
        GClueWebSource *web = GCLUE_WEB_SOURCE (user_data);
        GError *error = NULL;
        char *contents;
        GeocodeLocation *location;

        web->priv->query = NULL;

        if (query->status_code != SOUP_STATUS_OK) {
                g_warning ("Failed to query location: %s", query->reason_phrase);
		return;
	}

        contents = g_strndup (query->response_body->data, query->response_body->length);
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
}

static void
on_network_changed (GNetworkMonitor *monitor,
                    gboolean         available,
                    gpointer         user_data)
{
        GClueWebSource *web = GCLUE_WEB_SOURCE (user_data);
        GError *error = NULL;

        if (!available) {
                g_debug ("Network unreachable");
                return;
        }
        g_debug ("Network changed");

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
gclue_web_source_start (GClueLocationSource *source)
{
        GNetworkMonitor *monitor;
        GClueWebSourcePrivate *priv;

        g_return_if_fail (GCLUE_IS_WEB_SOURCE (source));
        priv = GCLUE_WEB_SOURCE (source)->priv;

        monitor = g_network_monitor_get_default ();
        priv->network_changed_id =
                g_signal_connect (monitor,
                                  "network-changed",
                                  G_CALLBACK (on_network_changed),
                                  source);

        if (g_network_monitor_get_network_available (monitor))
                on_network_changed (monitor, TRUE, source);
}

static void
gclue_web_source_stop (GClueLocationSource *source)
{
        GClueWebSourcePrivate *priv;

        g_return_if_fail (GCLUE_IS_WEB_SOURCE (source));
        priv = GCLUE_WEB_SOURCE (source)->priv;

        if (priv->network_changed_id) {
                g_signal_handler_disconnect (g_network_monitor_get_default (),
                                             priv->network_changed_id);
                priv->network_changed_id = 0;
        }

        if (priv->query == NULL)
                return;

        g_debug ("Cancelling query");
        soup_session_cancel_message (priv->soup_session,
                                     priv->query,
                                     SOUP_STATUS_CANCELLED);
        priv->query = NULL;
}
