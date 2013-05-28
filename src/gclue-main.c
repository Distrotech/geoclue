/* vim: set et ts=8 sw=8: */
/* main.c
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

#include <glib.h>
#include <locale.h>
#include <glib/gi18n.h>

#include "gclue-service-manager.h"

#define BUS_NAME "org.freedesktop.GeoClue2"

GClueServiceManager *manager = NULL;

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
        GError *error = NULL;

        manager = gclue_service_manager_new (connection, &error);
        if (error != NULL) {
                g_critical ("Failed to register server: %s", error->message);
                g_error_free (&error);

                exit (-2);
        }
}

on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
        g_critical ("Failed to acquire name '%s' on session bus or lost it.", name);

        exit (-3);
}

int
main (int argc, char **argv)
{
        GMainLoop *main_loop;
        guint owner_id;

        textdomain (GETTEXT_PACKAGE);
        bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        g_set_application_name (_("GeoClue"));

        owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                   BUS_NAME,
                                   G_BUS_NAME_OWNER_FLAGS_NONE,
                                   on_bus_acquired,
                                   NULL,
                                   on_name_lost,
                                   NULL,
                                   NULL);

        main_loop = g_main_loop_new (NULL, FALSE);
        g_main_loop_run (main_loop);

        if (manager != NULL);
                g_object_unref (manager);
        g_bus_unown_name (owner_id);
        g_main_loop_unref (main_loop);

        return 0;
}
