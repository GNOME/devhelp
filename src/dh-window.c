/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2008 Imendio AB
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
 * Copyright (C) 2012 Thomas Bechtold <toabctl@gnome.org>
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
#include <math.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#ifdef HAVE_WEBKIT2
#include <webkit2/webkit2.h>
#else
#include <webkit/webkit.h>
#endif

#include <libgd/gd.h>

#include "dh-book-manager.h"
#include "dh-book.h"
#include "dh-sidebar.h"
#include "dh-window.h"
#include "dh-util.h"
#include "dh-marshal.h"
#include "dh-enum-types.h"
#include "dh-settings.h"
#include "eggfindbar.h"

#define TAB_WIDTH_N_CHARS 15

struct _DhWindowPriv {
        GtkWidget      *main_box;
        GtkWidget      *hpaned;
        GtkWidget      *sidebar;
        GtkWidget      *notebook;
        GtkWidget      *toolbar;

        GtkWidget      *vbox;
        GtkWidget      *findbar;

        GtkBuilder     *builder;
        GtkActionGroup *action_group;

        DhLink         *selected_search_link;
        guint           find_source_id;
        DhSettings     *settings;
};

enum {
        OPEN_LINK,
        LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

static guint tab_accel_keys[] = {
        GDK_KEY_1, GDK_KEY_2, GDK_KEY_3, GDK_KEY_4, GDK_KEY_5,
        GDK_KEY_6, GDK_KEY_7, GDK_KEY_8, GDK_KEY_9, GDK_KEY_0
};

static const
struct
{
        gchar *name;
        double level;
}
zoom_levels[] =
{
        { N_("50%"), 0.5 },
        { N_("75%"), 0.8408964152 },
        { N_("100%"), 1.0 },
        { N_("125%"), 1.1892071149 },
        { N_("150%"), 1.4142135623 },
        { N_("175%"), 1.6817928304 },
        { N_("200%"), 2.0 },
        { N_("300%"), 2.8284271247 },
        { N_("400%"), 4.0 }
};

static const guint n_zoom_levels = G_N_ELEMENTS (zoom_levels);

#define ZOOM_MINIMAL    (zoom_levels[0].level)
#define ZOOM_MAXIMAL    (zoom_levels[n_zoom_levels - 1].level)
#define ZOOM_DEFAULT    (zoom_levels[2].level)

static void           dh_window_class_init           (DhWindowClass   *klass);
static void           dh_window_init                 (DhWindow        *window);
static void           window_populate                (DhWindow        *window);
static void           window_search_link_selected_cb (GObject         *ignored,
                                                      DhLink          *link,
                                                      DhWindow        *window);
static void           window_check_history           (DhWindow        *window,
                                                      WebKitWebView   *web_view);
static void           window_web_view_tab_accel_cb   (GtkAccelGroup   *accel_group,
                                                      GObject         *object,
                                                      guint            key,
                                                      GdkModifierType  mod,
                                                      DhWindow        *window);
static void           window_find_search_changed_cb  (GObject         *object,
                                                      GParamSpec      *arg1,
                                                      DhWindow        *window);
static void           window_find_case_changed_cb    (GObject         *object,
                                                      GParamSpec      *arg1,
                                                      DhWindow        *window);
static void           window_find_next_cb            (GtkWidget       *widget,
                                                      DhWindow        *window);
static void           findbar_find_next              (DhWindow        *window);
static void           window_find_previous_cb        (GtkWidget       *widget,
                                                      DhWindow        *window);
static void           findbar_find_previous          (DhWindow        *window);
static void           window_findbar_close_cb        (GtkWidget       *widget,
                                                      DhWindow        *window);
static GtkWidget *    window_new_tab_label           (DhWindow        *window,
                                                      const gchar     *label,
                                                      const GtkWidget *parent);
static int            window_open_new_tab            (DhWindow        *window,
                                                      const gchar     *location,
                                                      gboolean         switch_focus);
static WebKitWebView *window_get_active_web_view     (DhWindow        *window);
static GtkWidget *    window_get_active_info_bar     (DhWindow *window);
static void           window_update_title            (DhWindow        *window,
                                                      WebKitWebView   *web_view,
                                                      const gchar     *title);
static void           window_tab_set_title           (DhWindow        *window,
                                                      WebKitWebView   *web_view,
                                                      const gchar     *title);
static void           window_close_tab               (DhWindow *window,
                                                      gint      page_num);
static gboolean       do_search                      (DhWindow *window);

G_DEFINE_TYPE (DhWindow, dh_window, GTK_TYPE_APPLICATION_WINDOW);

#define GET_PRIVATE(instance) G_TYPE_INSTANCE_GET_PRIVATE \
  (instance, DH_TYPE_WINDOW, DhWindowPriv);

static void
new_tab_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
        DhWindow *window = user_data;

        window_open_new_tab (window, NULL, TRUE);
}

static void
print_cb (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
        DhWindow *window = user_data;
        WebKitWebView *web_view = window_get_active_web_view (window);
#ifdef HAVE_WEBKIT2
        WebKitPrintOperation *print_operation;

        print_operation = webkit_print_operation_new (web_view);
        webkit_print_operation_run_dialog (print_operation, GTK_WINDOW (window));
        g_object_unref (print_operation);
#else
        webkit_web_view_execute_script (web_view, "print();");
#endif
}

static void
window_close_tab (DhWindow *window,
                  gint      page_num)
{
        DhWindowPriv *priv;
        gint          pages;

        priv = window->priv;

        gtk_notebook_remove_page (GTK_NOTEBOOK (priv->notebook), page_num);

        pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook));

        if (pages == 0) {
                gtk_widget_destroy (GTK_WIDGET (window));
        }
        else if (pages == 1) {
                gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);
        }
}

