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
#include <config.h>
#include "gclue-wifi.h"
#include "wpa_supplicant-interface.h"
#include "gclue-config.h"
#include "gclue-error.h"
#include "geocode-glib/geocode-location.h"

/**
 * SECTION:gclue-wifi
 * @short_description: WiFi-based geolocation
 * @include: gclue-glib/gclue-wifi.h
 *
 * Contains functions to get the geolocation based on nearby WiFi networks. It
 * uses <ulink url="https://wiki.mozilla.org/CloudServices/Location">Mozilla
 * Location Service</ulink> to achieve that. The URL is kept in our
 * configuration file so its easy to switch to Google's API.
 **/

static gboolean
gclue_wifi_start (GClueLocationSource *source);
static gboolean
gclue_wifi_stop (GClueLocationSource *source);

struct _GClueWifiPrivate {
        WPASupplicant *supplicant;
        WPAInterface *interface;
        GHashTable *bss_proxies;
        GHashTable *ignored_bss_proxies;

        gulong bss_added_id;
        gulong bss_removed_id;

        guint refresh_timeout;

        GClueAccuracyLevel accuracy_level;
};

enum
{
        PROP_0,
        PROP_ACCURACY_LEVEL,
        LAST_PROP
};
static GParamSpec *gParamSpecs[LAST_PROP];

static SoupMessage *
gclue_wifi_create_query (GClueWebSource *source,
                         GError        **error);
static SoupMessage *
gclue_wifi_create_submit_query (GClueWebSource  *source,
                                GeocodeLocation *location,
                                GError         **error);
static GeocodeLocation *
gclue_wifi_parse_response (GClueWebSource *source,
                           const char     *json,
                           GError        **error);
static GClueAccuracyLevel
gclue_wifi_get_available_accuracy_level (GClueWebSource *source,
                                         gboolean        net_available);

G_DEFINE_TYPE (GClueWifi, gclue_wifi, GCLUE_TYPE_WEB_SOURCE)

static void
disconnect_bss_signals (GClueWifi *wifi);

static void
gclue_wifi_finalize (GObject *gwifi)
{
        GClueWifi *wifi = (GClueWifi *) gwifi;

        G_OBJECT_CLASS (gclue_wifi_parent_class)->finalize (gwifi);

        disconnect_bss_signals (wifi);
        g_clear_object (&wifi->priv->supplicant);
        g_clear_object (&wifi->priv->interface);
        g_clear_pointer (&wifi->priv->bss_proxies, g_hash_table_unref);
        g_clear_pointer (&wifi->priv->ignored_bss_proxies, g_hash_table_unref);
}

static void
gclue_wifi_constructed (GObject *object);

