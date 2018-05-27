/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2001-2008 Imendio AB
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
 * Copyright (C) 2012 Thomas Bechtold <toabctl@gnome.org>
 * Copyright (C) 2015-2018 Sébastien Wilmet <swilmet@gnome.org>
 *
 * Devhelp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Devhelp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Devhelp.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dh-window.h"
#include <glib/gi18n.h>
#include <webkit2/webkit2.h>
#include <devhelp/devhelp.h>
#include <amtk/amtk.h>
#include "dh-notebook.h"
#include "dh-settings-app.h"
#include "dh-tab.h"
#include "dh-tab-label.h"
#include "dh-util-app.h"
#include "dh-web-view.h"

typedef struct {
        GtkHeaderBar *header_bar;
        GtkMenuButton *window_menu_button;

        GtkPaned *hpaned;

        /* Left side of the @hpaned. */
        GtkWidget *grid_sidebar;
        DhSidebar *sidebar;

        /* Right side of the @hpaned. */
        GtkGrid *grid_documents;
        GtkSearchBar *search_bar;
        GtkSearchEntry *search_entry;
        GtkButton *search_prev_button;
        GtkButton *search_next_button;
        DhNotebook *notebook;
} DhWindowPrivate;

static void open_new_tab (DhWindow    *window,
                          const gchar *location,
                          gboolean     switch_focus);

G_DEFINE_TYPE_WITH_PRIVATE (DhWindow, dh_window, GTK_TYPE_APPLICATION_WINDOW);

static gboolean
dh_window_delete_event (GtkWidget   *widget,
                        GdkEventAny *event)
{
        DhSettingsApp *settings;

        settings = dh_settings_app_get_singleton ();
        dh_util_window_settings_save (GTK_WINDOW (widget),
                                      dh_settings_app_peek_window_settings (settings));

        if (GTK_WIDGET_CLASS (dh_window_parent_class)->delete_event == NULL)
                return GDK_EVENT_PROPAGATE;

        return GTK_WIDGET_CLASS (dh_window_parent_class)->delete_event (widget, event);
}

static void
dh_window_class_init (DhWindowClass *klass)
{
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        widget_class->delete_event = dh_window_delete_event;

        /* Bind class to template */
        gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/devhelp/dh-window.ui");
        gtk_widget_class_bind_template_child_private (widget_class, DhWindow, header_bar);
        gtk_widget_class_bind_template_child_private (widget_class, DhWindow, window_menu_button);
        gtk_widget_class_bind_template_child_private (widget_class, DhWindow, hpaned);
        gtk_widget_class_bind_template_child_private (widget_class, DhWindow, grid_sidebar);
        gtk_widget_class_bind_template_child_private (widget_class, DhWindow, grid_documents);
        gtk_widget_class_bind_template_child_private (widget_class, DhWindow, search_bar);
        gtk_widget_class_bind_template_child_private (widget_class, DhWindow, search_entry);
        gtk_widget_class_bind_template_child_private (widget_class, DhWindow, search_prev_button);
        gtk_widget_class_bind_template_child_private (widget_class, DhWindow, search_next_button);
}

/* Can return NULL during initialization and finalization, so it's better to
 * handle the NULL case with the return value of this function.
 */
static DhWebView *
get_active_web_view (DhWindow *window)
{
        DhWindowPrivate *priv = dh_window_get_instance_private (window);

        return dh_notebook_get_active_web_view (priv->notebook);
}

static void
update_window_title (DhWindow *window)
{
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        DhWebView *web_view;
        const gchar *title;

        web_view = get_active_web_view (window);
        if (web_view == NULL)
                return;

        title = dh_web_view_get_devhelp_title (web_view);
        gtk_header_bar_set_title (priv->header_bar, title);
}

static void
update_zoom_actions_sensitivity (DhWindow *window)
{
        DhWebView *web_view;
        GAction *action;
        gboolean enabled;

        web_view = get_active_web_view (window);
        if (web_view == NULL)
                return;

        enabled = dh_web_view_can_zoom_in (web_view);
        action = g_action_map_lookup_action (G_ACTION_MAP (window), "zoom-in");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);

        enabled = dh_web_view_can_zoom_out (web_view);
        action = g_action_map_lookup_action (G_ACTION_MAP (window), "zoom-out");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);

        enabled = dh_web_view_can_reset_zoom (web_view);
        action = g_action_map_lookup_action (G_ACTION_MAP (window), "zoom-default");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);
}

