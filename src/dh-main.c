/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2005 Imendio AB
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

#include "config.h"
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#ifdef HAVE_PLATFORM_X11
#include <gdk/gdkx.h>
#endif

#include "bacon-message-connection.h"
#include "dh-base.h"
#include "dh-window.h"

#define COMMAND_QUIT             "quit"
#define COMMAND_SEARCH           "search"
#define COMMAND_SEARCH_ASSISTANT "search-assistant"
#define COMMAND_FOCUS_SEARCH     "focus-search"
#define COMMAND_RAISE            "raise"

static void
extract_book_id (const gchar  *str,
                 gchar       **term,
                 gchar       **book_id)
{
        gchar   **strv;
        gint      i;
        GString  *term_string;
        
        *term = NULL;
        *book_id = NULL;

        term_string = g_string_new (NULL);

        strv = g_strsplit (str, " ", 0);

        i = 0;
        while (strv[i]) {
                if (!*book_id && g_str_has_prefix (strv[i], "book:")) {
                        *book_id = g_strdup (strv[i] + 5);
                } else {
                        if (i > 0 && term_string->len > 0) {
                                g_string_append_c (term_string, ' ');
                        }
                        g_string_append (term_string, strv[i]);
                }

                i++;
        }

        g_strfreev (strv);

        *term = g_string_free (term_string, FALSE);
}

static void
search_normal (DhWindow    *window,
               const gchar *str)
{
        gchar *term, *book_id;

        if (str[0] == '\0') {
                return;
        }

        extract_book_id (str, &term, &book_id);
        dh_window_search (window, term, book_id);
        g_free (term);
        g_free (book_id);
}

static gboolean
search_assistant (DhBase      *base,
                  const gchar *str)
{
        return FALSE;
}

static void
message_received_cb (const gchar *message,
                     DhBase      *base)
{
	GtkWidget *window;
	guint32    timestamp;

	if (strcmp (message, COMMAND_QUIT) == 0) {
		gtk_main_quit ();
		return;
	}

	if (g_str_has_prefix (message, COMMAND_SEARCH_ASSISTANT)) {
                search_assistant (base,
                                  message +
                                  strlen (COMMAND_SEARCH_ASSISTANT) + 1);
		return;
	}

	window = dh_base_get_window (base);
	if (g_str_has_prefix (message, COMMAND_SEARCH)) {
                search_normal (DH_WINDOW (window),
                               message + strlen (COMMAND_SEARCH) + 1);
	}
	else if (strcmp (message, COMMAND_FOCUS_SEARCH) == 0) {
		dh_window_focus_search (DH_WINDOW (window));
	}

#ifdef HAVE_PLATFORM_X11
	timestamp = gdk_x11_get_server_time (window->window);
#else
	timestamp = GDK_CURRENT_TIME;
#endif

	gtk_window_present_with_time (GTK_WINDOW (window), timestamp);
}

int
main (int argc, char **argv)
{
	gchar                  *option_search = NULL;
	gchar                  *option_search_assistant = NULL;
	gboolean                option_quit = FALSE;
	gboolean                option_focus_search = FALSE;
	gboolean                option_version = FALSE;
	gchar                  *display;
	gchar                  *connection_name;
	BaconMessageConnection *message_conn;
	DhBase                 *base;
	GtkWidget              *window;
	GError                 *error = NULL;
	GOptionEntry            options[] = {
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
       		{
			"version",
			'v',
			0,
			G_OPTION_ARG_NONE,
			&option_version,
			_("Display the version and exit"),
			NULL
		},
       		{
			"focus-search",
			'f',
			0,
			G_OPTION_ARG_NONE,
			&option_focus_search,
			_("Focus the devhelp window with the search field active"),
			NULL
		},
       		{
			"search-assistant",
			'a',
			0,
			G_OPTION_ARG_STRING,
			&option_search_assistant,
			_("Search and display any hit in the assistant window"),
			NULL
		},
		{
			NULL, '\0', 0, 0, NULL, NULL, NULL
		}
	};

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_thread_init (NULL);

	if (!gtk_init_with_args (&argc, &argv, NULL, options, GETTEXT_PACKAGE, &error)) {
		g_printerr ("%s\n", error->message);
		return 1;
	}

	if (option_version) {
		g_print ("%s\n", PACKAGE_STRING);
		return 0;
	}

	g_set_application_name (_("Devhelp"));
	gtk_window_set_default_icon_name ("devhelp");

	display = gdk_get_display ();
	connection_name = g_strdup_printf ("Devhelp-%s", display);
	message_conn = bacon_message_connection_new (connection_name);
	g_free (display);
	g_free (connection_name);

	if (!bacon_message_connection_get_is_server (message_conn)) {
		if (option_quit) {
			bacon_message_connection_send (message_conn, COMMAND_QUIT);
			return 0;
		}

		if (option_search) {
			gchar *command;

			command = g_strdup_printf ("%s %s",
						   COMMAND_SEARCH,
						   option_search);

			bacon_message_connection_send (message_conn, command);
			g_free (command);
		}
		else if (option_search_assistant) {
			gchar *command;

			command = g_strdup_printf ("%s %s",
						   COMMAND_SEARCH_ASSISTANT,
						   option_search_assistant);

			bacon_message_connection_send (message_conn, command);
			g_free (command);
		}
		else if (option_focus_search) {
			bacon_message_connection_send (message_conn, COMMAND_FOCUS_SEARCH);
		} else {
			bacon_message_connection_send (message_conn, COMMAND_RAISE);
		}

		gdk_notify_startup_complete ();
		return 0;
	}

	if (option_quit) {
		/* No running Devhelps so just quit */
		return 0;
	}

	base = dh_base_new ();

	bacon_message_connection_set_callback (
		message_conn,
		(BaconMessageReceivedFunc) message_received_cb,
		base);

	if (!option_search_assistant) {
		window = dh_base_new_window (base);

                if (option_search) {
                        search_normal (DH_WINDOW (window), option_search);
                }

		gtk_widget_show (window);
	} else {
		if (!search_assistant (base, option_search_assistant)) {
                        return 0;
                }
	}

	gtk_main ();

	g_object_unref (base);

	bacon_message_connection_free (message_conn);

	return 0;
}
