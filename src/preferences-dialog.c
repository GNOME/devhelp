/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
 * Copyright (C) 2001 Richard Hult <rhult@codefactory.se>
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
 * Author: Johan Dahlin <zilch.am@home.se>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <glade/glade-xml.h>

#include "preferences-dialog.h"
#include "main.h"
#include "preferences.h"

struct _PreferencesDialog {
	Preferences *prefs;
	GConfClient *client;
	GtkWidget   *dialog;
	GtkWidget   *maxhits_spinbutton;
	GtkWidget   *maxhits_button;
	GtkWidget   *sidebar_button;
	GtkWidget   *autocompletion_button;
	GtkWidget   *zoom_menu;
	GtkWidget   *autocompletion_speed_menu;

	gint         sidebar_visible_id;
	gint         zoom_level_id;
	gint         autocompletion_id;
};

static void setup_hack_option_menu (GtkOptionMenu        *option_menu,
				    const OptionMenuData *items,
				    GtkSignalFunc         func,
				    gpointer              user_data);


static void
gconf_zoom_level_changed_cb (GConfClient       *client,
			     guint              notify_id,
			     GConfEntry        *entry,
			     gpointer           data)
{
	PreferencesDialog *dialog;
	gint               level;

	dialog = PREFERENCES_DIALOG (data);
	level = gconf_value_get_int (entry->value);
	
	gtk_option_menu_set_history (GTK_OPTION_MENU (dialog->zoom_menu), level);
}

static void
gconf_sidebar_visible_changed_cb (GConfClient       *client,
				  guint              notify_id,
				  GConfEntry        *entry,
				  gpointer           data)
{
	PreferencesDialog *dialog;
	
	puts (__FUNCTION__);
	g_print ("meep\n");
	
	dialog = PREFERENCES_DIALOG (data);
	
	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (PREFERENCES_DIALOG (data)->sidebar_button),
		gconf_value_get_bool (entry->value));
}

static void
gconf_autocompletion_changed_cb (GConfClient       *client,
				 guint              notify_id,
				 GConfEntry        *entry,
				 gpointer           data)
{
	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (PREFERENCES_DIALOG (data)->autocompletion_button),
		gconf_value_get_bool (entry->value));
}

static gboolean
prefs_destroy_cb (GtkWidget *widget, PreferencesDialog *dialog)
{
	gconf_client_remove_dir (dialog->client, "/apps/devhelp", NULL);

	gconf_client_notify_remove (dialog->client, dialog->sidebar_visible_id);
	gconf_client_notify_remove (dialog->client, dialog->zoom_level_id);
	gconf_client_notify_remove (dialog->client, dialog->autocompletion_id);
	
	gtk_object_unref (GTK_OBJECT (dialog->client));
	dialog->client = NULL;
	dialog->dialog = NULL;
	g_free (dialog);
}

static void
prefs_button_close_clicked_cb (GtkWidget *button,
			       gpointer   user_data)
{
	PreferencesDialog *dialog;

	g_return_if_fail (user_data != NULL);

	dialog = PREFERENCES_DIALOG (user_data);

	gtk_widget_destroy (dialog->dialog);
}

static void
prefs_button_sidebar_toggled_cb (GtkToggleButton *tb, PreferencesDialog *dialog)
{
	preferences_set_sidebar_visible (dialog->prefs,
					 gtk_toggle_button_get_active (tb));
}

static void
prefs_menu_zoom_activate_cb (GtkMenuItem *item, PreferencesDialog *dialog)
{
	gint index;
	
	index = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (item), "index"));

	preferences_set_zoom_level (dialog->prefs, index);
}

static void
prefs_button_autocompletion_toggled_cb (GtkToggleButton *tb, PreferencesDialog *dialog)
{
	gboolean value, maxhits_enabled;

	value = gtk_toggle_button_get_active (tb);
	preferences_set_autocompletion (dialog->prefs, value);

	gtk_widget_set_sensitive (dialog->maxhits_button, FALSE);
	gtk_widget_set_sensitive (dialog->autocompletion_speed_menu, value);
}

static void
prefs_menu_speed_activate_cb (GtkMenuItem *item, PreferencesDialog *dialog)
{
	gint index;
	
	index = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (item), "index"));

	preferences_set_autocompletion_speed (dialog->prefs, index);
}

static void
prefs_button_maxhits_toggled_cb (GtkToggleButton *tb, PreferencesDialog *dialog)
{
	if (gtk_toggle_button_get_active (tb)) {
		gtk_widget_set_sensitive (dialog->maxhits_spinbutton, TRUE);
	} else {
		gtk_widget_set_sensitive (dialog->maxhits_spinbutton, FALSE);
	}
}

static void
prefs_button_maxhits_changed_cb (GtkWidget *widget, PreferencesDialog *dialog)
{
}

