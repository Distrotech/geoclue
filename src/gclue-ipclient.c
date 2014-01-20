/* vim: set et ts=8 sw=8: */
/*
 * Copyright (C) 2013 Red Hat, Inc.
 * Copyright (C) 2013 Satabdi Das
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
 * Authors: Satabdi Das <satabdidas@gmail.com>
 *          Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#include <stdlib.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <string.h>
#include "gclue-ipclient.h"
#include "gclue-error.h"
#include "geoip-server/geoip-server.h"

#define GEOIP_SERVER "https://geoip.fedoraproject.org/city"

/**
 * SECTION:gclue-ipclient
 * @short_description: GeoIP client
 * @include: gclue-glib/gclue-ipclient.h
 *
 * Contains functions to get the geolocation corresponding to IP addresses from a server.
 **/

enum {
        PROP_0,
        PROP_LOCATION,
        N_PROPERTIES
};

struct _GClueIpclientPrivate {
        SoupSession *soup_session;

        char *ip;

        GeocodeLocation *location;

        SoupMessage *query;

        gulong network_changed_id;
};

static void
gclue_ipclient_start (GClueLocationSource *source);
static void
gclue_ipclient_stop (GClueLocationSource *source);
static GeocodeLocation *
gclue_ipclient_get_location (GClueLocationSource *source);

static void
gclue_location_source_interface_init (GClueLocationSourceInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GClueIpclient, gclue_ipclient, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GCLUE_TYPE_LOCATION_SOURCE,
                                                gclue_location_source_interface_init))

static void
gclue_ipclient_finalize (GObject *gipclient)
{
        GClueIpclient *ipclient = (GClueIpclient *) gipclient;

        gclue_ipclient_stop (GCLUE_LOCATION_SOURCE (ipclient));
        g_clear_object (&ipclient->priv->soup_session);
        g_free (ipclient->priv->ip);

        G_OBJECT_CLASS (gclue_ipclient_parent_class)->finalize (gipclient);
}

