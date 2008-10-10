/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
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

#include "config.h"
#include <gtk/gtk.h>
#include <string.h>
#include "dh-util.h" 
#include "dh-preferences.h"
#include "ige-conf.h"
#include "dh-base.h"

typedef struct {
	GtkWidget *dialog;

	GtkWidget *system_fonts_button;
	GtkWidget *fonts_table;
	GtkWidget *variable_font_button;
	GtkWidget *fixed_font_button;

	guint      use_system_fonts_id;
	guint      system_var_id;
	guint      system_fixed_id;
	guint      var_id;
	guint      fixed_id;
} DhPreferences;

static void     preferences_font_set_cb               (GtkFontButton    *button,
                                                       gpointer          user_data);
static void     preferences_close_cb                  (GtkButton        *button,
                                                       gpointer          user_data);
static void     preferences_system_fonts_toggled_cb   (GtkToggleButton  *button,
                                                       gpointer          user_data);
static void     preferences_var_font_notify_cb        (IgeConf          *client,
                                                       const gchar      *path,
                                                       gpointer          user_data);
static void     preferences_fixed_font_notify_cb      (IgeConf          *client,
                                                       const gchar      *path,
                                                       gpointer          user_data);
static void     preferences_use_system_font_notify_cb (IgeConf          *client,
                                                       const gchar      *path,
                                                       gpointer          user_data);
static void     preferences_connect_conf_listeners    (void);
static void     preferences_get_font_names            (gboolean          use_system_fonts,
                                                       gchar           **variable,
                                                       gchar           **fixed);
static gboolean preferences_update_fonts              (gpointer          unused);

static DhPreferences *prefs;

static void
preferences_init (void)
{
	if (!prefs) {
                prefs = g_new0 (DhPreferences, 1);
        }
}

static void
preferences_font_set_cb (GtkFontButton *button, gpointer user_data)
{
	DhPreferences *prefs = user_data;
	const gchar   *font_name;
	const gchar   *key;

	font_name = gtk_font_button_get_font_name (button);

	if (GTK_WIDGET (button) == prefs->variable_font_button) {
		key = DH_CONF_VARIABLE_FONT;
	} else {
		key = DH_CONF_FIXED_FONT;
	}

	ige_conf_set_string (ige_conf_get (), key, font_name);
}

static void
preferences_close_cb (GtkButton *button, gpointer user_data)
{
	DhPreferences *prefs = user_data;

	gtk_widget_destroy (GTK_WIDGET (prefs->dialog));

	prefs->dialog = NULL;
	prefs->system_fonts_button = NULL;
	prefs->fonts_table = NULL;
	prefs->variable_font_button = NULL;
	prefs->fixed_font_button = NULL;
}

static void
preferences_system_fonts_toggled_cb (GtkToggleButton *button, 
				     gpointer         user_data)
{
	DhPreferences *prefs = user_data;
	gboolean       active;

	active = gtk_toggle_button_get_active (button);

	ige_conf_set_bool (ige_conf_get (),
                           DH_CONF_USE_SYSTEM_FONTS,
                           active);
	
	gtk_widget_set_sensitive (prefs->fonts_table, !active);
}

static void
preferences_var_font_notify_cb (IgeConf     *client,
				const gchar *path,
				gpointer     user_data)
{
	DhPreferences *prefs = user_data;
	gboolean       use_system_fonts;
	gchar         *font_name;

        ige_conf_get_bool (ige_conf_get (),
                           DH_CONF_USE_SYSTEM_FONTS,
                           &use_system_fonts);

	if (prefs->variable_font_button) {
		ige_conf_get_string (ige_conf_get (), path, &font_name);
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->variable_font_button),
					       font_name);
                g_free (font_name);
	}
	
	if (use_system_fonts) {
		return;
	}

	g_idle_add (preferences_update_fonts, NULL);
}

static void
preferences_fixed_font_notify_cb (IgeConf     *client,
				  const gchar *path,
				  gpointer     user_data)
{
	DhPreferences *prefs = user_data;
	gboolean       use_system_fonts;
	gchar         *font_name;

	ige_conf_get_bool (ige_conf_get (),
                           DH_CONF_USE_SYSTEM_FONTS,
                           &use_system_fonts);

	if (prefs->fixed_font_button) {
                ige_conf_get_string (ige_conf_get (), path, &font_name);
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->fixed_font_button),
					       font_name);
                g_free (font_name);
	}

	if (use_system_fonts) {
		return;
	}
	
	g_idle_add (preferences_update_fonts, NULL);
}

