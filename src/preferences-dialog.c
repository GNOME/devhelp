/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
 * Copyright (C) 2001 Richard Hult <rhult@codefactory.se>
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
 * Author: Johan Dahlin <zilch.am@home.se>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-i18n.h>
#include <glade/glade-xml.h>
#include "preferences-dialog.h"

struct _PreferencesDialog {
	Preferences   *prefs;
	GtkWidget     *dialog;
	GtkWidget     *maxhits_spinbutton;
	GtkWidget     *maxhits_button;
	GtkWidget     *sidebar_button;
	GtkWidget     *zoom_menu;
};

static void setup_hack_option_menu (GtkOptionMenu        *option_menu,
				    const OptionMenuData *items,
				    GCallback             func,
				    gpointer              user_data);


static void
pd_zoom_level_changed_cb (Preferences         *prefs,
			  gint                 zoom_level,
			  PreferencesDialog   *dialog)
{
	g_return_if_fail (dialog != NULL);

	gtk_option_menu_set_history (GTK_OPTION_MENU (dialog->zoom_menu), 
				     zoom_level);
}

static void
pd_sidebar_visible_changed_cb (Preferences         *prefs,
			       gboolean             visible,
			       PreferencesDialog   *dialog)
{
	g_return_if_fail (dialog != NULL);
	
	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (dialog->sidebar_button), visible);
}

static gboolean
prefs_destroy_cb (GtkWidget *widget, PreferencesDialog *dialog)
{
#if GNOME2_CONVERSION_COMPLETE   
	g_signal_handlers_disconnect_by_data (G_OBJECT (dialog->prefs),
			                      dialog);
#endif
	gtk_widget_destroy (dialog->dialog);

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
prefs_button_sidebar_toggled_cb (GtkToggleButton     *tb, 
				 PreferencesDialog   *dialog)
{
	g_return_if_fail (dialog != NULL);
	
	g_object_set (G_OBJECT (dialog->prefs), 
		      "sidebar_visible", gtk_toggle_button_get_active (tb),
		      NULL);
}

static void
prefs_menu_zoom_activate_cb (GtkMenuItem *item, PreferencesDialog *dialog)
{
	gint index;
	
	index = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), 
						    "index"));

	g_object_set (G_OBJECT (dialog->prefs),
		      "zoom_level", index,
		      NULL);
}

static void
prefs_button_maxhits_toggled_cb (GtkToggleButton     *tb, 
				 PreferencesDialog   *dialog)
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

static void
setup_hack_option_menu (GtkOptionMenu          *option_menu,
			const OptionMenuData   *items,
			GCallback               func,
			gpointer                user_data)
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

		g_object_set_data (G_OBJECT (menu_item),
				   "index",
				   GINT_TO_POINTER (i));

		g_signal_connect (G_OBJECT (menu_item),
				  "activate",
				  func,
				  user_data);
	}		

	gtk_widget_show (menu);
	gtk_option_menu_set_menu (option_menu, menu);
}

GtkWidget *
preferences_dialog_new (Preferences *prefs)
{
	PreferencesDialog *dialog;
	GladeXML          *gui;
	GtkWidget         *w;
	gboolean           sb_visible;
	gint               sb_position, zoom_level;
	
	g_return_if_fail (prefs != NULL);

	dialog = g_new (PreferencesDialog, 1);
	dialog->prefs = prefs;

	gui = glade_xml_new (DATA_DIR "/devhelp/glade/devhelp2.glade", 
			     "prefs_dialog",
			     NULL);
	
	dialog->dialog = glade_xml_get_widget (gui, "prefs_dialog");

	g_signal_connect (G_OBJECT (dialog->dialog),
			  "destroy",
			  G_CALLBACK (prefs_destroy_cb), 
			  dialog);

	g_signal_connect (G_OBJECT (prefs),
			  "sidebar_visible_changed",
			  G_CALLBACK (pd_sidebar_visible_changed_cb),
			  dialog);

	g_signal_connect (G_OBJECT (prefs),
			  "zoom_level_changed",
			  G_CALLBACK (pd_zoom_level_changed_cb),
			  dialog);
	
	w = glade_xml_get_widget (gui, "prefs_button_close");

	g_signal_connect (G_OBJECT (w),
			  "clicked",
			  G_CALLBACK (prefs_button_close_clicked_cb),
			  dialog);

	g_object_get (G_OBJECT (dialog->prefs), 
		      "sidebar_visible", &sb_visible,
		      "sidebar_position", &sb_position,
		      "zoom_level", &zoom_level,
		      NULL);
	
	dialog->sidebar_button = 
		glade_xml_get_widget (gui, "prefs_checkbutton_sidebar");

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->sidebar_button),
				      sb_visible);

	g_signal_connect (G_OBJECT (dialog->sidebar_button), 
			  "toggled",
			  G_CALLBACK (prefs_button_sidebar_toggled_cb), 
			  dialog);

	dialog->zoom_menu = glade_xml_get_widget (gui, "prefs_menu_zoom");

	setup_hack_option_menu (GTK_OPTION_MENU (dialog->zoom_menu),
				zoom_levels,
				G_CALLBACK (prefs_menu_zoom_activate_cb),
				dialog);

	gtk_option_menu_set_history (GTK_OPTION_MENU (dialog->zoom_menu), 
				     zoom_level);

	dialog->maxhits_button = glade_xml_get_widget (gui, 
						       "prefs_button_maxhits");

	g_signal_connect (G_OBJECT (dialog->maxhits_button), 
			  "toggled",
			  G_CALLBACK (prefs_button_maxhits_toggled_cb), 
			  dialog);

	dialog->maxhits_spinbutton = 
		glade_xml_get_widget (gui, "prefs_spinbutton_maxhits");

	g_signal_connect (G_OBJECT (dialog->maxhits_spinbutton), 
			  "changed",
			  G_CALLBACK (prefs_button_maxhits_changed_cb), 
			  dialog);
		
	g_object_unref (G_OBJECT (gui));

	return dialog->dialog;
}

