/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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
 * Author: Mikael Hallendal <micke@codefactory.se>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtkhpaned.h>
#include <gtk/gtklabel.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkvbox.h>
#include <bonobo.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-about.h>
#include "html-widget.h"
#include "GNOME_DevHelp.h"
#include "preferences.h"
#include "devhelp-window.h"

#define DEVHELP_WINDOW_UI "GNOME_MrProject_Client.ui"

static void devhelp_window_class_init        (DevHelpWindowClass   *klass);
static void devhelp_window_init              (DevHelpWindow        *index);
 
static void devhelp_window_destroy           (GtkObject            *object);


static void devhelp_window_populate          (DevHelpWindow        *window);

static void cmd_add_book_cb                  (BonoboUIComponent    *component,
                                              gpointer              data,
                                              const gchar          *cname);

static void cmd_print_cb                     (BonoboUIComponent    *component,
                                              gpointer              data,
                                              const gchar          *cname);

static void cmd_exit_cb                      (BonoboUIComponent    *component,
                                              gpointer              data,
                                              const gchar          *cname);

static void cmd_view_side_bar_cb             (BonoboUIComponent    *component,
                                              gpointer              data,
                                              const gchar          *cname);

static void cmd_preferences_cb               (BonoboUIComponent    *component,
                                              gpointer              data,
                                              const gchar          *cname);

static void cmd_about_cb                     (BonoboUIComponent    *component,
                                              gpointer              data,
                                              const gchar          *cname);

static void size_ui_listener                 (BonoboUIComponent            *component,
					      const char                   *path,
					      Bonobo_UIComponent_EventType  type,
					      const char                   *state,
					      gpointer                      user_data);

static void devhelp_window_uri_changed_cb    (BonoboListener       *listener,
					      gchar                *event_name,
					      CORBA_any            *arg,
					      CORBA_Environment    *ev,
					      gpointer              user_data);

static void devhelp_window_delete_cb         (GtkWidget            *widget,
					      GdkEventAny          *event,
					      gpointer              user_data);

static void devhelp_window_link_clicked_cb   (DevHelpWindow        *window,
					      gchar                *url,
					      gpointer              ignored);

static void devhelp_window_on_url_cb         (DevHelpWindow        *window,
					      gchar                *url,
					      gpointer              ignored);
static void
devhelp_window_zoom_level_changed_cb         (Preferences         *prefs,
					      gint                 zoom_level,
					      DevHelpWindow       *window);

static BonoboWindowClass *parent_class = NULL;

struct _DevHelpWindowPriv {
        BonoboUIComponent          *component;

	GNOME_DevHelp_Controller    controller;

        GtkWidget                  *notebook;
        GtkWidget                  *search_box;
        GtkWidget                  *index;
        GtkWidget                  *search_list;
        GtkWidget                  *search_entry;
	GtkWidget                  *html_widget;
	GtkWidget                  *statusbar;
	GtkWidget                  *hpaned;
	
	Preferences                *prefs;
};

static BonoboUIVerb verbs[] = {
        BONOBO_UI_VERB ("CmdPrint",          cmd_print_cb),
        BONOBO_UI_VERB ("CmdExit",           cmd_exit_cb),

        BONOBO_UI_VERB ("CmdViewSideBar",    cmd_view_side_bar_cb),

	BONOBO_UI_VERB ("CmdPrefs",          cmd_preferences_cb),

        BONOBO_UI_VERB ("CmdAbout",          cmd_about_cb),
        BONOBO_UI_VERB_END
};

GtkType
devhelp_window_get_type (void)
{
        static GtkType devhelp_window_type = 0;

        if (!devhelp_window_type) {
                static const GtkTypeInfo devhelp_window_info = {
                        "DevHelpWindow",
                        sizeof (DevHelpWindow),
                        sizeof (DevHelpWindowClass),
                        (GtkClassInitFunc)  devhelp_window_class_init,
                        (GtkObjectInitFunc) devhelp_window_init,
                        /* reserved_1 */ NULL,
                        /* reserved_2 */ NULL,
                        (GtkClassInitFunc) NULL,
                };

                devhelp_window_type = gtk_type_unique (bonobo_window_get_type (), 
                                                       &devhelp_window_info);
        }

        return devhelp_window_type;
}

