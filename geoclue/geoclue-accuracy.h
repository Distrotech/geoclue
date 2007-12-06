/*
 * Geoclue
 * geoclue-accuracy.h - Functions for manipulating GeoclueAccuracy structs
 *
 * Author: Iain Holmes <iain@openedhand.com>
 */

#ifndef _GEOCLUE_ACCURACY_H
#define _GEOCLUE_ACCURACY_H

#include <glib.h>
#include <geoclue/geoclue-types.h>

#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define GEOCLUE_ACCURACY_TYPE (dbus_g_type_get_struct ("GValueArray", G_TYPE_INT, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_INVALID))

typedef GPtrArray GeoclueAccuracy;

GeoclueAccuracy *geoclue_accuracy_new (GeoclueAccuracyLevel level,
				       double               horizontal_accuracy,
				       double               vertical_accuracy);
void geoclue_accuracy_free (GeoclueAccuracy *accuracy);
void geoclue_accuracy_get_details (GeoclueAccuracy      *accuracy,
				   GeoclueAccuracyLevel *level,
				   double               *horizontal_accuracy,
				   double               *vertical_accuracy);
void geoclue_accuracy_set_details (GeoclueAccuracy      *accuracy,
				   GeoclueAccuracyLevel  level,
				   double                horizontal_accuracy,
				   double                vertical_accuracy);

G_END_DECLS

#endif