static void
update_back_forward_actions_sensitivity (DhWindow *window)
{
        DhWebView *web_view;
        GAction *action;
        gboolean enabled;

        web_view = get_active_web_view (window);
        if (web_view == NULL)
                return;

        enabled = webkit_web_view_can_go_back (WEBKIT_WEB_VIEW (web_view));
        action = g_action_map_lookup_action (G_ACTION_MAP (window), "go-back");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);

        enabled = webkit_web_view_can_go_forward (WEBKIT_WEB_VIEW (web_view));
        action = g_action_map_lookup_action (G_ACTION_MAP (window), "go-forward");
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);
}

static void
new_tab_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);

        open_new_tab (window, NULL, TRUE);
}

static void
next_tab_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        gint current_page;
        gint n_pages;

        current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook));
        n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook));

        if (current_page < n_pages - 1)
                gtk_notebook_next_page (GTK_NOTEBOOK (priv->notebook));
        else
                /* Wrap around to the first tab. */
                gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), 0);
}

static void
prev_tab_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        gint current_page;

        current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook));

        if (current_page > 0)
                gtk_notebook_prev_page (GTK_NOTEBOOK (priv->notebook));
        else
                /* Wrap around to the last tab. */
                gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), -1);
}

static void
go_to_tab_cb (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        guint16 tab_num;

        tab_num = g_variant_get_uint16 (parameter);
        gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), tab_num);
}

static void
print_cb (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWebView *web_view;
        WebKitPrintOperation *print_operation;

        web_view = get_active_web_view (window);
        if (web_view == NULL)
                return;

        print_operation = webkit_print_operation_new (WEBKIT_WEB_VIEW (web_view));
        webkit_print_operation_run_dialog (print_operation, GTK_WINDOW (window));
        g_object_unref (print_operation);
}

static void
close_tab_cb (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        gint page_num;

        page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook));
        gtk_notebook_remove_page (GTK_NOTEBOOK (priv->notebook), page_num);
}

static void
copy_cb (GSimpleAction *action,
         GVariant      *parameter,
         gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        GtkWidget *widget;

        widget = gtk_window_get_focus (GTK_WINDOW (window));

        if (GTK_IS_EDITABLE (widget)) {
                gtk_editable_copy_clipboard (GTK_EDITABLE (widget));
        } else if (GTK_IS_TREE_VIEW (widget) &&
                   gtk_widget_is_ancestor (widget, GTK_WIDGET (priv->sidebar))) {
                DhLink *link;

                link = dh_sidebar_get_selected_link (priv->sidebar);
                if (link != NULL) {
                        GtkClipboard *clipboard;

                        clipboard = gtk_widget_get_clipboard (widget, GDK_SELECTION_CLIPBOARD);
                        gtk_clipboard_set_text (clipboard, dh_link_get_name (link), -1);
                        dh_link_unref (link);
                }
        } else {
                DhWebView *web_view;

                web_view = get_active_web_view (window);
                if (web_view == NULL)
                        return;

                webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (web_view),
                                                         WEBKIT_EDITING_COMMAND_COPY);
        }
}

static void
find_cb (GSimpleAction *action,
         GVariant      *parameter,
         gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWindowPrivate *priv = dh_window_get_instance_private (window);

        gtk_search_bar_set_search_mode (priv->search_bar, TRUE);
        gtk_widget_grab_focus (GTK_WIDGET (priv->search_entry));
}

static void
zoom_in_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWebView *web_view;

        web_view = get_active_web_view (window);
        if (web_view != NULL)
                dh_web_view_zoom_in (web_view);
}

static void
zoom_out_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWebView *web_view;

        web_view = get_active_web_view (window);
        if (web_view != NULL)
                dh_web_view_zoom_out (web_view);
}

static void
zoom_default_cb (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWebView *web_view;

        web_view = get_active_web_view (window);
        if (web_view != NULL)
                dh_web_view_reset_zoom (web_view);
}

