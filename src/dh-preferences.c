/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2006 Imendio AB
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
#include "dh-base.h"

typedef struct {
	GtkWidget *dialog;

	GtkWidget *advanced_options_button;

	GtkWidget *system_fonts_button;
	GtkWidget *fonts_table;
	GtkWidget *variable_font_button;
	GtkWidget *fixed_font_button;

	guint      use_system_fonts_id;
	guint      system_var_id;
	guint      system_fixed_id;
	guint      var_id;
	guint      fixed_id;
	guint      advanced_options_id;
} DhPreferences;

#define DH_PREFERENCES(x) ((DhPreferences *) x)

static void     preferences_font_set_cb                 (GtkFontButton    *button,
							 gpointer          user_data);
static void     preferences_close_cb                    (GtkButton        *button,
							 gpointer          user_data);
static void     preferences_advanced_options_toggled_cb (GtkToggleButton  *button,
							 gpointer          user_data);
static void     preferences_system_fonts_toggled_cb     (GtkToggleButton  *button,
							 gpointer          user_data);
static void     preferences_var_font_notify_cb          (GConfClient      *client,
							 guint             cnxn_id,
							 GConfEntry       *entry,
							 gpointer          user_data);
static void     preferences_fixed_font_notify_cb        (GConfClient      *client,
							 guint             cnxn_id,
							 GConfEntry       *entry,
							 gpointer          user_data);
static void     preferences_use_system_font_notify_cb   (GConfClient      *client,
							 guint             cnxn_id,
							 GConfEntry       *entry,
							 gpointer          user_data);
static void     preferences_advanced_options_notify_cb  (GConfClient      *client,
							 guint             cnxn_id,
							 GConfEntry       *entry,
							 gpointer          user_data);
static void     preferences_connect_gconf_listeners     (void);
static void     preferences_get_font_names              (gboolean          use_system_fonts,
							 gchar           **variable,
							 gchar           **fixed);
static gboolean preferences_update_fonts                (gpointer          unused);



static DhPreferences *prefs;

static void
preferences_font_set_cb (GtkFontButton *button, gpointer user_data)
{
	DhPreferences *prefs;
	const gchar   *font_name;
	const gchar   *key;
	GConfClient   *gconf_client;

	prefs = DH_PREFERENCES (user_data); 
	font_name = gtk_font_button_get_font_name (button);

	if (GTK_WIDGET (button) == prefs->variable_font_button) {
		key = GCONF_VARIABLE_FONT;
	} else {
		key = GCONF_FIXED_FONT;
	}

	gconf_client = dh_base_get_gconf_client (dh_base_get ());
	gconf_client_set_string (gconf_client,
				 key, font_name,
				 NULL);
}

/* FIXME: I'd rather see the dialog and preference handling split up. In which
 * case this code would be useful:
 */
/*
static void
preferences_destroy_cb (GtkWidget *dialog, DhPreferences *prefs)
{
	gconf_client_notify_remove (gconf_client,
				    prefs->use_system_fonts_id);
	
	gconf_client_notify_remove (gconf_client,
				    prefs->advanced_options_id);

	gconf_client_notify_remove (gconf_client,
				    prefs->system_var_id);

	gconf_client_notify_remove (gconf_client,
					 prefs->system_fixed_id);
	
	gconf_client_notify_remove (gconf_client,
				    prefs->var_id);
	
	gconf_client_notify_remove (gconf_client,
				    prefs->fixed_id);
}
*/

static void
preferences_close_cb (GtkButton *button, gpointer user_data)
{
	DhPreferences *prefs;

	prefs = DH_PREFERENCES (user_data);

	gtk_widget_destroy (GTK_WIDGET (prefs->dialog));

	prefs->dialog = NULL;
	prefs->system_fonts_button = NULL;
	prefs->fonts_table = NULL;
	prefs->variable_font_button = NULL;
	prefs->fixed_font_button = NULL;
}

static void
preferences_advanced_options_toggled_cb (GtkToggleButton *button, 
					 gpointer         user_data)
{
	DhPreferences *prefs;
	gboolean       active;
	GConfClient   *gconf_client;
	
	prefs = DH_PREFERENCES (user_data);

	active = gtk_toggle_button_get_active (button);

	gconf_client = dh_base_get_gconf_client (dh_base_get ());
	gconf_client_set_bool (gconf_client,
			       GCONF_ADVANCED_OPTIONS,
			       active, NULL);
}