static void
close_cb (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
        DhWindow *window = user_data;
        gint page_num;

        page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (window->priv->notebook));
        window_close_tab (window, page_num);
}

static void
copy_cb (GSimpleAction *action,
         GVariant      *parameter,
         gpointer       user_data)
{
        DhWindow *window = user_data;
        GtkWidget *widget;
        DhWindowPriv  *priv;

        priv = window->priv;

        widget = gtk_window_get_focus (GTK_WINDOW (window));

        if (GTK_IS_EDITABLE (widget)) {
                gtk_editable_copy_clipboard (GTK_EDITABLE (widget));
        } else if (GTK_IS_TREE_VIEW (widget) &&
                   gtk_widget_is_ancestor (widget, priv->sidebar) &&
                   priv->selected_search_link) {
                GtkClipboard *clipboard;
                clipboard = gtk_widget_get_clipboard (widget, GDK_SELECTION_CLIPBOARD);
                gtk_clipboard_set_text (clipboard,
                                dh_link_get_name(priv->selected_search_link), -1);
        } else {
                WebKitWebView *web_view;

                web_view = window_get_active_web_view (window);
#ifdef HAVE_WEBKIT2
                webkit_web_view_execute_editing_command (web_view, WEBKIT_EDITING_COMMAND_COPY);
#else
                webkit_web_view_copy_clipboard (web_view);
#endif
        }
}

static void
find_cb (GSimpleAction *action,
         GVariant      *parameter,
         gpointer       user_data)
{
        DhWindow *window = user_data;
        DhWindowPriv  *priv;
#ifndef HAVE_WEBKIT2
        WebKitWebView *web_view;
#endif
        priv = window->priv;

        gtk_widget_show (priv->findbar);
        gtk_widget_grab_focus (priv->findbar);

#ifdef HAVE_WEBKIT2
        /* The behaviour for WebKit1 is to re-enable highlighting without
           starting a new search. WebKit2 API does not allow that
           without invoking a new search. */
        do_search (window);
#else
        web_view = window_get_active_web_view (window);
        webkit_web_view_set_highlight_text_matches (web_view, TRUE);
#endif /* HAVE_WEBKIT2 */
}

static void
find_previous_cb (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
        findbar_find_previous (DH_WINDOW (user_data));
}

static void
find_next_cb (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
        findbar_find_next (DH_WINDOW (user_data));
}

static int
window_get_current_zoom_level_index (DhWindow *window)
{
        WebKitWebView *web_view;
        double previous, current, mean;
        double zoom_level = ZOOM_DEFAULT;
        int i;

        web_view = window_get_active_web_view (window);
        if (web_view)
                zoom_level = webkit_web_view_get_zoom_level (web_view);

        previous = zoom_levels[0].level;
        for (i = 1; i < n_zoom_levels; i++) {
                current = zoom_levels[i].level;
                mean = sqrt (previous * current);

                if (zoom_level <= mean)
                        return i - 1;

                previous = current;
        }

        return n_zoom_levels - 1;
}

static void
window_update_zoom_actions_state (DhWindow *window)
{
        GAction *action;
        int zoom_level_idx;
        gboolean enabled;

        zoom_level_idx = window_get_current_zoom_level_index (window);

        enabled = zoom_levels[zoom_level_idx].level < ZOOM_MAXIMAL;
        action = g_action_map_lookup_action (G_ACTION_MAP (window), "zoom-in");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);

        enabled = zoom_levels[zoom_level_idx].level > ZOOM_MINIMAL;
        action = g_action_map_lookup_action (G_ACTION_MAP (window), "zoom-out");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);

        enabled = zoom_levels[zoom_level_idx].level != ZOOM_DEFAULT;
        action = g_action_map_lookup_action (G_ACTION_MAP (window), "zoom-default");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);
}

static void
zoom_in_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
	DhWindow *window = user_data;
        int zoom_level_idx;

        zoom_level_idx = window_get_current_zoom_level_index (window);
        if (zoom_levels[zoom_level_idx].level < ZOOM_MAXIMAL) {
                WebKitWebView *web_view;

                web_view = window_get_active_web_view (window);
                webkit_web_view_set_zoom_level (web_view, zoom_levels[zoom_level_idx + 1].level);
                window_update_zoom_actions_state (window);
        }

}

static void
zoom_out_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
	DhWindow *window = user_data;
        int zoom_level_idx;

        zoom_level_idx = window_get_current_zoom_level_index (window);
        if (zoom_levels[zoom_level_idx].level > ZOOM_MINIMAL) {
                WebKitWebView *web_view;

                web_view = window_get_active_web_view (window);
                webkit_web_view_set_zoom_level (web_view, zoom_levels[zoom_level_idx - 1].level);
                window_update_zoom_actions_state (window);
        }
}

static void
zoom_default_cb (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
	DhWindow *window = user_data;
        WebKitWebView *web_view;

        web_view = window_get_active_web_view (window);
        webkit_web_view_set_zoom_level (web_view, ZOOM_DEFAULT);
        window_update_zoom_actions_state (window);
}

static void
go_back_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
        DhWindow      *window = user_data;
        DhWindowPriv  *priv;
        WebKitWebView *web_view;
        GtkWidget     *frame;

        priv = window->priv;

        frame = gtk_notebook_get_nth_page (
                GTK_NOTEBOOK (priv->notebook),
                gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)));
        web_view = g_object_get_data (G_OBJECT (frame), "web_view");

        webkit_web_view_go_back (web_view);
}

static void
go_forward_cb (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
        DhWindow      *window = user_data;
        DhWindowPriv  *priv;
        WebKitWebView *web_view;
        GtkWidget     *frame;

        priv = window->priv;

        frame = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook),
                                           gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)));
        web_view = g_object_get_data (G_OBJECT (frame), "web_view");

        webkit_web_view_go_forward (web_view);
}

