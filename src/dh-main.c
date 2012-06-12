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
#include <locale.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "dh-app.h"
#include "dh-window.h"
#include "dh-assistant.h"

static gboolean  option_new_window;
static gchar    *option_search;
static gchar    *option_search_assistant;
static gboolean  option_quit;
static gboolean  option_focus_search;
static gboolean  option_version;

static GOptionEntry options[] = {
        { "new-window", 'n',
          0, G_OPTION_ARG_NONE, &option_new_window,
          N_("Opens a new Devhelp window"),
          NULL
        },
        { "search", 's',
          0, G_OPTION_ARG_STRING, &option_search,
          N_("Search for a keyword"),
          N_("KEYWORD")
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
          N_("KEYWORD")
        },
        { NULL }
};

static void
run_action (DhApp *application,
            gboolean is_remote)
{
        if (option_new_window) {
                if (is_remote)
                        dh_app_new_window (application);
        } else if (option_quit) {
                dh_app_quit (application);
        } else if (option_search) {
                dh_app_search (application, option_search);
        } else if (option_search_assistant) {
                dh_app_search_assistant (application, option_search_assistant);
        } else if (option_focus_search) {
                dh_app_focus_search (application);
        } else {
                if (is_remote)
                        dh_app_raise (application);
        }
}

static void
activate_cb (GtkApplication *application)
{
        /* This is the primary instance */
        dh_app_new_window (DH_APP (application));

        /* Run the requested action from the command line */
        run_action (DH_APP (application), FALSE);
}

int
main (int argc, char **argv)
{
        DhApp   *application;
        GError  *error = NULL;
        gint     status;

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

        setlocale (LC_ALL, "");
        bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        if (!gtk_init_with_args (&argc, &argv, NULL, options, GETTEXT_PACKAGE, &error)) {
                g_printerr ("%s\n", error->message);
                return EXIT_FAILURE;
        }

        if (option_version) {
                g_print ("%s\n", PACKAGE_STRING);
                return EXIT_SUCCESS;
        }

        /* Create new DhApp */
        application = dh_app_new ();
        g_signal_connect (application, "activate", G_CALLBACK (activate_cb), NULL);

        /* Set it as the default application */
        g_application_set_default (G_APPLICATION (application));

        /* Try to register the application... */
        if (!g_application_register (G_APPLICATION (application), NULL, &error)) {
                g_printerr ("Couldn't register Devhelp instance: '%s'\n",
                            error ? error->message : "");
                g_object_unref (application);
                return EXIT_FAILURE;
        }

        /* Actions on a remote Devhelp already running? */
        if (g_application_get_is_remote (G_APPLICATION (application))) {
                /* Run the requested action from the command line */
                run_action (application, TRUE);
                g_object_unref (application);
                return EXIT_SUCCESS;
        }

        /* And run the GtkApplication */
        status = g_application_run (G_APPLICATION (application), argc, argv);

        g_object_unref (application);

        return status;
}
