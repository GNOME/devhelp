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

static gchar    *option_search = NULL;
static gchar    *option_search_assistant = NULL;
static gboolean  option_quit = FALSE;
static gboolean  option_focus_search = FALSE;
static gboolean  option_version = FALSE;

static GOptionEntry options[] = {
        { "search", 's',
          0, G_OPTION_ARG_STRING, &option_search,
          N_("Search for a keyword"),
          NULL
        },
        { "quit", 'q',
          0, G_OPTION_ARG_NONE, &option_quit,
          N_("Quit any running Devhelp"),
          NULL
        },
        { "version", 'v',
          0, G_OPTION_ARG_NONE, &option_version,
          N_("Display the version and exit"),
          NULL
        },
        { "focus-search",       'f',
          0, G_OPTION_ARG_NONE, &option_focus_search,
          N_("Focus the Devhelp window with the search field active"),
          NULL
        },
        { "search-assistant", 'a',
          0, G_OPTION_ARG_STRING, &option_search_assistant,
          N_("Search and display any hit in the assistant window"),
          NULL
        },
        { NULL }
};

static void
search_normal (DhWindow    *window,
               const gchar *str)
{
        if (str[0] == '\0') {
                return;
        }

        dh_window_search (window, str);
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
dh_quit (GAction  *action,
         GVariant *parameter,
         gpointer  data)
{
	gtk_main_quit ();
}

static void
dh_search (GAction  *action,
           GVariant *parameter,
           gpointer  data)
{
        DhBase *base = data;
	GtkWidget *window;

	window = dh_base_get_window (base);
	search_normal (DH_WINDOW (window),
                       g_variant_get_string (parameter, NULL));
	gtk_window_present (GTK_WINDOW (window));
}

static void
dh_search_assistant (GAction  *action,
                     GVariant *parameter,
                     gpointer  data)
{
        DhBase *base = data;

	search_assistant (base,
                          g_variant_get_string (parameter, NULL));
}

static void
dh_focus_search (GAction  *action,
                 GVariant *parameter,
                 gpointer  data)
{
        DhBase *base = data;
	GtkWidget *window;

	window = dh_base_get_window (base);
	dh_window_focus_search (DH_WINDOW (window));
	gtk_window_present (GTK_WINDOW (window));
}

static void
dh_raise (GAction  *action,
          GVariant *parameter,
          gpointer  data)
{
        DhBase *base = data;
	GtkWidget *window;

	window = dh_base_get_window (base);
	gtk_window_present (GTK_WINDOW (window));
}

enum
{
	ACTION_QUIT,
	ACTION_SEARCH,
	ACTION_SEARCH_ASSISTANT,
	ACTION_FOCUS_SEARCH,
	ACTION_RAISE,
	N_ACTIONS,
};

static const struct dh_action {
	const gchar * const name;
        const GVariantType *expected_type;
	void (* handler) (GAction *action, GVariant *parameter, gpointer data);
} actions[N_ACTIONS] = {
	[ACTION_QUIT]                  = { "quit",             NULL,                  dh_quit },
	[ACTION_SEARCH]                = { "search",           G_VARIANT_TYPE_STRING, dh_search },
	[ACTION_SEARCH_ASSISTANT]      = { "search-assistant", G_VARIANT_TYPE_STRING, dh_search_assistant },
	[ACTION_FOCUS_SEARCH]          = { "focus-search",     NULL,                  dh_focus_search },
	[ACTION_RAISE]                 = { "raise",            NULL,                  dh_raise },
};

static void
dh_register_actions (GApplication *application,
                     DhBase       *base)
{
	guint i;

        GSimpleActionGroup *action_group;
        GSimpleAction *action;

        action_group = g_simple_action_group_new ();

	for (i = 0; i < G_N_ELEMENTS (actions); i++) {
                action = g_simple_action_new (actions[i].name, actions[i].expected_type);
                g_signal_connect (action, "activate",  G_CALLBACK (actions[i].handler), base);
                g_simple_action_group_insert (action_group, G_ACTION (action));
                g_object_unref (action);
	}

        g_application_set_action_group (application, G_ACTION_GROUP (action_group));
        g_object_unref (action_group);
}

static int
activate (GApplication *application,
          gpointer      data)
{
        DhBase *base = data;

	if (option_quit) {
		/* No running Devhelps so just quit */
		return EXIT_SUCCESS;
	}

	if (!option_search_assistant) {
                GtkWidget *window;

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

        return EXIT_SUCCESS;
}

int
main (int argc, char **argv)
{
	DhBase                 *base;
	GError                 *error = NULL;
        int                     status;

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
		return EXIT_FAILURE;
	}

	if (option_version) {
		g_print ("%s\n", PACKAGE_STRING);
		return EXIT_SUCCESS;
	}

	/* i18n: Please don't translate "Devhelp" (it's marked as translatable
	 * for transliteration only) */
	g_set_application_name (_("Devhelp"));
	gtk_window_set_default_icon_name ("devhelp");

        /* Create our base application. Needs to be created before the GApplication,
         * as we will pass it as data to the 'activate' callback */
	base = dh_base_new ();

        /* Create new GApplication */
        application = g_application_new ("org.gnome.Devhelp", 0);

        /* Register all known actions */
        g_signal_connect (application, "activate", G_CALLBACK (activate), base);
        dh_register_actions (application, base);

        /* Try to register the application... */
        if (!g_application_register (application, NULL, &error)) {
                g_printerr ("Couldn't register Devhelp instance: '%s'\n",
                            error ? error->message : "");
		g_object_unref (application);
                g_object_unref (base);
                return EXIT_FAILURE;
        }

        /* Actions on a remote Devhelp already running? */
	if (g_application_get_is_remote (G_APPLICATION (application))) {
		if (option_quit) {
                        g_action_group_activate_action (G_ACTION_GROUP (application),
                                                        actions[ACTION_QUIT].name,
                                                        NULL);
		} else if (option_search) {
                        g_debug ("Searching in remote instance... '%s'", option_search);
                        g_action_group_activate_action (G_ACTION_GROUP (application),
                                                        actions[ACTION_SEARCH].name,
                                                        g_variant_new_string (option_search));
		} else if (option_search_assistant) {
                        g_action_group_activate_action (G_ACTION_GROUP (application),
                                                        actions[ACTION_SEARCH_ASSISTANT].name,
                                                        g_variant_new_string (option_search_assistant));
		} else if (option_focus_search) {
                        g_action_group_activate_action (G_ACTION_GROUP (application),
                                                        actions[ACTION_FOCUS_SEARCH].name,
                                                        NULL);
		} else {
                        g_action_group_activate_action (G_ACTION_GROUP (application),
                                                        actions[ACTION_RAISE].name,
                                                        NULL);
		}

		gdk_notify_startup_complete ();
		g_object_unref (application);
                g_object_unref (base);
		return EXIT_SUCCESS;
	}

        /* And run the GApplication */
        status = g_application_run (application, argc, argv);

	g_object_unref (base);
	g_object_unref (application);

	return status;
}