static void
window_open_link_cb (DhWindow *window,
                     const char *location,
                     DhOpenLinkFlags flags)
{
        if (flags & DH_OPEN_LINK_NEW_TAB) {
                window_open_new_tab (window, location, FALSE);
        }
        else if (flags & DH_OPEN_LINK_NEW_WINDOW) {
                dh_app_new_window (DH_APP (gtk_window_get_application (GTK_WINDOW (window))));
        }
}

static GActionEntry win_entries[] = {
        /* file */
        { "new-tab",          new_tab_cb,          NULL, NULL, NULL },
        { "print",            print_cb,            NULL, NULL, NULL },
        { "close",            close_cb,            NULL, NULL, NULL },
        /* edit */
        { "copy",             copy_cb,             NULL, NULL, NULL },
        { "find",             find_cb,             NULL, NULL, NULL },
        { "find-next",        find_next_cb,        NULL, NULL, NULL },
        { "find-previous",    find_previous_cb,    NULL, NULL, NULL },
        /* view */
        { "zoom-in",          zoom_in_cb,          NULL, NULL, NULL },
        { "zoom-out",         zoom_out_cb,         NULL, NULL, NULL },
        { "zoom-default",     zoom_default_cb,     NULL, NULL, NULL },
        /* go */
        { "go-back",          go_back_cb,          NULL, "false", NULL },
        { "go-forward",       go_forward_cb,       NULL, "false", NULL },
};

static void
settings_fonts_changed_cb (DhSettings *settings,
                           const gchar *font_name_fixed,
                           const gchar *font_name_variable,
                           gpointer user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWindowPriv *priv = window->priv;
        gint i;
        WebKitWebView *view;
        /* change font for all pages */
        for (i = 0; i < gtk_notebook_get_n_pages (GTK_NOTEBOOK(priv->notebook)); i++) {
                GtkWidget *page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), i);
                view = WEBKIT_WEB_VIEW (g_object_get_data (G_OBJECT (page), "web_view"));
                dh_util_view_set_font (view, font_name_fixed, font_name_variable);
        }
}

static gboolean
window_configure_event_cb (GtkWidget *window,
                           GdkEventConfigure *event,
                           gpointer user_data)
{
        DhWindow *dhwindow;
        DhWindowPriv  *priv;

        dhwindow = DH_WINDOW (user_data);
        priv = GET_PRIVATE (dhwindow);
        dh_util_window_settings_save (
                GTK_WINDOW (window),
                dh_settings_peek_window_settings (priv->settings), TRUE);
	return FALSE;
}

static void
dh_window_init (DhWindow *window)
{
        DhWindowPriv  *priv;
        GtkAccelGroup *accel_group;
        GClosure      *closure;
        gint           i;
        GError        *error = NULL;

        priv = GET_PRIVATE (window);
        window->priv = priv;

        gtk_window_set_hide_titlebar_when_maximized (GTK_WINDOW (window), TRUE);

        priv->selected_search_link = NULL;

        /* handle settings */
        priv->settings = dh_settings_get ();
        g_signal_connect (priv->settings,
                          "fonts-changed",
                          G_CALLBACK (settings_fonts_changed_cb),
                          window);

        /* Setup builder */
        priv->builder = gtk_builder_new ();
        if (!gtk_builder_add_from_resource (priv->builder, "/org/gnome/devhelp/devhelp.ui", &error)) {
                g_error ("Cannot add resource to builder: %s", error ? error->message : "unknown error");
                g_clear_error (&error);
        }

        priv->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_show (priv->main_box);

        gtk_container_add (GTK_CONTAINER (window), priv->main_box);

        g_signal_connect (window,
                          "open-link",
                          G_CALLBACK (window_open_link_cb),
                          window);

        g_action_map_add_action_entries (G_ACTION_MAP (window),
                                         win_entries, G_N_ELEMENTS (win_entries),
                                         window);

        accel_group = gtk_accel_group_new ();
        gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
        for (i = 0; i < G_N_ELEMENTS (tab_accel_keys); i++) {
                closure =  g_cclosure_new (G_CALLBACK (window_web_view_tab_accel_cb),
                                           window,
                                           NULL);
                gtk_accel_group_connect (accel_group,
                                         tab_accel_keys[i],
                                         GDK_MOD1_MASK,
                                         0,
                                         closure);
        }
}

static void
dispose (GObject *object)
{
	DhWindow *self = DH_WINDOW (object);
        g_clear_object (&self->priv->settings);
        g_clear_object (&self->priv->builder);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (dh_window_parent_class)->dispose (object);
}

static void
dh_window_class_init (DhWindowClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        g_type_class_add_private (klass, sizeof (DhWindowPriv));
        object_class->dispose = dispose;

        signals[OPEN_LINK] =
                g_signal_new ("open-link",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DhWindowClass, open_link),
                              NULL, NULL,
                              _dh_marshal_VOID__STRING_FLAGS,
                              G_TYPE_NONE,
                              2,
                              G_TYPE_STRING,
                              DH_TYPE_OPEN_LINK_FLAGS);

        gtk_rc_parse_string ("style \"devhelp-tab-close-button-style\"\n"
                             "{\n"
                             "GtkWidget::focus-padding = 0\n"
                             "GtkWidget::focus-line-width = 0\n"
                             "xthickness = 0\n"
                             "ythickness = 0\n"
                             "}\n"
                             "widget \"*.devhelp-tab-close-button\" "
                             "style \"devhelp-tab-close-button-style\"");
}

