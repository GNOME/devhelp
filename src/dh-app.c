/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "dh-app.h"
#include <stdlib.h>
#include <glib/gi18n-lib.h>
#include "dh-assistant.h"
#include "dh-preferences.h"
#include "dh-window.h"

G_DEFINE_TYPE (DhApp, dh_app, GTK_TYPE_APPLICATION);

static void
search (DhApp       *app,
        const gchar *keyword)
{
        g_action_group_activate_action (G_ACTION_GROUP (app),
                                        "search",
                                        g_variant_new_string (keyword));
}

static void
search_assistant (DhApp       *app,
                  const gchar *keyword)
{
        g_action_group_activate_action (G_ACTION_GROUP (app),
                                        "search-assistant",
                                        g_variant_new_string (keyword));
}

static void
new_window_cb (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
        DhApp *app = DH_APP (user_data);
        GtkWidget *window;

        window = dh_window_new (app);
        gtk_widget_show_all (window);
}

static void
preferences_cb (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
        DhApp *app = DH_APP (user_data);
        GtkWindow *window;

        window = dh_app_peek_first_window (app);

        dh_preferences_show_dialog (window);
}

static void
about_cb (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
        DhApp *app = DH_APP (user_data);
        GtkWindow *parent;

        const gchar *authors[] = {
                "Mikael Hallendal <micke@imendio.com>",
                "Richard Hult <richard@imendio.com>",
                "Johan Dahlin <johan@gnome.org>",
                "Ross Burton <ross@burtonini.com>",
                "Aleksander Morgado <aleksander@lanedo.com>",
                "Thomas Bechtold <toabctl@gnome.org>",
                "Frédéric Péters <fpeters@0d.be>",
                "Sébastien Wilmet <swilmet@gnome.org>",
                NULL
        };

        parent = dh_app_peek_first_window (app);

        gtk_show_about_dialog (parent,
                               /* i18n: Please don't translate "Devhelp" (it's marked as translatable
                                * for transliteration only) */
                               "name", _("Devhelp"),
                               "version", PACKAGE_VERSION,
                               "comments", _("A developers’ help browser for GNOME"),
                               "authors", authors,
                               "translator-credits", _("translator-credits"),
                               "website", PACKAGE_URL,
                               "website-label", _("Devhelp Website"),
                               "logo-icon-name", PACKAGE_TARNAME,
                               "license-type", GTK_LICENSE_GPL_2_0,
                               "copyright", "Copyright 2001-2017 – the Devhelp team",
                               NULL);
}

static void
quit_cb (GSimpleAction *action,
         GVariant      *parameter,
         gpointer       user_data)
{
        DhApp *app = DH_APP (user_data);
        GList *l;

        /* Remove all windows registered in the application */
        while ((l = gtk_application_get_windows (GTK_APPLICATION (app)))) {
                gtk_application_remove_window (GTK_APPLICATION (app),
                                               GTK_WINDOW (l->data));
        }
}

static void
search_cb (GSimpleAction *action,
           GVariant      *parameter,
           gpointer       user_data)
{
        DhApp *app = DH_APP (user_data);
        GtkWindow *window;
        const gchar *str;

        window = dh_app_peek_first_window (app);
        str = g_variant_get_string (parameter, NULL);
        if (str[0] == '\0') {
                g_warning ("Cannot search in application window: "
                           "No keyword given");
                return;
        }

        dh_window_search (DH_WINDOW (window), str);
        gtk_window_present (window);
}

static DhAssistant *
get_active_assistant_window (DhApp *app)
{
        GList *windows;
        GList *l;

        windows = gtk_application_get_windows (GTK_APPLICATION (app));

        for (l = windows; l != NULL; l = l->next) {
                GtkWindow *cur_window = GTK_WINDOW (l->data);

                if (DH_IS_ASSISTANT (cur_window))
                        return DH_ASSISTANT (cur_window);
        }

        return NULL;
}

