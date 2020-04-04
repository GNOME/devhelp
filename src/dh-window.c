/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2001-2008 Imendio AB
 * SPDX-FileCopyrightText: 2012 Aleksander Morgado <aleksander@gnu.org>
 * SPDX-FileCopyrightText: 2012 Thomas Bechtold <toabctl@gnome.org>
 * SPDX-FileCopyrightText: 2015-2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dh-window.h"
#include <glib/gi18n.h>
#include <webkit2/webkit2.h>
#include <devhelp/devhelp.h>
#include <amtk/amtk.h>
#include "dh-settings-app.h"
#include "dh-util-app.h"

typedef struct {
        GtkHeaderBar *header_bar;
        GtkMenuButton *menu_button;

        DhSidebar *sidebar;
        DhSearchBar *search_bar;
        DhNotebook *notebook;
} DhWindowPrivate;

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
dh_window_dispose (GObject *object)
{
        DhWindowPrivate *priv = dh_window_get_instance_private (DH_WINDOW (object));

        priv->header_bar = NULL;
        priv->menu_button = NULL;
        priv->sidebar = NULL;
        priv->search_bar = NULL;
        priv->notebook = NULL;

        G_OBJECT_CLASS (dh_window_parent_class)->dispose (object);
}

static void
dh_window_class_init (DhWindowClass *klass)
{
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        widget_class->delete_event = dh_window_delete_event;

        object_class->dispose = dh_window_dispose;
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
        DhWindowPrivate *priv = dh_window_get_instance_private (window);

        dh_notebook_open_new_tab (priv->notebook, NULL, TRUE);
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

        gtk_search_bar_set_search_mode (GTK_SEARCH_BAR (priv->search_bar), TRUE);
        dh_search_bar_grab_focus_to_search_entry (priv->search_bar);
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
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "app.help"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.show-window-menu"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.shortcuts-window"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.focus-search"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.find"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "app.new-window"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.new-tab"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.prev-tab"));
        gtk_container_add (group, amtk_factory_create_shortcut (factory, "win.next-tab"));
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
                                                 priv->sidebar,
                                                 "visible");
        g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (property_action));
        g_object_unref (property_action);

        property_action = g_property_action_new ("show-window-menu",
                                                 priv->menu_button,
                                                 "active");
        g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (property_action));
        g_object_unref (property_action);
}

static GMenuModel *
create_menu (void)
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
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "win.shortcuts-window"));
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "app.help"));
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "app.about"));
        /* Keep the Quit menu item. The GNOME goal recommends to remove it:
         * https://gitlab.gnome.org/GNOME/Initiatives/wikis/App-Menu-Retirement
         * “There is no need for the Quit menu item and the recommendation is to
         * remove it from all locations.”
         * In Devhelp, there *is* a need for the Quit menu item: after
         * installing/uninstalling books on the filesystem, it may be necessary
         * to restart Devhelp because the file monitoring is not perfect, see
         * the class description of DhBookListDirectory. Instead of closing the
         * Devhelp windows one by one, it's simpler to quit the whole
         * application at once. But this can be fixed by adding a “reload books”
         * action.
         * Another use-case with the Quit menu item is when the app bugs, just
         * restarting the app will most probably fix the problem (and then the
         * user needs to avoid repeating the actions that make the app to bug).
         */
        amtk_gmenu_append_item (section, amtk_factory_create_gmenu_item (factory, "app.quit"));
        amtk_gmenu_append_section (menu, NULL, section);

        g_object_unref (factory);
        g_menu_freeze (menu);

        return G_MENU_MODEL (menu);
}

static void
init_header_bar (DhWindow *window)
{
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        GtkWidget *back_forward_hgrid;
        GtkStyleContext *style_context;
        GtkWidget *back_button;
        GtkWidget *forward_button;
        GMenuModel *menu;
        GtkWidget *new_tab_button;

        g_assert (priv->header_bar == NULL);
        g_assert (priv->menu_button == NULL);

        priv->header_bar = GTK_HEADER_BAR (gtk_header_bar_new ());
        gtk_header_bar_set_show_close_button (priv->header_bar, TRUE);

        /* Back/forward buttons */
        back_forward_hgrid = gtk_grid_new ();
        style_context = gtk_widget_get_style_context (back_forward_hgrid);
        gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_LINKED);

        back_button = gtk_button_new_from_icon_name ("go-previous-symbolic", GTK_ICON_SIZE_BUTTON);
        gtk_actionable_set_action_name (GTK_ACTIONABLE (back_button), "win.go-back");
        gtk_widget_set_tooltip_text (back_button, _("Back"));

        forward_button = gtk_button_new_from_icon_name ("go-next-symbolic", GTK_ICON_SIZE_BUTTON);
        gtk_actionable_set_action_name (GTK_ACTIONABLE (forward_button), "win.go-forward");
        gtk_widget_set_tooltip_text (forward_button, _("Forward"));

        gtk_container_add (GTK_CONTAINER (back_forward_hgrid), back_button);
        gtk_container_add (GTK_CONTAINER (back_forward_hgrid), forward_button);
        gtk_header_bar_pack_start (priv->header_bar, back_forward_hgrid);

        /* Menu */
        priv->menu_button = GTK_MENU_BUTTON (gtk_menu_button_new ());
        gtk_menu_button_set_direction (priv->menu_button, GTK_ARROW_NONE);
        gtk_header_bar_pack_end (priv->header_bar, GTK_WIDGET (priv->menu_button));

        menu = create_menu ();
        gtk_menu_button_set_menu_model (priv->menu_button, menu);
        g_object_unref (menu);

        /* New tab button */
        new_tab_button = gtk_button_new_from_icon_name ("tab-new-symbolic", GTK_ICON_SIZE_BUTTON);
        gtk_actionable_set_action_name (GTK_ACTIONABLE (new_tab_button), "win.new-tab");
        gtk_widget_set_tooltip_text (new_tab_button, _("New Tab"));
        gtk_header_bar_pack_end (priv->header_bar, new_tab_button);

        gtk_widget_show_all (GTK_WIDGET (priv->header_bar));
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

