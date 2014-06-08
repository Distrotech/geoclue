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
#include <string.h>
#include <libxml/tree.h>
#include "gclue-3g.h"
#include "gclue-modem.h"
#include "geocode-glib/geocode-location.h"

#define URL "http://www.opencellid.org/cell/get?mcc=%u&mnc=%u" \
            "&lac=%lu&cellid=%lu&key=4f2d9207-1339-4718-a412-9683f1461de6"

/**
 * SECTION:gclue-3g
 * @short_description: WiFi-based geolocation
 * @include: gclue-glib/gclue-3g.h
 *
 * Contains functions to get the geolocation based on 3G cell towers. It
 * uses <ulink url="http://www.opencellid.org">OpenCellID</ulink> to translate
 * cell tower info to a location.
 **/

struct _GClue3GPrivate {
        GClueModem *modem;

        SoupSession *soup_session;
        SoupMessage *query;
        GCancellable *cancellable;

        gulong network_changed_id;
        gulong threeg_notify_id;
};

G_DEFINE_TYPE (GClue3G, gclue_3g, GCLUE_TYPE_LOCATION_SOURCE)

static gboolean
gclue_3g_start (GClueLocationSource *source);
static gboolean
gclue_3g_stop (GClueLocationSource *source);

static void
cancel_pending_query (GClue3G *source)
{
        GClue3GPrivate *priv = source->priv;

        if (priv->query != NULL) {
                g_debug ("Cancelling query");
                soup_session_cancel_message (priv->soup_session,
                                             priv->query,
                                             SOUP_STATUS_CANCELLED);
                priv->query = NULL;
        }
}

static void
refresh_accuracy_level (GClue3G *source)
{
        GClueAccuracyLevel new, existing;
        GNetworkMonitor *monitor = g_network_monitor_get_default ();

        existing = gclue_location_source_get_available_accuracy_level
                        (GCLUE_LOCATION_SOURCE (source));

        if (gclue_modem_get_is_3g_available (source->priv->modem) &&
            g_network_monitor_get_network_available (monitor))
                new = GCLUE_ACCURACY_LEVEL_NEIGHBORHOOD;
        else
                new = GCLUE_ACCURACY_LEVEL_NONE;

        if (new != existing) {
                g_debug ("Available accuracy level from %s: %u",
                         G_OBJECT_TYPE_NAME (source), new);
                g_object_set (G_OBJECT (source),
                              "available-accuracy-level", new,
                              NULL);
        }
}

static void
on_3g_enabled (GObject      *source_object,
               GAsyncResult *result,
               gpointer      user_data)
{
        GClue3G *source = GCLUE_3G (user_data);
        GError *error = NULL;

        if (!gclue_modem_enable_3g_finish (source->priv->modem,
                                           result,
                                           &error)) {
                g_warning ("Failed to enable 3GPP: %s", error->message);
                g_error_free (error);
        }
}

static void
on_is_3g_available_notify (GObject    *gobject,
                           GParamSpec *pspec,
                           gpointer    user_data)
{
        GClue3G *source = GCLUE_3G (user_data);
        GClue3GPrivate *priv = source->priv;

        refresh_accuracy_level (source);

        if (gclue_location_source_get_active (GCLUE_LOCATION_SOURCE (source)) &&
            gclue_modem_get_is_3g_available (priv->modem))
                gclue_modem_enable_3g (priv->modem,
                                       priv->cancellable,
                                       on_3g_enabled,
                                       source);
}

