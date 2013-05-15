/* vim: set et ts=8 sw=8: */
/* gclue-service-manager.c
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

#include <glib/gi18n.h>

#include "gclue-service-manager.h"
#include "gclue-service-client.h"

static void
gclue_manager_init (GClueManagerIface *iface);

G_DEFINE_TYPE_WITH_CODE (GClueServiceManager,
                         gclue_service_manager,
                         GCLUE_TYPE_MANAGER_SKELETON,
                         G_IMPLEMENT_INTERFACE (GCLUE_TYPE_MANAGER,
                                                gclue_manager_init))

struct _GClueServiceManagerPrivate
{
        GHashTable *clients;
};

enum
{
        PROP_0,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

char *
create_client_path_from_bus_name (const char *name)
{
        char *path;
        char *path_name;
        guint i, j;
        guint name_len;

        name_len = strlen (name);
        path_name = g_malloc (name_len);
        for (i = 0, j = 0; i < name_len; i++) {
                if (isalnum (name[i]) || name[i] == '_') {
                    path_name[j] = name[i];
                    j++;
                }
        }

        path = g_strdup_printf ("/org/freedesktop/GeoClue2/Client/%s",
                                path_name);
        g_free (path_name);

        return path;
}

static gboolean
gclue_service_manager_handle_get_client (GClueServiceManager   *manager,
                                         GDBusMethodInvocation *invocation)
{
        GClueServiceManagerPrivate *priv = manager->priv;
        GClueServiceClient *client;
        char *path;
        GError *error = NULL;

        path = create_client_path_from_bus_name
                (g_dbus_method_invocation_get_sender (invocation));

        client = g_hash_table_lookup (priv->clients, path);
        if (client != NULL) {
                gclue_manager_complete_get_client (manager, invocation, path);

                return TRUE;
        }

        client = gclue_service_client_new (path);
        if (!gclue_service_client_register
                        (client,
                         g_dbus_method_invocation_get_connection (invocation),
                         &error)) {
                g_dbus_method_invocation_return_dbus_error (invocation,
                                                            "Object registration failure",
                                                            error->message);
                return TRUE;
        }

        /* FIXME: Register to client-side bus name changes to free clients on associated app exitting */
        g_hash_table_insert (priv->clients, path, client);

        gclue_manager_complete_get_client (manager, invocation, path);

        return TRUE;
}

static void
gclue_service_manager_class_init (GClueServiceManagerClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (object_class, sizeof (GClueServiceManagerPrivate));
}

static void
gclue_service_manager_init (GClueServiceManager *manager)
{
        manager->priv = G_TYPE_INSTANCE_GET_PRIVATE (manager,
                                                     GClUE_TYPE_SERVICE_MANAGER,
                                                     GClueServiceManagerPrivate);

        manager->priv->clients = g_hash_table_new_full (g_str_hash,
                                                        g_str_equal,
                                                        g_free,
                                                        g_object_unref);
}

static void
gclue_manager_init (GClueManagerIface *iface)
{
        iface->handle_get_client = gclue_service_manager_handle_get_client;
}

GClueServiceManager *
gclue_service_manager_new (void)
{
        return g_object_new (GClUE_TYPE_SERVICE_MANAGER, NULL);
}

gboolean
gclue_service_manager_export (GClueServiceManager *manager,
                              GDBusConnection     *connection,
                              GError             **error)
{
        return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (manager),
                                                 connection,
                                                 "/org/freedesktop/GeoClue2/Manager",
                                                 error);
}
