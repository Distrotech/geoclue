/* vim: set et ts=8 sw=8: */
/* gclue-location-info.c
 *
 * Copyright (C) 2013 Red Hat, Inc.
 * Copyright (C) 2012 Bastien Nocera
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
 * Authors: Bastien Nocera <hadess@hadess.net>
 *          Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#include "gclue-location-info.h"
#include <math.h>

#define EARTH_RADIUS_KM 6372.795

/**
 * SECTION:gclue-location
 * @short_description: GClue location object
 * @include: gclue-glib/gclue-glib.h
 *
 * The #GClueLocationInfo instance represents a location on earth, with an
 * optional description.
 **/

struct _GClueLocationInfoPrivate {
        gdouble longitude;
        gdouble latitude;
        gdouble accuracy;
        guint64 timestamp;
        char   *description;
};

enum {
        PROP_0,

        PROP_LATITUDE,
        PROP_LONGITUDE,
        PROP_ACCURACY,
        PROP_DESCRIPTION,
        PROP_TIMESTAMP
};

G_DEFINE_TYPE (GClueLocationInfo, gclue_location_info, G_TYPE_OBJECT)

static void
gclue_location_info_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
        GClueLocationInfo *location = GCLUE_LOCATION_INFO (object);

        switch (property_id) {
        case PROP_DESCRIPTION:
                g_value_set_string (value,
                                    gclue_location_info_get_description (location));
                break;

        case PROP_LATITUDE:
                g_value_set_double (value,
                                    gclue_location_info_get_latitude (location));
                break;

        case PROP_LONGITUDE:
                g_value_set_double (value,
                                    gclue_location_info_get_longitude (location));
                break;

        case PROP_ACCURACY:
                g_value_set_double (value,
                                    gclue_location_info_get_accuracy (location));
                break;

        case PROP_TIMESTAMP:
                g_value_set_uint64 (value,
                                    gclue_location_info_get_timestamp (location));
                break;

        default:
                /* We don't have any other property... */
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gclue_location_info_set_latitude (GClueLocationInfo *loc,
                                  gdouble            latitude)
{
        g_return_if_fail (latitude >= -90.0 && latitude <= 90.0);

        loc->priv->latitude = latitude;
}

static void
gclue_location_info_set_longitude (GClueLocationInfo *loc,
                                   gdouble            longitude)
{
        g_return_if_fail (longitude >= -180.0 && longitude <= 180.0);

        loc->priv->longitude = longitude;
}

static void
gclue_location_info_set_accuracy (GClueLocationInfo *loc,
                                  gdouble            accuracy)
{
        g_return_if_fail (accuracy >= GCLUE_LOCATION_INFO_ACCURACY_UNKNOWN);

        loc->priv->accuracy = accuracy;
}

static void
gclue_location_info_set_property(GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
        GClueLocationInfo *location = GCLUE_LOCATION_INFO (object);

        switch (property_id) {
        case PROP_DESCRIPTION:
                gclue_location_info_set_description (location,
                                                     g_value_get_string (value));
                break;

        case PROP_LATITUDE:
                gclue_location_info_set_latitude (location,
                                                  g_value_get_double (value));
                break;

        case PROP_LONGITUDE:
                gclue_location_info_set_longitude (location,
                                                   g_value_get_double (value));
                break;

        case PROP_ACCURACY:
                gclue_location_info_set_accuracy (location,
                                                  g_value_get_double (value));
                break;

        default:
                /* We don't have any other property... */
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gclue_location_info_finalize (GObject *glocation)
{
        GClueLocationInfo *location = (GClueLocationInfo *) glocation;

        g_clear_pointer (&location->priv->description, g_free);

        G_OBJECT_CLASS (gclue_location_info_parent_class)->finalize (glocation);
}

static void
gclue_location_info_class_init (GClueLocationInfoClass *klass)
{
        GObjectClass *glocation_class = G_OBJECT_CLASS (klass);
        GParamSpec *pspec;

        glocation_class->finalize = gclue_location_info_finalize;
        glocation_class->get_property = gclue_location_info_get_property;
        glocation_class->set_property = gclue_location_info_set_property;

        g_type_class_add_private (klass, sizeof (GClueLocationInfoPrivate));

        /**
         * GClueLocationInfo:description:
         *
         * The description of this location.
         */
        pspec = g_param_spec_string ("description",
                                     "Description",
                                     "Description of this location",
                                     NULL,
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_STRINGS);
        g_object_class_install_property (glocation_class, PROP_DESCRIPTION, pspec);

        /**
         * GClueLocationInfo:latitude:
         *
         * The latitude of this location in degrees.
         */
        pspec = g_param_spec_double ("latitude",
                                     "Latitude",
                                     "The latitude of this location in degrees",
                                     -90.0,
                                     90.0,
                                     0.0,
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_STRINGS);
        g_object_class_install_property (glocation_class, PROP_LATITUDE, pspec);

        /**
         * GClueLocationInfo:longitude:
         *
         * The longitude of this location in degrees.
         */
        pspec = g_param_spec_double ("longitude",
                                     "Longitude",
                                     "The longitude of this location in degrees",
                                     -180.0,
                                     180.0,
                                     0.0,
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_STRINGS);
        g_object_class_install_property (glocation_class, PROP_LONGITUDE, pspec);

        /**
         * GClueLocationInfo:accuracy:
         *
         * The accuracy of this location in meters.
         */
        pspec = g_param_spec_double ("accuracy",
                                     "Accuracy",
                                     "The accuracy of this location in meters",
                                     GCLUE_LOCATION_INFO_ACCURACY_UNKNOWN,
                                     G_MAXDOUBLE,
                                     GCLUE_LOCATION_INFO_ACCURACY_UNKNOWN,
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_STRINGS);
        g_object_class_install_property (glocation_class, PROP_ACCURACY, pspec);

        /**
         * GClueLocationInfo:timestamp:
         *
         * A timestamp in seconds since
         * <ulink url="http://en.wikipedia.org/wiki/Unix_epoch">Epoch</ulink>.
         */
        pspec = g_param_spec_uint64 ("timestamp",
                                     "Timestamp",
                                     "The timestamp of this location "
                                     "in seconds since Epoch",
                                     0,
                                     G_MAXINT64,
                                     0,
                                     G_PARAM_READABLE |
                                     G_PARAM_STATIC_STRINGS);
        g_object_class_install_property (glocation_class, PROP_TIMESTAMP, pspec);
}

static void
gclue_location_info_init (GClueLocationInfo *location)
{
        GTimeVal tv;

        location->priv = G_TYPE_INSTANCE_GET_PRIVATE ((location),
                                                      GCLUE_TYPE_LOCATION_INFO,
                                                      GClueLocationInfoPrivate);

        g_get_current_time (&tv);
        location->priv->timestamp = tv.tv_sec;
}

/**
 * gclue_location_info_new:
 * @latitude: a valid latitude
 * @longitude: a valid longitude
 * @accuracy: accuracy of location in meters
 *
 * Creates a new #GClueLocationInfo object.
 *
 * Returns: a new #GClueLocationInfo object. Use g_object_unref() when done.
 **/
GClueLocationInfo *
gclue_location_info_new (gdouble latitude,
                         gdouble longitude,
                         gdouble accuracy)
{
        return g_object_new (GCLUE_TYPE_LOCATION_INFO,
                             "latitude", latitude,
                             "longitude", longitude,
                             "accuracy", accuracy,
                             NULL);
}

/**
 * gclue_location_info_new_with_description:
 * @latitude: a valid latitude
 * @longitude: a valid longitude
 * @accuracy: accuracy of location in meters
 * @description: a description for the location
 *
 * Creates a new #GClueLocationInfo object.
 *
 * Returns: a new #GClueLocationInfo object. Use g_object_unref() when done.
 **/
GClueLocationInfo *
gclue_location_info_new_with_description (gdouble     latitude,
                                          gdouble     longitude,
                                          gdouble     accuracy,
                                          const char *description)
{
        return g_object_new (GCLUE_TYPE_LOCATION_INFO,
                             "latitude", latitude,
                             "longitude", longitude,
                             "accuracy", accuracy,
                             "description", description,
                             NULL);
}

/**
 * gclue_location_info_set_description:
 * @description: a description for the location
 *
 * Sets the description of @loc to @description.
 **/
void
gclue_location_info_set_description (GClueLocationInfo *loc,
                                     const char    *description)
{
        g_return_if_fail (GCLUE_IS_LOCATION_INFO (loc));
        g_return_if_fail (description != NULL);

        g_free (loc->priv->description);
        loc->priv->description = g_strdup (description);
}

const char *
gclue_location_info_get_description (GClueLocationInfo *loc)
{
        g_return_val_if_fail (GCLUE_IS_LOCATION_INFO (loc), NULL);

        return loc->priv->description;
}

gdouble
gclue_location_info_get_latitude (GClueLocationInfo *loc)
{
        g_return_val_if_fail (GCLUE_IS_LOCATION_INFO (loc), 0.0);

        return loc->priv->latitude;
}

gdouble
gclue_location_info_get_longitude (GClueLocationInfo *loc)
{
        g_return_val_if_fail (GCLUE_IS_LOCATION_INFO (loc), 0.0);

        return loc->priv->longitude;
}

gdouble
gclue_location_info_get_accuracy (GClueLocationInfo *loc)
{
        g_return_val_if_fail (GCLUE_IS_LOCATION_INFO (loc),
                              GCLUE_LOCATION_INFO_ACCURACY_UNKNOWN);

        return loc->priv->accuracy;
}

guint64
gclue_location_info_get_timestamp (GClueLocationInfo *loc)
{
        g_return_val_if_fail (GCLUE_IS_LOCATION_INFO (loc), -1);

        return loc->priv->timestamp;
}

/**
 * gclue_location_info_get_distance_from:
 * @loca: a #GClueLocationInfo
 * @locb: a #GClueLocationInfo
 *
 * Calculates the distance in km, along the curvature of the Earth,
 * between 2 locations. Note that altitude changes are not
 * taken into account.
 *
 * Returns: a distance in km.
 **/
double
gclue_location_info_get_distance_from (GClueLocationInfo *loca,
                                       GClueLocationInfo *locb)
{
        gdouble dlat, dlon, lat1, lat2;
        gdouble a, c;

        g_return_val_if_fail (GCLUE_IS_LOCATION_INFO (loca), 0.0);
        g_return_val_if_fail (GCLUE_IS_LOCATION_INFO (locb), 0.0);

        /* Algorithm from:
         * http://www.movable-type.co.uk/scripts/latlong.html */

        dlat = (locb->priv->latitude - loca->priv->latitude) * M_PI / 180.0;
        dlon = (locb->priv->longitude - loca->priv->longitude) * M_PI / 180.0;
        lat1 = loca->priv->latitude * M_PI / 180.0;
        lat2 = locb->priv->latitude * M_PI / 180.0;

        a = sin (dlat / 2) * sin (dlat / 2) +
            sin (dlon / 2) * sin (dlon / 2) * cos (lat1) * cos (lat2);
        c = 2 * atan2 (sqrt (a), sqrt (1-a));
        return EARTH_RADIUS_KM * c;
}
