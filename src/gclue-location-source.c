/* vim: set et ts=8 sw=8: */
/*
 * Copyright (C) 2014 Red Hat, Inc.
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

#include <glib.h>
#include "gclue-location-source.h"

/**
 * SECTION:gclue-location-source
 * @short_description: GeoIP client
 * @include: gclue-glib/gclue-location-source.h
 *
 * The interface all geolocation sources must implement.
 **/

G_DEFINE_INTERFACE (GClueLocationSource, gclue_location_source, 0);

static void
gclue_location_source_default_init (GClueLocationSourceInterface *iface)
{
        GParamSpec *pspec;

        pspec = g_param_spec_object ("location",
                                     "Location",
                                     "Location",
                                     GEOCODE_TYPE_LOCATION,
                                     G_PARAM_READABLE);
        g_object_interface_install_property (iface, pspec);
}

/**
 * gclue_location_source_start:
 * @source: a #GClueLocationSource
 *
 * Start searching for location and keep an eye on location changes.
 **/
void
gclue_location_source_start (GClueLocationSource *source)
{
        g_return_if_fail (GCLUE_IS_LOCATION_SOURCE (source));

        GCLUE_LOCATION_SOURCE_GET_INTERFACE (source)->start (source);
}

/**
 * gclue_location_source_stop:
 * @source: a #GClueLocationSource
 *
 * Stop searching for location and no need to keep an eye on location changes
 * anymore.
 **/
void
gclue_location_source_stop (GClueLocationSource *source)
{
        g_return_if_fail (GCLUE_IS_LOCATION_SOURCE (source));

        GCLUE_LOCATION_SOURCE_GET_INTERFACE (source)->stop (source);
}

/**
 * gclue_location_source_get_location:
 * @source: a #GClueLocationSource
 *
 * Returns: (transfer none): The location, or NULL if unknown.
 **/
GeocodeLocation *
gclue_location_source_get_location (GClueLocationSource *source)
{
        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source), NULL);

        return GCLUE_LOCATION_SOURCE_GET_INTERFACE (source)->get_location (source);
}