static void
devhelp_window_class_init (DevHelpWindowClass *klass)
{
        GtkObjectClass   *object_class;
        
        parent_class = gtk_type_class (bonobo_window_get_type ());

        object_class = (GtkObjectClass *) klass;
        
        object_class->destroy = devhelp_window_destroy;
}

static void
devhelp_window_init (DevHelpWindow *window)
{
        DevHelpWindowPriv   *priv;

        priv         = g_new0 (DevHelpWindowPriv, 1);
        priv->prefs  = preferences_new ();
	
	gtk_signal_connect (GTK_OBJECT (priv->prefs),
			    "zoom_level_changed",
			    GTK_SIGNAL_FUNC (devhelp_window_zoom_level_changed_cb),
			    window);
	
	gtk_object_set (GTK_OBJECT (priv->prefs),
			"sidebar_visible", TRUE,
			NULL);
	
        window->priv = priv;
}

static void
devhelp_window_destroy (GtkObject *object)
{
}

static void
note_change_page (GtkWidget *child, GtkNotebook *notebook)
{
	gint page = gtk_notebook_page_num (notebook, child);

	gtk_notebook_set_page (notebook, page);
}

static void
note_page_mapped (GtkWidget *page, GtkAccelGroup *accel_group)
{
	GtkWidget *dialog = gtk_widget_get_toplevel (GTK_WIDGET (page));

	gtk_window_add_accel_group (GTK_WINDOW (dialog), accel_group);
}

static void
note_page_unmapped (GtkWidget *page, GtkAccelGroup *accel_group)
{
	GtkWidget *dialog = gtk_widget_get_toplevel (GTK_WIDGET (page));

	gtk_window_remove_accel_group (GTK_WINDOW (dialog), accel_group);
}

static void
note_page_setup_signals (GtkWidget *page, GtkAccelGroup *accel)
{
	gtk_accel_group_ref (accel);
	gtk_signal_connect_full (GTK_OBJECT (page),
			    "map",
			    GTK_SIGNAL_FUNC (note_page_mapped), NULL, 
			    accel, (GtkDestroyNotify) gtk_accel_group_unref,
			    FALSE, FALSE);
	gtk_accel_group_ref (accel);
	gtk_signal_connect_full (GTK_OBJECT (page),
			    "unmap",
			    GTK_SIGNAL_FUNC (note_page_unmapped), NULL,
			    accel, (GtkDestroyNotify) gtk_accel_group_unref,
			    FALSE, FALSE);
}

static void
notebook_append_page_with_accelerator (GtkNotebook   *notebook,
				       GtkWidget     *page,
				       gchar         *label_text,
				       GtkAccelGroup *accel)
{
	GtkWidget     *label;
	guint          key;

	label = gtk_label_new (NULL);
	key = gtk_label_parse_uline (GTK_LABEL (label), label_text);
	gtk_widget_show (label);

	note_page_setup_signals (page, accel);

	gtk_notebook_append_page (notebook, page, label);
	
	if (key) {
		gtk_widget_add_accelerator (page, "grab_focus",
					    accel,
					    key,
					    GDK_MOD1_MASK,
					    0);
		gtk_signal_connect (GTK_OBJECT (page),
				    "grab_focus",
				    GTK_SIGNAL_FUNC (note_change_page),
				    notebook);
	}
}

