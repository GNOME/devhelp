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
 * Author: Richard Hult <rhult@codefactory.se>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdkx.h>
#include "devhelp-window.h"
#include "help-browser.h"

static void help_browser_class_init (HelpBrowserClass *klass);
static void help_browser_init       (HelpBrowser *browser);

#define PARENT_TYPE BONOBO_X_OBJECT_TYPE
static BonoboXObjectClass *parent_class;

struct _HelpBrowserPriv {
/*  	GSList          *windows;  */

	GtkWidget       *window;
};

static void
eel_gdk_window_focus (GdkWindow *window, guint32 timestamp)
{
	gdk_error_trap_push ();
	XSetInputFocus (GDK_DISPLAY (),
			GDK_WINDOW_XWINDOW (window),
			RevertToNone,
			timestamp);
	gdk_flush();
	gdk_error_trap_pop ();
}

/**
 * eel_gdk_window_bring_to_front:
 * 
 * Raise window and give it focus.
 */
static void 
eel_gdk_window_bring_to_front (GdkWindow *window)
{
	/* This takes care of un-iconifying the window and
	 * raising it if needed.
	 */
	gdk_window_show (window);

	/* If the window was already showing, it would not have
	 * the focus at this point. Do a little X trickery to
	 * ensure it is focused.
	 */
	eel_gdk_window_focus (window, GDK_CURRENT_TIME);
}

/* GNOME::DevHelp::search (in string) impl
 *
 */
static void
impl_HelpBrowser_search (PortableServer_Servant   servant,
			 const CORBA_char        *str,
			 CORBA_Environment       *ev)
{
	HelpBrowser       *browser;
	HelpBrowserPriv   *priv;
	
	browser = HELP_BROWSER (bonobo_x_object (servant));

	priv = browser->priv;
		
	if (priv->window) {
		devhelp_window_search (DEVHELP_WINDOW (priv->window), str);
		eel_gdk_window_bring_to_front (priv->window->window);
	}
}

/* GNOME::DevHelp::searchInNewWindow (in string) impl
 *
 */
static void
impl_HelpBrowser_searchInNewWindow (PortableServer_Servant  servant,
				    const CORBA_char       *str,
				    CORBA_Environment      *ev)
{
	HelpBrowser *browser;

	browser = HELP_BROWSER (bonobo_x_object (servant));

	g_print ("Not yet impl.\n");
	/* FIX: SearchInNewWindow */
}

static void
help_browser_class_init (HelpBrowserClass *klass)
{
	GtkObjectClass                       *object_class;
	POA_GNOME_DevHelp_HelpBrowser__epv   *epv = &klass->epv;

	object_class = (GtkObjectClass *) klass;
	parent_class = gtk_type_class (PARENT_TYPE);

	epv->search            = impl_HelpBrowser_search;
	epv->searchInNewWindow = impl_HelpBrowser_searchInNewWindow;
}

static void
help_browser_init (HelpBrowser *browser)
{
	HelpBrowserPriv   *priv;
	
	priv          = g_new0 (HelpBrowserPriv, 1);
/* 	priv->windows = NULL; */
	priv->window  = NULL;
	browser->priv = priv;
}

HelpBrowser*
help_browser_new (void)
{
	HelpBrowser       *browser;
	HelpBrowserPriv   *priv;
/* 	GtkWidget         *window; */

	browser = gtk_type_new (HELP_BROWSER_TYPE);
	priv    = browser->priv;
	priv->window  = devhelp_window_new ();
	
/* 	priv->windows = g_slist_prepend (priv->windows, window); */
	
	gtk_widget_show (priv->window);
	
	return browser;
}

BONOBO_X_TYPE_FUNC_FULL (HelpBrowser, 
			 GNOME_DevHelp_HelpBrowser,
                         PARENT_TYPE, 
			 help_browser);