static void
focus_search_cb (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWindowPrivate *priv = dh_window_get_instance_private (window);

        dh_sidebar_set_search_focus (priv->sidebar);
}

static void
go_back_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWebView *web_view;

        web_view = get_active_web_view (window);
        if (web_view != NULL)
                webkit_web_view_go_back (WEBKIT_WEB_VIEW (web_view));
}

static void
go_forward_cb (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
        DhWindow *window = DH_WINDOW (user_data);
        DhWebView *web_view;

        web_view = get_active_web_view (window);
        if (web_view != NULL)
                webkit_web_view_go_forward (WEBKIT_WEB_VIEW (web_view));
}

static void
shortcuts_window_cb (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
        DhWindow *app_window = DH_WINDOW (user_data);
        GtkShortcutsWindow *shortcuts_window;
        GtkContainer *section;
        GtkContainer *group;
        AmtkFactory *factory;

        shortcuts_window = amtk_shortcuts_window_new (GTK_WINDOW (app_window));

        section = amtk_shortcuts_section_new (NULL);
        g_object_set (section,
                      "max-height", 10,
                      NULL);

        factory = amtk_factory_new (NULL);
        amtk_factory_set_default_flags (factory, AMTK_FACTORY_IGNORE_GACTION);

        /* General group */
        group = amtk_shortcuts_group_new (_("General"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.focus-search"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.find"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "app.new-window"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.new-tab"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.show-sidebar"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.go-back"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.go-forward"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.print"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.close-tab"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "app.quit"));
        gtk_container_add (section, GTK_WIDGET (group));

        /* Zoom group */
        group = amtk_shortcuts_group_new (_("Zoom"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.zoom-in"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.zoom-out"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.zoom-default"));
        gtk_container_add (section, GTK_WIDGET (group));

        g_object_unref (factory);

        gtk_container_add (GTK_CONTAINER (shortcuts_window), GTK_WIDGET (section));
        gtk_widget_show_all (GTK_WIDGET (shortcuts_window));
}

static void
add_actions (DhWindow *window)
{
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        GPropertyAction *property_action;

        const GActionEntry win_entries[] = {
                /* Tabs */
                { "new-tab", new_tab_cb },
                { "next-tab", next_tab_cb },
                { "prev-tab", prev_tab_cb },
                { "go-to-tab", go_to_tab_cb, "q" },
                { "print", print_cb },
                { "close-tab", close_tab_cb },

                /* Edit */
                { "copy", copy_cb },
                { "find", find_cb },

                /* View */
                { "zoom-in", zoom_in_cb },
                { "zoom-out", zoom_out_cb },
                { "zoom-default", zoom_default_cb },
                { "focus-search", focus_search_cb },

                /* Go */
                { "go-back", go_back_cb },
                { "go-forward", go_forward_cb },

                /* Help */
                { "shortcuts-window", shortcuts_window_cb },
        };

        amtk_action_map_add_action_entries_check_dups (G_ACTION_MAP (window),
                                                       win_entries,
                                                       G_N_ELEMENTS (win_entries),
                                                       window);

        property_action = g_property_action_new ("show-sidebar",
                                                 priv->grid_sidebar,
                                                 "visible");
        g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (property_action));
        g_object_unref (property_action);

        property_action = g_property_action_new ("show-window-menu",
                                                 priv->window_menu_button,
                                                 "active");
        g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (property_action));
        g_object_unref (property_action);
}

static GMenuModel *
create_window_menu_simple (void)
{
        GMenu *menu;
        GMenu *section;
        AmtkFactory *factory;

        menu = g_menu_new ();
        factory = amtk_factory_new (NULL);

        section = g_menu_new ();
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.show-sidebar"));
        amtk_gmenu_append_section (menu, NULL, section);

        section = g_menu_new ();
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.print"));
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.find"));
        amtk_gmenu_append_section (menu, NULL, section);

        section = g_menu_new ();
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.zoom-in"));
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.zoom-out"));
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.zoom-default"));
        amtk_gmenu_append_section (menu, NULL, section);

        g_object_unref (factory);
        g_menu_freeze (menu);

        return G_MENU_MODEL (menu);
}

