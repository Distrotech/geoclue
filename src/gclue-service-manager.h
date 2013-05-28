/* vim: set et ts=8 sw=8: */
/* gclue-service-manager.h
 *
 * Copyright (C) 2013 Red Hat, Inc.
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
 * Author: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 */

#ifndef GCLUE_SERVICE_MANAGER_H
#define GCLUE_SERVICE_MANAGER_H

#include <glib-object.h>
#include "geoclue-interface.h"

G_BEGIN_DECLS

#define GClUE_TYPE_SERVICE_MANAGER            (gclue_service_manager_get_type())
#define GCLUE_SERVICE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GClUE_TYPE_SERVICE_MANAGER, GClueServiceManager))
#define GCLUE_SERVICE_MANAGER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GClUE_TYPE_SERVICE_MANAGER, GClueServiceManager const))
#define GCLUE_SERVICE_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GClUE_TYPE_SERVICE_MANAGER, GClueServiceManagerClass))
#define GClUE_IS_SERVICE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GClUE_TYPE_SERVICE_MANAGER))
#define GClUE_IS_SERVICE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GClUE_TYPE_SERVICE_MANAGER))
#define GCLUE_SERVICE_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GClUE_TYPE_SERVICE_MANAGER, GClueServiceManagerClass))

typedef struct _GClueServiceManager        GClueServiceManager;
typedef struct _GClueServiceManagerClass   GClueServiceManagerClass;
typedef struct _GClueServiceManagerPrivate GClueServiceManagerPrivate;

struct _GClueServiceManager
{
        GClueManagerSkeleton parent;

        /*< private >*/
        GClueServiceManagerPrivate *priv;
};

struct _GClueServiceManagerClass
{
        GClueManagerSkeletonClass parent_class;
};

GType gclue_service_manager_get_type (void) G_GNUC_CONST;

GClueServiceManager * gclue_service_manager_new    (GDBusConnection *connection,
                                                    GError         **error);

G_END_DECLS

#endif /* GCLUE_SERVICE_MANAGER_H */
