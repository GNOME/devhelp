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

#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "dh-base.h"
#include "dh-window.h"
#include "dh-assistant.h"


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


static GApplication *application = NULL;


static void
dh_quit (DhBase         *base,
	 const char     *data)
{
	gtk_main_quit ();
}

static void
dh_search (DhBase       *base,
	   const char   *data)
{
	GtkWidget *window;

	window = dh_base_get_window (base);
	search_normal (DH_WINDOW (window), data);
	gtk_window_present (GTK_WINDOW (window));
}

static void
dh_search_assistant (DhBase     *base,
		     const char *data)
{
	search_assistant (base, data);
}

static void
dh_focus_search (DhBase         *base,
		 const char     *data)
{
	GtkWidget *window;

	window = dh_base_get_window (base);
	dh_window_focus_search (DH_WINDOW (window));
	gtk_window_present (GTK_WINDOW (window));
}

static void
dh_raise (DhBase        *base,
	  const char    *data)
{
	GtkWidget *window;

	window = dh_base_get_window (base);
	gtk_window_present (GTK_WINDOW (window));
}


enum
{
	COMMAND_QUIT,
	COMMAND_SEARCH,
	COMMAND_SEARCH_ASSISTANT,
	COMMAND_FOCUS_SEARCH,
	COMMAND_RAISE,
	N_COMMANDS,
};

static const struct dh_command {
	const gchar * const name;
	const gchar * const description;
	void (*handler)(DhBase *, const char *);
} commands[N_COMMANDS] = {
	[COMMAND_QUIT]                  = { "quit",             "quit any running devhelp",                              dh_quit },
	[COMMAND_SEARCH]                = { "search",           "search for a keyword",                                  dh_search },
	[COMMAND_SEARCH_ASSISTANT]      = { "search-assistant", "search and display any hit in the assitant window",     dh_search_assistant },
	[COMMAND_FOCUS_SEARCH]          = { "focus-search",     "focus the devhelp window with the search field active", dh_focus_search },
	[COMMAND_RAISE]                 = { "raise",            "raise any running devhelp window",                      dh_raise },
};


static void
dh_action_handler (GApplication *app,
		   gchar        *name,
		   GVariant     *platform_data,
		   gpointer      user_data)
{
	char    *data = NULL;
	DhBase  *base;
	guint    i;

	g_return_if_fail (DH_IS_BASE (user_data));

	base = DH_BASE (user_data);

	if (platform_data) {
		GVariantIter iter;
		const char *key;
		GVariant *value;

		g_variant_iter_init (&iter, platform_data);
		while (g_variant_iter_next (&iter, "{&sv}", &key, &value)) {
			if (g_strcmp0 (key, "data") == 0) {
				data = g_variant_dup_string (value, NULL);
				g_variant_unref (value);
				break;
			}
			g_variant_unref (value);
		}
	}

	for (i = 0; i < N_COMMANDS; i++) {
		if (g_strcmp0 (name, commands[i].name) == 0) {
			commands[i].handler (base, data);
		}
	}

	g_free (data);
}

static void
dh_register_commands (GApplication *application)
{
	guint i;

	for (i = 0; i < G_N_ELEMENTS (commands); i++) {
		g_application_add_action (application, commands[i].name, commands[i].description);
	}
}

int
main (int argc, char **argv)
{
	gchar                  *option_search = NULL;
	gchar                  *option_search_assistant = NULL;
	gboolean                option_quit = FALSE;
	gboolean                option_focus_search = FALSE;
	gboolean                option_version = FALSE;
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

	application = g_initable_new (G_TYPE_APPLICATION,
				      NULL,
				      NULL,
				      "application-id", "org.gnome.Devhelp",
				      "argv", g_variant_new_bytestring_array ((const char * const *) argv, argc),
				      "default-quit", FALSE,
				      NULL);

	if (g_application_is_remote (G_APPLICATION (application))) {
		GVariant *data = NULL;
		GVariantBuilder builder;

		g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));

		if (option_quit) {
			g_application_invoke_action (G_APPLICATION (application), commands[COMMAND_QUIT].name, data);
		}
		else if (option_search) {
			g_variant_builder_add (&builder, "{sv}", "data", g_variant_new_string (option_search));
			data = g_variant_builder_end (&builder);

			g_application_invoke_action (G_APPLICATION (application), commands[COMMAND_SEARCH].name, data);

			g_variant_unref (data);
		}
		else if (option_search_assistant) {
			g_variant_builder_add (&builder, "{sv}", "data", g_variant_new_string (option_search_assistant));
			data = g_variant_builder_end (&builder);

			g_application_invoke_action (G_APPLICATION (application), commands[COMMAND_SEARCH_ASSISTANT].name, data);

			g_variant_unref (data);
		}
		else if (option_focus_search) {
			g_application_invoke_action (G_APPLICATION (application), commands[COMMAND_FOCUS_SEARCH].name, data);
		} else {
			g_application_invoke_action (G_APPLICATION (application), commands[COMMAND_RAISE].name, data);
		}

		gdk_notify_startup_complete ();
		g_object_unref (application);
		return EXIT_SUCCESS;
	} else {
		dh_register_commands (G_APPLICATION (application));
	}

	if (option_quit) {
		/* No running Devhelps so just quit */
		return EXIT_SUCCESS;
	}

	base = dh_base_new ();

	g_signal_connect (G_APPLICATION (application), "action-with-data",
			  G_CALLBACK (dh_action_handler), base);

	if (!option_search_assistant) {
		window = dh_base_new_window (base);

		if (option_search) {
			search_normal (DH_WINDOW (window), option_search);
		}

		gtk_widget_show (window);
	} else {
		if (!search_assistant (base, option_search_assistant)) {
			return EXIT_SUCCESS;
		}
	}

	gtk_main ();

	g_object_unref (base);
	g_object_unref (application);

	return EXIT_SUCCESS;
}

