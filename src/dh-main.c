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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/devhelp-%s-socket"

static gboolean
dh_client_data_cb (GIOChannel   *source,
		   GIOCondition  condition,
		   gpointer      data)
{
	GIOChannel *channel;

	if (condition & G_IO_ERR) {
		return FALSE;
	}
	else if (condition & G_IO_HUP) {
		return FALSE;
	}
	
	if (condition & G_IO_IN) {
		gint  fd;
		gchar buf[256];
		gint  bytes_read;
		
		fd = g_io_channel_unix_get_fd (source);
		
		bytes_read = read (fd, buf, 256);

		if (bytes_read < 0) {
			/* Error */
			return FALSE;
		}		
		else if (bytes_read == 0) {
			/* EOF */
			return FALSE;
		} else {
			buf[bytes_read] = 0;
          
			if (buf[0] == 'S' && bytes_read > 1) {
				dh_window_search (DH_WINDOW (data), buf + 1);
				gtk_window_present (GTK_WINDOW (data));
			}
			else if (buf[0] == 'Q') {
				gtk_main_quit ();
			}
		}
	}
	
	return TRUE;
}

static gboolean
dh_connection_cb (GIOChannel   *source,
		  GIOCondition  condition,
		  gpointer      data)
{
	GIOChannel *channel;

	if (condition & G_IO_IN) {
		gint listen_fd;
		gint fd;
		
		listen_fd = g_io_channel_unix_get_fd (source);
		fd = accept (listen_fd, NULL, NULL);
		
		if (fd < 0) {
			g_warning ("Some error while connecting...");
			return FALSE;
		} else {

			channel = g_io_channel_unix_new (fd);
			g_io_add_watch (channel,
					G_IO_IN | G_IO_ERR | G_IO_HUP,
					dh_client_data_cb,
					data);
			g_io_channel_unref (channel);
		}
	}
	
	if (condition & G_IO_ERR)
		g_warning ("Error on listen socket.");
	
	if (condition & G_IO_HUP)
		g_warning ("Hangup on listen socket.");
	
	return TRUE;
}

static void
dh_create_socket (DhWindow *window)
{
	gint                fd;
	struct sockaddr_un  addr;
	gchar              *path;
	GIOChannel         *channel;	

	fd = socket (AF_LOCAL, SOCK_STREAM, 0);
	
	if (fd < 0) {
		g_warning ("Couldn't create socket.");
		return;
	}

	path = g_strdup_printf (SOCKET_PATH, g_get_user_name ());
	
	memset (&addr, sizeof (addr), 0);
	addr.sun_family = AF_LOCAL;
	strcpy (addr.sun_path, path);

	g_free (path);
	
	if (bind (fd, (struct sockaddr*) &addr, SUN_LEN (&addr)) < 0) {
		g_warning ("Couldn't bind socket.");
		return;
	}
	
	if (listen (fd, 5) < 0) {
		g_warning ("Couldn't listen to socket.");
		return;
	}
	
	channel = g_io_channel_unix_new (fd);
	g_io_add_watch (channel,
			G_IO_IN | G_IO_ERR | G_IO_HUP,
			dh_connection_cb,
			window);
	g_io_channel_unref (channel);
}

static void
dh_send_search_msg (gint fd, const gchar *msg)
{
	gchar *buf;

	buf = g_strdup_printf ("S%s\n", msg);
	write (fd, buf, strlen (buf) -1);
	g_free (buf);
}

static void
dh_send_quit_msg (gint fd)
{
	write (fd, "Q", 1);
}

static gint
dh_try_to_connect (void)
{
	gint                fd;
	struct sockaddr_un  addr;
	gchar              *path;
	gchar              *buf;

	path = g_strdup_printf (SOCKET_PATH, g_get_user_name ());

	if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
		g_free (path);
		return -1;
	}
	
	fd = socket (AF_LOCAL, SOCK_STREAM, 0);
	if (fd < 0) {
		goto fail;
	}
	
	memset (&addr, sizeof (addr), 0);
	addr.sun_family = AF_LOCAL;
	strcpy (addr.sun_path, path);

	if (connect (fd, (struct sockaddr*) &addr, sizeof (addr)) < 0) {
		goto fail;
	}

	return fd;

 fail:
	if (!g_file_test (path, G_FILE_TEST_IS_SYMLINK | G_FILE_TEST_IS_DIR)) {
		unlink (path);
	}
	g_free (path);

	return -1;
}

int 
main (int argc, char **argv)
{
	DhBase            *base;
	GtkWidget         *window;
	gchar             *option_search = NULL;
	gboolean           option_quit = FALSE;
	GnomeProgram      *program;
	gint               fd;
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
		{ 
			"quit",      
			'q',  
			POPT_ARG_NONE, 
			&option_quit,    
			0, 
			_("Quit any running Devhelp"),      
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

	fd = dh_try_to_connect ();
	if (fd < 0) {
		dh_create_socket (DH_WINDOW (window));
	}

	if (option_quit) {
		if (fd < 0) {
			return 0;
		} else {
			dh_send_quit_msg (fd);
			return 0;
		}
	}	

	if (option_search) {
		if (fd < 0) {
			dh_window_search (DH_WINDOW (window), option_search);
		} else {
			dh_send_search_msg (fd, option_search);
		}
	}

	/* Exit if we're already running. */
	if (fd >= 0) {
		return 0;
	}
	
	gtk_widget_show (window);
		
	gtk_main ();

	return 0;
}