static void
gclue_wifi_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
        GClueWifi *wifi = GCLUE_WIFI (object);

        switch (prop_id) {
        case PROP_ACCURACY_LEVEL:
                g_value_set_enum (value, wifi->priv->accuracy_level);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_wifi_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
        GClueWifi *wifi = GCLUE_WIFI (object);

        switch (prop_id) {
        case PROP_ACCURACY_LEVEL:
                wifi->priv->accuracy_level = g_value_get_enum (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_wifi_class_init (GClueWifiClass *klass)
{
        GClueWebSourceClass *web_class = GCLUE_WEB_SOURCE_CLASS (klass);
        GClueLocationSourceClass *source_class = GCLUE_LOCATION_SOURCE_CLASS (klass);
        GObjectClass *gwifi_class = G_OBJECT_CLASS (klass);

        source_class->start = gclue_wifi_start;
        source_class->stop = gclue_wifi_stop;
        web_class->create_submit_query = gclue_wifi_create_submit_query;
        web_class->create_query = gclue_wifi_create_query;
        web_class->parse_response = gclue_wifi_parse_response;
        web_class->get_available_accuracy_level =
                gclue_wifi_get_available_accuracy_level;
        gwifi_class->get_property = gclue_wifi_get_property;
        gwifi_class->set_property = gclue_wifi_set_property;
        gwifi_class->finalize = gclue_wifi_finalize;
        gwifi_class->constructed = gclue_wifi_constructed;

        g_type_class_add_private (klass, sizeof (GClueWifiPrivate));

        gParamSpecs[PROP_ACCURACY_LEVEL] = g_param_spec_enum ("accuracy-level",
                                                              "AccuracyLevel",
                                                              "Max accuracy level",
                                                              GCLUE_TYPE_ACCURACY_LEVEL,
                                                              GCLUE_ACCURACY_LEVEL_CITY,
                                                              G_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (gwifi_class,
                                         PROP_ACCURACY_LEVEL,
                                         gParamSpecs[PROP_ACCURACY_LEVEL]);
}

static gboolean
on_refresh_timeout (gpointer user_data)
{
        g_debug ("Refreshing location..");
        gclue_web_source_refresh (GCLUE_WEB_SOURCE (user_data));
        GCLUE_WIFI (user_data)->priv->refresh_timeout = 0;

        return FALSE;
}

static void
on_bss_added (WPAInterface *object,
              const gchar  *path,
              GVariant     *properties,
              gpointer      user_data);

static char *
variant_to_string (GVariant *variant, guint *len)
{
        guint n_bytes, i;
        char *ret;

        n_bytes = g_variant_n_children (variant);
        if (len != NULL)
                *len = n_bytes;
        if (n_bytes <= 0)
                return NULL;
        ret = g_malloc (n_bytes + 1);
        ret[n_bytes] = '\0';

        for (i = 0; i < n_bytes; i++)
                g_variant_get_child (variant,
                                     i,
                                     "y",
                                     &ret[i]);

        return ret;
}

static char *
get_ssid_from_bss (WPABSS *bss)
{
        GVariant *variant = wpa_bss_get_ssid (bss);

        return variant_to_string (variant, NULL);
}

static char *
get_bssid_from_bss (WPABSS *bss)
{
        GVariant *variant;
        char *raw_bssid;
        char *bssid;
        guint raw_len, len, i, j;

        variant = wpa_bss_get_bssid (bss);

        raw_bssid = variant_to_string (variant, &raw_len);
        len = raw_len * 2 + raw_len;
        bssid = g_malloc (len);
        for (i = 0, j = 0; i < len; i = i + 3, j++)
                g_snprintf (bssid + i,
                            4,
                           "%02x:",
                           (unsigned char) raw_bssid[j]);
        bssid[len - 1] = '\0';

        return bssid;
}

static void
on_bss_signal_notify (GObject    *gobject,
                      GParamSpec *pspec,
                      gpointer    user_data)
{
        GClueWifi *wifi = GCLUE_WIFI (user_data);
        WPABSS *bss = WPA_BSS (gobject);
        const char *path;

        if (wpa_bss_get_signal (bss) <= -90) {
                char *bssid = get_bssid_from_bss (bss);
                g_debug ("WiFi AP '%s' still has very low strength (%u dBm)"
                         ", ignoring again..",
                         bssid,
                         wpa_bss_get_signal (bss));
                g_free (bssid);
                return;
        }
        path = g_dbus_proxy_get_object_path (G_DBUS_PROXY (bss));

        g_signal_handlers_disconnect_by_func (G_OBJECT (bss),
                                              on_bss_signal_notify,
                                              user_data);
        on_bss_added (WPA_INTERFACE (wifi->priv->interface),
                      path,
                      NULL,
                      user_data);

        g_hash_table_replace (wifi->priv->bss_proxies,
                              g_strdup (path),
                              bss);
        g_hash_table_remove (wifi->priv->ignored_bss_proxies, path);
}

static gboolean
should_ignore_bss (WPABSS *bss)
{
        char *ssid, *bssid;

        ssid = get_ssid_from_bss (bss);
        bssid = get_bssid_from_bss (bss);
        if (ssid == NULL || g_str_has_suffix (ssid, "_nomap")) {
                g_debug ("SSID for WiFi AP '%s' missing or has '_nomap' suffix."
                         ", Ignoring..",
                         bssid);
                return TRUE;
        }
        g_free (ssid);
        g_free (bssid);

        return FALSE;
}

static void
on_bss_proxy_ready (GObject      *source_object,
                    GAsyncResult *res,
                    gpointer      user_data)
{
        GClueWifi *wifi = GCLUE_WIFI (user_data);
        WPABSS *bss;
        GError *error = NULL;
        const char *path;
        char *ssid;

        bss = wpa_bss_proxy_new_for_bus_finish (res, &error);
        if (bss == NULL) {
                g_debug ("%s", error->message);
                g_error_free (error);

                return;
        }
        path = g_dbus_proxy_get_object_path (G_DBUS_PROXY (bss));

        if (should_ignore_bss (bss)) {
                g_object_unref (bss);

                return;
        }

        ssid = get_ssid_from_bss (bss);
        g_debug ("WiFi AP '%s' added.", ssid);
        g_free (ssid);

        path = g_dbus_proxy_get_object_path (G_DBUS_PROXY (bss));
        if (wpa_bss_get_signal (bss) <= -90) {
                char *bssid = get_bssid_from_bss (bss);
                g_debug ("WiFi AP '%s' has very low strength (%u dBm)"
                         ", ignoring for now..",
                         bssid,
                         wpa_bss_get_signal (bss));
                g_free (bssid);
                g_signal_connect (G_OBJECT (bss),
                                  "notify::signal",
                                  G_CALLBACK (on_bss_signal_notify),
                                  user_data);
                g_hash_table_replace (wifi->priv->ignored_bss_proxies,
                                      g_strdup (path),
                                      bss);
                return;
        }


        /* There could be multiple devices being added/removed at the same time
         * so we don't immediately call refresh but rather wait 1 second.
         */
        if (wifi->priv->refresh_timeout != 0)
                g_source_remove (wifi->priv->refresh_timeout);
        wifi->priv->refresh_timeout = g_timeout_add_seconds (1,
                                                             on_refresh_timeout,
                                                             wifi);
        g_hash_table_replace (wifi->priv->bss_proxies,
                              g_strdup (path),
                              bss);
}

static void
on_bss_added (WPAInterface *object,
              const gchar  *path,
              GVariant     *properties,
              gpointer      user_data)
{
        wpa_bss_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                   G_DBUS_PROXY_FLAGS_NONE,
                                   "fi.w1.wpa_supplicant1",
                                   path,
                                   NULL,
                                   on_bss_proxy_ready,
                                   user_data);
}

static void
on_bss_removed (WPAInterface *object,
                const gchar  *path,
                gpointer      user_data)
{
        GClueWifiPrivate *priv = GCLUE_WIFI (user_data)->priv;

        g_hash_table_remove (priv->bss_proxies, path);
        g_hash_table_remove (priv->ignored_bss_proxies, path);
}

static void
connect_bss_signals (GClueWifi *wifi)
{
        GClueWifiPrivate *priv = wifi->priv;
        const gchar *const *bss_list;
        guint i;

        if (priv->bss_added_id != 0)
                return;
        if (priv->interface == NULL) {
                if (wifi->priv->accuracy_level == GCLUE_ACCURACY_LEVEL_CITY)
                        gclue_web_source_refresh (GCLUE_WEB_SOURCE (wifi));

                return;
        }

        priv->bss_added_id = g_signal_connect (priv->interface,
                                              "bss-added",
                                              G_CALLBACK (on_bss_added),
                                              wifi);
        priv->bss_removed_id = g_signal_connect (priv->interface,
                                                "bss-removed",
                                                G_CALLBACK (on_bss_removed),
                                                wifi);

        bss_list = wpa_interface_get_bsss (WPA_INTERFACE (priv->interface));
        for (i = 0; bss_list[i] != NULL; i++)
                on_bss_added (WPA_INTERFACE (priv->interface),
                              bss_list[i],
                              NULL,
                              wifi);
}

static void
disconnect_bss_signals (GClueWifi *wifi)
{
        GClueWifiPrivate *priv = wifi->priv;

        if (priv->bss_added_id == 0 || priv->interface == NULL)
                return;

        g_signal_handler_disconnect (priv->interface, priv->bss_added_id);
        priv->bss_added_id = 0;
        g_signal_handler_disconnect (priv->interface, priv->bss_removed_id);
        priv->bss_removed_id = 0;

        if (priv->refresh_timeout != 0) {
                g_source_remove (priv->refresh_timeout);
                priv->refresh_timeout = 0;
        }

        g_hash_table_remove_all (wifi->priv->bss_proxies);
        g_hash_table_remove_all (wifi->priv->ignored_bss_proxies);
}

static gboolean
gclue_wifi_start (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;

        g_return_val_if_fail (GCLUE_IS_WIFI (source), FALSE);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_wifi_parent_class);
        if (!base_class->start (source))
                return FALSE;

        connect_bss_signals (GCLUE_WIFI (source));
        return TRUE;
}

static gboolean
gclue_wifi_stop (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;

        g_return_val_if_fail (GCLUE_IS_WIFI (source), FALSE);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_wifi_parent_class);
        if (!base_class->stop (source))
                return FALSE;

        disconnect_bss_signals (GCLUE_WIFI (source));
        return TRUE;
}

static GClueAccuracyLevel
gclue_wifi_get_available_accuracy_level (GClueWebSource *source,
                                         gboolean        net_available)
{
        if (!net_available)
                return GCLUE_ACCURACY_LEVEL_NONE;
        else if (GCLUE_WIFI (source)->priv->interface != NULL)
                return GCLUE_ACCURACY_LEVEL_STREET;
        else
                return GCLUE_ACCURACY_LEVEL_CITY;
}

static void
on_interface_proxy_ready (GObject      *source_object,
                          GAsyncResult *res,
                          gpointer      user_data)
{
        GClueWifi *wifi = GCLUE_WIFI (user_data);
        WPAInterface *interface;
        GError *error = NULL;

        interface = wpa_interface_proxy_new_for_bus_finish (res, &error);
        if (interface == NULL) {
                g_debug ("%s", error->message);
                g_error_free (error);

                return;
        }

        if (wifi->priv->interface != NULL) {
                g_object_unref (interface);
                return;
        }

        wifi->priv->interface = interface;
        g_debug ("WiFi device '%s' added.",
                 wpa_interface_get_ifname (interface));

        if (gclue_location_source_get_active (GCLUE_LOCATION_SOURCE (wifi)))
                connect_bss_signals (wifi);
        else
                gclue_web_source_refresh (GCLUE_WEB_SOURCE (wifi));
}

static void
on_interface_added (WPASupplicant *supplicant,
                    const gchar   *path,
                    GVariant      *properties,
                    gpointer       user_data)
{
        GClueWifi *wifi = GCLUE_WIFI (user_data);

        if (wifi->priv->interface != NULL)
                return;

        wpa_interface_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         "fi.w1.wpa_supplicant1",
                                         path,
                                         NULL,
                                         on_interface_proxy_ready,
                                         wifi);
}

