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
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-about.h>
#include "dh-view.h"
#include "GNOME_DevHelp.h"
#include "dh-window.h"

#define DH_WINDOW_UI "GNOME_MrProject_Client.ui"

static void window_class_init                (DhWindowClass *klass);
static void window_init                      (DhWindow      *index);
 
static void window_destroy                   (GtkObject          *object);

static void window_populate                  (DhWindow      *window);

static void window_cmd_print_cb              (BonoboUIComponent  *component,
					      gpointer            data,
					      const gchar        *cname);

static void window_cmd_exit_cb               (BonoboUIComponent  *component,
					      gpointer            data,
					      const gchar        *cname);

static void window_cmd_view_side_bar_cb      (BonoboUIComponent  *component,
					      gpointer            data,
					      const gchar        *cname);

static void window_cmd_about_cb              (BonoboUIComponent  *component,
					      gpointer            data,
					      const gchar        *cname);

static void window_uri_changed_cb            (BonoboListener     *listener,
					      const gchar        *event_name,
					      const CORBA_any    *arg,
					      CORBA_Environment  *ev,
					      gpointer            user_data);

static void window_delete_cb                 (GtkWidget          *widget,
					      GdkEventAny        *event,
					      gpointer            user_data);

static void window_link_clicked_cb           (DhWindow      *ignored,
					      gchar              *url,
					      DhWindow      *window);

static void window_on_url_cb                 (DhWindow      *window,
					      gchar              *url,
					      gpointer            ignored);

static void window_note_change_page_cb       (GtkWidget          *child,
					      GtkNotebook        *notebook);

static void window_note_page_mapped_cb       (GtkWidget          *page, 
					      GtkAccelGroup      *accel_group);

static void window_note_page_unmapped_cb     (GtkWidget          *page, 
					      GtkAccelGroup      *accel_group);


static void window_note_page_setup_signals   (GtkWidget          *page, 
					      GtkAccelGroup      *accel);

static void 
window_notebook_append_page_with_accelerator (GtkNotebook        *notebook,
					      GtkWidget          *page,
					      gchar              *label_text,
					      GtkAccelGroup      *accel);


static BonoboWindowClass *parent_class = NULL;

struct _DhWindowPriv {
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
};

static BonoboUIVerb verbs[] = {
        BONOBO_UI_VERB ("CmdPrint",          window_cmd_print_cb),
        BONOBO_UI_VERB ("CmdExit",           window_cmd_exit_cb),

        BONOBO_UI_VERB ("CmdViewSideBar",    window_cmd_view_side_bar_cb),

        BONOBO_UI_VERB ("CmdAbout",          window_cmd_about_cb),
        BONOBO_UI_VERB_END
};

GtkType
dh_window_get_type (void)
{
        static GtkType dh_window_type = 0;

        if (!dh_window_type) {
                static const GtkTypeInfo dh_window_info = {
                        "DhWindow",
                        sizeof (DhWindow),
                        sizeof (DhWindowClass),
                        (GtkClassInitFunc)  window_class_init,
                        (GtkObjectInitFunc) window_init,
                        /* reserved_1 */ NULL,
                        /* reserved_2 */ NULL,
                        (GtkClassInitFunc) NULL,
                };

                dh_window_type = gtk_type_unique (bonobo_window_get_type (), 
                                                       &dh_window_info);
        }

        return dh_window_type;
}

static void
window_class_init (DhWindowClass *klass)
{
        GtkObjectClass   *object_class;
        
        parent_class = gtk_type_class (bonobo_window_get_type ());

        object_class = (GtkObjectClass *) klass;
        
        object_class->destroy = window_destroy;
}

static void
window_init (DhWindow *window)
{
        DhWindowPriv   *priv;

        priv         = g_new0 (DhWindowPriv, 1);
	
        window->priv = priv;
}

static void
window_destroy (GtkObject *object)
{
}

static void
window_note_change_page_cb (GtkWidget *child, GtkNotebook *notebook)
{
	gint page = gtk_notebook_page_num (notebook, child);

	gtk_notebook_set_page (notebook, page);
}

