/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio HB
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
 */

#include <config.h>

#include <glade/glade.h>
#include <gconf/gconf-client.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "dh-util.h" 
#include "dh-gecko-utils.h"
#include "dh-preferences.h"

typedef struct {
	GtkWidget *dialog;
	GtkWidget *variable_font;
	GtkWidget *fixed_font;
} DhPreferences;

#define DH_PREFERENCES(x) ((DhPreferences *) x)

static gboolean preferences_split_font       (const gchar    *font_name,
					      gchar         **name,
					      gint           *size);
static void preferences_font_set_cb          (GtkFontButton  *button,
					      gpointer        user_data);

GConfClient   *gconf_client = NULL;
DhPreferences *prefs;

static gboolean
preferences_split_font (const gchar  *font_name,
			gchar       **name,
			gint         *size)
{
	gchar *tmp_name, *ch;
	
	tmp_name = g_strdup (font_name);

	ch = g_utf8_strrchr (tmp_name, -1, ' ');
	if (!ch || ch == tmp_name) {
		return FALSE;
	}

	*ch = '\0';

	*name = g_strdup (tmp_name);
	*size = strtol (ch + 1, (char **) NULL, 10);
	
	return TRUE;
}


static void
preferences_font_set_cb (GtkFontButton *button, gpointer user_data)
{
	DhPreferences *prefs;
	const gchar   *font_name;
	gchar         *name;
	gint           size;
	gint           type;

	name = NULL;

	prefs = DH_PREFERENCES (user_data); 
	font_name = gtk_font_button_get_font_name (button);

	if (GTK_WIDGET (button) == prefs->variable_font) {
		type = DH_GECKO_PREF_FONT_VARIABLE;
	} else {
		type = DH_GECKO_PREF_FONT_FIXED;
	}

	if (preferences_split_font (font_name, &name, &size)) {
		g_print ("New variable font: %s (%d)\n", name, size);
		dh_gecko_utils_set_font (type, name, size);
	}
	else {
		g_print ("Couldn't parse font string: '%s'\n", font_name);
	}

	g_free (name);
}

void
dh_preferences_init (void)
{
	GladeXML *gui;

	gconf_client = gconf_client_get_default ();
	gconf_client_add_dir (gconf_client,
			      GCONF_PATH,
			      GCONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);

	prefs = g_new0 (DhPreferences, 1);
	
	gui = dh_glade_get_file (GLADEDIR "/devhelp.glade",
				 "preferences_dialog",
				 NULL,
				 "preferences_dialog", &prefs->dialog,
				 "variable_font", &prefs->variable_font,
				 "fixed_font", &prefs->fixed_font,
				 NULL);

	dh_glade_connect (gui,
			  prefs,
			  "variable_font", "font_set", preferences_font_set_cb,
			  "fixed_font", "font_set", preferences_font_set_cb,
			  NULL);

	g_object_unref (gui);
	
}

void
dh_preferences_show_dialog (void)
{
	gtk_window_present (GTK_WINDOW (prefs->dialog));
}
