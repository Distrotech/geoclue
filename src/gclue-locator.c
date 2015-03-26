/* vim: set et ts=8 sw=8: */
/* gclue-locator.c
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

#include "config.h"

#include <glib/gi18n.h>

#include "gclue-locator.h"
#include "public-api/gclue-enum-types.h"

#include "gclue-wifi.h"

#if GCLUE_USE_3G_SOURCE
#include "gclue-3g.h"
#endif

#if GCLUE_USE_CDMA_SOURCE
#include "gclue-cdma.h"
#endif

#if GCLUE_USE_MODEM_GPS_SOURCE
#include "gclue-modem-gps.h"
#endif

/* This class is like a master location source that hides all individual
 * location sources from rest of the code
 */

static gboolean
gclue_locator_start (GClueLocationSource *source);
static gboolean
gclue_locator_stop (GClueLocationSource *source);

G_DEFINE_TYPE (GClueLocator, gclue_locator, GCLUE_TYPE_LOCATION_SOURCE)

struct _GClueLocatorPrivate
{
        GList *sources;
        GList *active_sources;

        GClueAccuracyLevel accuracy_level;
};

enum
{
        PROP_0,
        PROP_ACCURACY_LEVEL,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static void
set_location (GClueLocator  *locator,
              GClueLocation *location)
{
        GClueLocation *cur_location;
        GeocodeLocation *gloc, *cur_gloc;

        cur_location = gclue_location_source_get_location
                        (GCLUE_LOCATION_SOURCE (locator));
        gloc = GEOCODE_LOCATION (location);
        cur_gloc = GEOCODE_LOCATION (cur_location);

        g_debug ("New location available");

        if (cur_location != NULL &&
            geocode_location_get_distance_from (gloc, cur_gloc) <
            geocode_location_get_accuracy (gloc) &&
            geocode_location_get_accuracy (gloc) >
            geocode_location_get_accuracy (cur_gloc)) {
                /* We only take the new location if either the previous one
                 * lies outside its accuracy circle or its more or as
                 * accurate as previous one.
                 */
                g_debug ("Ignoring less accurate new location");
                return;
        }

        gclue_location_set_speed_from_prev_location (location, cur_location);
        gclue_location_set_heading_from_prev_location (location, cur_location);

        gclue_location_source_set_location (GCLUE_LOCATION_SOURCE (locator),
                                            location);
}

static void
refresh_available_accuracy_level (GClueLocator *locator)
{
        GList *node;
        GClueAccuracyLevel new, existing;

        new = GCLUE_ACCURACY_LEVEL_NONE;
        for (node = locator->priv->sources; node != NULL; node = node->next) {
                GClueLocationSource *src;
                GClueAccuracyLevel level;

                src = GCLUE_LOCATION_SOURCE (node->data);
                level = gclue_location_source_get_available_accuracy_level (src);
                if (level > new)
                        new = level;
        }
        existing = gclue_location_source_get_available_accuracy_level
                        (GCLUE_LOCATION_SOURCE (locator));

        if (new != existing)
                g_object_set (G_OBJECT (locator),
                              "available-accuracy-level", new,
                              NULL);
}

static void
on_location_changed (GObject    *gobject,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
        GClueLocator *locator = GCLUE_LOCATOR (user_data);
        GClueLocationSource *source = GCLUE_LOCATION_SOURCE (gobject);
        GClueLocation *location;

        location = gclue_location_source_get_location (source);
        set_location (locator, location);
}

static gboolean
is_source_active (GClueLocator        *locator,
                  GClueLocationSource *src)
{
        return (g_list_find (locator->priv->active_sources, src) != NULL);
}

static void
start_source (GClueLocator        *locator,
              GClueLocationSource *src)
{
        GClueLocation *location;

        g_signal_connect (G_OBJECT (src),
                          "notify::location",
                          G_CALLBACK (on_location_changed),
                          locator);

        location = gclue_location_source_get_location (src);
        if (gclue_location_source_get_active (src) && location != NULL)
                set_location (locator, location);

        gclue_location_source_start (src);
}

static void
on_avail_accuracy_level_changed (GObject    *gobject,
                                 GParamSpec *pspec,
                                 gpointer    user_data)
{
        GClueLocationSource *src = GCLUE_LOCATION_SOURCE (gobject);
        GClueLocator *locator = GCLUE_LOCATOR (user_data);
        GClueLocatorPrivate *priv = locator->priv;
        GClueAccuracyLevel level;
        gboolean active;

        refresh_available_accuracy_level (locator);

        active = gclue_location_source_get_active
                (GCLUE_LOCATION_SOURCE (locator));
        if (!active)
                return;

        level = gclue_location_source_get_available_accuracy_level (src);
        if (level != GCLUE_ACCURACY_LEVEL_NONE &&
            priv->accuracy_level >= level &&
            !is_source_active (locator, src)) {
                start_source (locator, src);
        } else if ((level == GCLUE_ACCURACY_LEVEL_NONE ||
                    priv->accuracy_level > level) &&
                   is_source_active (locator, src)) {
                g_signal_handlers_disconnect_by_func (G_OBJECT (src),
                                                      G_CALLBACK (on_location_changed),
                                                      locator);
                gclue_location_source_stop (src);
                priv->active_sources = g_list_remove (priv->active_sources,
                                                      src);
        }
}

static void
gclue_locator_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
        GClueLocator *locator = GCLUE_LOCATOR (object);

