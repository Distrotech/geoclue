/* vim: set et ts=8 sw=8: */
/* gclue-location.h
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

#ifndef GCLUE_LOCATION_H
#define GCLUE_LOCATION_H

#include <glib-object.h>
#include "geocode-glib/geocode-location.h"

G_BEGIN_DECLS

#define GCLUE_TYPE_LOCATION            (gclue_location_get_type ())
#define GCLUE_LOCATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_LOCATION, GClueLocation))
#define GCLUE_IS_LOCATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_LOCATION))
#define GCLUE_LOCATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCLUE_TYPE_LOCATION, GClueLocationClass))
#define GCLUE_IS_LOCATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCLUE_TYPE_LOCATION))
#define GCLUE_LOCATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GCLUE_TYPE_LOCATION, GClueLocationClass))

typedef struct _GClueLocation        GClueLocation;
typedef struct _GClueLocationClass   GClueLocationClass;

struct _GClueLocation
{
        /* Parent instance structure */
        GeocodeLocation parent_instance;
};

struct _GClueLocationClass
{
        /* Parent class structure */
        GeocodeLocationClass parent_class;
};

GType gclue_location_get_type (void);

GClueLocation *gclue_location_new (gdouble latitude,
                                   gdouble longitude,
                                   gdouble accuracy);

GClueLocation *gclue_location_new_with_description
                                  (gdouble     latitude,
                                   gdouble     longitude,
                                   gdouble     accuracy,
                                   const char *description);

#endif /* GCLUE_LOCATION_H */