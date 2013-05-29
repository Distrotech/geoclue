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
#include "gclue-service-location.h"

static void
gclue_service_client_client_iface_init (GClueClientIface *iface);
static void
gclue_service_client_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GClueServiceClient,
                         gclue_service_client,
                         GCLUE_TYPE_CLIENT_SKELETON,
                         G_IMPLEMENT_INTERFACE (GCLUE_TYPE_CLIENT,
                                                gclue_service_client_client_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                gclue_service_client_initable_iface_init));

struct _GClueServiceClientPrivate
{
        const char *peer;
        const char *path;
        GDBusConnection *connection;

        GClueServiceLocation *location;
        GClueServiceLocation *prev_location;

        /* Number of times location has been updated */
        guint locations_updated;
};

enum
{
        PROP_0,
        PROP_PEER,
        PROP_PATH,
        PROP_CONNECTION,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static char *
next_location_path (GClueServiceClient *client)
{
        GClueServiceClientPrivate *priv = client->priv;
        char *path, *index_str;

        index_str = g_strdup_printf ("%llu", (priv->locations_updated)++),
        path = g_strjoin ("/", priv->path, "Location", index_str, NULL);
        g_free (index_str);

        return path;
}

static void
set_location (GClueServiceClient   *client,
              GClueServiceLocation *location,
              const char           *path)
{
        GClueServiceClientPrivate *priv = client->priv;
        const char *prev_path;

        if (priv->prev_location != NULL)
                g_object_unref (priv->prev_location);
        priv->prev_location = priv->location;
        if (priv->location != NULL)
                prev_path = gclue_service_location_get_path (priv->location);
        else
                prev_path = "/";
        priv->location = location;

        gclue_client_set_location (client, path);
        gclue_client_emit_location_updated (client, prev_path, path);
}

// FIXME: This function should be async
static gboolean
update_location (GClueServiceClient *client,
                 GError            **error)
{
        GClueServiceClientPrivate *priv = client->priv;
        GClueServiceLocation *location;
        char *path;

        path = next_location_path (client);
        location = gclue_service_location_new (priv->peer,
                                               path,
                                               priv->connection,
                                               0, 0, 0,
                                               error);
        if (*error != NULL) {
                g_free (path);

                return FALSE;
        }

        set_location (client, location, path);

        g_free (path);

        return TRUE;
}

static gboolean
gclue_service_client_handle_start (GClueClient           *client,
                                   GDBusMethodInvocation *invocation,
                                   gpointer               user_data)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (client)->priv;
        GError *error = NULL;

        if (strcmp (g_dbus_method_invocation_get_sender (invocation),
                    GCLUE_SERVICE_CLIENT (client)->priv->peer) != 0) {
                g_dbus_method_invocation_return_dbus_error (invocation,
                                                            "Access denied",
                                                            "Access denied");
                return TRUE;
        }

        if (!update_location (GCLUE_SERVICE_CLIENT (client), &error)) {
                g_dbus_method_invocation_return_dbus_error (invocation,
                                                            "Failed to update location info",
                                                            error->message);
                return TRUE;
        }

        gclue_client_complete_start (client, invocation);

        return TRUE;
}

static gboolean
gclue_service_client_handle_stop (GClueClient           *client,
                                  GDBusMethodInvocation *invocation,
                                  gpointer               user_data)
{
        if (strcmp (g_dbus_method_invocation_get_sender (invocation),
                    GCLUE_SERVICE_CLIENT (client)->priv->peer) != 0) {
                g_dbus_method_invocation_return_dbus_error (invocation,
                                                            "Access denied",
                                                            "Access denied");
                return TRUE;
        }

        gclue_client_complete_stop (client, invocation);

        return TRUE;
}

static void
gclue_service_client_finalize (GObject *object)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (object)->priv;

        g_clear_pointer (&priv->peer, g_free);
        g_clear_pointer (&priv->path, g_free);
        g_clear_object (&priv->connection);
        g_clear_object (&priv->location);
        g_clear_object (&priv->prev_location);

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
        case PROP_PEER:
                g_value_set_string (value, client->priv->peer);
                break;

        case PROP_PATH:
                g_value_set_string (value, client->priv->path);
                break;

        case PROP_CONNECTION:
                g_value_set_object (value, client->priv->connection);
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
        case PROP_PEER:
                client->priv->peer = g_value_dup_string (value);
                break;

        case PROP_PATH:
                client->priv->path = g_value_dup_string (value);
                break;

        case PROP_CONNECTION:
                client->priv->connection = g_value_dup_object (value);
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

        gParamSpecs[PROP_PEER] = g_param_spec_string ("peer",
                                                      "Peer",
                                                      "Bus name of client app",
                                                      NULL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_PEER,
                                         gParamSpecs[PROP_PEER]);

        gParamSpecs[PROP_PATH] = g_param_spec_string ("path",
                                                      "Path",
                                                      "Path",
                                                      NULL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_PATH,
                                         gParamSpecs[PROP_PATH]);

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
gclue_service_client_client_iface_init (GClueClientIface *iface)
{
        iface->handle_start = gclue_service_client_handle_start;
        iface->handle_stop = gclue_service_client_handle_stop;
}

static gboolean
gclue_service_client_initable_init (GInitable    *initable,
                                    GCancellable *cancellable,
                                    GError      **error)
{
        return g_dbus_interface_skeleton_export
                                (G_DBUS_INTERFACE_SKELETON (initable),
                                 GCLUE_SERVICE_CLIENT (initable)->priv->connection,
                                 GCLUE_SERVICE_CLIENT (initable)->priv->path,
                                 error);

}

static void
gclue_service_client_initable_iface_init (GInitableIface *iface)
{
        iface->init = gclue_service_client_initable_init;
}

static void
gclue_service_client_init (GClueServiceClient *client)
{
        client->priv = G_TYPE_INSTANCE_GET_PRIVATE (client,
                                                    GClUE_TYPE_SERVICE_CLIENT,
                                                    GClueServiceClientPrivate);
}

GClueServiceClient *
gclue_service_client_new (const char      *peer,
                          const char      *path,
                          GDBusConnection *connection,
                          GError         **error)
{
        return g_initable_new (GClUE_TYPE_SERVICE_CLIENT,
                               NULL,
                               error,
                               "peer", peer,
                               "path", path,
                               "connection", connection,
                               NULL);
}

const gchar *
gclue_service_client_get_path (GClueServiceClient *client)
{
        g_return_val_if_fail (GClUE_IS_SERVICE_CLIENT(client), NULL);

        return client->priv->path;
}
