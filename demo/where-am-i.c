/* vim: set et ts=8 sw=8: */
/* where-am-i.c
 *
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#include <config.h>

#include <stdlib.h>
#include <glib.h>
#include <locale.h>
#include <glib/gi18n.h>
#include <geoclue.h>

GMainLoop *main_loop;

static void
on_location_proxy_ready (GObject      *source_object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
        GClueManagerProxy *manager = GCLUE_MANAGER_PROXY (user_data);
        GClueLocationProxy *location;
        const char *desc;
        GError *error = NULL;

        location = gclue_location_proxy_new_for_bus_finish (res, &error);
        if (error != NULL) {
            g_critical ("Failed to connect to GeoClue2 service: %s", error->message);

            exit (-5);
        }

        g_print ("Latitude: %f\nLongitude: %f\nAccuracy (in meters): %f\n",
                 gclue_location_get_latitude (location),
                 gclue_location_get_longitude (location),
                 gclue_location_get_accuracy (location));
        desc = gclue_location_get_description (location);
        if (desc != NULL && desc[0] != NULL)
                g_print ("Description: %s\n", desc);

        g_object_unref (location);
        g_object_unref (manager);

        g_main_loop_quit (main_loop);
}

static void
on_location_updated (GClueClient *client,
                     const char  *old_path,
                     const char  *new_path,
                     gpointer     user_data)
{
        gclue_location_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                          G_DBUS_PROXY_FLAGS_NONE,
                                          "org.freedesktop.GeoClue2",
                                          new_path,
                                          NULL,
                                          on_location_proxy_ready,
                                          user_data);
        g_object_unref (client);
}

static void
on_start_ready (GObject      *source_object,
                GAsyncResult *res,
                gpointer      user_data)
{
        GClueManagerProxy *manager = GCLUE_MANAGER_PROXY (user_data);
        GClueClientProxy *client = GCLUE_CLIENT_PROXY (source_object);
        GError *error = NULL;

        if (!gclue_client_call_start_finish (client, res, &error)) {
            g_critical ("Failed to start GeoClue2 client: %s", error->message);

            exit (-4);
        }
}

static void
on_client_proxy_ready (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      user_data)
{
        GClueClientProxy *client;
        GError *error = NULL;

        client = gclue_client_proxy_new_for_bus_finish (res, &error);
        if (error != NULL) {
            g_critical ("Failed to connect to GeoClue2 service: %s", error->message);

            exit (-3);
        }

        g_signal_connect (client, "location-updated", on_location_updated, user_data);

        gclue_client_call_start (client, NULL, on_start_ready, user_data);
}

static void
on_get_client_ready (GObject      *source_object,
                     GAsyncResult *res,
                     gpointer      user_data)
{
        GClueManagerProxy *manager = GCLUE_MANAGER_PROXY (source_object);
        const char *client_path;
        GError *error = NULL;

        if (!gclue_manager_call_get_client_finish (manager, &client_path, res, &error)) {
            g_critical ("Failed to connect to GeoClue2 service: %s", error->message);

            exit (-2);
        }

        g_print ("Client object: %s\n", client_path);

        gclue_client_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                        G_DBUS_PROXY_FLAGS_NONE,
                                        "org.freedesktop.GeoClue2",
                                        client_path,
                                        NULL,
                                        on_client_proxy_ready,
                                        manager);
}

static void
on_manager_proxy_ready (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      user_data)
{
        GClueManagerProxy *manager;
        GError *error = NULL;

        manager = gclue_manager_proxy_new_for_bus_finish (res, &error);
        if (error != NULL) {
            g_critical ("Failed to connect to GeoClue2 service: %s", error->message);

            exit (-1);
        }

        gclue_manager_call_get_client (manager,
                                       NULL,
                                       on_get_client_ready,
                                       NULL);
}

gint
main (gint argc, gchar *argv[])
{
        textdomain (GETTEXT_PACKAGE);
        bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        g_set_application_name (_("Where Am I"));

        gclue_manager_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         "org.freedesktop.GeoClue2",
                                         "/org/freedesktop/GeoClue2/Manager",
                                         NULL,
                                         on_manager_proxy_ready,
                                         NULL);

        main_loop = g_main_loop_new (NULL, FALSE);
        g_main_loop_run (main_loop);

        return EXIT_SUCCESS;
}
