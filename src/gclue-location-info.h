/* vim: set et ts=8 sw=8: */
/* main.c
 *
 * Copyright (C) 2013 Red Hat, Inc.
 * Copyright (C) 2012 Bastien Nocera
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
 * Authors: Bastien Nocera <hadess@hadess.net>
 *          Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#ifndef GCLUE_LOCATION_INFO_H
#define GCLUE_LOCATION_INFO_H

#include <glib-object.h>

G_BEGIN_DECLS

GType gclue_location_info_get_type (void) G_GNUC_CONST;

#define GCLUE_TYPE_LOCATION_INFO            (gclue_location_info_get_type ())
#define GCLUE_LOCATION_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_LOCATION_INFO, GClueLocationInfo))
#define GCLUE_IS_LOCATION_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_LOCATION_INFO))
#define GCLUE_LOCATION_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GCLUE_TYPE_LOCATION_INFO, GClueLocationInfoClass))
#define GCLUE_IS_LOCATION_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GCLUE_TYPE_LOCATION_INFO))
#define GCLUE_LOCATION_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GCLUE_TYPE_LOCATION_INFO, GClueLocationInfoClass))

typedef struct _GClueLocationInfo        GClueLocationInfo;
typedef struct _GClueLocationInfoClass   GClueLocationInfoClass;
typedef struct _GClueLocationInfoPrivate GClueLocationInfoPrivate;

/**
 * GClueLocationInfo:
 *
 * All the fields in the #GClueLocationInfo structure are private and should never be accessed directly.
**/
struct _GClueLocationInfo {
        /* <private> */
        GObject parent_instance;
        GClueLocationInfoPrivate *priv;
};

/**
 * GClueLocationInfoClass:
 *
 * All the fields in the #GClueLocationInfoClass structure are private and should never be accessed directly.
**/
struct _GClueLocationInfoClass {
        /* <private> */
        GObjectClass parent_class;
};

#define GCLUE_LOCATION_INFO_ACCURACY_UNKNOWN   -1
#define GCLUE_LOCATION_INFO_ACCURACY_STREET    1000    /*    1 km */
#define GCLUE_LOCATION_INFO_ACCURACY_CITY      15000   /*   15 km */
#define GCLUE_LOCATION_INFO_ACCURACY_REGION    50000   /*   50 km */
#define GCLUE_LOCATION_INFO_ACCURACY_COUNTRY   300000  /*  300 km */
#define GCLUE_LOCATION_INFO_ACCURACY_CONTINENT 3000000 /* 3000 km */

GClueLocationInfo *gclue_location_info_new                  (gdouble latitude,
                                                             gdouble longitude,
                                                             gdouble accuracy);

GClueLocationInfo *gclue_location_info_new_with_description (gdouble     latitude,
                                                             gdouble     longitude,
                                                             gdouble     accuracy,
                                                             const char *description);

double gclue_location_info_get_distance_from                (GClueLocationInfo *loca,
                                                             GClueLocationInfo *locb);

void gclue_location_info_set_description                    (GClueLocationInfo *loc,
                                                             const char    *description);

const char *gclue_location_info_get_description             (GClueLocationInfo *loc);

gdouble gclue_location_info_get_latitude                    (GClueLocationInfo *loc);
gdouble gclue_location_info_get_longitude                   (GClueLocationInfo *loc);
gdouble gclue_location_info_get_accuracy                    (GClueLocationInfo *loc);
guint64 gclue_location_info_get_timestamp                   (GClueLocationInfo *loc);

G_END_DECLS

#endif /* GCLUE_LOCATION_INFO_H */
