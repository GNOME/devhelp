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
#include <libgnome/gnome-i18n.h>
#include "preferences-dialog.h"
#include "preferences.h"

#define ZOOM_LEVEL_TINY   75 
#define	ZOOM_LEVEL_SMALL  85
#define	ZOOM_LEVEL_MEDIUM 100
#define	ZOOM_LEVEL_LARGE  125
#define	ZOOM_LEVEL_HUGE   150

static void   preferences_init                  (Preferences      *prefs);
static void   preferences_class_init            (PreferencesClass *klass);
static void   preferences_destroy               (GObject          *object);
static void   preferences_get_property          (GObject          *object,
						 guint             property_id,
						 GValue           *value,
						 GParamSpec       *pspec);
static void   preferences_set_property          (GObject          *object,
						 guint             property_id,
						 const GValue     *value,
						 GParamSpec       *pspec);
static void   gconf_sidebar_visible_changed_cb  (GConfClient      *client,
						 guint             notify_id,
						 GConfEntry       *entry,
						 gpointer          data);
static void   gconf_sidebar_position_changed_cb (GConfClient      *client,
						 guint             notify_id,
						 GConfEntry       *entry,
						 gpointer          data);
static void   gconf_zoom_level_changed_cb       (GConfClient      *client,
						 guint             notify_id,
						 GConfEntry       *entry,
						 gpointer          data);

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

const OptionMenuData zoom_levels[] = {
	{ N_("Tiny"),   ZOOM_LEVEL_TINY },
	{ N_("Small"),  ZOOM_LEVEL_SMALL },
	{ N_("Medium"), ZOOM_LEVEL_MEDIUM },
	{ N_("Large"),  ZOOM_LEVEL_LARGE },
	{ N_("Huge"),   ZOOM_LEVEL_HUGE },
	{ NULL,         0 }
};

GType
preferences_get_type (void)
{
        static GType prefs_type = 0;

        if (!prefs_type) {
                static const GTypeInfo prefs_info = {
                        sizeof (PreferencesClass),
			NULL,
			NULL,
			(GClassInitFunc)  preferences_class_init,
			NULL,
			NULL,
			sizeof (Preferences),
			0,
			(GInstanceInitFunc) preferences_init,
                };
		
		prefs_type = g_type_register_static (G_TYPE_OBJECT,
						     "Preferences",
						     &prefs_info, 0);
        }

        return prefs_type;
}

static void
preferences_init (Preferences *prefs)
{
	PreferencesPriv   *priv;
	
	priv = g_new0 (PreferencesPriv, 1);

	priv->client = gconf_client_get_default ();
	
	gconf_client_add_dir (priv->client, "/apps/devhelp",
			      GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
#if 0
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
#endif
	prefs->priv = priv;
}

static void
preferences_class_init (PreferencesClass *klass)
{
	GObjectClass   *gobject_class;
	
	gobject_class = G_OBJECT_CLASS (klass);
	
/* 	object_class->destroy = preferences_destroy; */
	gobject_class->get_property = preferences_get_property;
	gobject_class->set_property = preferences_set_property;

	g_object_class_install_property (gobject_class,
					 ARG_SIDEBAR_VISIBLE,
					 g_param_spec_boolean ("sidebar_visible",
							       "Sidebar visibility",
							       "Sidebar visibility",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 ARG_SIDEBAR_POSITION,
					 g_param_spec_int ("sidebar_position",
							   "Sidebar position",
							   "Sidebar position",
							   0,
							   1<<16,
							   300,
							   G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 ARG_ZOOM_LEVEL,
					 g_param_spec_int ("zoom_level",
							   "Zoom Level",
							   "Zoom level",
							   0,
							   4,
							   2,
							   G_PARAM_READWRITE));
	
	/* Signals */
	signals[SIDEBAR_VISIBLE_CHANGED] = 
		g_signal_new ("sidebar_visible_changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (PreferencesClass,
					       sidebar_visible_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
			      1, G_TYPE_BOOLEAN);

	signals[SIDEBAR_POSITION_CHANGED] = 
		g_signal_new ("sidebar_position_changed",
			      G_TYPE_FROM_CLASS (klass),				
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (PreferencesClass,
					       sidebar_position_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1, G_TYPE_INT);
	
	signals[ZOOM_LEVEL_CHANGED] = 
		g_signal_new ("zoom_level_changed",
			      G_TYPE_FROM_CLASS (klass),				
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (PreferencesClass,
					       zoom_level_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1, G_TYPE_INT);

}

static void
preferences_destroy (GObject *object)
{
	/* FIX: Free up stuff and disconnect from GConf */
}

static void
preferences_get_property (GObject        *object,
			  guint           property_id,
			  GValue         *value,
			  GParamSpec     *pspec)
{
	Preferences       *prefs;
	PreferencesPriv   *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_PREFERENCES (object));

	prefs = PREFERENCES (object);
	priv  = prefs->priv;

	switch (property_id) {
	case ARG_SIDEBAR_VISIBLE:
		g_value_set_boolean (value,
				     gconf_client_get_bool (priv->client,
							    "/apps/devhelp/sidebar_visible",
							    NULL));

		break;
	case ARG_SIDEBAR_POSITION:
		g_value_set_int (value,
				 gconf_client_get_int (priv->client,
						       "/apps/devhelp/sidebar_position",
						       NULL));
		break;
	case ARG_ZOOM_LEVEL:
		g_value_set_int (value,
				 gconf_client_get_int (priv->client,
						       "/apps/devhelp/zoom_level",
						       NULL));
		break;
	default:
		value->g_type = G_TYPE_INVALID;

	}
}

static void
preferences_set_property (GObject        *object,
			  guint           property_id,
			  const GValue   *value,
			  GParamSpec     *pspec)
{
	Preferences       *prefs;
	PreferencesPriv   *priv;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_PREFERENCES (object));
	
	prefs = PREFERENCES (object);
	priv  = prefs->priv;
	
	switch (property_id) {
	case ARG_SIDEBAR_VISIBLE:
		gconf_client_set_bool (priv->client,
				       "/apps/devhelp/sidebar_visible",
				       g_value_get_boolean (value),
				       NULL);
		break;
	case ARG_SIDEBAR_POSITION:
		gconf_client_set_int (priv->client,
				      "/apps/devhelp/sidebar_position",
				      g_value_get_int (value),
				      NULL);
		break;
	case ARG_ZOOM_LEVEL:
		gconf_client_set_int (priv->client,
				      "/apps/devhelp/zoom_level",
				      g_value_get_int (value),
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
	g_return_if_fail (G_OBJECT (data));

	g_signal_emit (G_OBJECT (data), 
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
	g_return_if_fail (G_OBJECT (data));

	g_signal_emit (G_OBJECT (data), 
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
	g_return_if_fail (G_OBJECT (data));

	g_signal_emit (G_OBJECT (data), 
		       signals[ZOOM_LEVEL_CHANGED],
		       gconf_value_get_int (entry->value));
}

void
preferences_open_dialog (Preferences   *prefs)
{
	GtkWidget   *widget;
	
	widget = preferences_dialog_new (prefs);

	gtk_widget_show_all (widget);
}

Preferences *
preferences_new (void)
{
	return g_object_new (TYPE_PREFERENCES, NULL);
}
