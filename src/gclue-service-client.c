/* vim: set et ts=8 sw=8: */
/* gclue-service-client.c
 *
 * Copyright (C) 2013 Red Hat, Inc.
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
 * Author: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#include <glib/gi18n.h>

#include "gclue-service-client.h"
#include "gclue-service-location.h"
#include "gclue-locator.h"

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
        guint threshold;

        GClueLocator *locator;

        /* Number of times location has been updated */
        guint locations_updated;

        guint watch_id;
        gulong location_change_id;
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

enum {
        PEER_VANISHED,
        SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

static char *
next_location_path (GClueServiceClient *client)
{
        GClueServiceClientPrivate *priv = client->priv;
        char *path, *index_str;

        index_str = g_strdup_printf ("%u", (priv->locations_updated)++),
        path = g_strjoin ("/", priv->path, "Location", index_str, NULL);
        g_free (index_str);

        return path;
}

/* We don't use the gdbus-codegen provided gclue_client_emit_location_updated()
 * as that sends the signal to all listeners on the bus
 */
static gboolean
emit_location_updated (GClueServiceClient *client,
                       const char         *old,
                       const char         *new,
                       GError            **error)
{
        GClueServiceClientPrivate *priv = client->priv;
        GVariant *variant;

        variant = g_variant_new ("(oo)", old, new);

        return g_dbus_connection_emit_signal (priv->connection,
                                              priv->peer,
                                              priv->path,
                                              "org.freedesktop.GeoClue2.Client",
                                              "LocationUpdated",
                                              variant,
                                              error);
}

static gboolean
below_threshold (GClueServiceClient *client,
                 GClueLocationInfo  *location)
{
        GClueServiceClientPrivate *priv = client->priv;
        GClueLocationInfo *cur_location;
        gdouble distance;
        gdouble threshold_km;

        if (priv->threshold == 0)
                return FALSE;

        g_object_get (priv->location,
                      "location", &cur_location,
                      NULL);
        distance = gclue_location_info_get_distance_from (cur_location,
                                                          location);
        g_object_unref (cur_location);

        threshold_km = priv->threshold / 1000.0;
        if (distance < threshold_km)
                return TRUE;

        return FALSE;
}

static void
on_locator_location_changed (GObject    *gobject,
                             GParamSpec *pspec,
                             gpointer    user_data)
{
        GClueServiceClient *client = GCLUE_SERVICE_CLIENT (user_data);
        GClueServiceClientPrivate *priv = client->priv;
        GClueLocator *locator = GCLUE_LOCATOR (gobject);
        GClueLocationInfo *location_info;
        char *path = NULL;
        const char *prev_path;
        GError *error = NULL;

        location_info = gclue_locator_get_location (locator);

        if (priv->location != NULL && below_threshold (client, location_info)) {
                g_debug ("Updating location, below threshold");
                g_object_set (priv->location,
                              "location", location_info,
                              NULL);
                return;
        }

        g_clear_object (&priv->prev_location);
        priv->prev_location = priv->location;

        path = next_location_path (client);
        priv->location = gclue_service_location_new (priv->peer,
                                                     path,
                                                     priv->connection,
                                                     location_info,
                                                     &error);
        if (priv->location == NULL)
                goto error_out;

        if (priv->prev_location != NULL)
                prev_path = gclue_service_location_get_path (priv->prev_location);
        else
                prev_path = "/";

        gclue_client_set_location (GCLUE_CLIENT (client), path);

        if (!emit_location_updated (client, prev_path, path, &error))
                goto error_out;
        goto out;

error_out:
        g_warning ("Failed to update location info: %s", error->message);
        g_error_free (error);
out:
        g_free (path);
}

typedef struct
{
        GClueServiceClient *client;
        GDBusMethodInvocation *invocation;
} StartData;

static void
on_start_ready (GObject      *source_object,
                GAsyncResult *res,
                gpointer      user_data)
{
        StartData *data = (StartData *) user_data;
        GClueLocator *locator = GCLUE_LOCATOR (source_object);
        GError *error = NULL;

        if (!gclue_locator_start_finish (locator, res, &error))
                goto error_out;

        gclue_client_complete_start (GCLUE_CLIENT (data->client),
                                     data->invocation);
        goto out;

error_out:
        g_dbus_method_invocation_return_error (data->invocation,
                                               G_DBUS_ERROR,
                                               G_DBUS_ERROR_FAILED,
                                               "Failed to start: %s",
                                               error->message);
        g_error_free (error);

out:
        g_object_unref (data->client);
        g_object_unref (data->invocation);
        g_slice_free (StartData, data);
}

static gboolean
gclue_service_client_handle_start (GClueClient           *client,
                                   GDBusMethodInvocation *invocation)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (client)->priv;
        StartData *data;