static void
window_populate (DhWindow *window)
{
        DhWindowPriv    *priv;
        CORBA_Environment     ev;
        Bonobo_UIContainer    uic;
	BonoboControlFrame   *cf;
	Bonobo_EventSource    es;
	Bonobo_Control        control_co;
	GtkWidget            *html_sw;
	GtkWidget            *frame;
	 
        g_return_if_fail (window != NULL);
        g_return_if_fail (IS_DH_WINDOW (window));
        
        priv = window->priv;
        
        priv->notebook    = gtk_notebook_new ();
        priv->search_box  = gtk_vbox_new (FALSE, 0);
	priv->html_widget = dh_view_new ();
        priv->hpaned      = gtk_hpaned_new ();
	priv->statusbar   = gtk_statusbar_new ();
	html_sw           = gtk_scrolled_window_new (NULL, NULL);

        CORBA_exception_init (&ev);

	priv->controller = bonobo_get_object ("OAFIID:GNOME_DevHelp_Controller",
					      "IDL:GNOME/DevHelp/Controller:1.0",
					      &ev);

	if (!priv->controller || BONOBO_EX (&ev)) {
		gchar *exception_as_text = bonobo_exception_get_text (&ev);
		
		g_error ("Couldn't get interface GNOME/DevHelp/Controller:1.0 (%s)",
			 exception_as_text);
		g_free (exception_as_text);
		priv->controller = CORBA_OBJECT_NIL;
	}

	es = Bonobo_Unknown_queryInterface (priv->controller,
					    "IDL:Bonobo/EventSource:1.0",
					    &ev);
	
	if (!es || BONOBO_EX (&ev)) {
		g_error ("Couldn't get EventSource");
	}
	
 	bonobo_event_source_client_add_listener (es,
 						 window_uri_changed_cb,
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

	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
				  priv->index,
				  gtk_label_new_with_mnemonic (_("_Contents")));

	gtk_box_pack_start (GTK_BOX (priv->search_box), 
			    priv->search_entry, 
			    FALSE, FALSE, 0); 

	gtk_box_pack_end_defaults (GTK_BOX (priv->search_box),
				   priv->search_list); 

	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
				  priv->search_box,
				  gtk_label_new_with_mnemonic (_("_Search")));

	gtk_widget_show_all (priv->hpaned);

	bonobo_window_set_contents (BONOBO_WINDOW (window), priv->hpaned);

 	g_signal_connect_object (G_OBJECT (HTML_VIEW (priv->html_widget)->document),
				 "link_clicked", 
				 G_CALLBACK (window_link_clicked_cb),
				 G_OBJECT (window),
				 0);
	/* TODO: Look in gtkhtml2 code or ask jborg */
#if GNOME2_PORT_COMPLETE	
	g_signal_connect_object (G_OBJECT (HTML_VIEW (priv->html_widget)->document),
				 "on_url",
				 G_CALLBACK (window_on_url_cb),
				 G_OBJECT (window),
				 0);
#endif	
}

static void
window_cmd_print_cb (BonoboUIComponent   *component,
	      gpointer             data,
	      const gchar         *cname)
{
	DhWindow       *window;
	DhWindowPriv   *priv;
	
	g_return_if_fail (data != NULL);
	g_return_if_fail (IS_DH_WINDOW (data));
	
	window = DH_WINDOW (data);
	priv   = window->priv;

	g_message ("%s: FIXME!", __FUNCTION__);
#if 0	
	if (priv->html_widget) {
		html_widget_print (DH_VIEW (priv->html_widget));
	}
#endif	
}

static void
window_cmd_exit_cb (BonoboUIComponent   *component,
	     gpointer             data,
	     const gchar         *cname)
{
	bonobo_main_quit ();
}

static void
window_cmd_view_side_bar_cb (BonoboUIComponent   *component,
		      gpointer             data,
		      const gchar         *cname)
{
}

static void
window_cmd_about_cb (BonoboUIComponent    *component,
	      gpointer              data,
	      const gchar          *cname)
{
        GtkWidget *about;

        const gchar *authors[] = {
		"Johan Dahlin <jdahlin@telia.com>",
                "Mikael Hallendal <micke@codefactory.se>",
                "Richard Hult <rhult@codefactory.se>",
                NULL
        };

        about = gnome_about_new (PACKAGE, VERSION,
				 "(C) 2001 Johan Dahlin <jdahlin@telia.com>", 
				 _("A developer's help browser for GNOME 2"),
                                 authors,
                                 NULL,
                                 NULL,
                                 NULL);
                                
        gtk_widget_show (about);
}