void
menu_preferences_activate_cb (GtkMenuItem *menu_item,
			      DevHelp     *devhelp)
{
	PreferencesDialog *dialog;
	GladeXML          *gui;
	GtkWidget         *w;
	gint               index;

	g_return_if_fail (devhelp != NULL);

	dialog = g_new (PreferencesDialog, 1);

	dialog->client = gconf_client_get_default ();
	gconf_client_add_dir (dialog->client, "/apps/devhelp",
			      GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	
	gui = glade_xml_new (DATA_DIR "/devhelp/glade/devhelp.glade", "prefs_dialog");
	
	dialog->dialog = glade_xml_get_widget (gui, "prefs_dialog");
	gtk_signal_connect (GTK_OBJECT (dialog->dialog), "destroy",
			    GTK_SIGNAL_FUNC (prefs_destroy_cb), dialog);

	dialog->prefs = devhelp->preferences;
		
	w = glade_xml_get_widget (gui, "prefs_button_close");
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (prefs_button_close_clicked_cb), dialog);

	dialog->sidebar_button = glade_xml_get_widget (gui, "prefs_checkbutton_sidebar");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->sidebar_button),
				      preferences_get_sidebar_visible (dialog->prefs));
	gtk_signal_connect (GTK_OBJECT (dialog->sidebar_button), "toggled",
			    GTK_SIGNAL_FUNC (prefs_button_sidebar_toggled_cb), dialog);

	dialog->zoom_menu = glade_xml_get_widget (gui, "prefs_menu_zoom");
	setup_hack_option_menu (GTK_OPTION_MENU (dialog->zoom_menu),
				zoom_levels,
				GTK_SIGNAL_FUNC (prefs_menu_zoom_activate_cb),
				dialog);
	index = preferences_get_zoom_level (dialog->prefs);
	gtk_option_menu_set_history (GTK_OPTION_MENU (dialog->zoom_menu), index);

	dialog->autocompletion_button = glade_xml_get_widget (gui, "prefs_button_autocompletion");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->autocompletion_button),
				      preferences_get_autocompletion (dialog->prefs));
	gtk_signal_connect (GTK_OBJECT (dialog->autocompletion_button), "toggled",
			    GTK_SIGNAL_FUNC (prefs_button_autocompletion_toggled_cb), dialog);

	dialog->autocompletion_speed_menu = glade_xml_get_widget (gui, "prefs_menu_autocompletion_speed");
	setup_hack_option_menu (GTK_OPTION_MENU (dialog->autocompletion_speed_menu),
				autocompletion_speeds,
				GTK_SIGNAL_FUNC (prefs_menu_speed_activate_cb),
				dialog);
	index = preferences_get_autocompletion_speed (dialog->prefs);
	gtk_option_menu_set_history (GTK_OPTION_MENU (dialog->autocompletion_speed_menu),
				     index);

	dialog->maxhits_button = glade_xml_get_widget (gui, "prefs_button_maxhits");
	gtk_signal_connect (GTK_OBJECT (dialog->maxhits_button), "toggled",
			    GTK_SIGNAL_FUNC (prefs_button_maxhits_toggled_cb), dialog);

	dialog->maxhits_spinbutton = glade_xml_get_widget (gui, "prefs_spinbutton_maxhits");
	gtk_signal_connect (GTK_OBJECT (dialog->maxhits_spinbutton), "changed",
			    GTK_SIGNAL_FUNC (prefs_button_maxhits_changed_cb), dialog);
		
	gtk_object_unref (GTK_OBJECT (gui));

	dialog->sidebar_visible_id = gconf_client_notify_add (
		dialog->client, "/apps/devhelp/sidebar_visible",
		gconf_sidebar_visible_changed_cb, dialog,
		NULL, NULL);
	
	dialog->zoom_level_id = gconf_client_notify_add (
		dialog->client, "/apps/devhelp/zoom_level",
		gconf_zoom_level_changed_cb, dialog,
		NULL, NULL);
	
	dialog->autocompletion_id = gconf_client_notify_add (
        	dialog->client, "/apps/devhelp/autocompletion",
 		gconf_autocompletion_changed_cb, dialog,
		NULL, NULL);

	gtk_widget_show_all (dialog->dialog);
}

static void
setup_hack_option_menu (GtkOptionMenu        *option_menu,
			const OptionMenuData *items,
			GtkSignalFunc         func,
			gpointer              user_data)
{
	GtkWidget *menu, *menu_item;
	gint       i;
	
	/* Workaround for stupid option menu that has screwed up size
	 * allocation.
	 */

	menu = gtk_option_menu_get_menu (option_menu);
	gtk_widget_destroy (menu);
	
	menu = gtk_menu_new ();
	
	for (i = 0; items[i].label; i++) {
		gint level;
		
		menu_item = gtk_menu_item_new_with_label (_(items[i].label));
		gtk_widget_show (menu_item);
		gtk_menu_append (GTK_MENU (menu), menu_item);

		gtk_object_set_data (GTK_OBJECT (menu_item),
				     "index",
				     GINT_TO_POINTER (i));
		gtk_signal_connect (GTK_OBJECT (menu_item),
				    "activate",
				    func,
				    user_data);
	}		

	gtk_widget_show (menu);
	gtk_option_menu_set_menu (option_menu, menu);
}