static void
window_web_view_switch_page_cb (GtkNotebook     *notebook,
                                gpointer         page,
                                guint            new_page_num,
                                DhWindow        *window)
{
        DhWindowPriv *priv;
        GtkWidget    *new_page;

        priv = window->priv;

        new_page = gtk_notebook_get_nth_page (notebook, new_page_num);
        if (new_page) {
                WebKitWebView  *new_web_view;
                const gchar    *location;

                new_web_view = g_object_get_data (G_OBJECT (new_page), "web_view");

                /* Sync the book tree */
                location = webkit_web_view_get_uri (new_web_view);

                if (location)
                        dh_sidebar_select_uri (DH_SIDEBAR (priv->sidebar), location);

                window_check_history (window, new_web_view);

                window_update_title (window, new_web_view, NULL);
        } else {
                /* i18n: Please don't translate "Devhelp" (it's marked as translatable
                 * for transliteration only) */
                gtk_window_set_title (GTK_WINDOW (window), _("Devhelp"));
                window_check_history (window, NULL);
        }
}

static void
window_web_view_switch_page_after_cb (GtkNotebook     *notebook,
                                      gpointer         page,
                                      guint            new_page_num,
                                      DhWindow        *window)
{
        window_update_zoom_actions_state (window);
}

static void
window_populate (DhWindow *window)
{
        DhWindowPriv  *priv;
        DhBookManager *book_manager;
        GtkWidget     *back;
        GtkWidget     *forward;
        GtkWidget     *box;
        GtkWidget     *menu_button;
        GtkWidget     *menu;

        priv = window->priv;
        book_manager = dh_app_peek_book_manager (DH_APP (gtk_window_get_application (GTK_WINDOW (window))));

        priv->toolbar = gd_main_toolbar_new ();
        back = gd_main_toolbar_add_button (GD_MAIN_TOOLBAR (priv->toolbar),
                                           "go-previous-symbolic",
                                           _("Back"),
                                           TRUE);
        gtk_actionable_set_action_name ( GTK_ACTIONABLE (back), "win.go-back");
        forward = gd_main_toolbar_add_button (GD_MAIN_TOOLBAR (priv->toolbar),
                                              "go-next-symbolic",
                                              _("Forward"),
                                              TRUE);
        gtk_actionable_set_action_name ( GTK_ACTIONABLE (forward), "win.go-forward");
        box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_style_context_add_class (gtk_widget_get_style_context (box), "linked");
        gtk_widget_reparent (back, box);
        gtk_widget_reparent (forward, box);

        gd_main_toolbar_add_widget (GD_MAIN_TOOLBAR (priv->toolbar), box, TRUE);
        menu_button = gd_main_toolbar_add_menu (GD_MAIN_TOOLBAR (priv->toolbar),
                                                "emblem-system-symbolic",
                                                "",
                                                FALSE);
        menu = gtk_builder_get_object (priv->builder, "window-menu");
        gtk_menu_button_set_menu_model (menu_button, menu);

        /* Add toolbar to main box */
        gtk_box_pack_start (GTK_BOX (priv->main_box), priv->toolbar,
                            FALSE, FALSE, 0);

        priv->hpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
        gtk_box_pack_start (GTK_BOX (priv->main_box), priv->hpaned, TRUE, TRUE, 0);

        /* Sidebar */
        priv->sidebar = dh_sidebar_new (book_manager);
        gtk_paned_add1 (GTK_PANED (priv->hpaned), priv->sidebar);
        g_signal_connect (priv->sidebar,
                          "link-selected",
                          G_CALLBACK (window_search_link_selected_cb),
                          window);

        /* Document view */
        priv->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_paned_add2 (GTK_PANED (priv->hpaned), priv->vbox);

        /* HTML tabs notebook. */
        priv->notebook = gtk_notebook_new ();
        gtk_container_set_border_width (GTK_CONTAINER (priv->notebook), 0);
        gtk_notebook_set_show_border (GTK_NOTEBOOK (priv->notebook), FALSE);
        gtk_notebook_set_scrollable (GTK_NOTEBOOK (priv->notebook), TRUE);
        gtk_box_pack_start (GTK_BOX (priv->vbox), priv->notebook, TRUE, TRUE, 0);

        g_signal_connect (priv->notebook,
                          "switch-page",
                          G_CALLBACK (window_web_view_switch_page_cb),
                          window);
        g_signal_connect_after (priv->notebook,
                                "switch-page",
                                G_CALLBACK (window_web_view_switch_page_after_cb),
                                window);

        /* Create findbar */
        priv->findbar = egg_find_bar_new ();
        gtk_widget_set_no_show_all (priv->findbar, TRUE);
        gtk_box_pack_start (GTK_BOX (priv->vbox), priv->findbar, FALSE, FALSE, 0);

        g_signal_connect (priv->findbar,
                          "notify::search-string",
                          G_CALLBACK(window_find_search_changed_cb),
                          window);
        g_signal_connect (priv->findbar,
                          "notify::case-sensitive",
                          G_CALLBACK (window_find_case_changed_cb),
                          window);
        g_signal_connect (priv->findbar,
                          "previous",
                          G_CALLBACK (window_find_previous_cb),
                          window);
        g_signal_connect (priv->findbar,
                          "next",
                          G_CALLBACK (window_find_next_cb),
                          window);
        g_signal_connect (priv->findbar,
                          "close",
                          G_CALLBACK (window_findbar_close_cb),
                          window);

        gtk_widget_show_all (priv->hpaned);

        window_update_zoom_actions_state (window);
        window_open_new_tab (window, NULL, TRUE);
}