        if (priv->location_change_id)
                /* Already started */
                return TRUE;

        /* TODO: We need some kind of mechanism to ask user if location info can
         *       be shared with peer app.
         */

        priv->location_change_id =
                g_signal_connect (priv->locator,
                                  "notify::location",
                                  G_CALLBACK (on_locator_location_changed),
                                  client);

        data = g_slice_new (StartData);
        data->client = g_object_ref (client);
        data->invocation =  g_object_ref (invocation);
        gclue_locator_start (priv->locator,
                             NULL,
                             on_start_ready,
                             data);

        return TRUE;
}

static gboolean
gclue_service_client_handle_stop (GClueClient           *client,
                                  GDBusMethodInvocation *invocation)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (client)->priv;

        if (priv->location_change_id != 0) {
                g_signal_handler_disconnect (priv->locator,
                                             priv->location_change_id);
                priv->location_change_id = 0;
        }

        gclue_client_complete_stop (client, invocation);

        return TRUE;
}

static void
gclue_service_client_finalize (GObject *object)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (object)->priv;

        if (priv->location_change_id != 0) {
                g_signal_handler_disconnect (priv->locator,
                                             priv->location_change_id);
                priv->location_change_id = 0;
        }

        g_clear_pointer (&priv->peer, g_free);
        g_clear_pointer (&priv->path, g_free);
        g_clear_object (&priv->connection);
        g_clear_object (&priv->locator);
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
gclue_service_client_handle_method_call (GDBusConnection       *connection,
                                         const gchar           *sender,
                                         const gchar           *object_path,
                                         const gchar           *interface_name,
                                         const gchar           *method_name,
                                         GVariant              *parameters,
                                         GDBusMethodInvocation *invocation,
                                         gpointer               user_data)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (user_data)->priv;
        GDBusInterfaceSkeletonClass *skeleton_class;
        GDBusInterfaceVTable *skeleton_vtable;

        if (strcmp (sender, priv->peer) != 0) {
                g_dbus_method_invocation_return_error (invocation,
                                                       G_DBUS_ERROR,
                                                       G_DBUS_ERROR_ACCESS_DENIED,
                                                       "Access denied");
                return;
        }

        skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS (gclue_service_client_parent_class);
        skeleton_vtable = skeleton_class->get_vtable (G_DBUS_INTERFACE_SKELETON (user_data));
        skeleton_vtable->method_call (connection,
                                      sender,
                                      object_path,
                                      interface_name,
                                      method_name,
                                      parameters,
                                      invocation,
                                      user_data);
}

static GVariant *
gclue_service_client_handle_get_property (GDBusConnection *connection,
                                          const gchar     *sender,
                                          const gchar     *object_path,
                                          const gchar     *interface_name,
                                          const gchar     *property_name,
                                          GError         **error,
                                          gpointer        user_data)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (user_data)->priv;
        GDBusInterfaceSkeletonClass *skeleton_class;
        GDBusInterfaceVTable *skeleton_vtable;

        if (strcmp (sender, priv->peer) != 0) {
                g_set_error (error,
                             G_DBUS_ERROR,
                             G_DBUS_ERROR_ACCESS_DENIED,
                             "Access denied");
                return NULL;
        }

        skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS (gclue_service_client_parent_class);
        skeleton_vtable = skeleton_class->get_vtable (G_DBUS_INTERFACE_SKELETON (user_data));
        return skeleton_vtable->get_property (connection,
                                              sender,
                                              object_path,
                                              interface_name,
                                              property_name,
                                              error,
                                              user_data);
}

