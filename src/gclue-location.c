/* vim: set et ts=8 sw=8: */
/* gclue-location.c
 *
 * Copyright (C) 2012 Bastien Nocera
 * Copyright (C) 2015 Ankit (Verma)
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
 *    Authors: Bastien Nocera <hadess@hadess.net>
 *             Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 *             Ankit (Verma) <ankitstarski@gmail.com>
 */

#include "gclue-location.h"

struct _GClueLocationPrivate {
        gdouble speed;
};

enum {
        PROP_0,

        PROP_SPEED,
};

G_DEFINE_TYPE (GClueLocation, gclue_location, GEOCODE_TYPE_LOCATION);

static void
gclue_location_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
        GClueLocation *location = GCLUE_LOCATION (object);

        switch (property_id) {
        case PROP_SPEED:
                g_value_set_double (value,
                                    gclue_location_get_speed (location));
                break;

        default:
                /* We don't have any other property... */
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gclue_location_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
        GClueLocation *location = GCLUE_LOCATION (object);

        switch (property_id) {
        case PROP_SPEED:
                gclue_location_set_speed (location,
                                          g_value_get_double (value));
                break;

        default:
                /* We don't have any other property... */
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gclue_location_finalize (GObject *glocation)
{
        G_OBJECT_CLASS (gclue_location_parent_class)->finalize (glocation);
}

static void
gclue_location_class_init (GClueLocationClass *klass)
{
        GObjectClass *glocation_class = G_OBJECT_CLASS (klass);
        GParamSpec *pspec;

        glocation_class->finalize = gclue_location_finalize;
        glocation_class->get_property = gclue_location_get_property;
        glocation_class->set_property = gclue_location_set_property;

        g_type_class_add_private (klass, sizeof (GClueLocationPrivate));

        /**
         * GClueLocation:speed
         *
         * The speed in meters per second.
         */
        pspec = g_param_spec_double ("speed",
                                     "Speed",
                                     "Speed in meters per second",
                                     GCLUE_LOCATION_SPEED_UNKNOWN,
                                     G_MAXDOUBLE,
                                     GCLUE_LOCATION_SPEED_UNKNOWN,
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_STRINGS);
        g_object_class_install_property (glocation_class, PROP_SPEED, pspec);
}

static void
gclue_location_init (GClueLocation *location)
{
        location->priv = G_TYPE_INSTANCE_GET_PRIVATE ((location),
                                                      GCLUE_TYPE_LOCATION,
                                                      GClueLocationPrivate);

        location->priv->speed = GCLUE_LOCATION_SPEED_UNKNOWN;
}

/**
 * gclue_location_new:
 * @latitude: a valid latitude
 * @longitude: a valid longitude
 * @accuracy: accuracy of location in meters
 *
 * Creates a new #GClueLocation object.
 *
 * Returns: a new #GClueLocation object. Use g_object_unref() when done.
 **/
GClueLocation *
gclue_location_new (gdouble latitude,
                    gdouble longitude,
                    gdouble accuracy)
{
        return g_object_new (GCLUE_TYPE_LOCATION,
                             "latitude", latitude,
                             "longitude", longitude,
                             "accuracy", accuracy,
                             NULL);
}

/**
 * gclue_location_new_with_description:
 * @latitude: a valid latitude
 * @longitude: a valid longitude
 * @accuracy: accuracy of location in meters
 * @description: a description for the location
 *
 * Creates a new #GClueLocation object.
 *
 * Returns: a new #GClueLocation object. Use g_object_unref() when done.
 **/
GClueLocation *
gclue_location_new_with_description (gdouble     latitude,
                                     gdouble     longitude,
                                     gdouble     accuracy,
                                     const char *description)
{
        return g_object_new (GCLUE_TYPE_LOCATION,
                             "latitude", latitude,
                             "longitude", longitude,
                             "accuracy", accuracy,
                             "description", description,
                             NULL);
}

/**
 * gclue_location_get_speed:
 * @location: a #GClueLocation
 *
 * Gets the speed in meters per second.
 *
 * Returns: The speed, or %GCLUE_LOCATION_SPEED_UNKNOWN if speed in unknown.
 **/
gdouble
gclue_location_get_speed (GClueLocation *location)
{
        g_return_val_if_fail (GCLUE_IS_LOCATION (location),
                              GCLUE_LOCATION_SPEED_UNKNOWN);

        return location->priv->speed;
}

/**
 * gclue_location_set_speed:
 * @location: a #GClueLocation
 * @speed: speed in meters per second
 *
 * Sets the speed.
 **/
void
gclue_location_set_speed (GClueLocation *location,
                          gdouble        speed)
{
        location->priv->speed = speed;

        g_object_notify (G_OBJECT (location), "speed");
}

/**
 * gclue_location_set_speed_from_prev_location:
 * @location: a #GClueLocation
 * @prev_location: a #GClueLocation
 *
 * Calculates the speed based on provided previous location @prev_location
 * and sets it on @location.
 **/
void
gclue_location_set_speed_from_prev_location (GClueLocation *location,
                                             GClueLocation *prev_location)
{
        gdouble speed;
        guint64 timestamp, prev_timestamp;
        GeocodeLocation *gloc, *prev_gloc;

        g_return_if_fail (GCLUE_IS_LOCATION (location));
        g_return_if_fail (prev_location == NULL ||
                          GCLUE_IS_LOCATION (prev_location));

        if (prev_location == NULL) {
               location->priv->speed = GCLUE_LOCATION_SPEED_UNKNOWN;

               return;
        }

        gloc = GEOCODE_LOCATION (location);
        prev_gloc = GEOCODE_LOCATION (prev_location);

        timestamp = geocode_location_get_timestamp (gloc);
        prev_timestamp = geocode_location_get_timestamp (prev_gloc);

        speed = geocode_location_get_distance_from (gloc, prev_gloc) *
                1000.0 / (timestamp - prev_timestamp);

        location->priv->speed = speed;

        g_object_notify (G_OBJECT (location), "speed");
}