static void
search_assistant_cb (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
        DhApp *app = DH_APP (user_data);
        DhAssistant *assistant;
        const gchar *keyword;

        keyword = g_variant_get_string (parameter, NULL);
        if (keyword == NULL || keyword[0] == '\0') {
                g_warning ("Cannot look for keyword in Search Assistant: no keyword given.");
                return;
        }

        assistant = get_active_assistant_window (app);
        if (assistant == NULL)
                assistant = dh_assistant_new (app);

        dh_assistant_search (assistant, keyword);
        gtk_window_present (GTK_WINDOW (assistant));
}

static void
raise_cb (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
        DhApp *app = DH_APP (user_data);
        GtkWindow *window;

        /* Look for the first application window available and show it */
        window = dh_app_peek_first_window (app);
        gtk_window_present (window);
}

static void
add_action_entries (DhApp *app)
{
        const GActionEntry app_entries[] = {
                /* General actions */
                { "new-window", new_window_cb },
                { "preferences", preferences_cb },
                { "about", about_cb },
                { "quit", quit_cb },

                /* Additional commandline-specific actions */
                { "search", search_cb, "s" },
                { "search-assistant", search_assistant_cb, "s" },
                { "raise", raise_cb },
        };

        g_action_map_add_action_entries (G_ACTION_MAP (app),
                                         app_entries,
                                         G_N_ELEMENTS (app_entries),
                                         app);
}

static void
setup_accelerators (GtkApplication *app)
{
        const gchar *accels[] = {NULL, NULL, NULL, NULL};

        accels[0] = "<Primary>0";
        gtk_application_set_accels_for_action (app, "win.zoom-default", accels);

        accels[0] = "<Primary>minus";
        gtk_application_set_accels_for_action (app, "win.zoom-out", accels);

        accels[0] = "<Primary>plus";
        accels[1] = "<Primary>equal";
        gtk_application_set_accels_for_action (app, "win.zoom-in", accels);
        accels[1] = NULL;

        accels[0] = "<Primary>f";
        gtk_application_set_accels_for_action (app, "win.find", accels);

        accels[0] = "<Primary>c";
        gtk_application_set_accels_for_action (app, "win.copy", accels);

        accels[0] = "<Primary>p";
        gtk_application_set_accels_for_action (app, "win.print", accels);

        accels[0] = "<Primary>t";
        gtk_application_set_accels_for_action (app, "win.new-tab", accels);

        accels[0] = "<Primary>Page_Down";
        gtk_application_set_accels_for_action (app, "win.next-tab", accels);

        accels[0] = "<Primary>Page_Up";
        gtk_application_set_accels_for_action (app, "win.prev-tab", accels);

        accels[0] = "F9";
        gtk_application_set_accels_for_action (app, "win.show-sidebar", accels);

        accels[0] = "<Primary>w";
        gtk_application_set_accels_for_action (app, "win.close", accels);

        accels[0] = "F10";
        gtk_application_set_accels_for_action (app, "win.gear-menu", accels);

        accels[0] = "<Alt>Right";
        accels[1] = "Forward";
        gtk_application_set_accels_for_action (app, "win.go-forward", accels);

        accels[0] = "<Alt>Left";
        accels[1] = "Back";
        gtk_application_set_accels_for_action (app, "win.go-back", accels);

        accels[0] = "<Primary>k";
        accels[1] = "<Primary>s";
        accels[2] = "<Primary>l";
        gtk_application_set_accels_for_action (app, "win.focus-search", accels);
}

static void
set_app_menu_if_needed (GtkApplication *app)
{
	GMenu *manual_app_menu;

	manual_app_menu = gtk_application_get_menu_by_id (app, "manual-app-menu");

        /* Have the g_return in all cases, to catch problems in all cases. */
	g_return_if_fail (manual_app_menu != NULL);

	if (gtk_application_prefers_app_menu (app))
		gtk_application_set_app_menu (app, G_MENU_MODEL (manual_app_menu));
}

static void
dh_app_startup (GApplication *application)
{
        DhApp *app = DH_APP (application);

        g_application_set_resource_base_path (application, "/org/gnome/devhelp");

        if (G_APPLICATION_CLASS (dh_app_parent_class)->startup != NULL)
                G_APPLICATION_CLASS (dh_app_parent_class)->startup (application);

        add_action_entries (app);
        setup_accelerators (GTK_APPLICATION (app));
        set_app_menu_if_needed (GTK_APPLICATION (app));
}