static GMenuModel *
create_window_menu_plus_app_menu (void)
{
        GMenu *menu;
        GMenu *section;
        AmtkFactory *factory;

        menu = g_menu_new ();
        factory = amtk_factory_new (NULL);

        section = g_menu_new ();
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "app.new-window"));
        amtk_gmenu_append_section (menu, NULL, section);

        section = g_menu_new ();
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.show-sidebar"));
        amtk_gmenu_append_section (menu, NULL, section);

        section = g_menu_new ();
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.print"));
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.find"));
        amtk_gmenu_append_section (menu, NULL, section);

        section = g_menu_new ();
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.zoom-in"));
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.zoom-out"));
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.zoom-default"));
        amtk_gmenu_append_section (menu, NULL, section);

        section = g_menu_new ();
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "app.preferences"));
        amtk_gmenu_append_section (menu, NULL, section);

        section = g_menu_new ();
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.shortcuts-window"));
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "app.help"));
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "app.about"));
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "app.quit"));
        amtk_gmenu_append_section (menu, NULL, section);

        g_object_unref (factory);
        g_menu_freeze (menu);

        return G_MENU_MODEL (menu);
}

static void
set_window_menu (DhWindow *window)
{
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        GtkApplication *app;
        GMenuModel *window_menu;

        app = GTK_APPLICATION (g_application_get_default ());
        if (gtk_application_prefers_app_menu (app))
                window_menu = create_window_menu_simple ();
        else
                window_menu = create_window_menu_plus_app_menu ();

        gtk_menu_button_set_menu_model (priv->window_menu_button, window_menu);
        g_object_unref (window_menu);
}

static void
sidebar_link_selected_cb (DhSidebar *sidebar,
                          DhLink    *link,
                          DhWindow  *window)
{
        gchar *uri;
        DhWebView *web_view;

        uri = dh_link_get_uri (link);
        if (uri == NULL)
                return;

        web_view = get_active_web_view (window);
        if (web_view != NULL)
                webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), uri);

        g_free (uri);
}

static void
sync_active_web_view_uri_to_sidebar (DhWindow *window)
{
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        DhWebView *web_view;
        const gchar *uri = NULL;

        g_signal_handlers_block_by_func (priv->sidebar,
                                         sidebar_link_selected_cb,
                                         window);

        web_view = get_active_web_view (window);
        if (web_view != NULL)
                uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW (web_view));
        if (uri != NULL)
                dh_sidebar_select_uri (priv->sidebar, uri);

        g_signal_handlers_unblock_by_func (priv->sidebar,
                                           sidebar_link_selected_cb,
                                           window);
}

static void
update_search_in_web_view (DhWindow  *window,
                           DhWebView *view)
{
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        const gchar *search_text = NULL;

        if (gtk_search_bar_get_search_mode (priv->search_bar))
                search_text = gtk_entry_get_text (GTK_ENTRY (priv->search_entry));

        dh_web_view_set_search_text (view, search_text);
}

static void
update_search_in_active_web_view (DhWindow *window)
{
        DhWebView *web_view;

        web_view = get_active_web_view (window);
        if (web_view != NULL)
                update_search_in_web_view (window, web_view);
}

static void
update_search_in_all_web_views (DhWindow *window)
{
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        GList *web_views;
        GList *l;

        web_views = dh_notebook_get_all_web_views (priv->notebook);

        for (l = web_views; l != NULL; l = l->next) {
                DhWebView *web_view = DH_WEB_VIEW (l->data);
                update_search_in_web_view (window, web_view);
        }

        g_list_free (web_views);
}

static void
search_previous_in_active_web_view (DhWindow *window)
{
        DhWebView *web_view;

        web_view = get_active_web_view (window);
        if (web_view == NULL)
                return;

        update_search_in_web_view (window, web_view);
        dh_web_view_search_previous (web_view);
}

static void
search_next_in_active_web_view (DhWindow *window)
{
        DhWebView *web_view;

        web_view = get_active_web_view (window);
        if (web_view == NULL)
                return;

        update_search_in_web_view (window, web_view);
        dh_web_view_search_next (web_view);
}