static void
devhelp_window_populate (DevHelpWindow *window)
{
        DevHelpWindowPriv    *priv;
        CORBA_Environment     ev;
        Bonobo_UIContainer    uic;
	BonoboControlFrame   *cf;
	Bonobo_EventSource    es;
	Bonobo_Control        control_co;
	gint                  zoom_level;
	GtkWidget            *html_sw;
	GtkWidget            *frame;
	GtkAccelGroup        *accel;
	 
        g_return_if_fail (window != NULL);
        g_return_if_fail (IS_DEVHELP_WINDOW (window));
        
        priv = window->priv;
        
        priv->notebook    = gtk_notebook_new ();
        priv->search_box  = gtk_vbox_new (FALSE, 0);
	priv->html_widget = html_widget_new ();
        priv->hpaned      = gtk_hpaned_new ();
	priv->statusbar   = gtk_statusbar_new ();
	html_sw           = gtk_scrolled_window_new (NULL, NULL);

	gtk_object_get (GTK_OBJECT (priv->prefs),
			"zoom_level", &zoom_level,
			NULL);

	devhelp_window_zoom_level_changed_cb (priv->prefs, 
					      zoom_level,
					      window);

        CORBA_exception_init (&ev);

	priv->controller = bonobo_get_object ("OAFIID:GNOME_DevHelp_Controller",
					      "IDL:GNOME/DevHelp/Controller:1.0",
					      &ev);

	if (!priv->controller || BONOBO_EX (&ev)) {
		g_error ("Couldn't get interface GNOME/DevHelp/Controller:1.0");
	}

	es = Bonobo_Unknown_queryInterface (priv->controller,
					    "IDL:Bonobo/EventSource:1.0",
					    &ev);
	
	if (!es || BONOBO_EX (&ev)) {
		g_error ("Couldn't get EventSource");
	}
	
 	bonobo_event_source_client_add_listener (es,
 						 devhelp_window_uri_changed_cb,
 						 "GNOME/DevHelp:URI:changed",
 						 NULL, 
 						 window);

        uic = bonobo_ui_component_get_container (priv->component);

	GNOME_DevHelp_Controller_addMenus (priv->controller, uic, &ev);
	
	if (BONOBO_EX (&ev)) {
		g_warning ("Couldn't register component");
	}

	control_co = GNOME_DevHelp_Controller_getBookIndex (priv->controller,
							    &ev);
	
	if (!control_co || BONOBO_EX (&ev)) {
		g_error ("Argggh");
	}
	
	priv->index = bonobo_widget_new_control_from_objref (control_co, uic);
	
	control_co = GNOME_DevHelp_Controller_getSearchEntry (priv->controller,
							      &ev);
	
	if (!control_co || BONOBO_EX (&ev)) {
		g_error ("Argggh");
	}
	
	priv->search_entry = bonobo_widget_new_control_from_objref (control_co,
								    uic);
	control_co = GNOME_DevHelp_Controller_getSearchResultList (priv->controller,
								   &ev);
	
	if (!control_co || BONOBO_EX (&ev)) {
		g_error ("Argggh");
	}

	priv->search_list = bonobo_widget_new_control_from_objref (control_co,
								   uic);
	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (frame), priv->notebook);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

	gtk_paned_add1 (GTK_PANED (priv->hpaned), frame);
	
 	gtk_container_add (GTK_CONTAINER (html_sw), priv->html_widget);

	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (frame), html_sw);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

 	gtk_paned_add2 (GTK_PANED(priv->hpaned), frame);

 	gtk_paned_set_position (GTK_PANED (priv->hpaned), 250);

	accel = gtk_accel_group_new ();

	notebook_append_page_with_accelerator (GTK_NOTEBOOK (priv->notebook),
					       priv->index,
					       _("_Contents"),
					       accel);

	gtk_box_pack_start (GTK_BOX (priv->search_box), 
			    priv->search_entry, 
			    FALSE, FALSE, 0); 

	gtk_box_pack_end_defaults (GTK_BOX (priv->search_box),
				   priv->search_list); 

	notebook_append_page_with_accelerator (GTK_NOTEBOOK (priv->notebook),
					       priv->search_box,
					       _("_Search"),
					       accel);

	gtk_widget_show_all (priv->hpaned);

	bonobo_window_set_contents (BONOBO_WINDOW (window), priv->hpaned);

 	gtk_signal_connect_object (GTK_OBJECT (priv->html_widget),
				   "link_clicked", 
				   GTK_SIGNAL_FUNC (devhelp_window_link_clicked_cb),
				   GTK_OBJECT (window));
	
	gtk_signal_connect_object (GTK_OBJECT (priv->html_widget),
				   "on_url",
				   GTK_SIGNAL_FUNC (devhelp_window_on_url_cb),
				   GTK_OBJECT (window));
}

static void
cmd_print_cb (BonoboUIComponent   *component,
	      gpointer             data,
	      const gchar         *cname)
{
	DevHelpWindow       *window;
	DevHelpWindowPriv   *priv;
	gint                 zoom_level;
	
	g_return_if_fail (data != NULL);
	g_return_if_fail (IS_DEVHELP_WINDOW (data));
	
	window = DEVHELP_WINDOW (data);
	priv   = window->priv;

	if (priv->html_widget) {
		html_widget_print (HTML_WIDGET (priv->html_widget));
	}
}

static void
cmd_exit_cb (BonoboUIComponent   *component,
	     gpointer             data,
	     const gchar         *cname)
{
	gtk_main_quit ();
}

