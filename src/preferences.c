/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Ricard Hult <rhult@codefactory.se>
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Ricard Hult <rhult@codefactory.se>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <gconf/gconf-client.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <gtk/gtksignal.h>
#include "preferences-dialog.h"
#include "preferences.h"

#define ZOOM_LEVEL_TINY   75 
#define	ZOOM_LEVEL_SMALL  85
#define	ZOOM_LEVEL_MEDIUM 100
#define	ZOOM_LEVEL_LARGE  125
#define	ZOOM_LEVEL_HUGE   150

static void   preferences_init                  (Preferences        *prefs);
static void   preferences_class_init            (PreferencesClass   *klass);
static void   preferences_destroy               (GtkObject          *object);
static void   preferences_get_arg               (GtkObject          *object, 
						 GtkArg             *arg, 
						 guint               arg_id);
static void   preferences_set_arg               (GtkObject          *object, 
						 GtkArg             *arg, 
						 guint               arg_id);
static void   gconf_sidebar_visible_changed_cb  (GConfClient        *client,
						 guint               notify_id,
						 GConfEntry         *entry,
						 gpointer            data);
static void   gconf_sidebar_position_changed_cb (GConfClient        *client,
						 guint               notify_id,
						 GConfEntry         *entry,
						 gpointer            data);
static void   gconf_zoom_level_changed_cb       (GConfClient        *client,
						 guint               notify_id,
						 GConfEntry         *entry,
						 gpointer            data);

enum {
	ARG_0,
	ARG_SIDEBAR_VISIBLE,
	ARG_SIDEBAR_POSITION,
	ARG_ZOOM_LEVEL
};

enum {
	SIDEBAR_VISIBLE_CHANGED,
	SIDEBAR_POSITION_CHANGED,
	ZOOM_LEVEL_CHANGED,
	LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

struct _PreferencesPriv {
	GConfClient   *client;
};

static GtkObjectClass *parent_class = NULL;

const OptionMenuData zoom_levels[] = {
	{ N_("Tiny"),   ZOOM_LEVEL_TINY },
	{ N_("Small"),  ZOOM_LEVEL_SMALL },
	{ N_("Medium"), ZOOM_LEVEL_MEDIUM },
	{ N_("Large"),  ZOOM_LEVEL_LARGE },
	{ N_("Huge"),   ZOOM_LEVEL_HUGE },
	{ NULL,         0 }
};

GtkType
preferences_get_type (void)
{
        static GtkType type = 0;

        if (!type) {
                static const GtkTypeInfo info = {
                        "Preferences",
                        sizeof (Preferences),
                        sizeof (PreferencesClass),
                        (GtkClassInitFunc)  preferences_class_init,
                        (GtkObjectInitFunc) preferences_init,
                        /* reserved_1 */ NULL,
                        /* reserved_2 */ NULL,
                        (GtkClassInitFunc) NULL,
                };

                type = gtk_type_unique (gtk_object_get_type (), &info);
        }

        return type;
}

static void
preferences_init (Preferences *prefs)
{
	PreferencesPriv   *priv;
	
	priv = g_new0 (PreferencesPriv, 1);

	priv->client = gconf_client_get_default ();
	
	gconf_client_add_dir (priv->client, "/apps/devhelp",
			      GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);

	gconf_client_notify_add (priv->client, 
				 "/apps/devhelp/sidebar_position",
				 gconf_sidebar_position_changed_cb, 
				 prefs,
				 NULL, NULL);

	gconf_client_notify_add (priv->client, 
				 "/apps/devhelp/sidebar_visible",
				 gconf_sidebar_visible_changed_cb, 
				 prefs,
				 NULL, NULL);
	
	gconf_client_notify_add (priv->client, 
				 "/apps/devhelp/zoom_level",
				 gconf_zoom_level_changed_cb, 
				 prefs,
				 NULL, NULL);

	prefs->priv = priv;
}

static void
preferences_class_init (PreferencesClass *klass)
{
	GtkObjectClass   *object_class;
	
	object_class = (GtkObjectClass *) klass;
	
	parent_class = gtk_type_class (gtk_object_get_type ());

/* 	object_class->destroy = preferences_destroy; */
	object_class->get_arg = preferences_get_arg;
	object_class->set_arg = preferences_set_arg;

	/* Arguments */
	
	gtk_object_add_arg_type ("Preferences::sidebar_visible",
				 GTK_TYPE_BOOL,
				 GTK_ARG_READWRITE,
				 ARG_SIDEBAR_VISIBLE);
	
	gtk_object_add_arg_type ("Preferences::sidebar_position",
				 GTK_TYPE_INT,
				 GTK_ARG_READWRITE,
				 ARG_SIDEBAR_POSITION);
	
	gtk_object_add_arg_type ("Preferences::zoom_level",
				 GTK_TYPE_INT,
				 GTK_ARG_READWRITE,
				 ARG_ZOOM_LEVEL);

	/* Signals */
	
	signals[SIDEBAR_VISIBLE_CHANGED] = 
		gtk_signal_new ("sidebar_visible_changed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (PreferencesClass,
						   sidebar_visible_changed),
				gtk_marshal_NONE__BOOL,
				GTK_TYPE_NONE,
				1, GTK_TYPE_BOOL);

