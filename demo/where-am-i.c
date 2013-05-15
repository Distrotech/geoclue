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

gint
main (gint argc, gchar *argv[])
{
        GClueManagerProxy *manager;
        GClueClientProxy *client;
        const char *client_path;
        GError *error = NULL;

        textdomain (GETTEXT_PACKAGE);
        bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        g_set_application_name (_("Where Am I"));

        error = NULL;
        manager = gclue_manager_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                        G_DBUS_PROXY_FLAGS_NONE,
                                                        "org.freedesktop.GeoClue2",
                                                        "/org/freedesktop/GeoClue2/Manager",
                                                        NULL,
                                                        &error);
        if (error != NULL) {
            g_critical ("Failed to connect to GeoClue2 service: %s", error->message);

            exit (-1);
        }

        if (!gclue_manager_call_get_client_sync (manager,
                                                 &client_path,
                                                 NULL,
                                                 &error)) {
            g_critical ("Failed to connect to GeoClue2 service: %s", error->message);

            exit (-2);
        }

        g_print ("Client object: %s\n", client_path);

        client = gclue_client_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                      G_DBUS_PROXY_FLAGS_NONE,
                                                      "org.freedesktop.GeoClue2",
                                                      client_path,
                                                      NULL,
                                                      &error);
        if (error != NULL) {
            g_critical ("Failed to connect to GeoClue2 service: %s", error->message);

            exit (-3);
        }

        if (!gclue_client_call_start_sync (client, NULL, &error)) {
            g_critical ("Failed to start GeoClue2 client: %s", error->message);

            exit (-4);
        }

        g_object_unref (client);
        g_object_unref (manager);

        return EXIT_SUCCESS;
}
