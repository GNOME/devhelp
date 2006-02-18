/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio AB
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

#include "dh-util.h" 
#include "dh-gecko-utils.h"
#include "dh-preferences.h"

typedef struct {
	GtkWidget *dialog;
	GtkWidget *system_fonts_button;
	GtkWidget *fonts_table;
	GtkWidget *variable_font_button;
	GtkWidget *fixed_font_button;

	guint      system_var_cnxn;
	guint      system_fixed_cnxn;
	guint      var_cnxn;
	guint      fixed_cnxn;
} DhPreferences;

#define DH_PREFERENCES(x) ((DhPreferences *) x)

static void preferences_font_set_cb             (GtkFontButton   *button,
						 gpointer         user_data);
static void preferences_close_cb                (GtkButton       *button,
						 gpointer         user_data);
static void preferences_system_fonts_toggled_cb (GtkToggleButton *button,
						 gpointer         user_data);
static void preferences_var_font_changed        (GConfClient     *client,
						 guint            cnxn_id,
						 GConfEntry      *entry,
						 gpointer         user_data);
static void preferences_fixed_font_changed      (GConfClient     *client,
						 guint            cnxn_id,
						 GConfEntry      *entry,
						 gpointer         user_data);
static void preferences_use_system_font_changed (GConfClient     *client,
						 guint            cnxn_id,
						 GConfEntry      *entry,
						 gpointer         user_data);
static void preferences_connect_gconf_listeners (void);
static void preferences_get_font_names          (gboolean         use_system_fonts,
						 gchar          **variable,
						 gchar          **fixed);

GConfClient   *gconf_client = NULL;
DhPreferences *prefs;

static void
preferences_font_set_cb (GtkFontButton *button, gpointer user_data)
{
	DhPreferences *prefs;
	const gchar   *font_name;
	const gchar   *key;

	prefs = DH_PREFERENCES (user_data); 
	font_name = gtk_font_button_get_font_name (button);

	if (GTK_WIDGET (button) == prefs->variable_font_button) {
		key = GCONF_VARIABLE_FONT;
	} else {
		key = GCONF_FIXED_FONT;
	}

	gconf_client_set_string (gconf_client,
				 key, font_name,
				 NULL);
}

static void
preferences_close_cb (GtkButton *button, gpointer user_data)
{
	DhPreferences *prefs;

	prefs = DH_PREFERENCES (user_data);

	gtk_widget_hide (GTK_WIDGET (prefs->dialog));
}

static void
preferences_system_fonts_toggled_cb (GtkToggleButton *button, 
				     gpointer         user_data)
{
	DhPreferences *prefs;
	gboolean       active;
	
	prefs = DH_PREFERENCES (user_data);

	active = gtk_toggle_button_get_active (button);

	gconf_client_set_bool (gconf_client,
			       GCONF_USE_SYSTEM_FONTS,
			       active, NULL);
	
	gtk_widget_set_sensitive (prefs->fonts_table, !active);
}

static void
preferences_var_font_changed (GConfClient     *client,
			      guint            cnxn_id,
			      GConfEntry      *entry,
			      gpointer         user_data)
{
	DhPreferences *prefs;
	gboolean       use_system_fonts;
	const gchar   *font_name;

	prefs = DH_PREFERENCES (user_data);

	use_system_fonts = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs->system_fonts_button));
	font_name = gconf_value_get_string (gconf_entry_get_value (entry));

	if (cnxn_id == prefs->var_cnxn) {
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->variable_font_button),
					       font_name);
	}

	if ((use_system_fonts && cnxn_id == prefs->var_cnxn) ||
	    (!use_system_fonts && cnxn_id == prefs->system_var_cnxn)) {
		return;
	}

	dh_gecko_utils_set_font (DH_GECKO_PREF_FONT_VARIABLE, font_name);
}

static void
preferences_fixed_font_changed (GConfClient     *client,
				guint            cnxn_id,
				GConfEntry      *entry,
				gpointer         user_data)
{
	DhPreferences *prefs;
	gboolean       use_system_fonts;
	const gchar   *font_name;

	prefs = DH_PREFERENCES (user_data);

	use_system_fonts = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs->system_fonts_button));

	font_name = gconf_value_get_string (gconf_entry_get_value (entry));
	
	if (cnxn_id == prefs->fixed_cnxn) {
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->fixed_font_button),
					       font_name);
	}

	if ((use_system_fonts && cnxn_id == prefs->fixed_cnxn) ||
	    (!use_system_fonts && cnxn_id == prefs->system_fixed_cnxn)) {
		return;
	}
	
	dh_gecko_utils_set_font (DH_GECKO_PREF_FONT_FIXED, font_name);
}