static gchar *
find_library_equivalent (DhWindow    *window,
                         const gchar *uri)
{
        gchar **components;
        GList *iter;
        DhLink *link;
        DhBookManager *book_manager;
        gchar *book_id;
        gchar *filename;
        gchar *local_uri = NULL;
        GList *books;

        components = g_strsplit (uri, "/", 0);
        book_id = components[4];
        filename = components[6];

        book_manager = dh_app_peek_book_manager (DH_APP (gtk_window_get_application (GTK_WINDOW (window))));

        /* use list pointer to iterate */
        for (books = dh_book_manager_get_books (book_manager);
             !local_uri && books;
             books = g_list_next (books)) {
                DhBook *book = DH_BOOK (books->data);

                for (iter = dh_book_get_keywords (book);
                     iter;
                     iter = g_list_next (iter)) {
                        link = iter->data;
                        if (g_strcmp0 (dh_link_get_book_id (link), book_id) != 0) {
                                continue;
                        }
                        if (g_strcmp0 (dh_link_get_file_name (link), filename) != 0) {
                                continue;
                        }
                        local_uri = dh_link_get_uri (link);
                        break;
                }
        }

        g_strfreev (components);

        return local_uri;
}

#ifdef HAVE_WEBKIT2
static gboolean
window_web_view_decide_policy_cb (WebKitWebView           *web_view,
                                  WebKitPolicyDecision    *policy_decision,
                                  WebKitPolicyDecisionType type,
                                  DhWindow                *window)
#else
static gboolean
window_web_view_navigation_policy_decision_requested (WebKitWebView             *web_view,
                                                      WebKitWebFrame            *frame,
                                                      WebKitNetworkRequest      *request,
                                                      WebKitWebNavigationAction *navigation_action,
                                                      WebKitWebPolicyDecision   *policy_decision,
                                                      DhWindow                  *window)
#endif
{
        DhWindowPriv *priv;
        const char   *uri;
#ifdef HAVE_WEBKIT2
        WebKitNavigationPolicyDecision *navigation_decision;
#endif

        priv = window->priv;

#ifdef HAVE_WEBKIT2
        if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
                return FALSE;

        navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (policy_decision);
        uri = webkit_uri_request_get_uri (webkit_navigation_policy_decision_get_request (navigation_decision));
#else
        uri = webkit_network_request_get_uri (request);
#endif

        /* make sure to hide the info bar on page change */
        gtk_widget_hide (window_get_active_info_bar (window));

#ifdef HAVE_WEBKIT2
        if (webkit_navigation_policy_decision_get_mouse_button (navigation_decision) == 2) { /* middle click */
                webkit_policy_decision_ignore (policy_decision);
#else
        if (webkit_web_navigation_action_get_button (navigation_action) == 2) { /* middle click */
                webkit_web_policy_decision_ignore (policy_decision);
#endif
                g_signal_emit (window, signals[OPEN_LINK], 0, uri, DH_OPEN_LINK_NEW_TAB);
                return TRUE;
        }

        if (strcmp (uri, "about:blank") == 0) {
                return FALSE;
        }

        if (strncmp (uri, "http://library.gnome.org/devel/", 31) == 0) {
                gchar *local_uri = find_library_equivalent (window, uri);
                if (local_uri) {
#ifdef HAVE_WEBKIT2
                        webkit_policy_decision_ignore (policy_decision);
#else
                        webkit_web_policy_decision_ignore (policy_decision);
#endif
                        _dh_window_display_uri (window, local_uri);
                        g_free (local_uri);
                        return TRUE;
                }
        }

        if (strncmp (uri, "file://", 7) != 0) {
#ifdef HAVE_WEBKIT2
                webkit_policy_decision_ignore (policy_decision);
#else
                webkit_web_policy_decision_ignore (policy_decision);
#endif
                gtk_show_uri (NULL, uri, GDK_CURRENT_TIME, NULL);
                return TRUE;
        }

#ifndef HAVE_WEBKIT2
        /* We already do this in load_changed_cb() for webkit2 */
        if (web_view == window_get_active_web_view (window)) {
                dh_sidebar_select_uri (DH_SIDEBAR (priv->sidebar), uri);
                window_check_history (window, web_view);
        }
#endif

        return FALSE;
}

#ifdef HAVE_WEBKIT2
static void
window_web_view_load_changed_cb (WebKitWebView   *web_view,
                                 WebKitLoadEvent  load_event,
                                 DhWindow        *window)
{
        const gchar *uri;
        DhWindowPriv *priv;

        priv = window->priv;

        if (load_event != WEBKIT_LOAD_COMMITTED)
                return;

        uri = webkit_web_view_get_uri (web_view);
        dh_sidebar_select_uri (DH_SIDEBAR (priv->sidebar), uri);
        window_check_history (window, web_view);
}
#endif

#ifdef HAVE_WEBKIT2
static gboolean
window_web_view_load_failed_cb (WebKitWebView   *web_view,
                                WebKitLoadEvent  load_event,
                                const gchar     *uri,
                                GError          *web_error,
                                DhWindow        *window)
#else
static gboolean
window_web_view_load_error_cb (WebKitWebView  *web_view,
                               WebKitWebFrame *frame,
                               gchar          *uri,
                               gpointer       *web_error,
                               DhWindow       *window)
#endif
{
        GtkWidget *info_bar;
        GtkWidget *content_area;
        GtkWidget *message_label;
        GList     *children;
        gchar     *markup;

        info_bar = window_get_active_info_bar (window);
        markup = g_strdup_printf ("<b>%s</b>",
                       _("Error opening the requested link."));
        message_label = gtk_label_new (markup);
        gtk_misc_set_alignment (GTK_MISC (message_label), 0, 0.5);
        gtk_label_set_use_markup (GTK_LABEL (message_label), TRUE);
        content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar));
        children = gtk_container_get_children (GTK_CONTAINER (content_area));
        if (children) {
                gtk_container_remove (GTK_CONTAINER (content_area), children->data);
                g_list_free (children);
        }
        gtk_container_add (GTK_CONTAINER (content_area), message_label);
        gtk_widget_show (message_label);

        gtk_widget_show (info_bar);
        g_free (markup);

        return TRUE;
}