static void
on_interface_removed (WPASupplicant *supplicant,
                      const gchar   *path,
                      gpointer       user_data)
{
        GClueWifi *wifi = GCLUE_WIFI (user_data);
        GClueWifiPrivate *priv = wifi->priv;
        const char *object_path;

        if (priv->interface == NULL)
                return;

        object_path = g_dbus_proxy_get_object_path
                        (G_DBUS_PROXY (priv->interface));
        if (g_strcmp0 (object_path, path) != 0)
                return;

        g_debug ("WiFi device '%s' removed.",
                 wpa_interface_get_ifname (priv->interface));

        disconnect_bss_signals (wifi);
        g_clear_object (&wifi->priv->interface);

        gclue_web_source_refresh (GCLUE_WEB_SOURCE (wifi));
}

static void
gclue_wifi_init (GClueWifi *wifi)
{
        wifi->priv = G_TYPE_INSTANCE_GET_PRIVATE ((wifi), GCLUE_TYPE_WIFI, GClueWifiPrivate);

        wifi->priv->bss_proxies = g_hash_table_new_full (g_str_hash,
                                                         g_str_equal,
                                                         g_free,
                                                         g_object_unref);
        wifi->priv->ignored_bss_proxies = g_hash_table_new_full (g_str_hash,
                                                                 g_str_equal,
                                                                 g_free,
                                                                 g_object_unref);
}