static gboolean
gclue_service_client_handle_set_property (GDBusConnection *connection,
                                          const gchar     *sender,
                                          const gchar     *object_path,
                                          const gchar     *interface_name,
                                          const gchar     *property_name,
                                          GVariant        *variant,
                                          GError         **error,
                                          gpointer        user_data)
{
        GClueClient *client = GCLUE_CLIENT (user_data);
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (client)->priv;
        GDBusInterfaceSkeletonClass *skeleton_class;
        GDBusInterfaceVTable *skeleton_vtable;
        gboolean ret;

        if (strcmp (sender, priv->peer) != 0) {
                g_set_error (error,
                             G_DBUS_ERROR,
                             G_DBUS_ERROR_ACCESS_DENIED,
                             "Access denied");
                return FALSE;
        }

        skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS (gclue_service_client_parent_class);
        skeleton_vtable = skeleton_class->get_vtable (G_DBUS_INTERFACE_SKELETON (user_data));
        ret = skeleton_vtable->set_property (connection,
                                             sender,
                                             object_path,
                                             interface_name,
                                             property_name,
                                             variant,
                                             error,
                                             user_data);
        if (ret && strcmp (property_name, "DistanceThreshold") == 0) {
                priv->threshold = gclue_client_get_distance_threshold (client);
                g_debug ("New distance threshold: %u", priv->threshold);
        }

        return ret;
}

static const GDBusInterfaceVTable gclue_service_client_vtable =
{
        gclue_service_client_handle_method_call,
        gclue_service_client_handle_get_property,
        gclue_service_client_handle_set_property,
        {NULL}
};

static GDBusInterfaceVTable *
gclue_service_client_get_vtable (GDBusInterfaceSkeleton *skeleton G_GNUC_UNUSED)
{
        return (GDBusInterfaceVTable *) &gclue_service_client_vtable;
}

static void
gclue_service_client_class_init (GClueServiceClientClass *klass)
{
        GObjectClass *object_class;
        GDBusInterfaceSkeletonClass *skeleton_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = gclue_service_client_finalize;
        object_class->get_property = gclue_service_client_get_property;
        object_class->set_property = gclue_service_client_set_property;

        skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS (klass);
        skeleton_class->get_vtable = gclue_service_client_get_vtable;

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

        signals[PEER_VANISHED] =
                g_signal_new ("peer-vanished",
                              GCLUE_TYPE_SERVICE_CLIENT,
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GClueServiceClientClass,
                                               peer_vanished),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0,
                              G_TYPE_NONE);
}

static void
gclue_service_client_client_iface_init (GClueClientIface *iface)
{
        iface->handle_start = gclue_service_client_handle_start;
        iface->handle_stop = gclue_service_client_handle_stop;
}

static void
on_name_vanished (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
        g_signal_emit (GCLUE_SERVICE_CLIENT (user_data),
                       signals[PEER_VANISHED],
                       0);

        g_object_unref (G_OBJECT (user_data));
}

static gboolean
gclue_service_client_initable_init (GInitable    *initable,
                                    GCancellable *cancellable,
                                    GError      **error)
{
        GClueServiceClientPrivate *priv = GCLUE_SERVICE_CLIENT (initable)->priv;

        if (!g_dbus_interface_skeleton_export
                                (G_DBUS_INTERFACE_SKELETON (initable),
                                 GCLUE_SERVICE_CLIENT (initable)->priv->connection,
                                 GCLUE_SERVICE_CLIENT (initable)->priv->path,
                                 error))
                return FALSE;

        priv->watch_id = g_bus_watch_name_on_connection (priv->connection,
                                                         priv->peer,
                                                         G_BUS_NAME_WATCHER_FLAGS_NONE, 
                                                         NULL,
                                                         on_name_vanished,
                                                         g_object_ref (initable),
                                                         g_object_unref);
        return TRUE;
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
                                                    GCLUE_TYPE_SERVICE_CLIENT,
                                                    GClueServiceClientPrivate);

        client->priv->locator = gclue_locator_new ();
}

GClueServiceClient *
gclue_service_client_new (const char      *peer,
                          const char      *path,
                          GDBusConnection *connection,
                          GError         **error)
{
        return g_initable_new (GCLUE_TYPE_SERVICE_CLIENT,
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
        g_return_val_if_fail (GCLUE_IS_SERVICE_CLIENT(client), NULL);

        return client->priv->path;
}

const gchar *
gclue_service_client_get_peer (GClueServiceClient *client)
{
        g_return_val_if_fail (GCLUE_IS_SERVICE_CLIENT(client), NULL);

        return client->priv->peer;
}
