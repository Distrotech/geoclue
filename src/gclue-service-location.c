/* vim: set et ts=8 sw=8: */
/* gclue-service-location.c
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

#include "gclue-service-location.h"

static void
gclue_service_location_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GClueServiceLocation,
                         gclue_service_location,
                         GCLUE_TYPE_LOCATION_SKELETON,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                gclue_service_location_initable_iface_init));

struct _GClueServiceLocationPrivate
{
        const char *path;
        GDBusConnection *connection;
};

enum
{
        PROP_0,
        PROP_PATH,
        PROP_CONNECTION,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static void
gclue_service_location_finalize (GObject *object)
{
        GClueServiceLocationPrivate *priv = GCLUE_SERVICE_LOCATION (object)->priv;

        g_clear_pointer (&priv->path, g_free);
        g_clear_object (&priv->connection);

        /* Chain up to the parent class */
        G_OBJECT_CLASS (gclue_service_location_parent_class)->finalize (object);
}

static void
gclue_service_location_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
        GClueServiceLocation *location = GCLUE_SERVICE_LOCATION (object);

        switch (prop_id) {
        case PROP_PATH:
                g_value_set_string (value, location->priv->path);
                break;

        case PROP_CONNECTION:
                g_value_set_object (value, location->priv->connection);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_service_location_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
        GClueServiceLocation *location = GCLUE_SERVICE_LOCATION (object);

        switch (prop_id) {
        case PROP_PATH:
                location->priv->path = g_value_dup_string (value);
                break;

        case PROP_CONNECTION:
                location->priv->connection = g_value_dup_object (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_service_location_class_init (GClueServiceLocationClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = gclue_service_location_finalize;
        object_class->get_property = gclue_service_location_get_property;
        object_class->set_property = gclue_service_location_set_property;

        g_type_class_add_private (object_class, sizeof (GClueServiceLocationPrivate));

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
gclue_service_location_init (GClueServiceLocation *location)
{
        location->priv = G_TYPE_INSTANCE_GET_PRIVATE (location,
                                                      GClUE_TYPE_SERVICE_LOCATION,
                                                      GClueServiceLocationPrivate);
}

static gboolean
gclue_service_location_initable_init (GInitable    *initable,
                                      GCancellable *cancellable,
                                      GError      **error)
{
        return g_dbus_interface_skeleton_export
                (G_DBUS_INTERFACE_SKELETON (initable),
                 GCLUE_SERVICE_LOCATION (initable)->priv->connection,
                 GCLUE_SERVICE_LOCATION (initable)->priv->path,
                 error);
}

static void
gclue_service_location_initable_iface_init (GInitableIface *iface)
{
        iface->init = gclue_service_location_initable_init;
}

GClueServiceLocation *
gclue_service_location_new (const char      *path,
                            GDBusConnection *connection,
                            gdouble          latitude,
                            gdouble          longitude,
                            gdouble          accuracy,
                            GError         **error)
{
        return g_initable_new (GClUE_TYPE_SERVICE_LOCATION,
                               NULL,
                               error,
                               "path", path,
                               "connection", connection,
                               "latitude", latitude,
                               "longitude", longitude,
                               "accuracy", accuracy,
                               NULL);
}

const gchar *
gclue_service_location_get_path (GClueServiceLocation *location)
{
        g_return_val_if_fail (GClUE_IS_SERVICE_LOCATION(location), NULL);

        return location->priv->path;
}
