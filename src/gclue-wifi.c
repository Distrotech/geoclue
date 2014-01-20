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
#include <nm-client.h>
#include <nm-device-wifi.h>
#include "gclue-wifi.h"
#include "gclue-error.h"
#include "geocode-location.h"

/* Since we use the Geolocate API (rather than Search), we could easily switch
 * to Google geolocation service in future, if such a need arises.
 */
#define SERVER "https://location.services.mozilla.com/v1/geolocate"
/* #define SERVER "https://maps.googleapis.com/maps/api/browserlocation/json?browser=firefox&sensor=true" */

/**
 * SECTION:gclue-wifi
 * @short_description: WiFi-based geolocation
 * @include: gclue-glib/gclue-wifi.h
 *
 * Contains functions to get the geolocation based on nearby WiFi networks. It
 * uses <ulink url="https://wiki.mozilla.org/CloudServices/Location">Mozilla
 * Location Service</ulink> to achieve that.
 **/

enum {
        PROP_0,
        PROP_LOCATION,
        N_PROPERTIES
};

struct _GClueWifiPrivate {
        SoupSession *soup_session;

        GeocodeLocation *location;

        SoupMessage *query;

        gulong network_changed_id;
};

static void
gclue_wifi_start (GClueLocationSource *source);
static void
gclue_wifi_stop (GClueLocationSource *source);
static GeocodeLocation *
gclue_wifi_get_location (GClueLocationSource *source);

static void
gclue_location_source_interface_init (GClueLocationSourceInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GClueWifi, gclue_wifi, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GCLUE_TYPE_LOCATION_SOURCE,
                                                gclue_location_source_interface_init))

static void
gclue_wifi_finalize (GObject *gwifi)
{
        GClueWifi *wifi = (GClueWifi *) gwifi;

        gclue_wifi_stop (GCLUE_LOCATION_SOURCE (wifi));
        g_clear_object (&wifi->priv->soup_session);

        G_OBJECT_CLASS (gclue_wifi_parent_class)->finalize (gwifi);
}