static void
window_search_link_selected_cb (GObject  *ignored,
                                DhLink   *link,
                                DhWindow *window)
{
        DhWindowPriv  *priv;
        WebKitWebView *view;
        gchar         *uri;

        priv = window->priv;

        priv->selected_search_link = link;

        view = window_get_active_web_view (window);

        uri = dh_link_get_uri (link);
        webkit_web_view_load_uri (view, uri);
        g_free (uri);

        window_check_history (window, view);
}

static void
window_check_history (DhWindow      *window,
                      WebKitWebView *web_view)
{
        GAction       *action;
        gboolean       enabled;

        enabled = web_view ? webkit_web_view_can_go_forward (web_view) : FALSE;
        action = g_action_map_lookup_action (G_ACTION_MAP (window), "go-forward");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);

        enabled = web_view ? webkit_web_view_can_go_back (web_view) : FALSE;
        action = g_action_map_lookup_action (G_ACTION_MAP (window), "go-back");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);
}

static void
window_web_view_title_changed_cb (WebKitWebView *web_view,
                                  GParamSpec    *param_spec,
                                  DhWindow      *window)
{
        const gchar *title = webkit_web_view_get_title (web_view);

        if (web_view == window_get_active_web_view (window)) {
                window_update_title (window, web_view, title);
        }

        window_tab_set_title (window, web_view, title);
}

static gboolean
window_web_view_button_press_event_cb (WebKitWebView  *web_view,
                                       GdkEventButton *event,
                                       DhWindow       *window)
{
        if (event->button == 3) {
                return TRUE;
        }

        return FALSE;
}

static gboolean
do_search (DhWindow *window)
{
        DhWindowPriv         *priv = window->priv;
#ifdef HAVE_WEBKIT2
        WebKitFindController *find_controller;
        guint                 find_options = WEBKIT_FIND_OPTIONS_WRAP_AROUND;
        const gchar          *search_text;

        find_controller = webkit_web_view_get_find_controller (window_get_active_web_view (window));
        if (!egg_find_bar_get_case_sensitive (EGG_FIND_BAR (priv->findbar)))
                find_options |= WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE;

        search_text = egg_find_bar_get_search_string (EGG_FIND_BAR (priv->findbar));
        webkit_find_controller_search (find_controller, search_text, find_options, G_MAXUINT);
#else
        WebKitWebView *web_view;

        web_view = window_get_active_web_view (window);

        webkit_web_view_unmark_text_matches (web_view);
        webkit_web_view_mark_text_matches (
                web_view,
                egg_find_bar_get_search_string (EGG_FIND_BAR (priv->findbar)),
                egg_find_bar_get_case_sensitive (EGG_FIND_BAR (priv->findbar)), 0);
        webkit_web_view_set_highlight_text_matches (web_view, TRUE);

        webkit_web_view_search_text (
                web_view, egg_find_bar_get_search_string (EGG_FIND_BAR (priv->findbar)),
                egg_find_bar_get_case_sensitive (EGG_FIND_BAR (priv->findbar)),
                TRUE, TRUE);
#endif /* HAVE_WEBKIT2 */

        priv->find_source_id = 0;

	return FALSE;
}

static void
window_find_search_changed_cb (GObject    *object,
                               GParamSpec *pspec,
                               DhWindow   *window)
{
        DhWindowPriv *priv = window->priv;

        if (priv->find_source_id != 0) {
                g_source_remove (priv->find_source_id);
                priv->find_source_id = 0;
        }

        priv->find_source_id = g_timeout_add (300, (GSourceFunc)do_search, window);
}

static void
window_find_case_changed_cb (GObject    *object,
                             GParamSpec *pspec,
                             DhWindow   *window)
{
#ifdef HAVE_WEBKIT2
        do_search (window);
#else
        DhWindowPriv  *priv = window->priv;;
        WebKitWebView *view;
        const gchar   *string;
        gboolean       case_sensitive;

        view = window_get_active_web_view (window);

        string = egg_find_bar_get_search_string (EGG_FIND_BAR (priv->findbar));
        case_sensitive = egg_find_bar_get_case_sensitive (EGG_FIND_BAR (priv->findbar));

        webkit_web_view_unmark_text_matches (view);
        webkit_web_view_mark_text_matches (view, string, case_sensitive, 0);
        webkit_web_view_set_highlight_text_matches (view, TRUE);
#endif
}

static void
findbar_find_next (DhWindow *window)
{
        DhWindowPriv         *priv = window->priv;
        WebKitWebView        *view;
#ifdef HAVE_WEBKIT2
        WebKitFindController *find_controller;
#else
        const gchar          *string;
        gboolean              case_sensitive;
#endif
        view = window_get_active_web_view (window);

        gtk_widget_show (priv->findbar);
#ifdef HAVE_WEBKIT2
        find_controller = webkit_web_view_get_find_controller (view);
        webkit_find_controller_search_next(find_controller);
#else
        string = egg_find_bar_get_search_string (EGG_FIND_BAR (priv->findbar));
        case_sensitive = egg_find_bar_get_case_sensitive (EGG_FIND_BAR (priv->findbar));
        webkit_web_view_search_text (view, string, case_sensitive, TRUE, TRUE);
#endif
}

static void
window_find_next_cb (GtkWidget *widget,
                     DhWindow  *window)
{
        findbar_find_next (window);
}

static void
findbar_find_previous (DhWindow *window)
{
        DhWindowPriv         *priv = window->priv;
        WebKitWebView        *view;
#ifdef HAVE_WEBKIT2
        WebKitFindController *find_controller;
#else
        const gchar          *string;
        gboolean             case_sensitive;
#endif
        view = window_get_active_web_view (window);

        gtk_widget_show (priv->findbar);

#ifdef HAVE_WEBKIT2
        find_controller = webkit_web_view_get_find_controller (view);
        webkit_find_controller_search_previous(find_controller);
#else
        string = egg_find_bar_get_search_string (EGG_FIND_BAR (priv->findbar));
        case_sensitive = egg_find_bar_get_case_sensitive (EGG_FIND_BAR (priv->findbar));
        webkit_web_view_search_text (view, string, case_sensitive, FALSE, TRUE);
#endif
}