static void
gclue_wifi_constructed (GObject *object)
{
        GClueWifi *wifi = GCLUE_WIFI (object);
        GClueWifiPrivate *priv = wifi->priv;
        const gchar *const *interfaces;
        GError *error = NULL;

        G_OBJECT_CLASS (gclue_wifi_parent_class)->constructed (object);

        if (wifi->priv->accuracy_level == GCLUE_ACCURACY_LEVEL_CITY)
                goto refresh_n_exit;

        /* FIXME: We should be using async variant */
        priv->supplicant = wpa_supplicant_proxy_new_for_bus_sync
                        (G_BUS_TYPE_SYSTEM,
                         G_DBUS_PROXY_FLAGS_NONE,
                         "fi.w1.wpa_supplicant1",
                         "/fi/w1/wpa_supplicant1",
                         NULL,
                         &error);
        if (priv->supplicant == NULL) {
                g_warning ("Failed to connect to wpa_supplicant service: %s",
                           error->message);
                g_error_free (error);
                goto refresh_n_exit;
        }

        g_signal_connect (priv->supplicant,
                          "interface-added",
                          G_CALLBACK (on_interface_added),
                          wifi);
        g_signal_connect (priv->supplicant,
                          "interface-removed",
                          G_CALLBACK (on_interface_removed),
                          wifi);

        interfaces = wpa_supplicant_get_interfaces (priv->supplicant);
        if (interfaces != NULL && interfaces[0] != NULL)
                on_interface_added (priv->supplicant,
                                    interfaces[0],
                                    NULL,
                                    wifi);

refresh_n_exit:
        gclue_web_source_refresh (GCLUE_WEB_SOURCE (object));
}