static void
search_mode_enabled_notify_cb (GtkSearchBar *search_bar,
                               GParamSpec   *pspec,
                               DhWindow     *window)
{
        if (gtk_search_bar_get_search_mode (search_bar))
                update_search_in_active_web_view (window);
        else
                update_search_in_all_web_views (window);
}

static void
search_changed_cb (GtkEntry *entry,
                   DhWindow *window)
{
        /* Note that this callback is called after a small delay. */
        update_search_in_active_web_view (window);
}

static void
search_previous_match_cb (GtkSearchEntry *entry,
                          DhWindow       *window)
{
        search_previous_in_active_web_view (window);
}

static void
search_next_match_cb (GtkSearchEntry *entry,
                      DhWindow       *window)
{
        search_next_in_active_web_view (window);
}

static void
search_prev_button_clicked_cb (GtkButton *search_prev_button,
                               DhWindow  *window)
{
        search_previous_in_active_web_view (window);
}

static void
search_next_button_clicked_cb (GtkButton *search_next_button,
                               DhWindow  *window)
{
        search_next_in_active_web_view (window);
}

static void
notebook_page_removed_after_cb (GtkNotebook *notebook,
                                GtkWidget   *child,
                                guint        page_num,
                                DhWindow    *window)
{
        if (gtk_notebook_get_n_pages (notebook) == 0)
                gtk_window_close (GTK_WINDOW (window));
}

static void
notebook_switch_page_after_cb (GtkNotebook *notebook,
                               GtkWidget   *new_page,
                               guint        new_page_num,
                               DhWindow    *window)
{
        update_window_title (window);
        update_zoom_actions_sensitivity (window);
        update_back_forward_actions_sensitivity (window);
        update_search_in_active_web_view (window);
        sync_active_web_view_uri_to_sidebar (window);
}

static void
dh_window_init (DhWindow *window)
{
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        DhSettingsApp *settings;
        GSettings *paned_settings;

        gtk_widget_init_template (GTK_WIDGET (window));

        set_window_menu (window);

        settings = dh_settings_app_get_singleton ();
        paned_settings = dh_settings_app_peek_paned_settings (settings);
        g_settings_bind (paned_settings, "position",
                         priv->hpaned, "position",
                         G_SETTINGS_BIND_DEFAULT |
                         G_SETTINGS_BIND_NO_SENSITIVITY);

        /* Sidebar */
        priv->sidebar = dh_sidebar_new2 (NULL);
        gtk_widget_show (GTK_WIDGET (priv->sidebar));
        gtk_container_add (GTK_CONTAINER (priv->grid_sidebar),
                           GTK_WIDGET (priv->sidebar));

        g_signal_connect (priv->sidebar,
                          "link-selected",
                          G_CALLBACK (sidebar_link_selected_cb),
                          window);

        /* HTML tabs GtkNotebook */
        priv->notebook = dh_notebook_new ();
        gtk_widget_show (GTK_WIDGET (priv->notebook));
        gtk_container_add (GTK_CONTAINER (priv->grid_documents),
                           GTK_WIDGET (priv->notebook));

        g_signal_connect_after (priv->notebook,
                                "page-removed",
                                G_CALLBACK (notebook_page_removed_after_cb),
                                window);

        g_signal_connect_after (priv->notebook,
                                "switch-page",
                                G_CALLBACK (notebook_switch_page_after_cb),
                                window);

        /* Search bar above GtkNotebook */
        gtk_search_bar_connect_entry (priv->search_bar, GTK_ENTRY (priv->search_entry));

        g_signal_connect (priv->search_bar,
                          "notify::search-mode-enabled",
                          G_CALLBACK (search_mode_enabled_notify_cb),
                          window);

        g_signal_connect (priv->search_entry,
                          "search-changed",
                          G_CALLBACK (search_changed_cb),
                          window);

        g_signal_connect (priv->search_entry,
                          "previous-match",
                          G_CALLBACK (search_previous_match_cb),
                          window);

        g_signal_connect (priv->search_entry,
                          "next-match",
                          G_CALLBACK (search_next_match_cb),
                          window);

        g_signal_connect (priv->search_prev_button,
                          "clicked",
                          G_CALLBACK (search_prev_button_clicked_cb),
                          window);

        g_signal_connect (priv->search_next_button,
                          "clicked",
                          G_CALLBACK (search_next_button_clicked_cb),
                          window);

        add_actions (window);

        open_new_tab (window, NULL, TRUE);

        /* Focus search in sidebar by default. */
        dh_sidebar_set_search_focus (priv->sidebar);
}

