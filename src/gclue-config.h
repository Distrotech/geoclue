/* vim: set et ts=8 sw=8: */
/* gclue-config.h
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

#ifndef GCLUE_CONFIG_H
#define GCLUE_CONFIG_H

#include <gio/gio.h>
#include "geocode-location.h"
#include "gclue-client-info.h"
#include "gclue-config.h"

G_BEGIN_DECLS

#define GCLUE_TYPE_CONFIG            (gclue_config_get_type())
#define GCLUE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_CONFIG, GClueConfig))
#define GCLUE_CONFIG_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_CONFIG, GClueConfig const))
#define GCLUE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCLUE_TYPE_CONFIG, GClueConfigClass))
#define GCLUE_IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_CONFIG))
#define GCLUE_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCLUE_TYPE_CONFIG))
#define GCLUE_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GCLUE_TYPE_CONFIG, GClueConfigClass))

typedef struct _GClueConfig        GClueConfig;
typedef struct _GClueConfigClass   GClueConfigClass;
typedef struct _GClueConfigPrivate GClueConfigPrivate;

struct _GClueConfig
{
        GObject parent;

        /*< private >*/
        GClueConfigPrivate *priv;
};

struct _GClueConfigClass
{
        GObjectClass parent_class;
};

GType gclue_config_get_type (void) G_GNUC_CONST;

GClueConfig *       gclue_config_get_singleton    (void);
gboolean            gclue_config_is_agent_allowed (GClueConfig     *config,
                                                   GClueClientInfo *agent_info);
gboolean            gclue_config_is_app_allowed (GClueConfig     *config,
                                                 const char      *desktop_id,
                                                 GClueClientInfo *app_info);

G_END_DECLS

#endif /* GCLUE_CONFIG_H */