	signals[SIDEBAR_POSITION_CHANGED] = 
		gtk_signal_new ("sidebar_position_changed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (PreferencesClass,
						   sidebar_position_changed),
				gtk_marshal_NONE__INT,
				GTK_TYPE_NONE,
				1, GTK_TYPE_INT);

	signals[ZOOM_LEVEL_CHANGED] = 
		gtk_signal_new ("zoom_level_changed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (PreferencesClass,
						   zoom_level_changed),
				gtk_marshal_NONE__INT,
				GTK_TYPE_NONE,
				1, GTK_TYPE_INT);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
}

static void
preferences_destroy (GtkObject *object)
{
	/* FIX: Free up stuff and disconnect from GConf */
}

static void
preferences_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	Preferences       *prefs;
	PreferencesPriv   *priv;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_PREFERENCES (object));
	
	prefs = PREFERENCES (object);
	priv  = prefs->priv;
	
	switch (arg_id) {
	case ARG_SIDEBAR_VISIBLE:
		GTK_VALUE_BOOL (*arg) = 
			gconf_client_get_bool (priv->client,
					       "/apps/devhelp/sidebar_visible",
					       NULL);

		break;
	case ARG_SIDEBAR_POSITION:
		GTK_VALUE_INT (*arg) = 
			gconf_client_get_int (priv->client,
					      "/apps/devhelp/sidebar_position",
					      NULL);
		break;
	case ARG_ZOOM_LEVEL:
		GTK_VALUE_INT (*arg) = 
			gconf_client_get_int (priv->client,
					      "/apps/devhelp/zoom_level",
					      NULL);
		break;
	default:
		arg->type = GTK_TYPE_INVALID;
	}
}

static void
preferences_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	Preferences       *prefs;
	PreferencesPriv   *priv;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_PREFERENCES (object));
	
	prefs = PREFERENCES (object);
	priv  = prefs->priv;
	
	switch (arg_id) {
	case ARG_SIDEBAR_VISIBLE:
		gconf_client_set_bool (priv->client,
				       "/apps/devhelp/sidebar_visible",
				       GTK_VALUE_BOOL (*arg),
				       NULL);
		break;
	case ARG_SIDEBAR_POSITION:
		gconf_client_set_int (priv->client,
				      "/apps/devhelp/sidebar_position",
				      GTK_VALUE_INT (*arg),
				      NULL);
		break;
	case ARG_ZOOM_LEVEL:
		gconf_client_set_int (priv->client,
				      "/apps/devhelp/zoom_level",
				      GTK_VALUE_INT (*arg),
				      NULL);
		break;
	default:
		break;
	}
}

static void
gconf_sidebar_visible_changed_cb (GConfClient *client,
				  guint        notify_id,
				  GConfEntry  *entry,
				  gpointer     data)
{
	g_return_if_fail (data != NULL);
	g_return_if_fail (GTK_OBJECT (data));

	gtk_signal_emit (GTK_OBJECT (data), 
			 signals[SIDEBAR_VISIBLE_CHANGED],
			 gconf_value_get_bool (entry->value));
}

static void
gconf_sidebar_position_changed_cb (GConfClient *client,
				   guint        notify_id,
				   GConfEntry  *entry,
				   gpointer     data)
{
	g_return_if_fail (data != NULL);
	g_return_if_fail (GTK_OBJECT (data));

	gtk_signal_emit (GTK_OBJECT (data), 
			 signals[SIDEBAR_POSITION_CHANGED],
			 gconf_value_get_int (entry->value));
}

static void
gconf_zoom_level_changed_cb (GConfClient *client,
			     guint        notify_id,
			     GConfEntry  *entry,
			     gpointer     data)
{
	g_return_if_fail (data != NULL);
	g_return_if_fail (GTK_OBJECT (data));

	gtk_signal_emit (GTK_OBJECT (data), 
			 signals[ZOOM_LEVEL_CHANGED],
			 gconf_value_get_int (entry->value));
}

Preferences *
preferences_new (void)
{
	Preferences   *prefs;
	
	prefs = gtk_type_new (TYPE_PREFERENCES);

	return prefs;
}

void
preferences_open_dialog (Preferences   *prefs)
{
	GtkWidget   *widget;
	
	widget = preferences_dialog_new (prefs);

	gtk_widget_show_all (widget);
}


