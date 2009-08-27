/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2008 Imendio AB
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
#include <unique/unique.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "dh-base.h"
#include "dh-window.h"
#include "dh-assistant.h"

#define COMMAND_QUIT             1
#define COMMAND_SEARCH           2
#define COMMAND_SEARCH_ASSISTANT 3
#define COMMAND_FOCUS_SEARCH     4
#define COMMAND_RAISE            5

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
        static GtkWidget *assistant;

        if (str[0] == '\0') {
                return FALSE;
        }

        if (!assistant) {
                assistant = dh_base_new_assistant (base);
                g_signal_connect (assistant, "destroy",
                                  G_CALLBACK (gtk_widget_destroyed),
                                  &assistant);
        }

        return dh_assistant_search (DH_ASSISTANT (assistant), str);
}

static UniqueResponse
unique_app_message_cb (UniqueApp *unique_app,
                       gint command,
                       UniqueMessageData *data,
                       guint timestamp,
                       gpointer user_data)
{
	DhBase    *base = user_data;
	gchar     *search_string;
	GtkWidget *window;

	if (command == COMMAND_QUIT) {
		gtk_main_quit ();
		return UNIQUE_RESPONSE_OK;
	}

	if (command == COMMAND_SEARCH_ASSISTANT) {
		search_string = unique_message_data_get_text(data);
                search_assistant (base, search_string);
		g_free (search_string);
		return UNIQUE_RESPONSE_OK;
	}

	window = dh_base_get_window (base);
	if (command == COMMAND_SEARCH) {
		search_string = unique_message_data_get_text(data);
                search_normal (DH_WINDOW (window), search_string);
		g_free (search_string);
	}
	else if (command == COMMAND_FOCUS_SEARCH) {
		dh_window_focus_search (DH_WINDOW (window));
	}

#ifdef GDK_WINDOWING_X11
#if GTK_CHECK_VERSION (2,14,0)
	timestamp = gdk_x11_get_server_time (gtk_widget_get_window (window));
#else
	timestamp = gdk_x11_get_server_time (window->window);
#endif
#else
	timestamp = GDK_CURRENT_TIME;
#endif

	gtk_window_present_with_time (GTK_WINDOW (window), timestamp);
	return UNIQUE_RESPONSE_OK;
}

int
main (int argc, char **argv)
{
	gchar                  *option_search = NULL;
	gchar                  *option_search_assistant = NULL;
	gboolean                option_quit = FALSE;
	gboolean                option_focus_search = FALSE;
	gboolean                option_version = FALSE;
	UniqueApp              *unique_app;
	DhBase                 *base;
	GtkWidget              *window;
	GError                 *error = NULL;
        GOptionEntry            options[] = {
                { "search", 's',
                  0, G_OPTION_ARG_STRING, &option_search,
                  _("Search for a keyword"),
                  NULL
                },
                { "quit", 'q',
                  0, G_OPTION_ARG_NONE, &option_quit,
                  _("Quit any running Devhelp"),
                  NULL
                },
                { "version", 'v',
                  0, G_OPTION_ARG_NONE, &option_version,
                  _("Display the version and exit"),
                  NULL
                },
                { "focus-search",       'f',
                  0, G_OPTION_ARG_NONE, &option_focus_search,
                  _("Focus the Devhelp window with the search field active"),
                  NULL
                },
                { "search-assistant", 'a',
                  0, G_OPTION_ARG_STRING, &option_search_assistant,
                  _("Search and display any hit in the assistant window"),
                  NULL
                },
                { NULL }
        };

#ifdef GDK_WINDOWING_QUARTZ
        {
                gint i;

                for (i = 0; i < argc; i++) {
                        if (g_str_has_prefix (argv[i], "-psn_")) {
                                for (; i < argc-1; i++) {
                                        argv[i] = argv[i+1];
                                }
                                argc--;
                                break;
                        }
                }
        }
#endif

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

	/* i18n: Please don't translate "Devhelp" (it's marked as translatable
	 * for transliteration only) */
	g_set_application_name (_("Devhelp"));
	gtk_window_set_default_icon_name ("devhelp");

	unique_app = unique_app_new_with_commands ("org.gnome.Devhelp", NULL,
		"quit", COMMAND_QUIT,
		"search", COMMAND_SEARCH,
		"search_assistant", COMMAND_SEARCH_ASSISTANT,
		"focus_search", COMMAND_FOCUS_SEARCH,
		"raise", COMMAND_RAISE,
		NULL
		);

	if (unique_app_is_running (unique_app)) {
		UniqueMessageData *message_data = NULL;

		if (option_quit) {
			unique_app_send_message (unique_app, COMMAND_QUIT, NULL);
		}
		else if (option_search) {
			message_data = unique_message_data_new ();
			unique_message_data_set_text (message_data, option_search, -1);
			unique_app_send_message (unique_app, COMMAND_SEARCH, message_data);
			unique_message_data_free (message_data);
		}
		else if (option_search_assistant) {
			message_data = unique_message_data_new ();
			unique_message_data_set_text (message_data, option_search_assistant, -1);
			unique_app_send_message (unique_app, COMMAND_SEARCH_ASSISTANT, message_data);
			unique_message_data_free (message_data);
		}
		else if (option_focus_search) {
			unique_app_send_message (unique_app, COMMAND_FOCUS_SEARCH, NULL);
		} else {
			unique_app_send_message (unique_app, COMMAND_RAISE, NULL);
		}

		g_object_unref (unique_app);
		return 0;
	}

	if (option_quit) {
		/* No running Devhelps so just quit */
		return 0;
	}

	base = dh_base_new ();

	g_signal_connect (unique_app, "message-received",
	      G_CALLBACK (unique_app_message_cb), base);

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
	g_object_unref (unique_app);

	return 0;
}