static void
preferences_use_system_font_changed (GConfClient     *client,
				     guint            cnxn_id,
				     GConfEntry      *entry,
				     gpointer         user_data)
{
	DhPreferences *prefs;
	gboolean       use_system_fonts;
	gchar         *var_font_name;
	gchar         *fixed_font_name;

	prefs = DH_PREFERENCES (user_data);

	use_system_fonts = gconf_value_get_bool (gconf_entry_get_value (entry));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->system_fonts_button),
				      use_system_fonts);

	preferences_get_font_names (use_system_fonts, 
				    &var_font_name, &fixed_font_name);

	dh_gecko_utils_set_font (DH_GECKO_PREF_FONT_VARIABLE, var_font_name);
	dh_gecko_utils_set_font (DH_GECKO_PREF_FONT_FIXED, fixed_font_name);

	g_free (var_font_name);
	g_free (fixed_font_name);

	gtk_widget_set_sensitive (prefs->fonts_table, !use_system_fonts);
}

static void
preferences_connect_gconf_listeners (void)
{
	gconf_client_notify_add (gconf_client,
				 GCONF_USE_SYSTEM_FONTS,
				 preferences_use_system_font_changed,
				 prefs, NULL, NULL);
	
	prefs->system_var_cnxn = gconf_client_notify_add (gconf_client,
							  GCONF_SYSTEM_VARIABLE_FONT,
							  preferences_var_font_changed,
							  prefs, NULL, NULL);
	prefs->system_fixed_cnxn = gconf_client_notify_add (gconf_client,
							    GCONF_SYSTEM_FIXED_FONT,
							    preferences_fixed_font_changed,
							    prefs, NULL, NULL);
	prefs->var_cnxn = gconf_client_notify_add (gconf_client,
						   GCONF_VARIABLE_FONT,
						   preferences_var_font_changed,
						   prefs, NULL, NULL);
	prefs->fixed_cnxn = gconf_client_notify_add (gconf_client,
						     GCONF_FIXED_FONT,
						     preferences_fixed_font_changed,
						     prefs, NULL, NULL);
}

static void
preferences_get_font_names (gboolean   use_system_fonts, 
			    gchar    **variable,
			    gchar    **fixed)
{
	gchar    *var_font_name, *fixed_font_name;
	
	if (use_system_fonts) {
		var_font_name = gconf_client_get_string (gconf_client,
							 GCONF_SYSTEM_VARIABLE_FONT,
							 NULL);
		fixed_font_name = gconf_client_get_string (gconf_client,
							   GCONF_SYSTEM_FIXED_FONT,
							   NULL);
	} else {
		var_font_name = gconf_client_get_string (gconf_client,
							 GCONF_VARIABLE_FONT, 
							 NULL);
		fixed_font_name = gconf_client_get_string (gconf_client,
							   GCONF_FIXED_FONT,
							   NULL);
	}

	*variable = var_font_name;
	*fixed = fixed_font_name;
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
	
	gui = dh_glade_get_file (SHAREDIR "/devhelp.glade",
				 "preferences_dialog",
				 NULL,
				 "preferences_dialog", &prefs->dialog,
				 "fonts_table", &prefs->fonts_table,
				 "system_fonts_button", &prefs->system_fonts_button,
				 "variable_font_button", &prefs->variable_font_button,
				 "fixed_font_button", &prefs->fixed_font_button,
				 NULL);

	dh_glade_connect (gui,
			  prefs,
			  "variable_font_button", "font_set", preferences_font_set_cb,
			  "fixed_font_button", "font_set", preferences_font_set_cb,
			  "close_button", "clicked", preferences_close_cb,
			  "system_fonts_button", "toggled", preferences_system_fonts_toggled_cb,
			  NULL);

	g_object_unref (gui);
}

void
dh_preferences_setup_fonts (void)
{
	gboolean     use_system_fonts;
	gchar       *var_font_name, *fixed_font_name;
	const gchar *var_key, *fixed_key;

	var_key = GCONF_SYSTEM_VARIABLE_FONT;
	fixed_key = GCONF_SYSTEM_FIXED_FONT;
	
	use_system_fonts = gconf_client_get_bool (gconf_client,
						  GCONF_USE_SYSTEM_FONTS,
						  NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->system_fonts_button),
				      use_system_fonts);
	gtk_widget_set_sensitive (prefs->fonts_table, !use_system_fonts);

	preferences_get_font_names (use_system_fonts,
				    &var_font_name, &fixed_font_name);

	dh_gecko_utils_set_font (DH_GECKO_PREF_FONT_VARIABLE, var_font_name);
	dh_gecko_utils_set_font (DH_GECKO_PREF_FONT_FIXED, fixed_font_name);

	if (use_system_fonts) {
		g_free (var_font_name);
		g_free (fixed_font_name);

		preferences_get_font_names (FALSE,
					    &var_font_name, &fixed_font_name);
	}

	gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->variable_font_button),
				       var_font_name);
	gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->fixed_font_button),
				       fixed_font_name);

	g_free (var_font_name);
	g_free (fixed_font_name);

	preferences_connect_gconf_listeners ();
}

void
dh_preferences_show_dialog (GtkWindow *parent)
{
	gtk_window_set_transient_for (GTK_WINDOW (prefs->dialog), parent); 
	gtk_widget_show_all (prefs->dialog);
}
