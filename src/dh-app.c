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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "dh-app.h"

#include <stdlib.h>
#include <glib/gi18n-lib.h>

#include "devhelp.h"
#include "dh-preferences.h"

typedef struct {
        DhBookManager *book_manager;
} DhAppPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhApp, dh_app, GTK_TYPE_APPLICATION);

/**
 * dh_app_peek_book_manager:
 * @app: a #DhApp object
 *
 * Get the associated #DhBookManager.
 *
 * Returns: (transfer none): the book manager associated with this
 */
DhBookManager *
dh_app_peek_book_manager (DhApp *app)
{
        DhAppPrivate *priv;

        g_return_val_if_fail (DH_IS_APP (app), NULL);

        priv = dh_app_get_instance_private (app);

        return priv->book_manager;
}

/**
 * dh_app_peek_first_window:
 * @app: a #DhApp object
 *
 * Get the first #DhWindow.
 *
 * Returns: (transfer none): the first window
 */
GtkWindow *
dh_app_peek_first_window (DhApp *app)
{
        GList *l;

        g_return_val_if_fail (DH_IS_APP (app), NULL);

        for (l = gtk_application_get_windows (GTK_APPLICATION (app));
             l;
             l = g_list_next (l)) {
                if (DH_IS_WINDOW (l->data)) {
                        return GTK_WINDOW (l->data);
                }
        }

        /* Create a new window */
        dh_app_new_window (app);

        /* And look for the newly created window again */
        return dh_app_peek_first_window (app);
}

/**
 * dh_app_peek_assistant:
 * @app: a #DhApp object
 *
 * Get the associated #DhAssistant.
 *
 * Returns: (transfer none): the assistant
 */
GtkWindow *
dh_app_peek_assistant (DhApp *app)
{
        GList *l;

        g_return_val_if_fail (DH_IS_APP (app), NULL);

        for (l = gtk_application_get_windows (GTK_APPLICATION (app));
             l;
             l = g_list_next (l)) {
                if (DH_IS_ASSISTANT (l->data)) {
                        return GTK_WINDOW (l->data);
                }
        }

        return NULL;
}

gboolean
_dh_app_has_app_menu (DhApp *app)
{
        GtkSettings *gtk_settings;
        gboolean show_app_menu;
        gboolean show_menubar;

        g_return_val_if_fail (DH_IS_APP (app), FALSE);

        /* We have three cases:
         * - GNOME 3: show-app-menu true, show-menubar false -> use the app menu
         * - Unity, OSX: show-app-menu and show-menubar true -> use the normal menu
         * - Other WM, Windows: show-app-menu and show-menubar false -> use the normal menu
         */
        gtk_settings = gtk_settings_get_default ();
        g_object_get (G_OBJECT (gtk_settings),
                      "gtk-shell-shows-app-menu", &show_app_menu,
                      "gtk-shell-shows-menubar", &show_menubar,
                      NULL);

        return show_app_menu && !show_menubar;
}

/******************************************************************************/
/* Application action activators */

/**
 * dh_app_new_window:
 * @app: a #DhApp object
 *
 * Create a new #DhWindow.
 */
void
dh_app_new_window (DhApp *app)
{
        g_return_if_fail (DH_IS_APP (app));

        g_action_group_activate_action (G_ACTION_GROUP (app), "new-window", NULL);
}

/**
 * dh_app_quit:
 * @app: a #DhApp object
 *
 * Quit the application.
 */
void
dh_app_quit (DhApp *app)
{
        g_return_if_fail (DH_IS_APP (app));

        g_action_group_activate_action (G_ACTION_GROUP (app), "quit", NULL);
}

/**
 * dh_app_search:
 * @app: a #DhApp object
 * @keyword: the search request
 *
 * Search for @keyword in the entire application.
 */
void
dh_app_search (DhApp *app,
               const gchar *keyword)
{
        g_return_if_fail (DH_IS_APP (app));

        g_action_group_activate_action (G_ACTION_GROUP (app), "search", g_variant_new_string (keyword));
}

/**
 * dh_app_search_assistant:
 * @app: a #DhApp object
 * @keyword: the search request
 *
 * Search for @keyword in the entire application with a #DhAssistant.
 */