static void
dh_app_activate (GApplication *application)
{
        dh_app_new_window (DH_APP (application));
}

static gboolean  option_version;

static GOptionEntry options[] = {
        { "new-window", 'n',
          0, G_OPTION_ARG_NONE, NULL,
          N_("Opens a new Devhelp window"),
          NULL
        },
        { "search", 's',
          0, G_OPTION_ARG_STRING, NULL,
          N_("Search for a keyword"),
          N_("KEYWORD")
        },
        { "search-assistant", 'a',
          0, G_OPTION_ARG_STRING, NULL,
          N_("Search and display any hit in the assistant window"),
          N_("KEYWORD")
        },
        { "version", 'v',
          0, G_OPTION_ARG_NONE, &option_version,
          N_("Display the version and exit"),
          NULL
        },
        { "quit", 'q',
          0, G_OPTION_ARG_NONE, NULL,
          N_("Quit any running Devhelp"),
          NULL
        },
        { NULL }
};

static gint
dh_app_handle_local_options (GApplication *app,
                             GVariantDict *local_options)
{
  if (option_version)
    {
      g_print ("%s %s\n", g_get_application_name (), PACKAGE_VERSION);
      exit (0);
    }

  return -1;
}

static gint
dh_app_command_line (GApplication            *app,
                     GApplicationCommandLine *command_line)
{
        gboolean option_new_window = FALSE;
        const gchar *option_search = NULL;
        const gchar *option_search_assistant = NULL;
        gboolean option_quit = FALSE;
        GVariantDict *options_dict;

        options_dict = g_application_command_line_get_options_dict (command_line);
        g_variant_dict_lookup (options_dict, "new-window", "b", &option_new_window);
        g_variant_dict_lookup (options_dict, "search", "&s", &option_search);
        g_variant_dict_lookup (options_dict, "search-assistant", "&s", &option_search_assistant);
        g_variant_dict_lookup (options_dict, "quit", "b", &option_quit);

        if (option_new_window) {
                dh_app_new_window (DH_APP (app));
        } else if (option_quit) {
                g_action_group_activate_action (G_ACTION_GROUP (app), "quit", NULL);
        } else if (option_search) {
                search (DH_APP (app), option_search);
        } else if (option_search_assistant) {
                search_assistant (DH_APP (app), option_search_assistant);
        } else {
                g_action_group_activate_action (G_ACTION_GROUP (app), "raise", NULL);
        }

        return 0;
}

static void
dh_app_class_init (DhAppClass *klass)
{
        GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

        application_class->startup = dh_app_startup;
        application_class->activate = dh_app_activate;
        application_class->handle_local_options = dh_app_handle_local_options;
        application_class->command_line = dh_app_command_line;
}

static void
dh_app_init (DhApp *app)
{
        /* i18n: Please don't translate "Devhelp" (it's marked as translatable
         * for transliteration only) */
        g_set_application_name (_("Devhelp"));
        gtk_window_set_default_icon_name ("devhelp");

        g_application_add_main_option_entries (G_APPLICATION (app), options);
}

DhApp *
dh_app_new (void)
{
        return g_object_new (DH_TYPE_APP,
                             "application-id", "org.gnome.Devhelp",
                             "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
                             "register-session", TRUE,
                             NULL);
}

GtkWindow *
dh_app_peek_first_window (DhApp *app)
{
        GList *windows;
        GList *l;

        g_return_val_if_fail (DH_IS_APP (app), NULL);

        windows = gtk_application_get_windows (GTK_APPLICATION (app));

        for (l = windows; l != NULL; l = l->next) {
                GtkWindow *cur_window = GTK_WINDOW (l->data);

                if (DH_IS_WINDOW (cur_window))
                        return cur_window;
        }

        /* Create a new window */
        dh_app_new_window (app);

        /* And look for the newly created window again */
        return dh_app_peek_first_window (app);
}

void
dh_app_new_window (DhApp *app)
{
        g_return_if_fail (DH_IS_APP (app));

        g_action_group_activate_action (G_ACTION_GROUP (app), "new-window", NULL);
}