static void
preferences_use_system_font_notify_cb (IgeConf     *client,
                                       const gchar *path,
				       gpointer     user_data)
{
	DhPreferences *prefs = user_data;
	gboolean       use_system_fonts;

	ige_conf_get_bool (ige_conf_get (), path, &use_system_fonts);

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
preferences_connect_conf_listeners (void)
{
	IgeConf *conf;
	
	conf = ige_conf_get ();

	prefs->use_system_fonts_id =
		ige_conf_notify_add (conf,
                                     DH_CONF_USE_SYSTEM_FONTS,
                                     preferences_use_system_font_notify_cb,
                                     prefs);
	prefs->system_var_id =
		ige_conf_notify_add (conf,
                                     DH_CONF_SYSTEM_VARIABLE_FONT,
                                     preferences_var_font_notify_cb,
                                     prefs);
	prefs->system_fixed_id =
		ige_conf_notify_add (conf,
                                     DH_CONF_SYSTEM_FIXED_FONT,
                                     preferences_fixed_font_notify_cb,
                                     prefs);
	prefs->var_id =
		ige_conf_notify_add (conf,
                                     DH_CONF_VARIABLE_FONT,
                                     preferences_var_font_notify_cb,
                                     prefs);
	prefs->fixed_id =
		ige_conf_notify_add (conf,
                                     DH_CONF_FIXED_FONT,
                                     preferences_fixed_font_notify_cb,
                                     prefs);
}

static void
preferences_get_font_names (gboolean   use_system_fonts, 
			    gchar    **variable,
			    gchar    **fixed)
{
	gchar   *var_font_name, *fixed_font_name;
	IgeConf *conf;

	conf = ige_conf_get ();
	
	if (use_system_fonts) {
		ige_conf_get_string (conf,
                                     DH_CONF_SYSTEM_VARIABLE_FONT,
                                     &var_font_name);
		ige_conf_get_string (conf,
                                     DH_CONF_SYSTEM_FIXED_FONT,
                                     &fixed_font_name);
	} else {
		ige_conf_get_string (conf,
                                     DH_CONF_VARIABLE_FONT, 
                                     &var_font_name);
                ige_conf_get_string (conf,
                                     DH_CONF_FIXED_FONT,
                                     &fixed_font_name);
	}

	*variable = var_font_name;
	*fixed = fixed_font_name;
}

static gboolean
preferences_update_fonts (gpointer unused)
{
	IgeConf  *conf;
	gboolean  use_system_fonts;
	gchar    *var_font_name;
	gchar    *fixed_font_name;

	conf = ige_conf_get ();

	ige_conf_get_bool (conf,
                           DH_CONF_USE_SYSTEM_FONTS,
                           &use_system_fonts);

	preferences_get_font_names (use_system_fonts,
				    &var_font_name,
                                    &fixed_font_name);

        /* FIXME: Set WebKit font preferences using WebSettings. */

	g_free (var_font_name);
	g_free (fixed_font_name);

	return FALSE;
}

void
dh_preferences_setup_fonts (void)
{
        preferences_init ();

	preferences_update_fonts (NULL);
	preferences_connect_conf_listeners ();
}

void
dh_preferences_show_dialog (GtkWindow *parent)
{
        gchar      *path;
	GtkBuilder *builder;
	gboolean    use_system_fonts;
	gchar      *var_font_name, *fixed_font_name;

        preferences_init ();

	if (prefs->dialog != NULL) {
		gtk_window_present (GTK_WINDOW (prefs->dialog));
		return;
	}

        path = dh_util_build_data_filename ("devhelp", "ui",
                                            "devhelp.builder",
                                            NULL);
	builder = dh_util_builder_get_file (
                path,
                "preferences_dialog",
                NULL,
                "preferences_dialog", &prefs->dialog,
                "fonts_table", &prefs->fonts_table,
                "system_fonts_button", &prefs->system_fonts_button,
                "variable_font_button", &prefs->variable_font_button,
                "fixed_font_button", &prefs->fixed_font_button,
                NULL);
        g_free (path);

	dh_util_builder_connect (
                builder,
                prefs,
                "variable_font_button", "font_set", preferences_font_set_cb,
                "fixed_font_button", "font_set", preferences_font_set_cb,
                "close_button", "clicked", preferences_close_cb,
                "system_fonts_button", "toggled", preferences_system_fonts_toggled_cb,
                NULL);

	ige_conf_get_bool (ige_conf_get (),
                           DH_CONF_USE_SYSTEM_FONTS,
                           &use_system_fonts);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->system_fonts_button),
				      use_system_fonts);
	gtk_widget_set_sensitive (prefs->fonts_table, !use_system_fonts);

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
	
	g_object_unref (builder);
	
	gtk_window_set_transient_for (GTK_WINDOW (prefs->dialog), parent); 
	gtk_widget_show_all (prefs->dialog);
}
