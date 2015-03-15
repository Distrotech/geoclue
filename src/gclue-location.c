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

G_DEFINE_TYPE (GClueLocation, gclue_location, GEOCODE_TYPE_LOCATION);

static void
gclue_location_finalize (GObject *glocation)
{
        G_OBJECT_CLASS (gclue_location_parent_class)->finalize (glocation);
}

static void
gclue_location_class_init (GClueLocationClass *klass)
{
        GObjectClass *glocation_class = G_OBJECT_CLASS (klass);

        glocation_class->finalize = gclue_location_finalize;
}

static void
gclue_location_init (GClueLocation *location)
{

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
