/* vim: set et ts=8 sw=8: */
/* gclue-locator.c
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
 * Authors: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#include <glib/gi18n.h>

#include "gclue-locator.h"
#include "gclue-ipclient.h"

/* This class will be responsible for doing the actual geolocating. */

G_DEFINE_TYPE (GClueLocator, gclue_locator, G_TYPE_OBJECT)

struct _GClueLocatorPrivate
{
        GClueIpclient *ipclient;

        GClueLocationInfo *location;

        GCancellable *cancellable;

        gulong network_changed_id;
};

enum
{
        PROP_0,
        PROP_LOCATION,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static void
gclue_locator_stop_sync (GClueLocator *locator);

static void
gclue_locator_finalize (GObject *object)
{
        GClueLocatorPrivate *priv;

        priv = GCLUE_LOCATOR (object)->priv;

        gclue_locator_stop_sync (GCLUE_LOCATOR (object));
        g_clear_object (&priv->ipclient);
        g_clear_object (&priv->location);

        G_OBJECT_CLASS (gclue_locator_parent_class)->finalize (object);
}

static void
gclue_locator_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
        GClueLocator *locator = GCLUE_LOCATOR (object);

        switch (prop_id) {
        case PROP_LOCATION:
                g_value_set_object (value, locator->priv->location);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_locator_class_init (GClueLocatorClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = gclue_locator_finalize;
        object_class->get_property = gclue_locator_get_property;
        g_type_class_add_private (object_class, sizeof (GClueLocatorPrivate));

        gParamSpecs[PROP_LOCATION] = g_param_spec_object ("location",
                                                          "Location",
                                                          "Location",
                                                          GCLUE_TYPE_LOCATION_INFO,
                                                          G_PARAM_READABLE);
        g_object_class_install_property (object_class,
                                         PROP_LOCATION,
                                         gParamSpecs[PROP_LOCATION]);
}

static void
gclue_locator_init (GClueLocator *locator)
{
        locator->priv =
                G_TYPE_INSTANCE_GET_PRIVATE (locator,
                                            GCLUE_TYPE_LOCATOR,
                                            GClueLocatorPrivate);
        locator->priv->cancellable = g_cancellable_new ();
}

GClueLocator *
gclue_locator_new (void)
{
        return g_object_new (GCLUE_TYPE_LOCATOR, NULL);
}

void
gclue_locator_update_location (GClueLocator      *locator,
                               GClueLocationInfo *location)
{
        gdouble accuracy, new_accuracy, distance;
        const char *desc, *new_desc;

        if (locator->priv->location == NULL)
                goto update;

        accuracy = gclue_location_info_get_accuracy (locator->priv->location);
        new_accuracy = gclue_location_info_get_accuracy (location);
        desc = gclue_location_info_get_description (locator->priv->location);
        new_desc = gclue_location_info_get_description (location);
        distance = gclue_location_info_get_distance_from
                        (locator->priv->location, location);
        if (accuracy == new_accuracy &&
            g_strcmp0 (desc, new_desc) == 0 &&
            /* FIXME: Take threshold into account */
            distance == 0) {
                g_debug ("Location remain unchanged");
                return;
        }

        g_object_unref (locator->priv->location);
update:
        g_debug ("Updating location");
        locator->priv->location = g_object_ref (location);
        g_object_notify (G_OBJECT (locator), "location");
}

void
on_ipclient_search_ready (GObject      *source_object,
                          GAsyncResult *res,
                          gpointer      user_data)
{
        GClueIpclient *ipclient = GCLUE_IPCLIENT (source_object);
        GClueLocator *locator = GCLUE_LOCATOR (user_data);
        GClueLocationInfo *location;
        GError *error = NULL;

        location = gclue_ipclient_search_finish (ipclient, res, &error);
        if (location == NULL) {
                g_warning ("Error fetching location from geoip server: %s",
                           error->message);
                g_error_free (error);
                g_object_unref (locator);

                return;
        }

        g_debug ("New location available");
        gclue_locator_update_location (locator, location);
        g_object_unref (location);
        g_object_unref (locator);
}

void
on_network_changed (GNetworkMonitor *monitor,
                    gboolean         available,
                    gpointer         user_data)
{
        GClueLocator *locator = GCLUE_LOCATOR (user_data);

        if (!available) {
                g_debug ("Network unreachable");
                return;
        }
        g_debug ("Network changed");

        g_cancellable_cancel (locator->priv->cancellable);
        g_cancellable_reset (locator->priv->cancellable);
        gclue_ipclient_search_async (locator->priv->ipclient,
                                     locator->priv->cancellable,
                                     on_ipclient_search_ready,
                                     user_data);
}

void
gclue_locator_start (GClueLocator        *locator,
                     GCancellable        *cancellable,
                     GAsyncReadyCallback  callback,
                     gpointer             user_data)
{
        GSimpleAsyncResult *simple;

        g_return_if_fail (GCLUE_IS_LOCATOR (locator));

        locator->priv->ipclient = gclue_ipclient_new ();
        g_object_set (locator->priv->ipclient,
                      "server", "http://freegeoip.net/json/",
                      "compatibility-mode", TRUE,
                      NULL);

        g_cancellable_cancel (locator->priv->cancellable);
        g_cancellable_reset (locator->priv->cancellable);
        locator->priv->network_changed_id =
                g_signal_connect (g_network_monitor_get_default (),
                                  "network-changed",
                                  G_CALLBACK (on_network_changed),
                                  locator);

        gclue_ipclient_search_async (locator->priv->ipclient,
                                     cancellable,
                                     on_ipclient_search_ready,
                                     g_object_ref (locator));

        simple = g_simple_async_result_new (G_OBJECT (locator),
                                            callback,
                                            user_data,
                                            gclue_locator_start);
        g_simple_async_result_complete_in_idle (simple);

        g_object_unref (simple);
}

gboolean
gclue_locator_start_finish (GClueLocator  *locator,
                            GAsyncResult  *res,
                            GError       **error)
{
        GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);

        g_return_val_if_fail (GCLUE_IS_LOCATOR (locator), FALSE);
        g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == gclue_locator_start);