static void
on_wifi_destroyed (gpointer data,
                   GObject *where_the_object_was)
{
        GClueWifi **wifi = (GClueWifi **) data;

        *wifi = NULL;
}

/**
 * gclue_wifi_new:
 *
 * Get the #GClueWifi singleton, for the specified max accuracy level @level.
 *
 * Returns: (transfer full): a new ref to #GClueWifi. Use g_object_unref()
 * when done.
 **/
GClueWifi *
gclue_wifi_get_singleton (GClueAccuracyLevel level)
{
        static GClueWifi *wifi[] = { NULL, NULL };
        guint i;

        g_return_val_if_fail (level >= GCLUE_ACCURACY_LEVEL_CITY, NULL);

        i = (level == GCLUE_ACCURACY_LEVEL_CITY)? 0 : 1;
        if (wifi[i] == NULL) {
                wifi[i] = g_object_new (GCLUE_TYPE_WIFI,
                                        "accuracy-level", level,
                                        NULL);
                g_object_weak_ref (G_OBJECT (wifi[i]),
                                   on_wifi_destroyed,
                                   &wifi[i]);
        } else
                g_object_ref (wifi[i]);

        return wifi[i];
}

GClueAccuracyLevel
gclue_wifi_get_accuracy_level (GClueWifi *wifi)
{
        g_return_val_if_fail (GCLUE_IS_WIFI (wifi),
                              GCLUE_ACCURACY_LEVEL_NONE);

        return wifi->priv->accuracy_level;
}

static const char *
get_url (void)
{
        GClueConfig *config;

        config = gclue_config_get_singleton ();

        return gclue_config_get_wifi_url (config);
}

static GList *
get_bss_list (GClueWifi *wifi,
              GError   **error)
{
        if (wifi->priv->interface == NULL) {
                g_set_error_literal (error,
                                     G_IO_ERROR,
                                     G_IO_ERROR_FAILED,
                                     "No WiFi devices available");
                return NULL;
        }

        return g_hash_table_get_values (wifi->priv->bss_proxies);
}

static SoupMessage *
gclue_wifi_create_query (GClueWebSource *source,
                         GError        **error)
{
        GClueWifi *wifi = GCLUE_WIFI (source);
        GList *bss_list; /* As in Access Points */
        SoupMessage *ret = NULL;
        JsonBuilder *builder;
        JsonGenerator *generator;
        JsonNode *root_node;
        char *data;
        gsize data_len;
        const char *uri;

        builder = json_builder_new ();
        json_builder_begin_object (builder);

        bss_list = get_bss_list (wifi, NULL);
        /* We send pure geoip query using empty object if bss_list is NULL. */
        if (bss_list != NULL) {
                GList *iter;

                json_builder_set_member_name (builder, "wifiAccessPoints");
                json_builder_begin_array (builder);

                for (iter = bss_list; iter != NULL; iter = iter->next) {
                        WPABSS *bss = WPA_BSS (iter->data);
                        char *mac;
                        gint16 strength_dbm;

                        if (should_ignore_bss (bss))
                                continue;

                        json_builder_begin_object (builder);
                        json_builder_set_member_name (builder, "macAddress");
                        mac = get_bssid_from_bss (bss);
                        json_builder_add_string_value (builder, mac);
                        g_free (mac);

                        json_builder_set_member_name (builder, "signalStrength");
                        strength_dbm = wpa_bss_get_signal (bss);
                        json_builder_add_int_value (builder, strength_dbm);
                        json_builder_end_object (builder);
                }
                json_builder_end_array (builder);
        }
        json_builder_end_object (builder);

        generator = json_generator_new ();
        root_node = json_builder_get_root (builder);
        json_generator_set_root (generator, root_node);
        data = json_generator_to_data (generator, &data_len);

        json_node_free (root_node);
        g_object_unref (builder);
        g_object_unref (generator);

        uri = get_url ();
        ret = soup_message_new ("POST", uri);
        soup_message_set_request (ret,
                                  "application/json",
                                  SOUP_MEMORY_TAKE,
                                  data,
                                  data_len);
        g_debug ("Sending following request to '%s':\n%s", uri, data);

        return ret;
}