static void
gclue_ipclient_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
        GClueIpclient *ipclient = GCLUE_IPCLIENT (object);

        switch (prop_id) {
        case PROP_LOCATION:
                g_value_set_object (value, ipclient->priv->location);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_ipclient_class_init (GClueIpclientClass *klass)
{
        GObjectClass *gipclient_class = G_OBJECT_CLASS (klass);

        gipclient_class->finalize = gclue_ipclient_finalize;
        gipclient_class->get_property = gclue_ipclient_get_property;

        g_type_class_add_private (klass, sizeof (GClueIpclientPrivate));

        g_object_class_override_property (gipclient_class,
                                          PROP_LOCATION,
                                          "location");
}

static void
gclue_ipclient_init (GClueIpclient *ipclient)
{
        ipclient->priv = G_TYPE_INSTANCE_GET_PRIVATE ((ipclient), GCLUE_TYPE_IPCLIENT, GClueIpclientPrivate);

        ipclient->priv->soup_session = soup_session_new_with_options
                        (SOUP_SESSION_REMOVE_FEATURE_BY_TYPE,
                         SOUP_TYPE_PROXY_RESOLVER_DEFAULT,
                         NULL);
}

static void
gclue_location_source_interface_init (GClueLocationSourceInterface *iface)
{
        iface->start = gclue_ipclient_start;
        iface->stop = gclue_ipclient_stop;
        iface->get_location = gclue_ipclient_get_location;
}

/**
 * gclue_ipclient_new_for_ip:
 * @str: The IP address
 *
 * Returns: a new #GClueIpclient. Use g_object_unref() when done.
 **/
GClueIpclient *
gclue_ipclient_new_for_ip (const char *ip)
{
        GClueIpclient *ipclient;

        ipclient = g_object_new (GCLUE_TYPE_IPCLIENT, NULL);
        ipclient->priv->ip = g_strdup (ip);

        return ipclient;
}

/**
 * gclue_ipclient_new:
 *
 * Creates a new #GClueIpclient to fetch the geolocation data.
 * Here the IP address is not provided the by client, hence the server
 * will try to get the IP address from various proxy variables.
 * Use gclue_location_source_search_async() to query the server.
 *
 * Returns: a new #GClueIpclient. Use g_object_unref() when done.
 **/
GClueIpclient *
gclue_ipclient_new (void)
{
        return gclue_ipclient_new_for_ip (NULL);
}

static SoupMessage *
get_search_query (GClueIpclient *ipclient)
{
        SoupMessage *ret;
        char *uri;

        if (ipclient->priv->ip) {
                GHashTable *ht;
                char *query_string;

                ht = g_hash_table_new (g_str_hash, g_str_equal);
                g_hash_table_insert (ht, "ip", g_strdup (ipclient->priv->ip));
                query_string = soup_form_encode_hash (ht);
                g_hash_table_destroy (ht);

                uri = g_strdup_printf ("%s?%s", GEOIP_SERVER, query_string);
                g_free (query_string);
        } else
                uri = g_strdup (GEOIP_SERVER);

        ret = soup_message_new ("GET", uri);
        g_free (uri);

        return ret;
}

static gboolean
parse_server_error (JsonObject *object, GError **error)
{
        GeoipServerError server_error_code;
        int error_code;
        const char *error_message;

        if (!json_object_has_member (object, "error_code"))
            return FALSE;

        server_error_code = json_object_get_int_member (object, "error_code");
        switch (server_error_code) {
                case INVALID_IP_ADDRESS_ERR:
                        error_code = GCLUE_ERROR_INVALID_ARGUMENTS;
                        break;
                case INVALID_ENTRY_ERR:
                        error_code = GCLUE_ERROR_NO_MATCHES;
                        break;
                case DATABASE_ERR:
                        error_code = GCLUE_ERROR_INTERNAL_SERVER;
                        break;
                default:
                        g_assert_not_reached ();
        }

        error_message = json_object_get_string_member (object, "error_message");

        g_set_error_literal (error,
                             GCLUE_ERROR,
                             error_code,
                             error_message);

        return TRUE;
}

static gdouble
get_accuracy_from_string (const char *str)
{
        if (strcmp (str, "street") == 0)
                return GEOCODE_LOCATION_ACCURACY_STREET;
        else if (strcmp (str, "city") == 0)
                return GEOCODE_LOCATION_ACCURACY_CITY;
        else if (strcmp (str, "region") == 0)
                return GEOCODE_LOCATION_ACCURACY_REGION;
        else if (strcmp (str, "country") == 0)
                return GEOCODE_LOCATION_ACCURACY_COUNTRY;
        else if (strcmp (str, "continent") == 0)
                return GEOCODE_LOCATION_ACCURACY_CONTINENT;
        else
                return GEOCODE_LOCATION_ACCURACY_UNKNOWN;
}

static gdouble
get_accuracy_from_json_location (JsonObject *object)
{
        if (json_object_has_member (object, "accuracy")) {
                const char *str;

                str = json_object_get_string_member (object, "accuracy");
                return get_accuracy_from_string (str);
        } else if (json_object_has_member (object, "street")) {
                return GEOCODE_LOCATION_ACCURACY_STREET;
        } else if (json_object_has_member (object, "city")) {
                return GEOCODE_LOCATION_ACCURACY_CITY;
        } else if (json_object_has_member (object, "region_name")) {
                return GEOCODE_LOCATION_ACCURACY_REGION;
        } else if (json_object_has_member (object, "country_name")) {
                return GEOCODE_LOCATION_ACCURACY_COUNTRY;
        } else if (json_object_has_member (object, "continent")) {
                return GEOCODE_LOCATION_ACCURACY_CONTINENT;
        } else {
                return GEOCODE_LOCATION_ACCURACY_UNKNOWN;
        }
}

static GeocodeLocation *
_gclue_ip_json_to_location (const char *json,
                            GError    **error)
{
        JsonParser *parser;
        JsonNode *node;
        JsonObject *object;
        GeocodeLocation *location;
        gdouble latitude, longitude, accuracy;
        char *desc = NULL;

        parser = json_parser_new ();

        if (!json_parser_load_from_data (parser, json, -1, error))
                return NULL;

        node = json_parser_get_root (parser);
        object = json_node_get_object (node);

        if (parse_server_error (object, error))
                return NULL;

        latitude = json_object_get_double_member (object, "latitude");
        longitude = json_object_get_double_member (object, "longitude");
        accuracy = get_accuracy_from_json_location (object);

        location = geocode_location_new (latitude, longitude, accuracy);

        if (json_object_has_member (object, "country_name")) {
                if (json_object_has_member (object, "region_name")) {
                        if (json_object_has_member (object, "city")) {
                                desc = g_strdup_printf ("%s, %s, %s",
                                                        json_object_get_string_member (object, "city"),
                                                        json_object_get_string_member (object, "region_name"),
                                                        json_object_get_string_member (object, "country_name"));
                        } else {
                                desc = g_strdup_printf ("%s, %s",
                                                        json_object_get_string_member (object, "region_name"),
                                                        json_object_get_string_member (object, "country_name"));
                        }
                } else {
                        desc = g_strdup_printf ("%s",
                                                json_object_get_string_member (object, "country_name"));
                }
        }

        if (desc != NULL) {
                geocode_location_set_description (location, desc);
                g_free (desc);
        }

        g_object_unref (parser);

        return location;
}

static void
gclue_ipclient_update_location (GClueIpclient   *ipclient,
                                GeocodeLocation *location)
{
        GClueIpclientPrivate *priv = ipclient->priv;

        if (priv->location == NULL)
                priv->location = g_object_new (GEOCODE_TYPE_LOCATION, NULL);

        g_object_set (priv->location,
                      "latitude", geocode_location_get_latitude (location),
                      "longitude", geocode_location_get_longitude (location),
                      "accuracy", geocode_location_get_accuracy (location),
                      "description", geocode_location_get_description (location),
                      NULL);

        g_object_notify (G_OBJECT (ipclient), "location");
}

static void
query_callback (SoupSession *session,
                SoupMessage *query,
                gpointer     user_data)
{
        GClueIpclient *ipclient = GCLUE_IPCLIENT (user_data);
        GError *error = NULL;
        char *contents;
        GeocodeLocation *location;

        ipclient->priv->query = NULL;

        if (query->status_code != SOUP_STATUS_OK) {
                g_warning ("Failed to query location: %s", query->reason_phrase);
		return;
	}

        contents = g_strndup (query->response_body->data, query->response_body->length);
        location = _gclue_ip_json_to_location (contents, &error);
        g_free (contents);
        if (error != NULL) {
                g_warning ("Failed to query location: %s", error->message);
                return;
        }

        gclue_ipclient_update_location (ipclient, location);
}

static void
on_network_changed (GNetworkMonitor *monitor,
                    gboolean         available,
                    gpointer         user_data)
{
        GClueIpclient *ipclient = GCLUE_IPCLIENT (user_data);

        if (!available) {
                g_debug ("Network unreachable");
                return;
        }
        g_debug ("Network changed");

        ipclient->priv->query = get_search_query (ipclient);
        soup_session_queue_message (ipclient->priv->soup_session,
                                    ipclient->priv->query,
                                    query_callback,
                                    ipclient);
}

static void
gclue_ipclient_start (GClueLocationSource *source)
{
        GNetworkMonitor *monitor;
        GClueIpclient *ipclient;

        g_return_if_fail (GCLUE_IS_IPCLIENT (source));
        ipclient = GCLUE_IPCLIENT (source);

        if (ipclient->priv->network_changed_id)
                return; /* Already started */

        monitor = g_network_monitor_get_default ();
        ipclient->priv->network_changed_id =
                g_signal_connect (monitor,
                                  "network-changed",
                                  G_CALLBACK (on_network_changed),
                                  ipclient);

        if (g_network_monitor_get_network_available (monitor))
                on_network_changed (monitor, TRUE, ipclient);
}

static void
gclue_ipclient_stop (GClueLocationSource *source)
{
        GClueIpclientPrivate *priv;

        g_return_if_fail (GCLUE_IS_IPCLIENT (source));
        priv = GCLUE_IPCLIENT (source)->priv;

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

static GeocodeLocation *
gclue_ipclient_get_location (GClueLocationSource *source)
{
        g_return_val_if_fail (GCLUE_IS_IPCLIENT (source), NULL);

        return GCLUE_IPCLIENT (source)->priv->location;
}