        switch (prop_id) {
        case PROP_ACCURACY_LEVEL:
                g_value_set_enum (value, locator->priv->accuracy_level);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_locator_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
        GClueLocator *locator = GCLUE_LOCATOR (object);

        switch (prop_id) {
        case PROP_ACCURACY_LEVEL:
                locator->priv->accuracy_level = g_value_get_enum (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_locator_finalize (GObject *gsource)
{
        GClueLocator *locator = GCLUE_LOCATOR (gsource);
        GClueLocatorPrivate *priv = locator->priv;
        GList *node;

        G_OBJECT_CLASS (gclue_locator_parent_class)->finalize (gsource);

        for (node = locator->priv->sources; node != NULL; node = node->next)
                g_signal_handlers_disconnect_by_func
                        (G_OBJECT (node->data),
                         G_CALLBACK (on_avail_accuracy_level_changed),
                         locator);
        for (node = locator->priv->active_sources; node != NULL; node = node->next) {
                g_signal_handlers_disconnect_by_func
                        (G_OBJECT (node->data),
                         G_CALLBACK (on_location_changed),
                         locator);
                gclue_location_source_stop (GCLUE_LOCATION_SOURCE (node->data));
        }
        g_list_free_full (priv->sources, g_object_unref);
        priv->sources = NULL;
        priv->active_sources = NULL;
}

static void
gclue_locator_constructed (GObject *object)
{
        GClueLocator *locator = GCLUE_LOCATOR (object);
        GClueLocationSource *submit_source = NULL;
        GList *node;

        G_OBJECT_CLASS (gclue_locator_parent_class)->constructed (object);

#if GCLUE_USE_3G_SOURCE
        GClue3G *source = gclue_3g_get_singleton ();
        locator->priv->sources = g_list_append (locator->priv->sources, source);
#endif
#if GCLUE_USE_CDMA_SOURCE
        GClueCDMA *cdma = gclue_cdma_get_singleton ();
        locator->priv->sources = g_list_append (locator->priv->sources, cdma);
#endif
        GClueWifi *wifi = gclue_wifi_get_singleton (locator->priv->accuracy_level);
        locator->priv->sources = g_list_append (locator->priv->sources,
                                                wifi);
#if GCLUE_USE_MODEM_GPS_SOURCE
        GClueModemGPS *gps = gclue_modem_gps_get_singleton ();
        locator->priv->sources = g_list_append (locator->priv->sources, gps);
        submit_source = GCLUE_LOCATION_SOURCE (gps);
#endif

        for (node = locator->priv->sources; node != NULL; node = node->next) {
                g_signal_connect (G_OBJECT (node->data),
                                  "notify::available-accuracy-level",
                                  G_CALLBACK (on_avail_accuracy_level_changed),
                                  locator);

                if (submit_source != NULL && GCLUE_IS_WEB_SOURCE (node->data))
                        gclue_web_source_set_submit_source
                                (GCLUE_WEB_SOURCE (node->data), submit_source);
        }
        refresh_available_accuracy_level (locator);
}

static void
gclue_locator_class_init (GClueLocatorClass *klass)
{
        GClueLocationSourceClass *source_class = GCLUE_LOCATION_SOURCE_CLASS (klass);
        GObjectClass *object_class;

        source_class->start = gclue_locator_start;
        source_class->stop = gclue_locator_stop;

        object_class = G_OBJECT_CLASS (klass);
        object_class->get_property = gclue_locator_get_property;
        object_class->set_property = gclue_locator_set_property;
        object_class->finalize = gclue_locator_finalize;
        object_class->constructed = gclue_locator_constructed;
        g_type_class_add_private (object_class, sizeof (GClueLocatorPrivate));

        gParamSpecs[PROP_ACCURACY_LEVEL] = g_param_spec_enum ("accuracy-level",
                                                              "AccuracyLevel",
                                                              "Accuracy level",
                                                              GCLUE_TYPE_ACCURACY_LEVEL,
                                                              GCLUE_ACCURACY_LEVEL_CITY,
                                                              G_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_ACCURACY_LEVEL,
                                         gParamSpecs[PROP_ACCURACY_LEVEL]);
}

static void
gclue_locator_init (GClueLocator *locator)
{
        locator->priv =
                G_TYPE_INSTANCE_GET_PRIVATE (locator,
                                            GCLUE_TYPE_LOCATOR,
                                            GClueLocatorPrivate);
}

static gboolean
gclue_locator_start (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;
        GClueLocator *locator;
        GList *node;

        g_return_val_if_fail (GCLUE_IS_LOCATOR (source), FALSE);
        locator = GCLUE_LOCATOR (source);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_locator_parent_class);
        if (!base_class->start (source))
                return FALSE;

        for (node = locator->priv->sources; node != NULL; node = node->next) {
                GClueLocationSource *src = GCLUE_LOCATION_SOURCE (node->data);
                GClueAccuracyLevel level;

                level = gclue_location_source_get_available_accuracy_level (src);
                if (level > locator->priv->accuracy_level ||
                    level == GCLUE_ACCURACY_LEVEL_NONE) {
                        g_debug ("Not starting %s (accuracy level: %u). "
                                 "Requested accuracy level: %u.",
                                 G_OBJECT_TYPE_NAME (src),
                                 level,
                                 locator->priv->accuracy_level);
                        continue;
                }

                locator->priv->active_sources = g_list_append (locator->priv->active_sources,
                                                               src);

                start_source (locator, src);
        }

        return TRUE;
}

static gboolean
gclue_locator_stop (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;
        GClueLocator *locator;
        GList *node;

        g_return_val_if_fail (GCLUE_IS_LOCATOR (source), FALSE);
        locator = GCLUE_LOCATOR (source);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_locator_parent_class);
        if (!base_class->stop (source))
                return FALSE;

        for (node = locator->priv->active_sources; node != NULL; node = node->next) {
                GClueLocationSource *src = GCLUE_LOCATION_SOURCE (node->data);

                g_signal_handlers_disconnect_by_func (G_OBJECT (src),
                                                      G_CALLBACK (on_location_changed),
                                                      locator);
                gclue_location_source_stop (src);
                g_debug ("Requested %s to stop", G_OBJECT_TYPE_NAME (src));
        }

        g_list_free (locator->priv->active_sources);
        locator->priv->active_sources = NULL;
        return TRUE;
}

GClueLocator *
gclue_locator_new (GClueAccuracyLevel level)
{
        GClueAccuracyLevel accuracy_level = level;

        if (accuracy_level == GCLUE_ACCURACY_LEVEL_COUNTRY)
                /* There is no source that provides country-level accuracy.
                 * Since Wifi (as geoip) source is the best we can do, accuracy
                 * really is country-level many times from this source and its
                 * doubtful app (or user) will mind being given slighly more
                 * accurate location, lets just map this to city-level accuracy.
                 */
                accuracy_level = GCLUE_ACCURACY_LEVEL_CITY;

        return g_object_new (GCLUE_TYPE_LOCATOR,
                             "accuracy-level", accuracy_level,
                             NULL);
}

GClueAccuracyLevel
gclue_locator_get_accuracy_level (GClueLocator *locator)
{
        g_return_val_if_fail (GCLUE_IS_LOCATOR (locator),
                              GCLUE_ACCURACY_LEVEL_NONE);

        return locator->priv->accuracy_level;
}
