/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2004 Imendio AB
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

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <libxml/parser.h>
#include <gconf/gconf.h>
#include <glib/gi18n.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkmain.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <gtkmozembed.h>

#include "bacon-message-connection.h"
#include "dh-base.h"
#include "dh-gecko-utils.h"
#include "dh-window.h"

#define COMMAND_QUIT   "quit"
#define COMMAND_SEARCH "search"
#define COMMAND_RAISE  "raise"

gchar *geometry = NULL;

static void
message_received_cb (const gchar *message, DhWindow *window)
{
	if (strcmp (message, COMMAND_QUIT) == 0) {
		gtk_main_quit ();
	}
	else if (strncmp (message, COMMAND_SEARCH, strlen (COMMAND_SEARCH)) == 0) {
		dh_window_search (window,
				  message + strlen (COMMAND_SEARCH) + 1);
	}
	else if (strcmp (message, COMMAND_RAISE) == 0) {
		gtk_window_present (GTK_WINDOW (window));
	}
}

static void
devhelp_init_i18n (void)
{
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (PACKAGE);
}

int 
main (int argc, char **argv)
{
	gchar                  *option_search = NULL;
	gboolean                option_quit = FALSE;
	/*GnomeProgram           *program;*/
	BaconMessageConnection *message_conn;
	DhBase                 *base;
	GtkWidget              *window;
	
	GOptionEntry  options[] = {
		{
			"geometry",
			'g',
			0,
			G_OPTION_ARG_STRING,
			&geometry,
			N_("Specify the size and location of the window"),
			N_("WIDTHxHEIGHT+XOFF+YOFF")
		},
		{ 
			"search",      
			's',
			0,
			G_OPTION_ARG_STRING, 
			&option_search,    
			_("Search for a function"),      
			NULL 
		},
		{ 
			"quit",      
			'q',  
			0,
			G_OPTION_ARG_NONE, 
			&option_quit,    
			_("Quit any running Devhelp"),      
			NULL 
		},
		{ NULL, '\0', 0, 0, NULL, NULL, NULL }
	};

	devhelp_init_i18n ();

	/* Initialize the widget set */
	gtk_init_with_args (&argc, &argv, NULL, options, NULL, NULL);

	g_thread_init (NULL);

	LIBXML_TEST_VERSION;

	gnome_vfs_init ();

	dh_gecko_utils_init_services ();
	//gtk_moz_embed_set_comp_path (MOZILLA_HOME);

	message_conn = bacon_message_connection_new ("Devhelp");
	if (!bacon_message_connection_get_is_server (message_conn)) {

		if (option_quit) {
			bacon_message_connection_send (message_conn, COMMAND_QUIT);
			return 0;
		}

		if (option_search) {
			gchar *command;

			command = g_strdup_printf ("%s %s", COMMAND_SEARCH,
						   option_search);
			bacon_message_connection_send (message_conn, command);
			g_free (command);
		}

		gdk_notify_startup_complete ();
		bacon_message_connection_send (message_conn, COMMAND_RAISE);
		return 0;
	}
	
	if (option_quit) {
		/* No running Devhelps so just quit */
		return 0;
	}
		
	base = dh_base_new ();
	window = dh_base_new_window (base);

	bacon_message_connection_set_callback (message_conn,
					       (BaconMessageReceivedFunc) message_received_cb,
					       window);

	if (option_search) {
		dh_window_search (DH_WINDOW (window), option_search);
	}
	
	
	gtk_widget_show (window);
		
	gtk_main ();

	return 0;
}