static gboolean
parse_server_error (JsonObject *object, GError **error)
{
        JsonObject *error_obj;
        int code;
        const char *message;

        if (!json_object_has_member (object, "error"))
            return FALSE;

        error_obj = json_object_get_object_member (object, "error");
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

static const char *
get_submit_config (const char **nick)
{
        GClueConfig *config;

        config = gclue_config_get_singleton ();
        if (!gclue_config_get_wifi_submit_data (config))
                return NULL;

        *nick = gclue_config_get_wifi_submit_nick (config);

        return gclue_config_get_wifi_submit_url (config);
}

static SoupMessage *
gclue_wifi_create_submit_query (GClueWebSource  *source,
                                GeocodeLocation *location,
                                GError         **error)
{
        GClueWifi *wifi = GCLUE_WIFI (source);
        SoupMessage *ret = NULL;
        JsonBuilder *builder;
        JsonGenerator *generator;
        JsonNode *root_node;
        char *data, *timestamp;
        const char *url, *nick;
        gsize data_len;
        GList *bss_list, *iter; /* As in Access Points */
        gdouble lat, lon, accuracy, altitude;
        GTimeVal tv;

        url = get_submit_config (&nick);
        if (url == NULL)
                goto out;

        bss_list = get_bss_list (wifi, error);
        if (bss_list == NULL)
                goto out;

        builder = json_builder_new ();
        json_builder_begin_object (builder);

        json_builder_set_member_name (builder, "items");
        json_builder_begin_array (builder);

        json_builder_begin_object (builder);

        lat = geocode_location_get_latitude (location);
        json_builder_set_member_name (builder, "lat");
        json_builder_add_double_value (builder, lat);

        lon = geocode_location_get_longitude (location);
        json_builder_set_member_name (builder, "lon");
        json_builder_add_double_value (builder, lon);

        accuracy = geocode_location_get_accuracy (location);
        if (accuracy != GEOCODE_LOCATION_ACCURACY_UNKNOWN) {
                json_builder_set_member_name (builder, "accuracy");
                json_builder_add_double_value (builder, accuracy);
        }

        altitude = geocode_location_get_altitude (location);
        if (altitude != GEOCODE_LOCATION_ALTITUDE_UNKNOWN) {
                json_builder_set_member_name (builder, "altitude");
                json_builder_add_double_value (builder, altitude);
        }

        tv.tv_sec = geocode_location_get_timestamp (location);
        tv.tv_usec = 0;
        timestamp = g_time_val_to_iso8601 (&tv);
        json_builder_set_member_name (builder, "time");
        json_builder_add_string_value (builder, timestamp);
        g_free (timestamp);

        json_builder_set_member_name (builder, "wifi");
        json_builder_begin_array (builder);

        for (iter = bss_list; iter != NULL; iter = iter->next) {
                WPABSS *bss = WPA_BSS (iter->data);
                char *mac;
                gint16 strength_dbm;
                guint16 frequency;

                if (should_ignore_bss (bss))
                        continue;

                json_builder_begin_object (builder);
                json_builder_set_member_name (builder, "key");
                mac = get_bssid_from_bss (bss);
                json_builder_add_string_value (builder, mac);
                g_free (mac);

                json_builder_set_member_name (builder, "signal");
                strength_dbm = wpa_bss_get_signal (bss);
                json_builder_add_int_value (builder, strength_dbm);

                json_builder_set_member_name (builder, "frequency");
                frequency = wpa_bss_get_frequency (bss);
                json_builder_add_int_value (builder, frequency);
                json_builder_end_object (builder);
        }

        json_builder_end_array (builder); /* wifi */
        json_builder_end_object (builder);
        json_builder_end_array (builder); /* items */
        json_builder_end_object (builder);

        generator = json_generator_new ();
        root_node = json_builder_get_root (builder);
        json_generator_set_root (generator, root_node);
        data = json_generator_to_data (generator, &data_len);

        json_node_free (root_node);
        g_object_unref (builder);
        g_object_unref (generator);

        ret = soup_message_new ("POST", url);
        if (nick != NULL && nick[0] != '\0')
                soup_message_headers_append (ret->request_headers,
                                             "X-Nickname",
                                             nick);
        soup_message_set_request (ret,
                                  "application/json",
                                  SOUP_MEMORY_TAKE,
                                  data,
                                  data_len);
        g_debug ("Sending following request to '%s':\n%s", url, data);

out:
        return ret;
}
