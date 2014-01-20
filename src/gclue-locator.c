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
#include "gclue-wifi.h"
#include "gclue-enum-types.h"

/* This class will be responsible for doing the actual geolocating. */

G_DEFINE_TYPE (GClueLocator, gclue_locator, G_TYPE_OBJECT)

struct _GClueLocatorPrivate
{
        GList *sources;

        GeocodeLocation *location;

        gboolean active;

        GClueAccuracyLevel accuracy_level;
};

enum
{
        PROP_0,
        PROP_LOCATION,
        PROP_ACCURACY_LEVEL,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static void
gclue_locator_finalize (GObject *object)
{
        gclue_locator_stop (GCLUE_LOCATOR (object));

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

        case PROP_ACCURACY_LEVEL:
                g_value_set_enum (value, locator->priv->accuracy_level);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_locator_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
        GClueLocator *locator = GCLUE_LOCATOR (object);

        switch (prop_id) {
        case PROP_ACCURACY_LEVEL:
                locator->priv->accuracy_level = g_value_get_enum (value);
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
        object_class->set_property = gclue_locator_set_property;
        g_type_class_add_private (object_class, sizeof (GClueLocatorPrivate));

        gParamSpecs[PROP_LOCATION] = g_param_spec_object ("location",
                                                          "Location",
                                                          "Location",
                                                          GEOCODE_TYPE_LOCATION,
                                                          G_PARAM_READABLE);
        g_object_class_install_property (object_class,
                                         PROP_LOCATION,
                                         gParamSpecs[PROP_LOCATION]);

        gParamSpecs[PROP_ACCURACY_LEVEL] = g_param_spec_enum ("accuracy-level",
                                                              "AccuracyLevel",
                                                              "Accuracy level",
                                                              GCLUE_TYPE_ACCURACY_LEVEL,
                                                              GCLUE_ACCURACY_LEVEL_CITY,
                                                              G_PARAM_READWRITE);
        g_object_class_install_property (object_class,
                                         PROP_ACCURACY_LEVEL,
                                         gParamSpecs[PROP_ACCURACY_LEVEL]);
}

static void
gclue_locator_init (GClueLocator *locator)
{
        locator->priv =
                G_TYPE_INSTANCE_GET_PRIVATE (locator,
                                            GCLUE_TYPE_LOCATOR,
                                            GClueLocatorPrivate);
}

GClueLocator *
gclue_locator_new (void)
{
        return g_object_new (GCLUE_TYPE_LOCATOR, NULL);
}

static void
on_location_changed (GObject    *gobject,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
        GClueLocator *locator = GCLUE_LOCATOR (user_data);
        GClueLocatorPrivate *priv = locator->priv;
        GClueLocationSource *source = GCLUE_LOCATION_SOURCE (gobject);
        GeocodeLocation *location = gclue_location_source_get_location (source);

        g_debug ("New location available");

        if (priv->location == NULL)
                priv->location = g_object_new (GEOCODE_TYPE_LOCATION, NULL);
        else if (geocode_location_get_accuracy (location) >=
                 geocode_location_get_accuracy (priv->location)) {
                /* We only take the new location if its more or as accurate as
                 * the previous one.
                 */
                g_debug ("Ignoring less accurate new location");
                return;
        }

        g_object_set (priv->location,
                      "latitude", geocode_location_get_latitude (location),
                      "longitude", geocode_location_get_longitude (location),
                      "accuracy", geocode_location_get_accuracy (location),
                      "description", geocode_location_get_description (location),
                      NULL);

        g_object_notify (G_OBJECT (locator), "location");
}

void
gclue_locator_start (GClueLocator *locator)
{
        GClueIpclient *ipclient;
        GClueWifi *wifi;
        GList *node;

        g_return_if_fail (GCLUE_IS_LOCATOR (locator));

        if (locator->priv->active)
                return; /* Already started */
        locator->priv->active = TRUE;

        /* FIXME: Only use sources that provide <= requested accuracy level. */
        ipclient = gclue_ipclient_new ();
        locator->priv->sources = g_list_append (locator->priv->sources,
                                                ipclient);
        wifi = gclue_wifi_new ();
        locator->priv->sources = g_list_append (locator->priv->sources,
                                                wifi);

        for (node = locator->priv->sources; node != NULL; node = node->next) {
                GClueLocationSource *source = GCLUE_LOCATION_SOURCE (node->data);

                g_signal_connect (source,
                                  "notify::location",
                                  G_CALLBACK (on_location_changed),
                                  locator);
                gclue_location_source_start (source);
        }
}

void
gclue_locator_stop (GClueLocator *locator)
{
        GClueLocatorPrivate *priv = locator->priv;
        GList *node;

        for (node = locator->priv->sources; node != NULL; node = node->next)
                gclue_location_source_stop (GCLUE_LOCATION_SOURCE (node->data));
        g_list_free_full (priv->sources, g_object_unref);
        priv->sources = NULL;
        g_clear_object (&priv->location);
        locator->priv->active = FALSE;
}

GeocodeLocation * gclue_locator_get_location (GClueLocator *locator)
{
        g_return_val_if_fail (GCLUE_IS_LOCATOR (locator), NULL);

        return locator->priv->location;
}

GClueAccuracyLevel
gclue_locator_get_accuracy_level (GClueLocator *locator)
{
        g_return_val_if_fail (GCLUE_IS_LOCATOR (locator),
                              GCLUE_ACCURACY_LEVEL_COUNTRY);

        return locator->priv->accuracy_level;
}

void
gclue_locator_set_accuracy_level (GClueLocator      *locator,
                                  GClueAccuracyLevel level)
{
        g_return_if_fail (GCLUE_IS_LOCATOR (locator));

        locator->priv->accuracy_level = level;
        g_object_notify (G_OBJECT (locator), "accuracy-level");
}