static void
window_find_previous_cb (GtkWidget *widget,
                         DhWindow  *window)
{
        findbar_find_previous (window);
}

static void
window_findbar_close_cb (GtkWidget *widget,
                         DhWindow  *window)
{
        DhWindowPriv         *priv = window->priv;
        WebKitWebView        *view;
#ifdef HAVE_WEBKIT2
        WebKitFindController *find_controller;
#endif
        view = window_get_active_web_view (window);

        gtk_widget_hide (priv->findbar);
#ifdef HAVE_WEBKIT2
        find_controller = webkit_web_view_get_find_controller (view);
        webkit_find_controller_search_finish (find_controller);
#else
        webkit_web_view_set_highlight_text_matches (view, FALSE);
#endif
}

#if 0
static void
window_web_view_open_new_tab_cb (WebKitWebView *web_view,
                                 const gchar   *location,
                                 DhWindow      *window)
{
        window_open_new_tab (window, location);
}
#endif

static void
window_web_view_tab_accel_cb (GtkAccelGroup   *accel_group,
                              GObject         *object,
                              guint            key,
                              GdkModifierType  mod,
                              DhWindow        *window)
{
        DhWindowPriv *priv;
        gint          i, num;

        priv = window->priv;

        num = -1;
        for (i = 0; i < G_N_ELEMENTS (tab_accel_keys); i++) {
                if (tab_accel_keys[i] == key) {
                        num = i;
                        break;
                }
        }

        if (num != -1) {
                gtk_notebook_set_current_page (
                        GTK_NOTEBOOK (priv->notebook), num);
        }
}

static int
window_open_new_tab (DhWindow    *window,
                     const gchar *location,
                     gboolean     switch_focus)
{
        DhWindowPriv *priv;
        GtkWidget    *view;
        GtkWidget    *vbox;
        GtkWidget    *label;
        gint          num;
        GtkWidget    *info_bar;
        gchar *font_fixed = NULL;
        gchar *font_variable = NULL;
#ifndef HAVE_WEBKIT2
        GtkWidget    *scrolled_window;
#endif

        priv = window->priv;

        /* Prepare the web view */
        view = webkit_web_view_new ();
        gtk_widget_show (view);
        /* get the current fonts and set them on the new view */
        dh_settings_get_selected_fonts (priv->settings, &font_fixed, &font_variable);
        dh_util_view_set_font (WEBKIT_WEB_VIEW (view), font_fixed, font_variable);
        g_free (font_fixed);
        g_free (font_variable);

        /* Prepare the info bar */
        info_bar = gtk_info_bar_new ();
        gtk_widget_set_no_show_all (info_bar, TRUE);
        gtk_info_bar_add_button (GTK_INFO_BAR (info_bar),
                                 GTK_STOCK_CLOSE, GTK_RESPONSE_OK);
        gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar),
                                       GTK_MESSAGE_ERROR);
        g_signal_connect (info_bar, "response",
                          G_CALLBACK (gtk_widget_hide), NULL);

#if 0
        /* Leave this in for now to make it easier to experiment. */
        {
                WebKitWebSettings *settings;
                settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (view));

                g_object_set (settings,
                              "user-stylesheet-uri", "file://" DATADIR "/devhelp/devhelp.css",
                              NULL);
        }
#endif

        vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_show (vbox);

        /* XXX: Really it would be much better to use real structures */
        g_object_set_data (G_OBJECT (vbox), "web_view", view);
        g_object_set_data (G_OBJECT (vbox), "info_bar", info_bar);

        gtk_box_pack_start (GTK_BOX(vbox), info_bar, FALSE, TRUE, 0);

#ifdef HAVE_WEBKIT2
        gtk_box_pack_start (GTK_BOX(vbox), view, TRUE, TRUE, 0);
#else
        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_AUTOMATIC,
                                        GTK_POLICY_AUTOMATIC);
        gtk_container_add (GTK_CONTAINER (scrolled_window), view);
        gtk_widget_show (scrolled_window);
        gtk_box_pack_start (GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
#endif

        label = window_new_tab_label (window, _("Empty Page"), vbox);
        gtk_widget_show_all (label);

        g_signal_connect (view, "notify::title",
                          G_CALLBACK (window_web_view_title_changed_cb),
                          window);
        g_signal_connect (view, "button-press-event",
                          G_CALLBACK (window_web_view_button_press_event_cb),
                          window);
#ifdef HAVE_WEBKIT2
        g_signal_connect (view, "decide-policy",
                          G_CALLBACK (window_web_view_decide_policy_cb),
                          window);
#else
        g_signal_connect (view, "navigation-policy-decision-requested",
                          G_CALLBACK (window_web_view_navigation_policy_decision_requested),
                          window);
#endif
#ifdef HAVE_WEBKIT2
        g_signal_connect (view, "load-changed",
                          G_CALLBACK (window_web_view_load_changed_cb),
                          window);
        g_signal_connect (view, "load-failed",
                          G_CALLBACK (window_web_view_load_failed_cb),
                          window);
#else
        g_signal_connect (view, "load-error",
                          G_CALLBACK (window_web_view_load_error_cb),
                          window);
#endif

        num = gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                                        vbox, NULL);
        gtk_container_child_set (GTK_CONTAINER (priv->notebook), vbox,
                                 "tab-expand", TRUE,
                                 NULL);
        gtk_notebook_set_tab_label (GTK_NOTEBOOK (priv->notebook),
                                    vbox, label);

        if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook)) > 1) {
                gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), TRUE);
        } else {
                gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);
        }

        if (location) {
                webkit_web_view_load_uri (WEBKIT_WEB_VIEW (view), location);
        } else {
                webkit_web_view_load_uri (WEBKIT_WEB_VIEW (view), "about:blank");
        }

        if (switch_focus) {
                gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), num);
        }

        return num;
}

