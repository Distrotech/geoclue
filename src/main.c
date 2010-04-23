/*
 * Geoclue
 * main.c - Master process
 *
 * Author: Iain Holmes <iain@openedhand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <gconf/gconf-client.h>

#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "master.h"

static GMainLoop *mainloop;
static GHashTable *options;
static GcMaster *master;


#define GEOCLUE_GCONF_TOP "/apps/geoclue/master"
#define GEOCLUE_MASTER_NAME "org.freedesktop.Geoclue.Master"

static GValue *
gconf_value_to_value (GConfValue *value)
{
	GValue *gvalue;

	g_return_val_if_fail (value != NULL, NULL);
	g_return_val_if_fail (value->type == GCONF_VALUE_STRING ||
			      value->type == GCONF_VALUE_INT, NULL);

	if (value->type == GCONF_VALUE_STRING) {
		const char *str;

		gvalue = g_new0 (GValue, 1);
		str = gconf_value_get_string (value);

		/* Don't add empty strings in the hashtable */
		if (str != NULL && str[0] == '\0')
			str = NULL;

		g_value_init (gvalue, G_TYPE_STRING);
		g_value_set_string (gvalue, str);
	} else if (value->type == GCONF_VALUE_INT) {
		int i;

		gvalue = g_new0 (GValue, 1);
		i = gconf_value_get_int (value);
		g_value_init (gvalue, G_TYPE_INT);
		g_value_set_int (gvalue, i);
	}

	return gvalue;
}

static void
debug_print_key (gboolean init,
		 const char *key,
		 GValue     *gvalue)
{
	const char *message;
	char *string;

	if (init)
		message = "GConf key '%s' initialised to '%s'";
	else
		message = "GConf key '%s' changed to '%s'";

	if (G_VALUE_TYPE (gvalue) == G_TYPE_STRING) {
		string = g_value_dup_string (gvalue);
	} else if (G_VALUE_TYPE (gvalue) == G_TYPE_INT) {
		string = g_strdup_printf ("%d", g_value_get_int (gvalue));
	} else {
		return;
	}

	g_message (message, key, string);
	g_free (string);
}

static void
gconf_key_changed (GConfClient *client,
		   guint cnxn_id,
		   GConfEntry *entry,
		   gpointer user_data)
{
	const char *key;
	GConfValue *v;
	GValue *gvalue;

	key = gconf_entry_get_key (entry);
	v = gconf_entry_get_value (entry);
	gvalue = gconf_value_to_value (v);
	if (gvalue == NULL)
		return;

	debug_print_key (FALSE, key, gvalue);

	g_hash_table_insert (options, g_path_get_basename (key), gvalue);

	g_signal_emit_by_name (G_OBJECT (master), "options-changed", options);
}

static void
free_gvalue (GValue *value)
{
	if (value == NULL)
		return;
	g_value_unset (value);
	g_free (value);
}

static GHashTable *
load_options (void)
{
        GHashTable *ht = NULL;
        GConfClient *client = gconf_client_get_default ();
        GSList *entries, *e;
        GError *error = NULL;

        gconf_client_add_dir (client, GEOCLUE_GCONF_TOP,
        		      GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);

        entries = gconf_client_all_entries (client, GEOCLUE_GCONF_TOP, &error);
        if (error != NULL) {
                g_warning ("Error loading master options: %s", error->message);
                g_error_free (error);
                return NULL;
        }

        /* Setup keys monitoring */
	gconf_client_notify_add (client, GEOCLUE_GCONF_TOP,
				 (GConfClientNotifyFunc) gconf_key_changed,
				 NULL, NULL, NULL);

        ht = g_hash_table_new_full (g_str_hash, g_str_equal,
				    g_free, (GDestroyNotify) free_gvalue);
        g_print ("Master options:\n");
        for (e = entries; e; e = e->next) {
                GConfEntry *entry = e->data;
                const char *key;
                GConfValue *v;
                GValue *gvalue;

                key = gconf_entry_get_key (entry);
                v = gconf_entry_get_value (entry);
                gvalue = gconf_value_to_value (v);
                if (gvalue == NULL)
			continue;

		debug_print_key (TRUE, key, gvalue);

                g_hash_table_insert (ht, g_path_get_basename (key), gvalue);
		gconf_entry_free (entry);
         }
         g_slist_free (entries);

         return ht;
 }

GHashTable *
geoclue_get_main_options (void)
{
        return options;
}

int
main (int    argc,
      char **argv)
{
	DBusGConnection *conn;
	DBusGProxy *proxy;
	GError *error = NULL;
	guint32 request_name_ret;

	g_type_init ();

	mainloop = g_main_loop_new (NULL, FALSE);

	conn = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
	if (!conn) {
		g_error ("Error getting bus: %s", error->message);
		return 1;
	}

	proxy = dbus_g_proxy_new_for_name (conn,
					   DBUS_SERVICE_DBUS,
					   DBUS_PATH_DBUS,
					   DBUS_INTERFACE_DBUS);
	if (!org_freedesktop_DBus_request_name (proxy, GEOCLUE_MASTER_NAME,
						0, &request_name_ret, &error)) {
		g_error ("Error registering D-Bus service %s: %s",
			 GEOCLUE_MASTER_NAME, error->message);
		return 1;
	}

	/* Just quit if master is already running */
	if (request_name_ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		return 1;
	}

        /* Load options */
        options = load_options ();

	master = g_object_new (GC_TYPE_MASTER, NULL);
	dbus_g_connection_register_g_object (conn, 
					     "/org/freedesktop/Geoclue/Master", 
					     G_OBJECT (master));

	g_main_loop_run (mainloop);
	return 0;
}