static GeocodeLocation *
parse_response (GClue3G    *source,
                const char *xml)
{
        xmlDocPtr doc;
        xmlNodePtr node;
        xmlChar *prop;
        GeocodeLocation *location;
        gdouble latitude, longitude;

        doc = xmlParseDoc ((const xmlChar *) xml);
        if (doc == NULL) {
                g_warning ("Failed to parse following response:\n%s",  xml);
                return NULL;
        }
        node = xmlDocGetRootElement (doc);
        if (node == NULL || strcmp ((const char *) node->name, "rsp") != 0) {
                g_warning ("Expected 'rsp' root node in response:\n%s",  xml);
                return NULL;
        }

        /* Find 'cell' node */
        for (node = node->children; node != NULL; node = node->next) {
                if (strcmp ((const char *) node->name, "cell") == 0)
                        break;
        }
        if (node == NULL) {
                g_warning ("Failed to find node 'cell' in response:\n%s",  xml);
                return NULL;
        }

        /* Get latitude and longitude from 'cell' node */
        prop = xmlGetProp (node, (xmlChar *) "lat");
        if (prop == NULL) {
                g_warning ("No 'lat' property node on 'cell' in response:\n%s",
                           xml);
                return NULL;
        }
        latitude = g_ascii_strtod ((const char *) prop, NULL);
        xmlFree (prop);

        prop = xmlGetProp (node, (xmlChar *) "lon");
        if (prop == NULL) {
                g_warning ("No 'lon' property node on 'cell' in response:\n%s",
                           xml);
                return NULL;
        }
        longitude = g_ascii_strtod ((const char *) prop, NULL);
        xmlFree (prop);

        /* FIXME: Is 3km a good average coverage area for a cell tower */
        location = geocode_location_new (latitude, longitude, 3000);

        xmlFreeDoc (doc);

        return location;
}

static void
query_callback (SoupSession *session,
                SoupMessage *query,
                gpointer     user_data)
{
        GClue3G *source;
        char *contents;
        GeocodeLocation *location;
        SoupURI *uri;

        if (query->status_code == SOUP_STATUS_CANCELLED)
                return;

        source = GCLUE_3G (user_data);
        source->priv->query = NULL;

        if (query->status_code != SOUP_STATUS_OK) {
                g_warning ("Failed to query location: %s", query->reason_phrase);
		return;
	}

        contents = g_strndup (query->response_body->data,
                              query->response_body->length);
        uri = soup_message_get_uri (query);
        g_debug ("Got following response from '%s':\n%s",
                 soup_uri_to_string (uri, FALSE),
                 contents);
        location = parse_response (source, contents);
        g_free (contents);
        if (location == NULL)
                return;

        gclue_location_source_set_location (GCLUE_LOCATION_SOURCE (source),
                                            location);
}

static void
on_network_changed (GNetworkMonitor *monitor,
                    gboolean         available,
                    gpointer         user_data)
{
        GClue3G *source = GCLUE_3G (user_data);
        GClue3GPrivate *priv = source->priv;

        refresh_accuracy_level (source);

        if (available && priv->query != NULL)
                soup_session_queue_message (priv->soup_session,
                                            priv->query,
                                            query_callback,
                                            user_data);
}

static void
gclue_3g_finalize (GObject *g3g)
{
        GClue3G *source = (GClue3G *) g3g;
        GClue3GPrivate *priv = source->priv;

        G_OBJECT_CLASS (gclue_3g_parent_class)->finalize (g3g);

        g_cancellable_cancel (priv->cancellable);
        cancel_pending_query (source);

        g_signal_handler_disconnect (g_network_monitor_get_default (),
                                     priv->network_changed_id);
        priv->network_changed_id = 0;

        g_signal_handler_disconnect (priv->modem,
                                     priv->threeg_notify_id);
        priv->threeg_notify_id = 0;

        g_clear_object (&priv->modem);
        g_clear_object (&priv->soup_session);
        g_clear_object (&priv->cancellable);
}

static void
gclue_3g_class_init (GClue3GClass *klass)
{
        GClueLocationSourceClass *source_class = GCLUE_LOCATION_SOURCE_CLASS (klass);
        GObjectClass *g3g_class = G_OBJECT_CLASS (klass);

        g3g_class->finalize = gclue_3g_finalize;

        source_class->start = gclue_3g_start;
        source_class->stop = gclue_3g_stop;

        g_type_class_add_private (klass, sizeof (GClue3GPrivate));
}

