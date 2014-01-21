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

struct _GClueWifiPrivate {
};

static SoupMessage *
gclue_wifi_create_query (GClueWebSource *source,
                         GError        **error);
static GeocodeLocation *
gclue_wifi_parse_response (GClueWebSource *source,
                           const char     *json,
                           GError        **error);

G_DEFINE_TYPE (GClueWifi, gclue_wifi, GCLUE_TYPE_WEB_SOURCE)

static void
gclue_wifi_class_init (GClueWifiClass *klass)
{
        GClueWebSourceClass *source_class = GCLUE_WEB_SOURCE_CLASS (klass);

        source_class->create_query = gclue_wifi_create_query;
        source_class->parse_response = gclue_wifi_parse_response;

        g_type_class_add_private (klass, sizeof (GClueWifiPrivate));
}

static void
gclue_wifi_init (GClueWifi *wifi)
{
        wifi->priv = G_TYPE_INSTANCE_GET_PRIVATE ((wifi), GCLUE_TYPE_WIFI, GClueWifiPrivate);
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
gclue_wifi_create_query (GClueWebSource *source,
                         GError        **error)
{
        SoupMessage *ret = NULL;
        JsonBuilder *builder;
        JsonGenerator *generator;
        JsonNode *root_node;
        char *data;
        gsize data_len;
        const GPtrArray *devices;
        const GPtrArray *aps; /* As in Access Points */
        NMClient *client;
        NMDeviceWifi *wifi_device = NULL;
        guint i;

        client = nm_client_new (); /* FIXME: Use async variant */

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
                goto out;
        }

        aps = nm_device_wifi_get_access_points (wifi_device);
        if (aps->len == 0) {
                g_set_error_literal (error,
                                     G_IO_ERROR,
                                     G_IO_ERROR_FAILED,
                                     "No WiFi access points around");
                goto out;
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

out:
        g_object_unref (client);
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
gclue_wifi_parse_response (GClueWebSource *source,
                           const char     *json,
                           GError        **error)
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