static void
notebook_page_added_after_cb (GtkNotebook *notebook,
                              GtkWidget   *child,
                              guint        page_num,
                              DhWindow    *window)
{
        DhTab *tab;
        DhWebView *web_view;
        WebKitBackForwardList *back_forward_list;

        g_return_if_fail (DH_IS_TAB (child));

        tab = DH_TAB (child);
        web_view = dh_tab_get_web_view (tab);

        g_signal_connect (web_view,
                          "notify::title",
                          G_CALLBACK (web_view_title_notify_cb),
                          window);

        g_signal_connect (web_view,
                          "notify::zoom-level",
                          G_CALLBACK (web_view_zoom_level_notify_cb),
                          window);

        back_forward_list = webkit_web_view_get_back_forward_list (WEBKIT_WEB_VIEW (web_view));
        g_signal_connect_object (back_forward_list,
                                 "changed",
                                 G_CALLBACK (update_back_forward_actions_sensitivity),
                                 window,
                                 G_CONNECT_AFTER | G_CONNECT_SWAPPED);
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
}

static void
dh_window_init (DhWindow *window)
{
        DhWindowPrivate *priv = dh_window_get_instance_private (window);
        GtkPaned *hpaned;
        DhSettingsApp *settings;
        GSettings *paned_settings;
        GtkWidget *contents_vgrid;

        /* Header bar */
        init_header_bar (window);
        gtk_window_set_titlebar (GTK_WINDOW (window), GTK_WIDGET (priv->header_bar));

        /* Horizontal paned */
        hpaned = GTK_PANED (gtk_paned_new (GTK_ORIENTATION_HORIZONTAL));

        settings = dh_settings_app_get_singleton ();
        paned_settings = dh_settings_app_peek_paned_settings (settings);
        g_settings_bind (paned_settings, "position",
                         hpaned, "position",
                         G_SETTINGS_BIND_DEFAULT |
                         G_SETTINGS_BIND_NO_SENSITIVITY);

        /* Left side of hpaned */
        priv->sidebar = dh_sidebar_new2 (NULL);

        /* Right side of hpaned */
        contents_vgrid = gtk_grid_new ();
        gtk_orientable_set_orientation (GTK_ORIENTABLE (contents_vgrid), GTK_ORIENTATION_VERTICAL);

        // DhNotebook
        priv->notebook = dh_notebook_new (NULL);

        dh_application_window_bind_sidebar_and_notebook (priv->sidebar, priv->notebook);

        g_signal_connect_after (priv->notebook,
                                "page-added",
                                G_CALLBACK (notebook_page_added_after_cb),
                                window);

        g_signal_connect_after (priv->notebook,
                                "page-removed",
                                G_CALLBACK (notebook_page_removed_after_cb),
                                window);

        g_signal_connect_after (priv->notebook,
                                "switch-page",
                                G_CALLBACK (notebook_switch_page_after_cb),
                                window);

        // DhSearchBar
        priv->search_bar = dh_search_bar_new (priv->notebook);

        /* Packing */
        gtk_container_add (GTK_CONTAINER (contents_vgrid),
                           GTK_WIDGET (priv->search_bar));
        gtk_container_add (GTK_CONTAINER (contents_vgrid),
                           GTK_WIDGET (priv->notebook));
        gtk_paned_pack1 (hpaned, GTK_WIDGET (priv->sidebar), FALSE, FALSE);
        gtk_paned_add2 (hpaned, contents_vgrid);
        gtk_widget_show_all (GTK_WIDGET (hpaned));
        gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (hpaned));

        add_actions (window);

        dh_notebook_open_new_tab (priv->notebook, NULL, TRUE);

        /* Focus search in sidebar by default. */
        dh_sidebar_set_search_focus (priv->sidebar);
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
