/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Ricard Hult <rhult@codefactory.se>
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

#include "preferences.h"

#define ZOOM_LEVEL_TINY   75 
#define	ZOOM_LEVEL_SMALL  85
#define	ZOOM_LEVEL_MEDIUM 100
#define	ZOOM_LEVEL_LARGE  125
#define	ZOOM_LEVEL_HUGE   150

const OptionMenuData zoom_levels[] = {
	{ N_("Tiny"),   ZOOM_LEVEL_TINY },
	{ N_("Small"),  ZOOM_LEVEL_SMALL },
	{ N_("Medium"), ZOOM_LEVEL_MEDIUM },
	{ N_("Large"),  ZOOM_LEVEL_LARGE },
	{ N_("Huge"),   ZOOM_LEVEL_HUGE },
	{ NULL,         0 }
};

#define AUTOCOMP_SPEED_SLOW 500
#define AUTOCOMP_SPEED_MEDIUM 250
#define AUTOCOMP_SPEED_FAST 100

const OptionMenuData autocompletion_speeds[] = {
	{ N_("Slow"),   AUTOCOMP_SPEED_SLOW },
	{ N_("Medium"), AUTOCOMP_SPEED_MEDIUM },
	{ N_("Fast"),   AUTOCOMP_SPEED_FAST },
	{ NULL,         0 }
};

struct _Preferences {
	GConfClient *client;
};

static void
gconf_autocompletion_changed_cb (GConfClient *client,
				 guint        notify_id,
				 GConfEntry  *entry,
				 gpointer     data)
{
	DevHelp *devhelp = data;

	devhelp_ui_set_autocompletion (devhelp, gconf_value_get_bool (entry->value));
}

static void
gconf_zoom_level_changed_cb (GConfClient *client,
			     guint        notify_id,
			     GConfEntry  *entry,
			     gpointer     data)
{
	DevHelp *devhelp = data;

	devhelp_ui_set_zoom_level (devhelp, gconf_value_get_int (entry->value));
}

static void
gconf_sidebar_position_changed_cb (GConfClient *client,
				   guint        notify_id,
				   GConfEntry  *entry,
				   gpointer     data)
{
}

static void
gconf_sidebar_visible_changed_cb (GConfClient *client,
				  guint        notify_id,
				  GConfEntry  *entry,
				  gpointer     data)
{
	DevHelp *devhelp = data;
	
	devhelp_ui_show_sidebar (devhelp, gconf_value_get_bool (entry->value));
}

void
preferences_set_zoom_level (Preferences *preferences, gint value)
{
	gconf_client_set_int (preferences->client,
			      "/apps/devhelp/zoom_level",
			      value,
			      NULL);
}

gint
preferences_get_zoom_level (Preferences *preferences)

{
	return gconf_client_get_int (preferences->client,
				      "/apps/devhelp/zoom_level",
				      NULL);
}

void
preferences_set_sidebar_visible (Preferences *preferences, gboolean value)
{
	gconf_client_set_bool (preferences->client,
			       "/apps/devhelp/sidebar_visible",
			       value,
			       NULL);
}

gboolean
preferences_get_sidebar_visible (Preferences *preferences)
{
	return gconf_client_get_bool (preferences->client,
				      "/apps/devhelp/sidebar_visible",
				      NULL);
}

void
preferences_set_sidebar_position (Preferences *preferences, gint width)
{
	gconf_client_set_int (preferences->client,
			      "/apps/devhelp/sidebar_position",
			      width,
			      NULL);
}

gint
preferences_get_sidebar_position (Preferences *preferences)
{
	return gconf_client_get_int (preferences->client,
				     "/apps/devhelp/sidebar_position",
				     NULL);
}

void
preferences_set_autocompletion (Preferences *preferences, gboolean value)
{
	gconf_client_set_bool (preferences->client,
			       "/apps/devhelp/autocompletion",
			       value,
			       NULL);
}

gboolean
preferences_get_autocompletion (Preferences *preferences)
{
	return gconf_client_get_bool (preferences->client,
				      "/apps/devhelp/autocompletion",
				      NULL);
}

void
preferences_set_autocompletion_speed (Preferences *preferences, gint value)
{
	gconf_client_set_int (preferences->client,
			      "/apps/devhelp/autocompletion_speed",
			      value,
			      NULL);
}

gint
preferences_get_autocompletion_speed (Preferences *preferences)
{
	return gconf_client_get_int (preferences->client,
				     "/apps/devhelp/autocompletion_speed",
				     NULL);
}

Preferences *
preferences_new (DevHelp *devhelp)
{
	Preferences *prefs;
	GConfClient *client;
	
	g_return_if_fail (devhelp != NULL);

	prefs = g_new0 (Preferences, 1);

	client = gconf_client_get_default ();
	
	/* Add the GConf client and notification handlers. */
	prefs->client = client;

	gconf_client_add_dir (prefs->client, "/apps/devhelp",
			      GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);

	gconf_client_notify_add (
		prefs->client, "/apps/devhelp/autocompletion",
		gconf_autocompletion_changed_cb, devhelp,
		NULL, NULL);
	
	gconf_client_notify_add (
		prefs->client, "/apps/devhelp/sidebar_position",
		gconf_sidebar_position_changed_cb, devhelp,
		NULL, NULL);
	
	gconf_client_notify_add (
		prefs->client, "/apps/devhelp/sidebar_visible",
		gconf_sidebar_visible_changed_cb, devhelp,
		NULL, NULL);
	
	gconf_client_notify_add (
		prefs->client, "/apps/devhelp/zoom_level",
		gconf_zoom_level_changed_cb, devhelp,
		NULL, NULL);
	
	/* Set the default values. */
	/*devhelp_set_autocompletion (devhelp,
	  gconf_client_get_bool (prefs->client, "/apps/devhelp/autocompletion", NULL));*/

	return prefs;
}