static void
preferences_system_fonts_toggled_cb (GtkToggleButton *button, 
				     gpointer         user_data)
{
	DhPreferences *prefs;
	gboolean       active;
	GConfClient   *gconf_client;

	prefs = DH_PREFERENCES (user_data);

	active = gtk_toggle_button_get_active (button);

	gconf_client = dh_base_get_gconf_client (dh_base_get ());
	gconf_client_set_bool (gconf_client,
			       GCONF_USE_SYSTEM_FONTS,
			       active, NULL);
	
	gtk_widget_set_sensitive (prefs->fonts_table, !active);
}

static void
preferences_var_font_notify_cb (GConfClient *client,
				guint        cnxn_id,
				GConfEntry  *entry,
				gpointer     user_data)
{
	DhPreferences *prefs;
	gboolean       use_system_fonts;
	const gchar   *font_name;
	GConfClient   *gconf_client;

	prefs = DH_PREFERENCES (user_data);

	gconf_client = dh_base_get_gconf_client (dh_base_get ());
	use_system_fonts = gconf_client_get_bool (gconf_client,
						  GCONF_USE_SYSTEM_FONTS,
						  NULL);

	if (prefs->variable_font_button) {
		font_name = gconf_value_get_string (gconf_entry_get_value (entry));
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->variable_font_button),
					       font_name);
	}
	
	if (use_system_fonts) {
		return;
	}

	g_idle_add (preferences_update_fonts, NULL);
}

static void
preferences_fixed_font_notify_cb (GConfClient *client,
				  guint        cnxn_id,
				  GConfEntry  *entry,
				  gpointer     user_data)
{
	DhPreferences *prefs;
	gboolean       use_system_fonts;
	const gchar   *font_name;
	GConfClient   *gconf_client;

	prefs = DH_PREFERENCES (user_data);

	gconf_client = dh_base_get_gconf_client (dh_base_get ());
	use_system_fonts = gconf_client_get_bool (gconf_client,
						  GCONF_USE_SYSTEM_FONTS,
						  NULL);

	if (prefs->fixed_font_button) {
		font_name = gconf_value_get_string (gconf_entry_get_value (entry));
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->fixed_font_button),
					       font_name);
	}

	if (use_system_fonts) {
		return;
	}
	
	g_idle_add (preferences_update_fonts, NULL);
}

static void
preferences_use_system_font_notify_cb (GConfClient *client,
				       guint        cnxn_id,
				       GConfEntry  *entry,
				       gpointer     user_data)
{
	DhPreferences *prefs;
	gboolean       use_system_fonts;

	prefs = DH_PREFERENCES (user_data);

	use_system_fonts = gconf_value_get_bool (gconf_entry_get_value (entry));

	if (prefs->system_fonts_button) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->system_fonts_button),
					      use_system_fonts);
	}
	
	if (prefs->fonts_table) {
		gtk_widget_set_sensitive (prefs->fonts_table, !use_system_fonts);
	}
			
	g_idle_add (preferences_update_fonts, NULL);
}

static void
preferences_advanced_options_notify_cb (GConfClient *client,
					guint        cnxn_id,
					GConfEntry  *entry,
					gpointer     user_data)
{
	DhPreferences *prefs;
	gboolean       advanced_options;
	
	prefs = DH_PREFERENCES (user_data);

	advanced_options = gconf_value_get_bool (gconf_entry_get_value (entry));

	if (prefs->advanced_options_button) { 
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->advanced_options_button),
					      advanced_options);
	}
}

