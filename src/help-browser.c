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
 * Author: Richard Hult <rhult@codefactory.se>
 */

#include "help-browser.h"

static void help_browser_class_init (HelpBrowserClass *klass);
static void help_browser_init       (HelpBrowser *browser);

#define PARENT_TYPE BONOBO_X_OBJECT_TYPE
static BonoboXObjectClass *parent_class;

enum { 
	SEARCH,
	SEARCH_IN_NEW_WINDOW,
	LAST_SIGNAL 
}; 

static gint signals[LAST_SIGNAL] = { 0 };

/* GNOME::DevHelp::search (in string) impl
 *
 */
static void
impl_HelpBrowser_search (PortableServer_Servant   servant,
			 const CORBA_char        *str,
			 CORBA_Environment       *ev)
{
	HelpBrowser *browser;

	browser = HELP_BROWSER (bonobo_x_object (servant));
	gtk_signal_emit (GTK_OBJECT (browser),
			 signals[SEARCH],
			 str ? str : "");
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
	gtk_signal_emit (GTK_OBJECT (browser),
			 signals[SEARCH_IN_NEW_WINDOW],
			 str ? str : "");
}

static void
help_browser_class_init (HelpBrowserClass *klass)
{
	POA_GNOME_DevHelp_HelpBrowser__epv *epv = &klass->epv;
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (PARENT_TYPE);

	epv->search = impl_HelpBrowser_search;
	epv->searchInNewWindow = impl_HelpBrowser_searchInNewWindow;

	signals[SEARCH] =
		gtk_signal_new ("search",
				GTK_RUN_LAST,
				object_class->type,
				0,
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE,
				1, GTK_TYPE_POINTER);

	signals[SEARCH_IN_NEW_WINDOW] =
		gtk_signal_new ("search_in_new_window",
				GTK_RUN_LAST,
				object_class->type,
				0,
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE,
				1, GTK_TYPE_POINTER);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
}

static void
help_browser_init (HelpBrowser *browser)
{
}

HelpBrowser*
help_browser_new (void)
{
	return gtk_type_new (HELP_BROWSER_TYPE);
}
                                     
BONOBO_X_TYPE_FUNC_FULL (HelpBrowser,
                         GNOME_DevHelp_HelpBrowser,
                         PARENT_TYPE,
                         help_browser);

