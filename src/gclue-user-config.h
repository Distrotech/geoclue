/* vim: set et ts=8 sw=8: */
/* gclue-user-config.h
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

#ifndef GCLUE_USER_CONFIG_H
#define GCLUE_USER_CONFIG_H

#include <gio/gio.h>
#include "gclue-config.h"
#include "gclue-enums.h"

G_BEGIN_DECLS

#define GCLUE_TYPE_USER_CONFIG            (gclue_user_config_get_type())
#define GCLUE_USER_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_USER_CONFIG, GClueUserConfig))
#define GCLUE_USER_CONFIG_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_USER_CONFIG, GClueUserConfig const))
#define GCLUE_USER_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCLUE_TYPE_USER_CONFIG, GClueUserConfigClass))
#define GCLUE_IS_USER_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_USER_CONFIG))
#define GCLUE_IS_USER_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCLUE_TYPE_USER_CONFIG))
#define GCLUE_USER_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GCLUE_TYPE_USER_CONFIG, GClueUserConfigClass))

typedef struct _GClueUserConfig        GClueUserConfig;
typedef struct _GClueUserConfigClass   GClueUserConfigClass;
typedef struct _GClueUserConfigPrivate GClueUserConfigPrivate;

struct _GClueUserConfig
{
        GObject parent;

        /*< private >*/
        GClueUserConfigPrivate *priv;
};

struct _GClueUserConfigClass
{
        GObjectClass parent_class;
};

GType gclue_user_config_get_type (void) G_GNUC_CONST;

GClueUserConfig * gclue_user_config_new               (guint32 uid);
GClueAccuracyLevel gclue_user_config_get_max_accuracy (GClueUserConfig   *config);
void               gclue_user_config_set_max_accuracy (GClueUserConfig   *config,
                                                       GClueAccuracyLevel max_accuracy);

G_END_DECLS

#endif /* GCLUE_USER_CONFIG_H */
