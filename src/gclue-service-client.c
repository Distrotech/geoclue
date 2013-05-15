/* vim: set et ts=8 sw=8: */
/* gclue-service-client.c
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

#include "gclue-service-client.h"

static void
gclue_client_init (GClueClientIface *iface);

G_DEFINE_TYPE_WITH_CODE (GClueServiceClient,
                         gclue_service_client,
                         GCLUE_TYPE_CLIENT_SKELETON,
                         G_IMPLEMENT_INTERFACE (GCLUE_TYPE_CLIENT,
                                                gclue_client_init))

struct _GClueServiceClientPrivate
{
        const char *path;
};

enum
{
        PROP_0,
        PROP_PATH,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static gboolean
gclue_service_client_handle_start (GClueClient           *client,
                                   GDBusMethodInvocation *invocation,
                                   gpointer               user_data)
{
        gclue_client_complete_start (client, invocation);

        return TRUE;
}

static gboolean
gclue_service_client_handle_stop (GClueClient           *client,
                                  GDBusMethodInvocation *invocation,
                                  gpointer               user_data)
{
        gclue_client_complete_stop (client, invocation);

        return TRUE;
}

static void
gclue_service_client_finalize (GObject *object)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (object)->priv;

        if (priv->path) {
                g_free (priv->path);
                priv->path = NULL;
        }

        /* Chain up to the parent class */
        G_OBJECT_CLASS (gclue_service_client_parent_class)->finalize (object);
}

static void
gclue_service_client_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
        GClueServiceClient *client = GCLUE_SERVICE_CLIENT (object);

        switch (prop_id) {
        case PROP_PATH:
                g_value_set_string (value, client->priv->path);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_service_client_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
        GClueServiceClient *client = GCLUE_SERVICE_CLIENT (object);

        switch (prop_id) {
        case PROP_PATH:
                client->priv->path = g_value_dup_string (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_service_client_class_init (GClueServiceClientClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = gclue_service_client_finalize;
        object_class->get_property = gclue_service_client_get_property;
        object_class->set_property = gclue_service_client_set_property;

        g_type_class_add_private (object_class, sizeof (GClueServiceClientPrivate));

        gParamSpecs[PROP_PATH] = g_param_spec_string ("path",
                                                      "Path",
                                                      "Path",
                                                      NULL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_PATH,
                                         gParamSpecs[PROP_PATH]);
}

static void
gclue_client_init (GClueClientIface *iface)
{
        iface->handle_start = gclue_service_client_handle_start;
        iface->handle_stop = gclue_service_client_handle_stop;
}

static void
gclue_service_client_init (GClueServiceClient *client)
{
        client->priv = G_TYPE_INSTANCE_GET_PRIVATE (client,
                                                    GClUE_TYPE_SERVICE_CLIENT,
                                                    GClueServiceClientPrivate);
}

GClueServiceClient *
gclue_service_client_new (const char *path)
{
        return g_object_new (GClUE_TYPE_SERVICE_CLIENT,
                             "path", path,
                             NULL);
}

gboolean
gclue_service_client_register (GClueServiceClient *client,
                               GDBusConnection    *connection,
                               GError            **error)
{
        return (g_dbus_connection_register_object
                        (connection,
                         gclue_service_client_get_path (client),
                         g_dbus_interface_skeleton_get_info (client),
                         g_dbus_interface_skeleton_get_vtable (client),
                         client,
                         NULL,
                         error) != 0);
}

const gchar *
gclue_service_client_get_path (GClueServiceClient *client)
{
        g_return_val_if_fail (GClUE_IS_SERVICE_CLIENT(client), NULL);

        return client->priv->path;
}