static void
web_view_title_notify_cb (DhWebView  *web_view,
                          GParamSpec *param_spec,
                          DhWindow   *window)
{
        if (web_view == get_active_web_view (window))
                update_window_title (window);
}

static void
web_view_zoom_level_notify_cb (DhWebView  *web_view,
                               GParamSpec *pspec,
                               DhWindow   *window)
{
        if (web_view == get_active_web_view (window))
                update_zoom_actions_sensitivity (window);
}

static gchar *
find_equivalent_local_uri (const gchar *uri)
{
        gchar **components;
        guint n_components;
        const gchar *book_id;
        const gchar *relative_url;
        DhBookList *book_list;
        GList *books;
        GList *book_node;
        gchar *local_uri = NULL;

        g_return_val_if_fail (uri != NULL, NULL);

        components = g_strsplit (uri, "/", 0);
        n_components = g_strv_length (components);

        if ((g_str_has_prefix (uri, "http://library.gnome.org/devel/") ||
             g_str_has_prefix (uri, "https://library.gnome.org/devel/")) &&
            n_components >= 7) {
                book_id = components[4];
                relative_url = components[6];
        } else if ((g_str_has_prefix (uri, "http://developer.gnome.org/") ||
                    g_str_has_prefix (uri, "https://developer.gnome.org/")) &&
                   n_components >= 6) {
                /* E.g. http://developer.gnome.org/gio/stable/ch02.html */
                book_id = components[3];
                relative_url = components[5];
        } else {
                goto out;
        }

        book_list = dh_book_list_get_default ();
        books = dh_book_list_get_books (book_list);

        for (book_node = books; book_node != NULL; book_node = book_node->next) {
                DhBook *cur_book = DH_BOOK (book_node->data);
                GList *links;
                GList *link_node;

                if (g_strcmp0 (dh_book_get_id (cur_book), book_id) != 0)
                        continue;

                links = dh_book_get_links (cur_book);

                for (link_node = links; link_node != NULL; link_node = link_node->next) {
                        DhLink *cur_link = link_node->data;

                        if (dh_link_match_relative_url (cur_link, relative_url)) {
                                local_uri = dh_link_get_uri (cur_link);
                                goto out;
                        }
                }
        }

out:
        g_strfreev (components);
        return local_uri;
}

static gboolean
web_view_decide_policy_cb (WebKitWebView            *web_view,
                           WebKitPolicyDecision     *policy_decision,
                           WebKitPolicyDecisionType  type,
                           DhWindow                 *window)
{
        const gchar *uri;
        WebKitNavigationPolicyDecision *navigation_decision;
        WebKitNavigationAction *navigation_action;
        gchar *local_uri;
        gint button;
        gint state;

        if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
                return GDK_EVENT_PROPAGATE;

        navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (policy_decision);
        navigation_action = webkit_navigation_policy_decision_get_navigation_action (navigation_decision);
        uri = webkit_uri_request_get_uri (webkit_navigation_action_get_request (navigation_action));
        g_return_val_if_fail (uri != NULL, GDK_EVENT_PROPAGATE);

        /* middle click or ctrl-click -> new tab */
        button = webkit_navigation_action_get_mouse_button (navigation_action);
        state = webkit_navigation_action_get_modifiers (navigation_action);
        if (button == 2 || (button == 1 && state == GDK_CONTROL_MASK)) {
                webkit_policy_decision_ignore (policy_decision);
                open_new_tab (window, uri, FALSE);
                return GDK_EVENT_STOP;
        }

        if (g_str_equal (uri, "about:blank"))
                return GDK_EVENT_PROPAGATE;

        local_uri = find_equivalent_local_uri (uri);
        if (local_uri != NULL) {
                webkit_policy_decision_ignore (policy_decision);
                webkit_web_view_load_uri (web_view, local_uri);
                g_free (local_uri);
                return GDK_EVENT_STOP;
        }

        if (!g_str_has_prefix (uri, "file://")) {
                GError *error = NULL;

                webkit_policy_decision_ignore (policy_decision);
                gtk_show_uri_on_window (GTK_WINDOW (window), uri, GDK_CURRENT_TIME, &error);

                if (error != NULL) {
                        g_warning ("Error when opening URI “%s” externally: %s",
                                   uri,
                                   error->message);
                        g_clear_error (&error);
                }

                return GDK_EVENT_STOP;
        }

        return GDK_EVENT_PROPAGATE;
}

