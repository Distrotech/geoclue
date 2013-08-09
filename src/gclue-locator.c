/* vim: set et ts=8 sw=8: */
#include <glib/gi18n.h>

#include "gclue-locator.h"
#include "gclue-ipclient.h"

/* This class will be responsible for doing the actual geolocating. */

G_DEFINE_TYPE (GClueLocator, gclue_locator, G_TYPE_OBJECT)

struct _GClueLocatorPrivate
{
        GClueIpclient *ipclient;

        GClueLocationInfo *location;
};

enum
{
        PROP_0,
        PROP_LOCATION,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static void
gclue_locator_finalize (GObject *object)
{
        GClueLocatorPrivate *priv;

        priv = GCLUE_LOCATOR (object)->priv;

        g_clear_object (&priv->ipclient);
        g_clear_object (&priv->location);

        G_OBJECT_CLASS (gclue_locator_parent_class)->finalize (object);
}

static void
gclue_locator_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
        GClueLocator *locator = GCLUE_LOCATOR (object);

        switch (prop_id) {
        case PROP_LOCATION:
                g_value_set_object (value, locator->priv->location);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_locator_class_init (GClueLocatorClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = gclue_locator_finalize;
        object_class->get_property = gclue_locator_get_property;
        g_type_class_add_private (object_class, sizeof (GClueLocatorPrivate));

        gParamSpecs[PROP_LOCATION] = g_param_spec_object ("location",
                                                          "Location",
                                                          "Location",
                                                          GCLUE_TYPE_LOCATION_INFO,
                                                          G_PARAM_READABLE);
        g_object_class_install_property (object_class,
                                         PROP_LOCATION,
                                         gParamSpecs[PROP_LOCATION]);
}

static void
gclue_locator_init (GClueLocator *locator)
{
        locator->priv =
                G_TYPE_INSTANCE_GET_PRIVATE (locator,
                                            GCLUE_TYPE_LOCATOR,
                                            GClueLocatorPrivate);
}

GClueLocator *
gclue_locator_new (void)
{
        return g_object_new (GCLUE_TYPE_LOCATOR, NULL);
}

void on_ipclient_search_ready (GObject      *source_object,
                               GAsyncResult *res,
                               gpointer      user_data)
{
        GClueIpclient *ipclient = GCLUE_IPCLIENT (source_object);
        GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
        GClueLocator *locator;
        GError *error = NULL;

        locator = g_simple_async_result_get_op_res_gpointer (simple);
        locator->priv->location = gclue_ipclient_search_finish (ipclient,
                                                                res,
                                                                &error);
        if (locator->priv->location == NULL) {
                g_simple_async_result_take_error (simple, error);
                g_simple_async_result_complete_in_idle (simple);
                g_object_unref (simple);

                return;
        }

        g_simple_async_result_complete_in_idle (simple);

        g_object_unref (simple);
}

void
gclue_locator_start (GClueLocator        *locator,
                     GCancellable        *cancellable,
                     GAsyncReadyCallback  callback,
                     gpointer             user_data)
{
        GSimpleAsyncResult *simple;

        g_return_if_fail (GCLUE_IS_LOCATOR (locator));

        locator->priv->ipclient = gclue_ipclient_new ();
        g_object_set (locator->priv->ipclient,
                      "server", "http://freegeoip.net/json/",
                      "compatibility-mode", TRUE,
                      NULL);

        simple = g_simple_async_result_new (G_OBJECT (locator),
                                            callback,
                                            user_data,
                                            gclue_locator_start);
        g_simple_async_result_set_op_res_gpointer (simple, locator, NULL);

        gclue_ipclient_search_async (locator->priv->ipclient,
                                       cancellable,
                                       on_ipclient_search_ready,
                                       simple);
}

gboolean
gclue_locator_start_finish (GClueLocator  *locator,
                            GAsyncResult  *res,
                            GError       **error)
{
        GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);

        g_return_val_if_fail (GCLUE_IS_LOCATOR (locator), FALSE);
        g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == gclue_locator_start);

        if (g_simple_async_result_propagate_error (simple, error))
                return FALSE;

        return TRUE;
}

void
gclue_locator_stop (GClueLocator        *locator,
                    GCancellable        *cancellable,
                    GAsyncReadyCallback  callback,
                    gpointer             user_data)
{
        GSimpleAsyncResult *simple;

        g_return_if_fail (GCLUE_IS_LOCATOR (locator));

        g_clear_object (&locator->priv->ipclient);
        g_clear_object (&locator->priv->location);

        simple = g_simple_async_result_new (G_OBJECT (locator),
                                            callback,
                                            user_data,
                                            gclue_locator_stop);
        g_simple_async_result_complete_in_idle (simple);

        g_object_unref (simple);
}

gboolean
gclue_locator_stop_finish (GClueLocator  *locator,
                            GAsyncResult  *res,
                            GError       **error)
{
        GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);

        g_return_val_if_fail (GCLUE_IS_LOCATOR (locator), FALSE);
        g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == gclue_locator_stop);

        if (g_simple_async_result_propagate_error (simple, error))
                return FALSE;

        return TRUE;
}

GClueLocationInfo * gclue_locator_get_location (GClueLocator *locator)
{
        g_return_val_if_fail (GCLUE_IS_LOCATOR (locator), NULL);

        return locator->priv->location;
}