static void
cmd_view_side_bar_cb (BonoboUIComponent   *component,
		      gpointer             data,
		      const gchar         *cname)
{
}

static void
cmd_preferences_cb (BonoboUIComponent   *component,
		    gpointer             data,
		    const gchar         *cname)
{
	DevHelpWindow       *window;
	DevHelpWindowPriv   *priv;
	
	g_return_if_fail (data != NULL);
	g_return_if_fail (IS_DEVHELP_WINDOW (data));
	
	window = DEVHELP_WINDOW (data);
	priv   = window->priv;
	
	preferences_open_dialog (priv->prefs);
}

static void
cmd_about_cb (BonoboUIComponent    *component,
               gpointer              data,
               const gchar          *cname)
{
	GtkWidget    *about;
	const gchar  *authors[] = {
		"Johan Dahlin <zilch.am@home.se>",
		"Mikael Hallendal <micke@codefactory.se>",
		"Rickard Hult <rhult@codefactory.se>",
		NULL
	};
	 
	about = gnome_about_new ("DevHelp", 
                                 VERSION,
				 _("(C) Copyright 2001, Johan Dahlin"), 
                                 authors,
				 _("A developer's help browser"), 
                                 NULL);

	gtk_widget_show (GTK_WIDGET (about));
}

static void
size_ui_listener (BonoboUIComponent            *component,
		  const char                   *path,
		  Bonobo_UIComponent_EventType  type,
		  const char                   *state,
		  gpointer                      user_data)
{
	
	DevHelpWindow       *window;
	DevHelpWindowPriv   *priv;
	gint                 zoom_level;
	
	g_return_if_fail (user_data != NULL);
	g_return_if_fail (IS_DEVHELP_WINDOW (user_data));
	
	window = DEVHELP_WINDOW (user_data);
	priv   = window->priv;
	
	/* If it's not selected */
	if (strcmp (state, "1") != 0) {
		return;
	}

	if (!strcmp (path, "CmdSizeTiny")) {
		zoom_level = ZOOM_TINY_INDEX;
	}
	else if (!strcmp (path, "CmdSizeSmall")) {
		zoom_level = ZOOM_SMALL_INDEX;
	}
	else if (!strcmp (path, "CmdSizeMedium")) {
		zoom_level = ZOOM_MEDIUM_INDEX;
	}
	else if (!strcmp (path, "CmdSizeLarge")) {
		zoom_level = ZOOM_LARGE_INDEX;
	}
	else if (!strcmp (path, "CmdSizeHuge")) {
		zoom_level = ZOOM_HUGE_INDEX;
	} else {
		g_warning ("Unsupported size");
	}

	gtk_object_set (GTK_OBJECT (priv->prefs),
			"zoom_level", zoom_level,
			NULL);
}

static void
devhelp_window_uri_changed_cb (BonoboListener      *listener,
			       gchar               *event_name,
			       CORBA_any           *any,
			       CORBA_Environment   *ev,
			       gpointer             user_data)
{
	DevHelpWindow       *window;
	DevHelpWindowPriv   *priv;
	gchar               *value;
	GnomeVFSURI         *uri;
	
	g_return_if_fail (user_data != NULL);
	g_return_if_fail (IS_DEVHELP_WINDOW (user_data));
	
	window = DEVHELP_WINDOW (user_data);
	priv   = window->priv;
	value  = g_strdup (any->_value);
	uri    = gnome_vfs_uri_new (value);

	if (uri) {
		html_widget_open_uri (HTML_WIDGET (priv->html_widget), uri);
		gnome_vfs_uri_unref (uri);
	}
	
	g_free (value);
}

static void
devhelp_window_delete_cb (GtkWidget     *widget,
			  GdkEventAny   *event,
			  gpointer       user_data)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (IS_DEVHELP_WINDOW (widget));
	
	gtk_main_quit ();
}

static void
devhelp_window_link_clicked_cb (DevHelpWindow   *window,
				gchar           *url,
				gpointer         ignored)
{
	DevHelpWindowPriv   *priv;

	g_return_if_fail (window != NULL);
	g_return_if_fail (IS_DEVHELP_WINDOW (window));
	
	priv = window->priv;

	GNOME_DevHelp_Controller_openURI (priv->controller, url, NULL);
}