static void
gclue_wifi_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
        GClueWifi *wifi = GCLUE_WIFI (object);

        switch (prop_id) {
        case PROP_LOCATION:
                g_value_set_object (value, wifi->priv->location);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_wifi_class_init (GClueWifiClass *klass)
{
        GObjectClass *gwifi_class = G_OBJECT_CLASS (klass);

        gwifi_class->finalize = gclue_wifi_finalize;
        gwifi_class->get_property = gclue_wifi_get_property;

        g_type_class_add_private (klass, sizeof (GClueWifiPrivate));

        g_object_class_override_property (gwifi_class,
                                          PROP_LOCATION,
                                          "location");
}

static void
gclue_wifi_init (GClueWifi *wifi)
{
        wifi->priv = G_TYPE_INSTANCE_GET_PRIVATE ((wifi), GCLUE_TYPE_WIFI, GClueWifiPrivate);

        wifi->priv->soup_session = soup_session_new_with_options
                        (SOUP_SESSION_REMOVE_FEATURE_BY_TYPE,
                         SOUP_TYPE_PROXY_RESOLVER_DEFAULT,
                         NULL);
}

static void
gclue_location_source_interface_init (GClueLocationSourceInterface *iface)
{
        iface->start = gclue_wifi_start;
        iface->stop = gclue_wifi_stop;
        iface->get_location = gclue_wifi_get_location;
}

/**
 * gclue_wifi_new:
 *
 * Creates a new #GClueWifi to fetch the geolocation data.
 *
 * Returns: a new #GClueWifi. Use g_object_unref() when done.
 **/
GClueWifi *
gclue_wifi_new (void)
{
        return g_object_new (GCLUE_TYPE_WIFI, NULL);
}

static SoupMessage *
get_search_query (GClueWifi *wifi,
                  NMClient  *client,
                  GError   **error)
{
        SoupMessage *ret;
        JsonBuilder *builder;
        JsonGenerator *generator;
        JsonNode *root_node;
        char *data;
        gsize data_len;
        const GPtrArray *devices;
        const GPtrArray *aps; /* As in Access Points */
        NMDeviceWifi *wifi_device = NULL;
        guint i;

        devices = nm_client_get_devices (client);
        for (i = 0; i < devices->len; i++) {
                NMDevice *device = g_ptr_array_index (devices, i);
                if (NM_IS_DEVICE_WIFI (device)) {
                    wifi_device = NM_DEVICE_WIFI (device);

                    break;
                }
        }
        if (wifi_device == NULL) {
                g_set_error_literal (error,
                                     G_IO_ERROR,
                                     G_IO_ERROR_FAILED,
                                     "No WiFi devices available");
                return NULL;
        }

        aps = nm_device_wifi_get_access_points (wifi_device);
        if (aps->len == 0) {
                g_set_error_literal (error,
                                     G_IO_ERROR,
                                     G_IO_ERROR_FAILED,
                                     "No WiFi access points around");
                return NULL;
        }

        builder = json_builder_new ();
        json_builder_begin_object (builder);

        json_builder_set_member_name (builder, "wifiAccessPoints");
        json_builder_begin_array (builder);

        for (i = 0; i < aps->len; i++) {
                NMAccessPoint *ap = g_ptr_array_index (aps, i);
                const char *mac;
                gint8 strength_dbm;

                json_builder_begin_object (builder);
                json_builder_set_member_name (builder, "macAddress");
                mac = nm_access_point_get_hw_address (ap);
                json_builder_add_string_value (builder, mac);

                json_builder_set_member_name (builder, "signalStrength");
                strength_dbm = nm_access_point_get_strength (ap) / 2 - 100;
                json_builder_add_int_value (builder, strength_dbm);
                json_builder_end_object (builder);
        }

        json_builder_end_array (builder);
        json_builder_end_object (builder);

        generator = json_generator_new ();
        root_node = json_builder_get_root (builder);
        json_generator_set_root (generator, root_node);
        data = json_generator_to_data (generator, &data_len);

        json_node_free (root_node);
        g_object_unref (builder);
        g_object_unref (generator);

        ret = soup_message_new ("POST", SERVER);
        soup_message_set_request (ret,
                                  "application/json",
                                  SOUP_MEMORY_TAKE,
                                  data,
                                  data_len);
        g_debug ("Sending following request to '%s':\n%s", SERVER, data);

        return ret;
}

static gboolean
parse_server_error (JsonObject *object, GError **error)
{
        JsonObject *error_obj;
        int code;
        const char *message;

        error_obj = json_object_get_object_member (object, "error");
        if (error_obj == NULL)
            return FALSE;

        code = json_object_get_int_member (error_obj, "code");
        message = json_object_get_string_member (error_obj, "message");

        g_set_error_literal (error, G_IO_ERROR, code, message);

        return TRUE;
}

static GeocodeLocation *
_gclue_wifi_json_to_location (const char *json,
                              GError    **error)
{
        JsonParser *parser;
        JsonNode *node;
        JsonObject *object, *loc_object;
        GeocodeLocation *location;
        gdouble latitude, longitude, accuracy;

        g_debug ("Got following response from '%s':\n%s", SERVER, json);

        parser = json_parser_new ();

        if (!json_parser_load_from_data (parser, json, -1, error))
                return NULL;

        node = json_parser_get_root (parser);
        object = json_node_get_object (node);

        if (parse_server_error (object, error))
                return NULL;

        loc_object = json_object_get_object_member (object, "location");
        latitude = json_object_get_double_member (loc_object, "lat");
        longitude = json_object_get_double_member (loc_object, "lng");

        accuracy = json_object_get_double_member (object, "accuracy");

        location = geocode_location_new (latitude, longitude, accuracy);

        g_object_unref (parser);

        return location;
}

static void
gclue_wifi_update_location (GClueWifi       *wifi,
                            GeocodeLocation *location)
{
        GClueWifiPrivate *priv = wifi->priv;

        if (priv->location == NULL)
                priv->location = g_object_new (GEOCODE_TYPE_LOCATION, NULL);

        g_object_set (priv->location,
                      "latitude", geocode_location_get_latitude (location),
                      "longitude", geocode_location_get_longitude (location),
                      "accuracy", geocode_location_get_accuracy (location),
                      "description", geocode_location_get_description (location),
                      NULL);

        g_object_notify (G_OBJECT (wifi), "location");
}

static void
query_callback (SoupSession *session,
                SoupMessage *query,
                gpointer     user_data)
{
        GClueWifi *wifi = GCLUE_WIFI (user_data);
        GError *error = NULL;
        char *contents;
        GeocodeLocation *location;

        wifi->priv->query = NULL;

        if (query->status_code != SOUP_STATUS_OK) {
                g_warning ("Failed to query location: %s", query->reason_phrase);
		return;
	}

        contents = g_strndup (query->response_body->data, query->response_body->length);
        location = _gclue_wifi_json_to_location (contents, &error);
        g_free (contents);
        if (error != NULL) {
                g_warning ("Failed to query location: %s", error->message);
                return;
        }

        gclue_wifi_update_location (wifi, location);
}

static void
on_network_changed (GNetworkMonitor *monitor,
                    gboolean         available,
                    gpointer         user_data)
{
        GClueWifi *wifi = GCLUE_WIFI (user_data);
        NMClient *client;
        GError *error = NULL;

        if (!available) {
                g_debug ("Network unreachable");
                return;
        }
        g_debug ("Network changed");

        client = nm_client_new (); /* FIXME: Use async variant */
        wifi->priv->query = get_search_query (wifi, client, &error);
        g_object_unref (client);

        if (wifi->priv->query == NULL) {
                g_warning ("Failed to create query: %s", error->message);
                g_error_free (error);
                return;
        }

        soup_session_queue_message (wifi->priv->soup_session,
                                    wifi->priv->query,
                                    query_callback,
                                    wifi);
}

static void
gclue_wifi_start (GClueLocationSource *source)
{
        GNetworkMonitor *monitor;
        GClueWifi *wifi;

        g_return_if_fail (GCLUE_IS_WIFI (source));
        wifi = GCLUE_WIFI (source);

        if (wifi->priv->network_changed_id)
                return; /* Already started */

        monitor = g_network_monitor_get_default ();
        wifi->priv->network_changed_id =
                g_signal_connect (monitor,
                                  "network-changed",
                                  G_CALLBACK (on_network_changed),
                                  wifi);

        if (g_network_monitor_get_network_available (monitor))
                on_network_changed (monitor, TRUE, wifi);
}

static void
gclue_wifi_stop (GClueLocationSource *source)
{
        GClueWifiPrivate *priv;

        g_return_if_fail (GCLUE_IS_WIFI (source));
        priv = GCLUE_WIFI (source)->priv;

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
gclue_wifi_get_location (GClueLocationSource *source)
{
        g_return_val_if_fail (GCLUE_IS_WIFI (source), NULL);

        return GCLUE_WIFI (source)->priv->location;
}
