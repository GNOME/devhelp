/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include <gnome-xml/parser.h>
#include <gnome-xml/xmlmemory.h>
#include <gconf/gconf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-generic-factory.h>

#include "GNOME_DevHelp.h"
#include "install.h"
#include "help-browser.h"
#include "devhelp-window.h"

#define DEVHELP_FACTORY_OAFIID "OAFIID:GNOME_DevHelp_Factory"

static BonoboObject *   devhelp_factory   (BonoboGenericFactory   *this, 
					   void                   *data);

static BonoboObject *
devhelp_factory (BonoboGenericFactory *this, void *data)
{
	static HelpBrowser   *help_browser = NULL;
	
	if (help_browser == NULL) {
		help_browser = help_browser_new ();
	} else {
		bonobo_object_ref (BONOBO_OBJECT (help_browser));
	}

	return BONOBO_OBJECT (help_browser);
}

static void
activate_and_search (gpointer data, gboolean new_window)
{
	CORBA_Environment ev;
	CORBA_Object      devhelp;

	CORBA_exception_init (&ev);

	devhelp = oaf_activate_from_id ("OAFIID:GNOME_DevHelp", 0, NULL, &ev);

	if (BONOBO_EX (&ev) || devhelp == CORBA_OBJECT_NIL) {
		g_warning (_("Could not activate devhelp: '%s'"),
			   bonobo_exception_get_text (&ev));
		exit (1);
	}

	if (data) {
		GNOME_DevHelp_HelpBrowser_search (devhelp, data, &ev);

		if (BONOBO_EX (&ev)) {
			g_warning (_("Could not search for string."));
			exit (1);
		}
	}
}

static gboolean
idle_activate_and_search_quit (gpointer data)
{
	activate_and_search (data, FALSE);
	
	gtk_main_quit ();
	
	return FALSE;
}

static gboolean
idle_activate_and_search (gpointer data)
{
	activate_and_search (data, TRUE);
	
	return FALSE;
}

int 
main (int argc, char **argv)
{
	CORBA_ORB            orb;
	CORBA_Environment    ev;
	gchar               *option_search = NULL;
	CORBA_Object         factory;
	struct poptOption    options[] = {
		{ 
			"search",      
			's',  
			POPT_ARG_STRING, 
			&option_search,    
			0, 
			_("Search for a function"),      
			NULL 
		},
		{ NULL, '\0', 0, NULL, 0, NULL, NULL }
	};

	CORBA_exception_init (&ev);

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);
#endif

	gnomelib_register_popt_table (oaf_popt_options,
				      oaf_get_popt_table_name ());

	orb = oaf_init (argc, argv);
	gnome_init_with_popt_table (PACKAGE, VERSION, argc, argv, 
				    options, 0, NULL);

	LIBXML_TEST_VERSION;
	g_atexit (xmlCleanupParser);
	
	gdk_rgb_init ();
	gconf_init (argc, argv, NULL);
	gnome_vfs_init ();
	glade_gnome_init ();
	
	if (!bonobo_init (orb, CORBA_OBJECT_NIL, CORBA_OBJECT_NIL)) {
		g_error ("Could not initialize Bonobo");
	}

	install_create_directories (NULL);
	
	factory = oaf_activate_from_id (DEVHELP_FACTORY_OAFIID,
					OAF_FLAG_EXISTING_ONLY, NULL, NULL);
	
	if (!factory) {
		BonoboGenericFactory   *factory;
		/* Not started, start now */

		factory = bonobo_generic_factory_new (DEVHELP_FACTORY_OAFIID,
						      devhelp_factory,
						      NULL);
		bonobo_running_context_auto_exit_unref (BONOBO_OBJECT (factory));
		g_idle_add (idle_activate_and_search, 
			    (gpointer) option_search);
	} else {
		g_idle_add (idle_activate_and_search_quit,
			    (gpointer) option_search);
	}

	bonobo_main ();

	return 0;
}
