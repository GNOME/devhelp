/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@codefactory.se>
 * Copyright (C) 2001 Richard Hult <rhult@codefactory.se>
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
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
#include <gconf/gconf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-program.h>
#include <libgnomeui/gnome-ui-init.h>
#include <libgnomevfs/gnome-vfs-init.h>

#include "dh-base.h"
#include "dh-window.h"

int 
main (int argc, char **argv)
{
	DhBase            *base;
	GtkWidget         *window;
	gchar             *option_search = NULL;
	GnomeProgram      *program;
	struct poptOption  options[] = {
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
	
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (PACKAGE);

	g_thread_init (NULL);
	
	program = gnome_program_init (PACKAGE, VERSION,
				      LIBGNOMEUI_MODULE,
                                      argc, argv,
                                      GNOME_PROGRAM_STANDARD_PROPERTIES,
				      GNOME_PARAM_POPT_TABLE, options,
                                      NULL);
	LIBXML_TEST_VERSION;

 	base = dh_base_new ();
 	window = dh_base_new_window (base, NULL);

	if (option_search) {
		dh_window_search (DH_WINDOW (window), option_search);
	}

	gtk_widget_show (window);

	gtk_main ();

	return 0;
}
