/* vim: set et ts=8 sw=8: */
/* gclue-user-config.c
 *
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

#include <glib/gi18n.h>
#include <sys/types.h>
#include <pwd.h>

#include "gclue-user-config.h"

#define CONFIG_FILE_PATH SYSCONFDIR "/geoclue/%s"
#define DEFAULT_MAX_ACCURACY_LEVEL GCLUE_ACCURACY_LEVEL_EXACT

/* This class will be responsible for fetching configuration. */

G_DEFINE_TYPE (GClueUserConfig, gclue_user_config, G_TYPE_OBJECT)

struct _GClueUserConfigPrivate
{
        GKeyFile *key_file;
        char *file_path;

        guint32 uid;
        char *username;
};

enum
{
        PROP_0,
        PROP_UID,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

static void
gclue_user_config_finalize (GObject *object)
{
        GClueUserConfigPrivate *priv;

        priv = GCLUE_USER_CONFIG (object)->priv;

        g_clear_pointer (&priv->key_file, g_key_file_unref);
        g_clear_pointer (&priv->file_path, g_free);
        g_clear_pointer (&priv->username, g_free);

        G_OBJECT_CLASS (gclue_user_config_parent_class)->finalize (object);
}

static void
gclue_user_config_constructed (GObject *object)
{
        GClueUserConfigPrivate *priv = GCLUE_USER_CONFIG (object)->priv;
        struct passwd *pass;
        GError *error = NULL;

        pass = getpwuid (priv->uid);
        if (pass == NULL) {
                priv->username = g_strdup_printf ("%u", priv->uid);
                g_debug ("Failed to find username for UID %u", priv->uid);
        } else {
                priv->username = g_strdup (pass->pw_name);
        }
        priv->file_path = g_strdup_printf (CONFIG_FILE_PATH, priv->username);

        priv->key_file = g_key_file_new ();
        g_key_file_load_from_file (priv->key_file,
                                   priv->file_path,
                                   0,
                                   &error);
        if (error != NULL) {
                g_debug ("Failed to load configuration file '%s': %s",
                         priv->file_path, error->message);
                g_error_free (error);
        }
}

static void
gclue_user_config_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
        GClueUserConfig *config = GCLUE_USER_CONFIG (object);

        switch (prop_id) {
        case PROP_UID:
                g_value_set_uint (value, config->priv->uid);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_user_config_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
        GClueUserConfig *config = GCLUE_USER_CONFIG (object);

        switch (prop_id) {
        case PROP_UID:
                config->priv->uid = g_value_get_uint (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_user_config_class_init (GClueUserConfigClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = gclue_user_config_finalize;
        object_class->constructed = gclue_user_config_constructed;
        object_class->get_property = gclue_user_config_get_property;
        object_class->set_property = gclue_user_config_set_property;

        gParamSpecs[PROP_UID] = g_param_spec_uint ("uid",
                                                   "UID",
                                                   "User ID",
                                                   0,
                                                   G_MAXUINT32,
                                                   0,
                                                   G_PARAM_READWRITE |
                                                   G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_UID,
                                         gParamSpecs[PROP_UID]);

        g_type_class_add_private (object_class, sizeof (GClueUserConfigPrivate));
}

static void
gclue_user_config_init (GClueUserConfig *config)
{
        config->priv =
                G_TYPE_INSTANCE_GET_PRIVATE (config,
                                            GCLUE_TYPE_USER_CONFIG,
                                            GClueUserConfigPrivate);
}

GClueUserConfig *
gclue_user_config_new (guint32 uid)
{
        return g_object_new (GCLUE_TYPE_USER_CONFIG,
                             "uid", uid,
                             NULL);
}

GClueAccuracyLevel
gclue_user_config_get_max_accuracy (GClueUserConfig *config)
{
        GClueUserConfigPrivate *priv = config->priv;
        GClueAccuracyLevel max_accuracy;
        GError *error = NULL;

        max_accuracy = g_key_file_get_integer (priv->key_file,
                                               "accuracy",
                                               "max-allowed",
                                               &error);
        if (error != NULL) {
                g_debug ("Failed to fetch max accuracy level from configuration"
                         " for user '%s': %s",
                         config->priv->username,
                         error->message);
                max_accuracy = DEFAULT_MAX_ACCURACY_LEVEL;
        }

        return max_accuracy;
}

void
gclue_user_config_set_max_accuracy (GClueUserConfig   *config,
                                    GClueAccuracyLevel max_accuracy)
{
        GClueUserConfigPrivate *priv = config->priv;
        char *data;
        gsize data_len;
        GError *error = NULL;

        g_key_file_set_integer (priv->key_file,
                                "accuracy",
                                "max-allowed",
                                max_accuracy);

        data = g_key_file_to_data (priv->key_file, &data_len, NULL);
        g_file_set_contents (priv->file_path, data, data_len, &error);
        if (error != NULL) {
                g_debug ("Failed to save %s: %s",
                         priv->file_path,
                         error->message);
                g_error_free (error);
        }

        g_free (data);
}