static void
gclue_3g_init (GClue3G *source)
{
        GClue3GPrivate *priv;
        GNetworkMonitor *monitor;

        source->priv = G_TYPE_INSTANCE_GET_PRIVATE ((source), GCLUE_TYPE_3G, GClue3GPrivate);
        priv = source->priv;

        priv->cancellable = g_cancellable_new ();

        priv->soup_session = soup_session_new_with_options
                        (SOUP_SESSION_REMOVE_FEATURE_BY_TYPE,
                         SOUP_TYPE_PROXY_RESOLVER_DEFAULT,
                         NULL);
        priv->modem = gclue_modem_get_singleton ();
        priv->threeg_notify_id =
                        g_signal_connect (priv->modem,
                                          "notify::is-3g-available",
                                          G_CALLBACK (on_is_3g_available_notify),
                                          source);
        monitor = g_network_monitor_get_default ();
        priv->network_changed_id =
                        g_signal_connect (monitor,
                                          "network-changed",
                                          G_CALLBACK (on_network_changed),
                                          source);
}

static void
on_3g_destroyed (gpointer data,
                 GObject *where_the_object_was)
{
        GClue3G **source = (GClue3G **) data;

        *source = NULL;
}

/**
 * gclue_3g_new:
 *
 * Get the #GClue3G singleton.
 *
 * Returns: (transfer full): a new ref to #GClue3G. Use g_object_unref()
 * when done.
 **/
GClue3G *
gclue_3g_get_singleton (void)
{
        static GClue3G *source = NULL;

        if (source == NULL) {
                source = g_object_new (GCLUE_TYPE_3G, NULL);
                g_object_weak_ref (G_OBJECT (source),
                                   on_3g_destroyed,
                                   &source);
        } else
                g_object_ref (source);

        return source;
}

static SoupMessage *
create_query (GClue3G *source,
              guint    mcc,
              guint    mnc,
              gulong   lac,
              gulong   cell_id)
{
        SoupMessage *ret = NULL;
        char *uri;

        uri = g_strdup_printf (URL,
                               mcc,
                               mnc,
                               lac,
                               cell_id);
        ret = soup_message_new ("GET", uri);
        g_debug ("Will send request '%s'", uri);

        g_free (uri);

        return ret;
}

static void
on_fix_3g (GClueModem *modem,
           guint       mcc,
           guint       mnc,
           gulong      lac,
           gulong      cell_id,
           gpointer    user_data)
{
        GClue3G *source = GCLUE_3G (user_data);
        GClue3GPrivate *priv = source->priv;
        GNetworkMonitor *monitor;

        cancel_pending_query (source);

        priv->query = create_query (source, mcc, mnc, lac, cell_id);

        monitor = g_network_monitor_get_default ();
        if (!g_network_monitor_get_network_available (monitor)) {
                g_debug ("No network, will send request later");

                return;
        }

        soup_session_queue_message (priv->soup_session,
                                    priv->query,
                                    query_callback,
                                    user_data);
}

static gboolean
gclue_3g_start (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;
        GClue3GPrivate *priv;

        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source), FALSE);
        priv = GCLUE_3G (source)->priv;

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_3g_parent_class);
        if (!base_class->start (source))
                return FALSE;

        g_signal_connect (priv->modem,
                          "fix-3g",
                          G_CALLBACK (on_fix_3g),
                          source);

        if (gclue_modem_get_is_3g_available (priv->modem))
                gclue_modem_enable_3g (priv->modem,
                                       priv->cancellable,
                                       on_3g_enabled,
                                       source);
        return TRUE;
}

static gboolean
gclue_3g_stop (GClueLocationSource *source)
{
        GClue3GPrivate *priv = GCLUE_3G (source)->priv;
        GClueLocationSourceClass *base_class;
        GError *error = NULL;

        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source), FALSE);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_3g_parent_class);
        if (!base_class->stop (source))
                return FALSE;

        g_signal_handlers_disconnect_by_func (G_OBJECT (priv->modem),
                                              G_CALLBACK (on_fix_3g),
                                              source);

        if (gclue_modem_get_is_3g_available (priv->modem))
                if (!gclue_modem_disable_3g (priv->modem,
                                             priv->cancellable,
                                             &error)) {
                        g_warning ("Failed to disable 3GPP: %s",
                                   error->message);
                        g_error_free (error);
                }

        return TRUE;
}