static void
close_button_clicked_cb (GtkButton *button,
                         DhWindow  *window)
{
        GtkWidget *parent_tab;
        gint       pages;
        gint       i;

        parent_tab = g_object_get_data (G_OBJECT (button), "parent_tab");
        pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));
        for (i=0; i<pages; i++) {
                if (gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->priv->notebook), i) == parent_tab) {
                        window_close_tab (window, i);
                        break;
                }
        }
}

static GtkWidget*
window_new_tab_label (DhWindow        *window,
                      const gchar     *str,
                      const GtkWidget *parent)
{
        GtkWidget *label;
        GtkWidget *hbox;
        GtkWidget *close_button;
        GtkWidget *image;

        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

        label = gtk_label_new (str);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

        close_button = gtk_button_new ();
        gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);
        gtk_button_set_focus_on_click (GTK_BUTTON (close_button), FALSE);
        gtk_widget_set_name (close_button, "devhelp-tab-close-button");
        g_object_set_data (G_OBJECT (close_button), "parent_tab", (gpointer) parent);

        image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
        g_signal_connect (close_button, "clicked",
                          G_CALLBACK (close_button_clicked_cb),
                          window);
        gtk_container_add (GTK_CONTAINER (close_button), image);

        gtk_box_pack_start (GTK_BOX (hbox), close_button, FALSE, FALSE, 0);

        g_object_set_data (G_OBJECT (hbox), "label", label);
        g_object_set_data (G_OBJECT (hbox), "close-button", close_button);

        return hbox;
}

static WebKitWebView *
window_get_active_web_view (DhWindow *window)
{
        DhWindowPriv *priv;
        gint          page_num;
        GtkWidget    *page;

        priv = window->priv;

        page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook));
        if (page_num == -1) {
                return NULL;
        }

        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), page_num);

        return g_object_get_data (G_OBJECT (page), "web_view");
}

static GtkWidget *
window_get_active_info_bar (DhWindow *window)
{
        DhWindowPriv *priv;
        gint          page_num;
        GtkWidget    *page;

        priv = window->priv;

        page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook));
        if (page_num == -1) {
                return NULL;
        }

        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), page_num);

        return g_object_get_data (G_OBJECT (page), "info_bar");
}

static void
window_update_title (DhWindow      *window,
                     WebKitWebView *web_view,
                     const gchar   *web_view_title)
{
        DhWindowPriv *priv;

        priv = window->priv;

        if (!web_view_title)
                web_view_title = webkit_web_view_get_title (web_view);

        if (web_view_title && *web_view_title == '\0') {
                web_view_title = NULL;
        }

        gd_main_toolbar_set_labels (GD_MAIN_TOOLBAR (priv->toolbar),
                web_view_title, NULL);
}

static void
window_tab_set_title (DhWindow      *window,
                      WebKitWebView *web_view,
                      const gchar   *title)
{
        DhWindowPriv *priv;
        gint          num_pages, i;
        GtkWidget    *page;
        GtkWidget    *hbox;
        GtkWidget    *label;
        GtkWidget    *page_web_view;

        priv = window->priv;

        if (!title || title[0] == '\0') {
                title = _("Empty Page");
        }

        num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook));
        for (i = 0; i < num_pages; i++) {
                page = gtk_notebook_get_nth_page (
                        GTK_NOTEBOOK (priv->notebook), i);
                page_web_view = g_object_get_data (G_OBJECT (page), "web_view");

                /* The web_view widget is inside a frame. */
                if (page_web_view == GTK_WIDGET (web_view)) {
                        hbox = gtk_notebook_get_tab_label (
                                GTK_NOTEBOOK (priv->notebook), page);

                        if (hbox) {
                                label = g_object_get_data (G_OBJECT (hbox), "label");
                                gtk_label_set_text (GTK_LABEL (label), title);
                        }
                        break;
                }
        }
}

GtkWidget *
dh_window_new (DhApp *application)
{
        DhWindow     *window;
        DhWindowPriv *priv;

        window = g_object_new (DH_TYPE_WINDOW, NULL);
        priv = window->priv;

        gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (application));

        window_populate (window);

        gtk_window_set_icon_name (GTK_WINDOW (window), "devhelp");

        g_signal_connect (window, "configure-event",
                          G_CALLBACK (window_configure_event_cb),
                          window);

        dh_util_window_settings_restore (
                GTK_WINDOW (window),
                dh_settings_peek_window_settings (priv->settings), TRUE);

        g_settings_bind (dh_settings_peek_paned_settings (priv->settings),
                         "position", G_OBJECT (priv->hpaned),
                         "position", G_SETTINGS_BIND_DEFAULT);

        return GTK_WIDGET (window);
}

void
dh_window_search (DhWindow    *window,
                  const gchar *str)
{
        DhWindowPriv *priv;

        g_return_if_fail (DH_IS_WINDOW (window));

        priv = window->priv;

        dh_sidebar_set_search_string (DH_SIDEBAR (priv->sidebar), str);
}

/* Only call this with a URI that is known to be in the docs. */
void
_dh_window_display_uri (DhWindow    *window,
                        const gchar *uri)
{
        DhWindowPriv  *priv;
        WebKitWebView *web_view;

        g_return_if_fail (DH_IS_WINDOW (window));
        g_return_if_fail (uri != NULL);

        priv = window->priv;

        web_view = window_get_active_web_view (window);
        webkit_web_view_load_uri (web_view, uri);
        dh_sidebar_select_uri (DH_SIDEBAR (priv->sidebar), uri);
}