static void
window_uri_changed_cb (BonoboListener      *listener,
		   const gchar         *event_name,
		   const CORBA_any     *any,
		   CORBA_Environment   *ev,
		   gpointer             user_data)
{
	DhWindow       *window;
	DhWindowPriv   *priv;
	gchar               *uri;
	
	g_return_if_fail (user_data != NULL);
	g_return_if_fail (IS_DH_WINDOW (user_data));
	
	window = DH_WINDOW (user_data);
	priv   = window->priv;
	uri  = g_strdup (any->_value);

	if (uri) {
		dh_view_open_uri (DH_VIEW (priv->html_widget), uri);
	}
	
	g_free (uri);
}

static void
window_delete_cb (GtkWidget     *widget,
	      GdkEventAny   *event,
	      gpointer       user_data)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (IS_DH_WINDOW (widget));
	
	bonobo_main_quit ();
}

static void
window_link_clicked_cb (DhWindow   *ignored,
		    gchar           *url,
		    DhWindow   *window)
{
	DhWindowPriv   *priv;

	g_return_if_fail (window != NULL);
	g_return_if_fail (IS_DH_WINDOW (window));
	
	priv = window->priv;

	GNOME_DevHelp_Controller_openURI (priv->controller, url, NULL);
}

static void
window_on_url_cb (DhWindow *window, gchar *url, gpointer ignored)
{
	DhWindowPriv   *priv;
	gchar               *status_text;
	
	g_return_if_fail (window != NULL);
	g_return_if_fail (IS_DH_WINDOW (window));
	
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

GtkWidget *
dh_window_new (void)
{
        DhWindow       *window;
        DhWindowPriv   *priv;
        GtkWidget           *widget;
        BonoboUIContainer   *ui_container;
	BonoboUIEngine      *ui_engine;
	CORBA_Environment    ev;
	GdkPixbuf           *icon;
	
        window = gtk_type_new (TYPE_DH_WINDOW);
        priv   = window->priv;

	CORBA_exception_init (&ev);
	
        ui_container = bonobo_ui_container_new ();
        bonobo_ui_container_set_engine (BONOBO_UI_CONTAINER (ui_container),
                                        bonobo_window_get_ui_engine (BONOBO_WINDOW (window)));

        priv->component = bonobo_ui_component_new ("DevHelp");

        bonobo_ui_engine_config_set_path (
                bonobo_window_get_ui_engine (BONOBO_WINDOW (window)),
		"/apps/devhelp/ui-config/bonobo");
	
        bonobo_ui_component_set_container (priv->component, 
                                           BONOBO_OBJREF (ui_container),
					   &ev);
        
        bonobo_ui_component_freeze (priv->component, NULL);

        bonobo_ui_util_set_ui (priv->component,
			       DATA_DIR,
                               "GNOME_DevHelp.ui",
			       "devhelp",
			       &ev);

        bonobo_ui_component_add_verb_list_with_data (priv->component,
                                                     verbs,
                                                     window);

        widget = bonobo_window_construct (BONOBO_WINDOW (window),
					  BONOBO_UI_CONTAINER (ui_container),
                                          "DevHelp",
					  "DevHelp");

        gtk_window_set_policy (GTK_WINDOW (widget), TRUE, TRUE, FALSE);
        
        gtk_window_set_default_size (GTK_WINDOW (widget), 700, 500);
	
	gtk_window_set_wmclass (GTK_WINDOW (window), "devhelp", "DevHelp");

	g_signal_connect (GTK_OBJECT (window), 
			  "delete_event",
			  G_CALLBACK (window_delete_cb),
			  NULL);

        window_populate (window);

	icon = gdk_pixbuf_new_from_file (DATA_DIR "/pixmaps/devhelp.png", NULL);
	if (icon) {
		gtk_window_set_icon (GTK_WINDOW (window), icon);
		g_object_unref (icon);
	}
	
        bonobo_ui_component_thaw (priv->component, NULL);

	CORBA_exception_free (&ev);
	
	return widget;
}

void
dh_window_search (DhWindow *window, const gchar *str)
{
	DhWindowPriv   *priv;
	CORBA_Environment    ev;
	
	g_return_if_fail (window != NULL);
	g_return_if_fail (IS_DH_WINDOW (window));
	
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