static void
preferences_connect_gconf_listeners (void)
{
	GConfClient *gconf_client;
	
	gconf_client = dh_base_get_gconf_client (dh_base_get ());

	prefs->use_system_fonts_id =
		gconf_client_notify_add (gconf_client,
					 GCONF_USE_SYSTEM_FONTS,
					 preferences_use_system_font_notify_cb,
					 prefs, NULL, NULL);
	
	prefs->advanced_options_id =
		gconf_client_notify_add (gconf_client,
					 GCONF_ADVANCED_OPTIONS,
					 preferences_advanced_options_notify_cb,
					 prefs, NULL, NULL);

	prefs->system_var_id =
		gconf_client_notify_add (gconf_client,
					 GCONF_SYSTEM_VARIABLE_FONT,
					 preferences_var_font_notify_cb,
					 prefs, NULL, NULL);

	prefs->system_fixed_id =
		gconf_client_notify_add (gconf_client,
					 GCONF_SYSTEM_FIXED_FONT,
					 preferences_fixed_font_notify_cb,
					 prefs, NULL, NULL);

	prefs->var_id =
		gconf_client_notify_add (gconf_client,
					 GCONF_VARIABLE_FONT,
					 preferences_var_font_notify_cb,
					 prefs, NULL, NULL);

	prefs->fixed_id =
		gconf_client_notify_add (gconf_client,
					 GCONF_FIXED_FONT,
					 preferences_fixed_font_notify_cb,
					 prefs, NULL, NULL);
}

static void
preferences_get_font_names (gboolean   use_system_fonts, 
			    gchar    **variable,
			    gchar    **fixed)
{
	gchar       *var_font_name, *fixed_font_name;
	GConfClient *gconf_client;

	gconf_client = dh_base_get_gconf_client (dh_base_get ());
	
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

static gboolean
preferences_update_fonts (gpointer unused)
{
	GConfClient *gconf_client;
	gboolean     use_system_fonts;
	gchar       *var_font_name;
	gchar       *fixed_font_name;

	gconf_client = dh_base_get_gconf_client (dh_base_get ());
	use_system_fonts = gconf_client_get_bool (gconf_client,
						  GCONF_USE_SYSTEM_FONTS,
						  NULL);

	preferences_get_font_names (use_system_fonts,
				    &var_font_name, &fixed_font_name);

	dh_gecko_utils_set_font (DH_GECKO_PREF_FONT_VARIABLE, var_font_name);
	dh_gecko_utils_set_font (DH_GECKO_PREF_FONT_FIXED, fixed_font_name);

	g_free (var_font_name);
	g_free (fixed_font_name);

	return FALSE;
}

void
dh_preferences_init (void)
{
	if (prefs) {
		return;
	}
	
	prefs = g_new0 (DhPreferences, 1);
}

void
dh_preferences_setup_fonts (void)
{
	preferences_update_fonts (NULL);
	preferences_connect_gconf_listeners ();
}


void
dh_preferences_show_dialog (GtkWindow *parent)
{
	gboolean     use_system_fonts;
	gboolean     advanced_options;
	gchar       *var_font_name, *fixed_font_name;
	GladeXML    *gui;
	GConfClient *gconf_client;

	if (prefs->dialog != NULL) {
		/* Do something useful, although it should be visible at this
		 * point anyway.
		 */
		gtk_window_present (GTK_WINDOW (prefs->dialog));
		return;
	}

	gui = dh_glade_get_file (SHAREDIR "/devhelp.glade",
				 "preferences_dialog",
				 NULL,
				 "preferences_dialog", &prefs->dialog,
				 "advanced_button", &prefs->advanced_options_button,
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
			  "advanced_button", "toggled", preferences_advanced_options_toggled_cb,
			  NULL);

	gconf_client = dh_base_get_gconf_client (dh_base_get ());
	
	use_system_fonts = gconf_client_get_bool (gconf_client,
						  GCONF_USE_SYSTEM_FONTS,
						  NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->system_fonts_button),
				      use_system_fonts);
	gtk_widget_set_sensitive (prefs->fonts_table, !use_system_fonts);

	advanced_options = gconf_client_get_bool (gconf_client,
						  GCONF_ADVANCED_OPTIONS,
						  NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->advanced_options_button),
				      advanced_options);

	preferences_get_font_names (FALSE, &var_font_name, &fixed_font_name);
	
	if (var_font_name) {
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->variable_font_button),
					       var_font_name);
		g_free (var_font_name);
	}

	if (fixed_font_name) {
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->fixed_font_button),
					       fixed_font_name);
		g_free (fixed_font_name);
	}
	
	g_object_unref (gui);
	
	gtk_window_set_transient_for (GTK_WINDOW (prefs->dialog), parent); 
	gtk_widget_show_all (prefs->dialog);
}