static void
web_view_load_changed_cb (DhWebView       *web_view,
                          WebKitLoadEvent  load_event,
                          DhWindow        *window)
{
        if (load_event == WEBKIT_LOAD_COMMITTED &&
            web_view == get_active_web_view (window)) {
                sync_active_web_view_uri_to_sidebar (window);
        }
}

static void
open_new_tab (DhWindow    *window,
              const gchar *location,
              gboolean     switch_focus)
{
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        DhTab *tab;
        DhWebView *web_view;
        GtkWidget *label;
        gint page_num;
        WebKitBackForwardList *back_forward_list;

        tab = dh_tab_new ();
        gtk_widget_show (GTK_WIDGET (tab));

        web_view = dh_tab_get_web_view (tab);

        g_signal_connect (web_view,
                          "notify::title",
                          G_CALLBACK (web_view_title_notify_cb),
                          window);

        g_signal_connect (web_view,
                          "notify::zoom-level",
                          G_CALLBACK (web_view_zoom_level_notify_cb),
                          window);

        g_signal_connect (web_view,
                          "decide-policy",
                          G_CALLBACK (web_view_decide_policy_cb),
                          window);

        g_signal_connect (web_view,
                          "load-changed",
                          G_CALLBACK (web_view_load_changed_cb),
                          window);

        back_forward_list = webkit_web_view_get_back_forward_list (WEBKIT_WEB_VIEW (web_view));
        g_signal_connect_object (back_forward_list,
                                 "changed",
                                 G_CALLBACK (update_back_forward_actions_sensitivity),
                                 window,
                                 G_CONNECT_AFTER | G_CONNECT_SWAPPED);

        label = dh_tab_label_new (tab);
        gtk_widget_show (label);

        page_num = gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                                             GTK_WIDGET (tab),
                                             label);

        gtk_container_child_set (GTK_CONTAINER (priv->notebook),
                                 GTK_WIDGET (tab),
                                 "tab-expand", TRUE,
                                 "reorderable", TRUE,
                                 NULL);

        if (location != NULL)
                webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), location);
        else
                webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), "about:blank");

        if (switch_focus)
                gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), page_num);
}

GtkWidget *
dh_window_new (GtkApplication *application)
{
        DhWindow *window;
        DhSettingsApp *settings;

        g_return_val_if_fail (GTK_IS_APPLICATION (application), NULL);

        window = g_object_new (DH_TYPE_WINDOW,
                               "application", application,
                               NULL);

        settings = dh_settings_app_get_singleton ();
        gtk_widget_realize (GTK_WIDGET (window));
        dh_util_window_settings_restore (GTK_WINDOW (window),
                                         dh_settings_app_peek_window_settings (settings));

        return GTK_WIDGET (window);
}

void
dh_window_search (DhWindow    *window,
                  const gchar *str)
{
        DhWindowPrivate *priv;

        g_return_if_fail (DH_IS_WINDOW (window));

        priv = dh_window_get_instance_private (window);

        dh_sidebar_set_search_string (priv->sidebar, str);
}

/* Only call this with a URI that is known to be in the docs. */
void
_dh_window_display_uri (DhWindow    *window,
                        const gchar *uri)
{
        DhWebView *web_view;

        g_return_if_fail (DH_IS_WINDOW (window));
        g_return_if_fail (uri != NULL);

        web_view = get_active_web_view (window);
        if (web_view == NULL)
                return;

        webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), uri);
}