        if (g_simple_async_result_propagate_error (simple, error))
                return FALSE;

        return TRUE;
}

static void
gclue_locator_stop_sync (GClueLocator *locator)
{
        if (locator->priv->network_changed_id) {
                g_signal_handler_disconnect (g_network_monitor_get_default (),
                                             locator->priv->network_changed_id);
                locator->priv->network_changed_id = 0;
        }

        g_cancellable_cancel (locator->priv->cancellable);
        g_cancellable_reset (locator->priv->cancellable);
        g_clear_object (&locator->priv->ipclient);
}

void
gclue_locator_stop (GClueLocator        *locator,
                    GCancellable        *cancellable,
                    GAsyncReadyCallback  callback,
                    gpointer             user_data)
{
        GSimpleAsyncResult *simple;

        g_return_if_fail (GCLUE_IS_LOCATOR (locator));

        gclue_locator_stop_sync (locator);

        simple = g_simple_async_result_new (G_OBJECT (locator),
                                            callback,
                                            user_data,
                                            gclue_locator_stop);
        g_simple_async_result_complete_in_idle (simple);

        g_object_unref (simple);
}

gboolean
gclue_locator_stop_finish (GClueLocator  *locator,
                            GAsyncResult  *res,
                            GError       **error)
{
        GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);

        g_return_val_if_fail (GCLUE_IS_LOCATOR (locator), FALSE);
        g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == gclue_locator_stop);

        if (g_simple_async_result_propagate_error (simple, error))
                return FALSE;

        return TRUE;
}

GClueLocationInfo * gclue_locator_get_location (GClueLocator *locator)
{
        g_return_val_if_fail (GCLUE_IS_LOCATOR (locator), NULL);

        return locator->priv->location;
}
