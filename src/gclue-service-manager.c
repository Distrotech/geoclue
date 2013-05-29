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
gclue_service_manager_manager_iface_init (GClueManagerIface *iface);
static void
gclue_service_manager_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GClueServiceManager,
                         gclue_service_manager,
                         GCLUE_TYPE_MANAGER_SKELETON,
                         G_IMPLEMENT_INTERFACE (GCLUE_TYPE_MANAGER,
                                                gclue_service_manager_manager_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                gclue_service_manager_initable_iface_init))

struct _GClueServiceManagerPrivate
{
        GDBusConnection *connection;
        GHashTable *clients;

        guint num_clients;
};

enum
{
        PROP_0,
        PROP_CONNECTION,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static gboolean
gclue_service_manager_handle_get_client (GClueServiceManager   *manager,
                                         GDBusMethodInvocation *invocation)
{
        GClueServiceManagerPrivate *priv = manager->priv;
        GClueServiceClient *client;
        char *path;
        GError *error = NULL;

        path = g_strdup_printf ("/org/freedesktop/GeoClue2/Client/%llu",
                                ++priv->num_clients);

        client = g_hash_table_lookup (priv->clients, path);
        if (client != NULL) {
                gclue_manager_complete_get_client (manager, invocation, path);

                return TRUE;
        }

        client = gclue_service_client_new (path, priv->connection, &error);
        if (error != NULL) {
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
gclue_service_manager_finalize (GObject *object)
{
        GClueServiceManagerPrivate *priv = GCLUE_SERVICE_MANAGER (object)->priv;

        g_clear_object (&priv->connection);
        g_clear_pointer (&priv->clients, g_hash_table_unref);

        /* Chain up to the parent class */
        G_OBJECT_CLASS (gclue_service_manager_parent_class)->finalize (object);
}

static void
gclue_service_manager_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
        GClueServiceManager *manager = GCLUE_SERVICE_MANAGER (object);

        switch (prop_id) {
        case PROP_CONNECTION:
                g_value_set_object (value, manager->priv->connection);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_service_manager_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
        GClueServiceManager *manager = GCLUE_SERVICE_MANAGER (object);

        switch (prop_id) {
        case PROP_CONNECTION:
                manager->priv->connection = g_value_dup_object (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}


static void
gclue_service_manager_class_init (GClueServiceManagerClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = gclue_service_manager_finalize;
        object_class->get_property = gclue_service_manager_get_property;
        object_class->set_property = gclue_service_manager_set_property;

        g_type_class_add_private (object_class, sizeof (GClueServiceManagerPrivate));

        gParamSpecs[PROP_CONNECTION] = g_param_spec_object ("connection",
                                                            "Connection",
                                                            "DBus Connection",
                                                            G_TYPE_DBUS_CONNECTION,
                                                            G_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_CONNECTION,
                                         gParamSpecs[PROP_CONNECTION]);
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

static gboolean
gclue_service_manager_initable_init (GInitable    *initable,
                                     GCancellable *cancellable,
                                     GError      **error)
{
        return g_dbus_interface_skeleton_export
                (G_DBUS_INTERFACE_SKELETON (initable),
                 GCLUE_SERVICE_MANAGER (initable)->priv->connection,
                 "/org/freedesktop/GeoClue2/Manager",
                 error);
}

static void
gclue_service_manager_manager_iface_init (GClueManagerIface *iface)
{
        iface->handle_get_client = gclue_service_manager_handle_get_client;
}

static void
gclue_service_manager_initable_iface_init (GInitableIface *iface)
{
        iface->init = gclue_service_manager_initable_init;
}

GClueServiceManager *
gclue_service_manager_new (GDBusConnection *connection,
                           GError         **error)
{
        return g_initable_new (GClUE_TYPE_SERVICE_MANAGER,
                               NULL,
                               error,
                               "connection", connection,
                               NULL);
}