static void
devhelp_window_on_url_cb (DevHelpWindow *window, gchar *url, gpointer ignored)
{
	DevHelpWindowPriv   *priv;
	gchar               *status_text;
	
	g_return_if_fail (window != NULL);
	g_return_if_fail (IS_DEVHELP_WINDOW (window));
	
	priv = window->priv;

	bonobo_ui_component_set_status (priv->component, "", NULL);

	if (url) {
		status_text = g_strdup_printf (_("Open %s"), url);
		bonobo_ui_component_set_status (priv->component,
						status_text,
						NULL);
		g_free (status_text);
	}
}

static void
devhelp_window_zoom_level_changed_cb (Preferences     *prefs,
				      gint             zoom_level,
				      DevHelpWindow   *window)
{
	DevHelpWindowPriv   *priv;
	gdouble              magnification;

	g_return_if_fail (window != NULL);
	g_return_if_fail (IS_DEVHELP_WINDOW (window));
	
	priv = window->priv;

	magnification = zoom_levels[zoom_level].data / 100.0;
	magnification = CLAMP (magnification, 0.05, 20.0);
	
	gtk_html_set_magnification (GTK_HTML (priv->html_widget),
				    magnification);

	/* FIX: Show this in the menu somehow */
}

GtkWidget *
devhelp_window_new (void)
{
        DevHelpWindow       *window;
        DevHelpWindowPriv   *priv;
        GtkWidget           *widget;
        BonoboUIContainer   *ui_container;
	
        window = gtk_type_new (TYPE_DEVHELP_WINDOW);
        priv   = window->priv;

        ui_container = bonobo_ui_container_new ();

        bonobo_ui_container_set_win (ui_container, BONOBO_WINDOW (window));

        bonobo_ui_engine_config_set_path (
                bonobo_window_get_ui_engine (BONOBO_WINDOW (window)),
                "DevHelp/UIConf/kvps");
	
        priv->component = bonobo_ui_component_new ("DevHelp");

	bonobo_ui_component_add_listener (priv->component, "CmdSizeTiny",
					  size_ui_listener, window);
	
	bonobo_ui_component_add_listener (priv->component, "CmdSizeSmall",
					  size_ui_listener, window);
	
	bonobo_ui_component_add_listener (priv->component, "CmdSizeMedium",
					  size_ui_listener, window);
	
	bonobo_ui_component_add_listener (priv->component, "CmdSizeLarge",
					  size_ui_listener, window);
	
	bonobo_ui_component_add_listener (priv->component, "CmdSizeHuge",
					  size_ui_listener, window);	
	
        bonobo_ui_component_set_container (priv->component, 
                                           BONOBO_OBJREF (ui_container));
        
        bonobo_ui_component_freeze (priv->component, NULL);

        bonobo_ui_util_set_ui (priv->component, DATA_DIR,
                               "GNOME_DevHelp.ui", "devhelp");

        bonobo_ui_component_add_verb_list_with_data (priv->component,
                                                     verbs,
                                                     window);

        widget = bonobo_window_construct (BONOBO_WINDOW (window), 
                                          "DevHelp", "DevHelp");

        gtk_window_set_policy (GTK_WINDOW (widget), TRUE, TRUE, FALSE);
        
        gtk_window_set_default_size (GTK_WINDOW (widget), 700, 500);
	
	gtk_window_set_wmclass (GTK_WINDOW (window), "devhelp", "DevHelp");

	gtk_signal_connect (GTK_OBJECT (window), 
			    "delete_event",
			    GTK_SIGNAL_FUNC (devhelp_window_delete_cb),
			    NULL);

        devhelp_window_populate (window);

        bonobo_ui_component_thaw (priv->component, NULL);
	
	return widget;
}

void
devhelp_window_search (DevHelpWindow *window, const gchar *str)
{
	DevHelpWindowPriv   *priv;
	CORBA_Environment    ev;
	
	g_return_if_fail (window != NULL);
	g_return_if_fail (IS_DEVHELP_WINDOW (window));
	
	priv = window->priv;
	
	CORBA_exception_init (&ev);
	
	if (priv->controller) {
		GNOME_DevHelp_Controller_search (priv->controller, str, &ev);
	}

	if (BONOBO_EX (&ev)) {
		g_warning ("Error while sending search request");
	}
 
 	gtk_notebook_set_page (GTK_NOTEBOOK (priv->notebook), 1);

	CORBA_exception_free (&ev);
}

