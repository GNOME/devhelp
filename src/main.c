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

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <gconf/gconf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-generic-factory.h>

#include "GNOME_Devhelp.h"
#include "main.h"
#include "ui.h"
#include "install.h"
#include "help-browser.h"

static void
corba_search_cb (HelpBrowser *help_browser,
		 const gchar *search,
		 DevHelp     *devhelp)
{
	g_return_if_fail (devhelp != NULL);
		
	gtk_entry_set_text (devhelp->entry, search);
	gtk_notebook_set_page (devhelp->notebook, 1);
	gdk_window_raise (devhelp->window->window);
}

static void
corba_search_in_new_window_cb (HelpBrowser *help_browser,
			       const gchar *search,
			       DevHelp     *devhelp)
{
	g_warning (_("searchInNewWindow not implemented."));
}

static BonoboObject *
factory_fn (BonoboGenericFactory *this,
	    void                 *data)
{
	static HelpBrowser *help_browser = NULL;
	static DevHelp *devhelp = NULL;
	
	if (help_browser == NULL) {
		devhelp = devhelp_create_ui ();
		help_browser = help_browser_new ();
		gtk_signal_connect (GTK_OBJECT (help_browser),
				    "search",
				    corba_search_cb,
				    devhelp);
		gtk_signal_connect (GTK_OBJECT (help_browser),
				    "search_in_new_window",
				    corba_search_in_new_window_cb,
				    devhelp);
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
	devhelp = oaf_activate_from_id ("OAFIID:GNOME_Devhelp",
					0,
					NULL,
					&ev);
	if (BONOBO_EX (&ev) || devhelp == CORBA_OBJECT_NIL) {
		g_warning (_("Could not activate devhelp: '%s'"),
			   bonobo_exception_get_text (&ev));
		exit (1);
	}

	if (data) {
		GNOME_Devhelp_HelpBrowser_search (devhelp, data, &ev);
		if (BONOBO_EX (&ev)) {
			g_warning (_("Could not search for string."));
			exit (1);
		}
	}
}

static gboolean
idle_activate_and_search_quit (gpointer data)
{
	g_return_if_fail (data != NULL);
	
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

static void
devhelp_factory_main (const gchar *search_string)
{
	CORBA_Object factory;
	
	factory = oaf_activate_from_id ("OAFIID:GNOME_Devhelp_Factory",
					OAF_FLAG_EXISTING_ONLY,
					NULL, NULL);
	if (factory == NULL) {
		BonoboGenericFactory *factory;
			
		factory = bonobo_generic_factory_new ("OAFIID:GNOME_Devhelp_Factory", factory_fn, NULL);
		bonobo_running_context_auto_exit_unref (BONOBO_OBJECT (factory));
		g_idle_add (idle_activate_and_search, (gpointer)search_string);
	} else {
		g_idle_add (idle_activate_and_search_quit, (gpointer)search_string);
	}
		
	bonobo_main ();
}

/* No factory, be a normal app. */
static void
devhelp_normal_main (const gchar *search_string)
{
	DevHelp *devhelp;
		
	devhelp = devhelp_create_ui ();

	if (search_string != NULL) {
		gtk_entry_set_text (devhelp->entry, search_string);
		gtk_notebook_set_page (devhelp->notebook, 1);
	}

	gtk_main ();
}

int
main (int argc, char *argv[])
{
	CORBA_ORB          orb;
	poptContext        popt_context;
	gchar             *option_search;
	gboolean           option_factory;
	struct poptOption  options[] = {
		{ "search",      's',  POPT_ARG_STRING, &option_search,    0, _("Search for a function"),      NULL },
		{ "use-factory", 'f',  POPT_ARG_NONE,   &option_factory,   0, _("Use devhelp factory server"), NULL },
		{ NULL,         '\0',  0,               NULL,              0, NULL,                            NULL }
	};

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);
#endif

	option_search = NULL;
	option_factory = FALSE;

	g_thread_init (NULL);

	gnomelib_register_popt_table (oaf_popt_options, oaf_get_popt_table_name ());
	orb = oaf_init (argc, argv);	
	gnome_init_with_popt_table (PACKAGE, VERSION,
				    argc, argv, options,
				    0, &popt_context);

	LIBXML_TEST_VERSION;
	g_atexit (xmlCleanupParser);

	gdk_rgb_init ();
	gconf_init (argc, argv, NULL);
	gnome_vfs_init ();
	bonobo_init (orb, CORBA_OBJECT_NIL, CORBA_OBJECT_NIL);

	install_create_directories (NULL);

	if (option_factory) {
		devhelp_factory_main (option_search);
	} else {
		devhelp_normal_main (option_search);
	}		

	/*g_print ("Trying to shut down GnomeVFS, jobs: %d\n",
		 gnome_vfs_backend_get_job_count ());
		 gnome_vfs_shutdown ();
		 g_print ("Done.\n");*/
	return 0;
}


