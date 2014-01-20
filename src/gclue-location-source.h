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

#ifndef GCLUE_LOCATION_SOURCE_H
#define GCLUE_LOCATION_SOURCE_H

#include <glib.h>
#include <gio/gio.h>
#include "geocode-location.h"

G_BEGIN_DECLS

GType gclue_location_source_get_type (void) G_GNUC_CONST;

#define GCLUE_TYPE_LOCATION_SOURCE                  (gclue_location_source_get_type ())
#define GCLUE_LOCATION_SOURCE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_LOCATION_SOURCE, GClueLocationSource))
#define GCLUE_IS_LOCATION_SOURCE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_LOCATION_SOURCE))
#define GCLUE_LOCATION_SOURCE_GET_INTERFACE(obj)    (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GCLUE_TYPE_LOCATION_SOURCE, GClueLocationSourceInterface))

typedef struct _GClueLocationSource GClueLocationSource;  /* dummy object */
typedef struct _GClueLocationSourceInterface GClueLocationSourceInterface;

struct _GClueLocationSourceInterface
{
        GTypeInterface parent_iface;

        void              (*start)        (GClueLocationSource *source);
        void              (*stop)         (GClueLocationSource *source);
        GeocodeLocation * (*get_location) (GClueLocationSource *source);
};

void              gclue_location_source_start (GClueLocationSource *source);
void              gclue_location_source_stop  (GClueLocationSource *source);
GeocodeLocation * gclue_location_source_get_location
                                              (GClueLocationSource *source);

G_END_DECLS

#endif /* GCLUE_LOCATION_SOURCE_H */
