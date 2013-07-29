/* vim: set et ts=8 sw=8: */
#ifndef GCLUE_LOCATOR_H
#define GCLUE_LOCATOR_H

#include <glib-object.h>
#include <geocode-glib/geocode-glib.h>

G_BEGIN_DECLS

#define GCLUE_TYPE_LOCATOR            (gclue_locator_get_type())
#define GCLUE_LOCATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_LOCATOR, GClueLocator))
#define GCLUE_LOCATOR_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCLUE_TYPE_LOCATOR, GClueLocator const))
#define GCLUE_LOCATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCLUE_TYPE_LOCATOR, GClueLocatorClass))
#define GCLUE_IS_LOCATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCLUE_TYPE_LOCATOR))
#define GCLUE_IS_LOCATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCLUE_TYPE_LOCATOR))
#define GCLUE_LOCATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GCLUE_TYPE_LOCATOR, GClueLocatorClass))

typedef struct _GClueLocator        GClueLocator;
typedef struct _GClueLocatorClass   GClueLocatorClass;
typedef struct _GClueLocatorPrivate GClueLocatorPrivate;

struct _GClueLocator
{
        GObject parent;

        /*< private >*/
        GClueLocatorPrivate *priv;
};

struct _GClueLocatorClass
{
        GObjectClass parent_class;
};

GType gclue_locator_get_type (void) G_GNUC_CONST;

GClueLocator *    gclue_locator_new           (void);
void              gclue_locator_start         (GClueLocator       *locator,
                                               GCancellable       *cancellable,
                                               GAsyncReadyCallback callback,
                                               gpointer            user_data);
gboolean          gclue_locator_start_finish  (GClueLocator *locator,
                                               GAsyncResult *res,
                                               GError      **error);
void              gclue_locator_stop          (GClueLocator       *locator,
                                               GCancellable       *cancellable,
                                               GAsyncReadyCallback callback,
                                               gpointer            user_data);
gboolean          gclue_locator_stop_finish   (GClueLocator *locator,
                                               GAsyncResult *res,
                                               GError      **error);
GeocodeLocation * gclue_locator_get_location  (GClueLocator *locator);

G_END_DECLS

#endif /* GCLUE_LOCATOR_H */