void
dh_app_search_assistant (DhApp *app,
                         const gchar *keyword)
{
        g_return_if_fail (DH_IS_APP (app));

        g_action_group_activate_action (G_ACTION_GROUP (app), "search-assistant", g_variant_new_string (keyword));
}

/**
 * dh_app_raise:
 * @app: a #DhApp object
 *
 * Present the main window of the application.
 */
void
dh_app_raise (DhApp *app)
{
        g_return_if_fail (DH_IS_APP (app));

        g_action_group_activate_action (G_ACTION_GROUP (app), "raise", NULL);
}

/******************************************************************************/
/* Application actions setup */

static void
new_window_cb (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
        DhApp *app = DH_APP (user_data);
        GtkWidget *window;

        window = dh_window_new (app);
        gtk_application_add_window (GTK_APPLICATION (app), GTK_WINDOW (window));
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
shortcuts_cb (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
        DhApp *app = DH_APP (user_data);
        static GtkWidget *shortcuts_window;
        GtkWindow *window;

        window = dh_app_peek_first_window (app);

        if (shortcuts_window == NULL)
        {
                GtkBuilder *builder;

                builder = gtk_builder_new_from_resource ("/org/gnome/devhelp/help-overlay.ui");
                shortcuts_window = GTK_WIDGET (gtk_builder_get_object (builder, "help_overlay"));

                g_signal_connect (shortcuts_window,
                                  "destroy",
                                  G_CALLBACK (gtk_widget_destroyed),
                                  &shortcuts_window);

                g_object_unref (builder);
        }

        if (GTK_WINDOW (window) != gtk_window_get_transient_for (GTK_WINDOW (shortcuts_window)))
        {
                gtk_window_set_transient_for (GTK_WINDOW (shortcuts_window), GTK_WINDOW (window));
        }

        gtk_widget_show_all (shortcuts_window);
        gtk_window_present (GTK_WINDOW (shortcuts_window));
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

        /* i18n: Please don't translate "Devhelp" (it's marked as translatable
         * for transliteration only) */
        gtk_show_about_dialog (parent,
                               "name", _("Devhelp"),
                               "version", PACKAGE_VERSION,
                               "comments", _("A developers' help browser for GNOME"),
                               "authors", authors,
                               "translator-credits", _("translator-credits"),
                               "website", PACKAGE_URL,
                               "website-label", _("Devhelp Website"),
                               "logo-icon-name", PACKAGE_TARNAME,
                               "license-type", GTK_LICENSE_GPL_2_0,
                               "copyright", "Copyright 2001-2016 – the Devhelp team",
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

static void
search_assistant_cb (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
        DhApp *app = DH_APP (user_data);
        GtkWindow *assistant;
        const gchar *str;

        str = g_variant_get_string (parameter, NULL);
        if (str[0] == '\0') {
                g_warning ("Cannot look for keyword in Search Assistant: "
                           "No keyword given");
                return;
        }

        /* Look for an already registered assistant */
        assistant = dh_app_peek_assistant (app);
        if (!assistant) {
                assistant = GTK_WINDOW (dh_assistant_new (app));
                gtk_application_add_window (GTK_APPLICATION (app), assistant);
        }

        dh_assistant_search (DH_ASSISTANT (assistant), str);
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

static GActionEntry app_entries[] = {
        /* general  actions */
        { "new-window",       new_window_cb,       NULL, NULL, NULL },
        { "preferences",      preferences_cb,      NULL, NULL, NULL },
        { "shortcuts",        shortcuts_cb,        NULL, NULL, NULL },
        { "about",            about_cb,            NULL, NULL, NULL },
        { "quit",             quit_cb,             NULL, NULL, NULL },
        /* additional commandline-specific actions */
        { "search",           search_cb,           "s",  NULL, NULL },
        { "search-assistant", search_assistant_cb, "s",  NULL, NULL },
        { "raise",            raise_cb,            NULL, NULL, NULL },
};

/******************************************************************************/

static void
setup_accelerators (DhApp *self)
{
        const gchar *accels[] = {NULL, NULL, NULL, NULL};

        accels[0] = "<Primary>0";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.zoom-default", accels);

        accels[0] = "<Primary>minus";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.zoom-out", accels);

        accels[0] = "<Primary>plus";
        accels[1] = "<Primary>equal";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.zoom-in", accels);
        accels[0] = NULL;

        accels[0] = "<Primary>f";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.find", accels);

        accels[0] = "<Primary>c";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.copy", accels);

        accels[0] = "<Primary>p";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.print", accels);

        accels[0] = "<Primary>t";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.new-tab", accels);

        accels[0] = "<Primary>Page_Down";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.next-tab", accels);

        accels[0] = "<Primary>Page_Up";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.prev-tab", accels);

        accels[0] = "F9";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.show-sidebar", accels);

        accels[0] = "<Primary>w";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.close", accels);

        accels[0] = "F10";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.gear-menu", accels);

        accels[0] = "<Primary>F1";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "app.shortcuts", accels);

        accels[0] = "<Alt>Right";
        accels[1] = "Forward";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.go-forward", accels);

        accels[0] = "<Alt>Left";
        accels[1] = "Back";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.go-back", accels);

        accels[0] = "<Primary>k";
        accels[1] = "<Primary>s";
        accels[2] = "<Primary>l";
        gtk_application_set_accels_for_action (GTK_APPLICATION (self), "win.focus-search", accels);
}

/******************************************************************************/

static void
dh_app_startup (GApplication *application)
{
        DhApp *app = DH_APP (application);
        DhAppPrivate *priv = dh_app_get_instance_private (app);

        g_application_set_resource_base_path (application, "/org/gnome/devhelp");

        /* Chain up parent's startup */
        G_APPLICATION_CLASS (dh_app_parent_class)->startup (application);

        /* Setup actions */
        g_action_map_add_action_entries (G_ACTION_MAP (app),
                                         app_entries, G_N_ELEMENTS (app_entries),
                                         app);

        if (_dh_app_has_app_menu (app)) {
                GtkBuilder *builder;
                GError *error = NULL;

                /* Setup menu */
                builder = gtk_builder_new ();

                if (!gtk_builder_add_from_resource (builder,
                                                    "/org/gnome/devhelp/devhelp-menu.ui",
                                                    &error)) {
                        g_warning ("loading menu builder file: %s", error->message);
                        g_error_free (error);
                } else {
                        GMenuModel *app_menu;

                        app_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu"));
                        gtk_application_set_app_menu (GTK_APPLICATION (application),
                                                      app_menu);
                }

                g_object_unref (builder);
        }

        /* Setup accelerators */
        setup_accelerators (app);

        /* Load the book manager */
        priv->book_manager = dh_book_manager_new ();
        dh_book_manager_populate (priv->book_manager);
}

static void
dh_app_activate (GApplication *application)
{
        dh_app_new_window (DH_APP (application));
}

/******************************************************************************/

/**
 * dh_app_new:
 *
 * Create a new #DhApp object.
 *
 * Returns: a new #DhApp object
 */
DhApp *
dh_app_new (void)
{
        return g_object_new (DH_TYPE_APP,
                             "application-id",   "org.gnome.Devhelp",
                             "flags",            G_APPLICATION_HANDLES_COMMAND_LINE,
                             "register-session", TRUE,
                             NULL);
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
                dh_app_quit (DH_APP (app));
        } else if (option_search) {
                dh_app_search (DH_APP (app), option_search);
        } else if (option_search_assistant) {
                dh_app_search_assistant (DH_APP (app), option_search_assistant);
        } else {
                dh_app_raise (DH_APP (app));
        }

        return 0;
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

static void
dh_app_dispose (GObject *object)
{
        DhApp *app = DH_APP (object);
        DhAppPrivate *priv = dh_app_get_instance_private (app);

        g_clear_object (&priv->book_manager);

        G_OBJECT_CLASS (dh_app_parent_class)->dispose (object);
}

static void
dh_app_class_init (DhAppClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

        application_class->startup = dh_app_startup;
        application_class->activate = dh_app_activate;
        application_class->handle_local_options = dh_app_handle_local_options;
        application_class->command_line = dh_app_command_line;

        object_class->dispose = dh_app_dispose;
}
