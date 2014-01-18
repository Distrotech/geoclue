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
        /* add properties and signals to the interface here */
}

/**
 * gclue_location_source_search_async:
 * @source: a #GClueLocationSource
 * @cancellable: optional #GCancellable forward, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously search for location.
 *
 * When the operation is finished, @callback will be called. You can then call
 * gclue_location_source_search_finish() to get the result of the operation.
 **/
void
gclue_location_source_search_async (GClueLocationSource *source,
                                    GCancellable        *cancellable,
                                    GAsyncReadyCallback  callback,
                                    gpointer             user_data)
{
        g_return_if_fail (GCLUE_IS_LOCATION_SOURCE (source));

        GCLUE_LOCATION_SOURCE_GET_INTERFACE (source)->search_async (source,
                                                                    cancellable,
                                                                    callback,
                                                                    user_data);
}

/**
 * gclue_location_source_search_finish:
 * @source: a #GClueLocationSourc
 * @res: a #GAsyncResult
 * @error: a #GError
 *
 * Finishes a geolocation search operation. See
 * gclue_location_source_search_async().
 *
 * Returns: (transfer full): A #GeocodeLocation object or %NULL in case of
 * errors. Free the returned object with g_object_unref() when done.
 **/
GeocodeLocation *
gclue_location_source_search_finish (GClueLocationSource *source,
                                     GAsyncResult        *res,
                                     GError             **error)
{
        g_return_val_if_fail (GCLUE_IS_LOCATION_SOURCE (source), NULL);

        return GCLUE_LOCATION_SOURCE_GET_INTERFACE (source)->search_finish (source,
                                                                            res,
                                                                            error);
}
