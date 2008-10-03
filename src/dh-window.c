/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <webkit/webkit.h>

#include "dh-book-tree.h"
#include "dh-preferences.h"
#include "dh-search.h"
#include "dh-window.h"
#include "eggfindbar.h"

#ifdef GDK_WINDOWING_QUARTZ
#include <ige-mac-integration.h>
#endif

struct _DhWindowPriv {
        DhBase         *base;

        GConfClient    *gconf_client;

        GtkWidget      *main_box;
        GtkWidget      *menu_box;
        GtkWidget      *hpaned;
        GtkWidget      *control_notebook;
        GtkWidget      *book_tree;
        GtkWidget      *search;
        GtkWidget      *web_view_notebook;

        GtkWidget      *vbox;
        GtkWidget      *findbar;

        gint            zoom_level;

        GtkUIManager   *manager;
        GtkActionGroup *action_group;
};

static guint tab_accel_keys[] = {
        GDK_1, GDK_2, GDK_3, GDK_4, GDK_5,
        GDK_6, GDK_7, GDK_8, GDK_9, GDK_0
};

static const
struct
{
        gchar *name;
        float level;
}
zoom_levels[] =
{
        { N_("50%"), 0.7071067811 },
        { N_("75%"), 0.8408964152 },
        { N_("100%"), 1.0 },
        { N_("125%"), 1.1892071149 },
        { N_("150%"), 1.4142135623 },
        { N_("175%"), 1.6817928304 },
        { N_("200%"), 2.0 },
        { N_("300%"), 2.8284271247 },
        { N_("400%"), 4.0 }
};

#define ZOOM_MINIMAL    (zoom_levels[0].level)
#define ZOOM_MAXIMAL    (zoom_levels[8].level)
#define ZOOM_DEFAULT    (zoom_levels[2].level)

/* People have reported problems with the default values in GConf so I'm
 * adding this to make sure that the window isn't started 1x1 pixels or the
 * paned having size 0
 */
#define DEFAULT_WIDTH     700
#define DEFAULT_HEIGHT    500
#define DEFAULT_PANED_LOC 250

static void       dh_window_class_init            (DhWindowClass   *klass);
static void       dh_window_init                  (DhWindow        *window);
static void       window_populate                 (DhWindow        *window);
static void       window_activate_new_window      (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_new_tab         (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_print           (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_close           (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_quit            (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_copy            (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_find            (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_preferences     (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_back            (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_forward         (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_show_contents   (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_show_search     (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_about           (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_zoom_default    (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_zoom_in         (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_activate_zoom_out        (GtkAction       *action,
                                                   DhWindow        *window);
static void       window_save_state               (DhWindow        *window);
static void       window_restore_state            (DhWindow        *window);
static gboolean   window_delete_cb                (GtkWidget       *widget,
                                                   GdkEventAny     *event,
                                                   gpointer         user_data);
static void       window_tree_link_selected_cb    (GObject         *ignored,
                                                   DhLink          *link,
                                                   DhWindow        *window);
static void       window_search_link_selected_cb  (GObject         *ignored,
                                                   DhLink          *link,
                                                   DhWindow        *window);
static void       window_manager_add_widget       (GtkUIManager    *manager,
                                                   GtkWidget       *widget,
                                                   DhWindow        *window);
static void       window_check_history            (DhWindow        *window,
                                                   WebKitWebView          *web_view);
#if 0
static void       window_web_view_location_changed_cb (WebKitWebView          *web_view,
                                                   const gchar     *location,
                                                   DhWindow        *window);
#endif
static void       window_web_view_title_changed_cb    (WebKitWebView          *web_view,
                                                   WebKitWebFrame  *web_frame,
                                                   const gchar     *location,
                                                   DhWindow        *window);
static gboolean   window_web_view_open_uri_cb         (WebKitWebView          *web_view,
                                                   const gchar     *uri,
                                                   DhWindow        *window);
#if 0
static void       window_web_view_open_new_tab_cb     (WebKitWebView          *web_view,
                                                   const gchar     *location,
                                                   DhWindow        *window);
#endif
static void       window_web_view_tab_accel_cb        (GtkAccelGroup   *accel_group,
                                                   GObject         *object,
                                                   guint            key,
                                                   GdkModifierType  mod,
                                                   DhWindow        *window);
static void       window_findbar_search_changed_cb(GObject         *object,
                                                   GParamSpec      *arg1,
                                                   DhWindow        *window);
static void       window_findbar_case_sensitive_changed_cb (GObject         *object,
                                                            GParamSpec      *arg1,
                                                            DhWindow        *window);
static void       window_find_previous_cb         (GtkEntry        *entry,
                                                   DhWindow        *window);
static void       window_find_next_cb             (GtkEntry        *entry,
                                                   DhWindow        *window);
static void       window_findbar_close_cb         (GtkWidget       *widget,
                                                   DhWindow        *window);
static GtkWidget *window_new_tab_label            (DhWindow        *window,
                                                   const gchar     *label);
static void       window_open_new_tab             (DhWindow        *window,
                                                   const gchar     *location);
static WebKitWebView *   window_get_active_web_view          (DhWindow        *window);
static void       window_update_title             (DhWindow        *window,
                                                   WebKitWebView          *web_view,
                           const gchar            *title);
static void       window_tab_set_title            (DhWindow        *window,
                                                   WebKitWebView          *web_view,
                                                   const gchar     *title);

G_DEFINE_TYPE (DhWindow, dh_window, GTK_TYPE_WINDOW);

#define GET_PRIVATE(instance) G_TYPE_INSTANCE_GET_PRIVATE \
  (instance, DH_TYPE_WINDOW, DhWindowPriv);

static const GtkActionEntry actions[] = {
        { "FileMenu", NULL, N_("_File") },
        { "EditMenu", NULL, N_("_Edit") },
        { "ViewMenu", NULL, N_("_View") },
        { "GoMenu",   NULL, N_("_Go") },
        { "HelpMenu", NULL, N_("_Help") },

        /* File menu */
        { "NewWindow", GTK_STOCK_NEW, N_("_New Window"), "<control>N", NULL,
          G_CALLBACK (window_activate_new_window) },
        { "NewTab", GTK_STOCK_NEW, N_("New _Tab"), "<control>T", NULL,
          G_CALLBACK (window_activate_new_tab) },
        { "Print", GTK_STOCK_PRINT, N_("_Print..."), "<control>P", NULL,
          G_CALLBACK (window_activate_print) },
        { "Close", GTK_STOCK_CLOSE, NULL, NULL, NULL,
          G_CALLBACK (window_activate_close) },
        { "Quit", GTK_STOCK_QUIT, NULL, NULL, NULL,
          G_CALLBACK (window_activate_quit) },

        /* Edit menu */
        { "Copy", GTK_STOCK_COPY, NULL, "<control>C", NULL,
          G_CALLBACK (window_activate_copy) },
        { "Find", GTK_STOCK_FIND, NULL, "<control>F", NULL,
          G_CALLBACK (window_activate_find) },
        { "Find Next", GTK_STOCK_GO_FORWARD, N_("Find Next"), "<control>G", NULL,
          G_CALLBACK (window_find_next_cb) },
        { "Find Previous", GTK_STOCK_GO_BACK, N_("Find Previous"), "<shift><control>G", NULL,
          G_CALLBACK (window_find_previous_cb) },
        { "Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL,
          G_CALLBACK (window_activate_preferences) },

        /* Go menu */
        { "Back", GTK_STOCK_GO_BACK, NULL, "<alt>Left",
          N_("Go to the previous page"),
          G_CALLBACK (window_activate_back) },
        { "Forward", GTK_STOCK_GO_FORWARD, NULL, "<alt>Right",
          N_("Go to the next page"),
          G_CALLBACK (window_activate_forward) },

        { "ShowContentsTab", NULL, N_("_Contents Tab"), "<ctrl>B", NULL,
          G_CALLBACK (window_activate_show_contents) },

        { "ShowSearchTab", NULL, N_("_Search Tab"), "<ctrl>S", NULL,
          G_CALLBACK (window_activate_show_search) },

        /* View menu */
        { "ZoomIn", GTK_STOCK_ZOOM_IN, N_("_Larger Text"), "<ctrl>plus",
          N_("Increase the text size"),
          G_CALLBACK (window_activate_zoom_in) },
        { "ZoomOut", GTK_STOCK_ZOOM_OUT, N_("S_maller Text"), "<ctrl>minus",
          N_("Decrease the text size"),
          G_CALLBACK (window_activate_zoom_out) },
        { "ZoomDefault", GTK_STOCK_ZOOM_100, N_("_Normal size"), "<ctrl>0",
          N_("Use the normal text size"),
          G_CALLBACK (window_activate_zoom_default) },

        /* About menu */
        { "About", GTK_STOCK_ABOUT, NULL, NULL, NULL,
          G_CALLBACK (window_activate_about) },
};

static void
window_finalize (GObject *object)
{
        DhWindowPriv *priv = GET_PRIVATE (object);

        g_object_unref (priv->base);

        G_OBJECT_CLASS (dh_window_parent_class)->finalize (object);
}

static void
dh_window_class_init (DhWindowClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = window_finalize;

        gtk_rc_parse_string ("style \"devhelp-tab-close-button-style\"\n"
                             "{\n"
                             "GtkWidget::focus-padding = 0\n"
                             "GtkWidget::focus-line-width = 0\n"
                             "xthickness = 0\n"
                             "ythickness = 0\n"
                             "}\n"
                             "widget \"*.devhelp-tab-close-button\" style \"devhelp-tab-close-button-style\"");

        g_type_class_add_private (klass, sizeof (DhWindowPriv));
}

static void
dh_window_init (DhWindow *window)
{
        DhWindowPriv  *priv;
        GtkAction     *action;
        GtkAccelGroup *accel_group;
        GClosure      *closure;
        gint           i;

        priv = GET_PRIVATE (window);
        window->priv = priv;

        priv->manager = gtk_ui_manager_new ();

        priv->gconf_client = gconf_client_get_default ();

        accel_group = gtk_ui_manager_get_accel_group (priv->manager);
        gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

        priv->zoom_level = 2;

        priv->main_box = gtk_vbox_new (FALSE, 0);
        gtk_widget_show (priv->main_box);

        priv->menu_box = gtk_vbox_new (FALSE, 0);
        gtk_widget_show (priv->menu_box);
        gtk_container_set_border_width (GTK_CONTAINER (priv->menu_box), 0);
        gtk_box_pack_start (GTK_BOX (priv->main_box), priv->menu_box,
                            FALSE, TRUE, 0);

        gtk_container_add (GTK_CONTAINER (window), priv->main_box);

        g_signal_connect (priv->manager,
                          "add-widget",
                          G_CALLBACK (window_manager_add_widget),
                          window);

        priv->action_group = gtk_action_group_new ("MainWindow");

        gtk_action_group_set_translation_domain (priv->action_group,
                                                 GETTEXT_PACKAGE);

        gtk_action_group_add_actions (priv->action_group,
                                      actions,
                                      G_N_ELEMENTS (actions),
                                      window);

        gtk_ui_manager_insert_action_group (priv->manager,
                                            priv->action_group,
                                            0);

        action = gtk_action_group_get_action (priv->action_group,
                                              "Back");
        g_object_set (action, "sensitive", FALSE, NULL);

        action = gtk_action_group_get_action (priv->action_group,
                                              "Forward");
        g_object_set (action, "sensitive", FALSE, NULL);

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

/* The ugliest hack. When switching tabs, the selection and cursor is changed
 * for the tree view so the web_view content is changed. Block the signal during
 * switch.
 */
static void
window_control_switch_page_cb (GtkWidget       *notebook,
                               GtkNotebookPage *page,
                               guint            page_num,
                               DhWindow        *window)
{
        DhWindowPriv *priv;

        priv = window->priv;

        g_signal_handlers_block_by_func (priv->book_tree,
                                         window_tree_link_selected_cb, window);
}

static void
window_control_after_switch_page_cb (GtkWidget       *notebook,
                                     GtkNotebookPage *page,
                                     guint            page_num,
                                     DhWindow        *window)
{
        DhWindowPriv *priv;

        priv = window->priv;

        g_signal_handlers_unblock_by_func (priv->book_tree,
                                           window_tree_link_selected_cb, window);
}

static void
window_web_view_switch_page_cb (GtkNotebook     *notebook,
                                GtkNotebookPage *page,
                                guint            new_page_num,
                                DhWindow        *window)
{
        DhWindowPriv *priv;
        GtkWidget    *new_page;

        priv = window->priv;

        new_page = gtk_notebook_get_nth_page (notebook, new_page_num);
        if (new_page) {
                WebKitWebView  *new_web_view;
                const gchar    *title, *location;
                WebKitWebFrame *web_frame;

                new_web_view = g_object_get_data (G_OBJECT (new_page), "web_view");

                window_update_title (window, new_web_view, NULL);

                return;

                /* FIXME: WebKit: where did the return above come from?! */

                web_frame = webkit_web_view_get_main_frame (new_web_view);
                title = webkit_web_frame_get_title (web_frame);
                gtk_window_set_title (GTK_WINDOW (window), title);

                /* Sync the book tree. */
                location = webkit_web_frame_get_uri (web_frame);
                if (location) {
                        dh_book_tree_select_uri (DH_BOOK_TREE (priv->book_tree),
                                                 location);
                }
                window_check_history (window, new_web_view);

        } else {
                gtk_window_set_title (GTK_WINDOW (window), "Devhelp");
                window_check_history (window, NULL);
        }
}

static void
window_populate (DhWindow *window)
{
        DhWindowPriv *priv;
        GtkWidget    *book_tree_sw;
        GNode        *contents_tree;
        GList        *keywords;
        gint          hpaned_position;

        priv = window->priv;

        gtk_ui_manager_add_ui_from_file (priv->manager,
                                         DATADIR "/devhelp/ui/window.ui",
                                         NULL);
        gtk_ui_manager_ensure_update (priv->manager);

#ifdef GDK_WINDOWING_QUARTZ
        {
                GtkWidget       *widget;
                IgeMacMenuGroup *group;

                /* Hide toolbar labels. */
                widget = gtk_ui_manager_get_widget (priv->manager, "/Toolbar");
                gtk_toolbar_set_style (GTK_TOOLBAR (widget), GTK_TOOLBAR_ICONS);

                /* Setup menubar. */
                widget = gtk_ui_manager_get_widget (priv->manager, "/MenuBar");
                ige_mac_menu_set_menu_bar (GTK_MENU_SHELL (widget));
                gtk_widget_hide (widget);

                widget = gtk_ui_manager_get_widget (priv->manager, "/MenuBar/FileMenu/Quit");
                ige_mac_menu_set_quit_menu_item (GTK_MENU_ITEM (widget));

                group =  ige_mac_menu_add_app_menu_group ();
                widget = gtk_ui_manager_get_widget (priv->manager, "/MenuBar/HelpMenu/About");
                ige_mac_menu_add_app_menu_item (group, GTK_MENU_ITEM (widget),
                                                _("About Devhelp"));

                group =  ige_mac_menu_add_app_menu_group ();
                widget = gtk_ui_manager_get_widget (priv->manager, "/MenuBar/EditMenu/Preferences");
                ige_mac_menu_add_app_menu_item (group, GTK_MENU_ITEM (widget),
                                                _("Preferences..."));
        }
#endif

        priv->hpaned = gtk_hpaned_new ();

        gtk_box_pack_start (GTK_BOX (priv->main_box), priv->hpaned, TRUE, TRUE, 0);

        hpaned_position = gconf_client_get_int (priv->gconf_client,
                                                GCONF_PANED_LOCATION,
                                                NULL);

        /* This workaround for broken schema installs is not really working that
         * well, since it makes having a 0 location not possible.
         */
        if (hpaned_position <= 0) {
                hpaned_position = DEFAULT_PANED_LOC;
        }
        gtk_paned_set_position (GTK_PANED (priv->hpaned), hpaned_position);

        /* Search and contents notebook. */
        priv->control_notebook = gtk_notebook_new ();

        gtk_paned_add1 (GTK_PANED (priv->hpaned), priv->control_notebook);

        g_signal_connect (priv->control_notebook,
                          "switch-page",
                          G_CALLBACK (window_control_switch_page_cb),
                          window);

        g_signal_connect_after (priv->control_notebook,
                                "switch-page",
                                G_CALLBACK (window_control_after_switch_page_cb),
                                window);

        book_tree_sw = gtk_scrolled_window_new (NULL, NULL);

        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (book_tree_sw),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (book_tree_sw),
                                             GTK_SHADOW_IN);
        gtk_container_set_border_width (GTK_CONTAINER (book_tree_sw), 2);

        contents_tree = dh_base_get_book_tree (priv->base);
        keywords = dh_base_get_keywords (priv->base);

        priv->book_tree = dh_book_tree_new (contents_tree);

        gtk_container_add (GTK_CONTAINER (book_tree_sw),
                           priv->book_tree);

        gtk_notebook_append_page (GTK_NOTEBOOK (priv->control_notebook),
                                  book_tree_sw,
                                  gtk_label_new (_("Contents")));
        g_signal_connect (priv->book_tree,
                          "link-selected",
                          G_CALLBACK (window_tree_link_selected_cb),
                          window);

        priv->search = dh_search_new (keywords);

        gtk_notebook_append_page (GTK_NOTEBOOK (priv->control_notebook),
                                  priv->search,
                                  gtk_label_new (_("Search")));

        g_signal_connect (priv->search,
                          "link-selected",
                          G_CALLBACK (window_search_link_selected_cb),
                          window);

        priv->vbox = gtk_vbox_new (FALSE, 0);
        gtk_paned_add2 (GTK_PANED (priv->hpaned), priv->vbox);

        /* HTML tabs notebook. */
        priv->web_view_notebook = gtk_notebook_new ();
        gtk_box_pack_start (GTK_BOX (priv->vbox), priv->web_view_notebook, TRUE, TRUE, 0);

        g_signal_connect (priv->web_view_notebook,
                          "switch-page",
                          G_CALLBACK (window_web_view_switch_page_cb),
                          window);

        /* Create findbar. */
        priv->findbar = egg_find_bar_new ();
        gtk_widget_set_no_show_all (priv->findbar, TRUE);
        gtk_box_pack_start (GTK_BOX (priv->vbox), priv->findbar, FALSE, FALSE, 0);

        g_signal_connect (priv->findbar,
                          "notify::search-string",
                          G_CALLBACK(window_findbar_search_changed_cb),
                          window);
        g_signal_connect (priv->findbar,
                          "notify::case-sensitive",
                          G_CALLBACK (window_findbar_case_sensitive_changed_cb),
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

        dh_preferences_setup_fonts ();

        gtk_widget_show_all (priv->hpaned);

        window_open_new_tab (window, NULL);
}

static void
window_activate_new_window (GtkAction *action,
                            DhWindow  *window)
{
        DhWindowPriv *priv;
        GtkWidget    *new_window;

        priv = window->priv;

        new_window = dh_base_new_window (priv->base);
        gtk_widget_show (new_window);
}

static void
window_activate_new_tab (GtkAction *action,
                         DhWindow  *window)
{
        DhWindowPriv *priv;

        priv = window->priv;

        window_open_new_tab (window, NULL);
}

static void
window_activate_print (GtkAction *action,
                       DhWindow  *window)
{
    WebKitWebView *web_view;

    web_view = window_get_active_web_view (window);
    webkit_web_view_execute_script (web_view, "print();");
}

static void
window_activate_close (GtkAction *action,
                       DhWindow  *window)
{
        DhWindowPriv *priv;
        gint          page_num;

        priv = window->priv;

        page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->web_view_notebook));
        gtk_notebook_remove_page (GTK_NOTEBOOK (priv->web_view_notebook), page_num);

        if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->web_view_notebook)) == 0) {
                window_save_state (window);
                gtk_widget_destroy (GTK_WIDGET (window));
        }
}

static void
window_activate_quit (GtkAction *action,
                      DhWindow  *window)
{
        gtk_main_quit ();
}

static void
window_activate_copy (GtkAction *action,
                      DhWindow  *window)
{
        GtkWidget *widget;

        widget = gtk_window_get_focus (GTK_WINDOW (window));

        if (GTK_IS_EDITABLE (widget)) {
                gtk_editable_copy_clipboard (GTK_EDITABLE (widget));
        } else {
                WebKitWebView *web_view;

                web_view = window_get_active_web_view (window);
                webkit_web_view_copy_clipboard (web_view);
        }
}

static void
window_activate_find (GtkAction *action,
                      DhWindow  *window)
{
        DhWindowPriv  *priv;
        WebKitWebView *web_view;

        priv = window->priv;
        web_view = window_get_active_web_view (window);

        gtk_widget_show (priv->findbar);
        gtk_widget_grab_focus (priv->findbar);

        webkit_web_view_set_highlight_text_matches (web_view, TRUE);
}

static void
window_activate_zoom_in (GtkAction *action,
                         DhWindow  *window)
{
        DhWindowPriv  *priv;
        WebKitWebView *web_view;

        web_view = window_get_active_web_view (window);
        priv = window->priv;

        if (zoom_levels[priv->zoom_level].level < ZOOM_MAXIMAL) {
                priv->zoom_level++;
                g_object_set (web_view, "zoom-level", zoom_levels[priv->zoom_level].level, NULL);
        }
}

static void
window_activate_zoom_out (GtkAction *action,
                          DhWindow  *window)
{
        WebKitWebView *web_view;
        DhWindowPriv  *priv;

        web_view = window_get_active_web_view (window);
        priv = window->priv;

        if (zoom_levels[priv->zoom_level].level > ZOOM_MINIMAL) {
                priv->zoom_level--;
                g_object_set (web_view, "zoom-level", zoom_levels[priv->zoom_level].level, NULL);
        }
}

static void
window_activate_zoom_default (GtkAction *action,
                              DhWindow  *window)
{
        WebKitWebView *web_view;
        DhWindowPriv  *priv;

        web_view = window_get_active_web_view (window);
        priv = window->priv;

        g_object_set (web_view, "zoom-level", ZOOM_DEFAULT, NULL);
}

static void
window_activate_preferences (GtkAction *action,
                             DhWindow  *window)
{
        dh_preferences_show_dialog (GTK_WINDOW (window));
}

static void
window_activate_back (GtkAction *action,
                      DhWindow  *window)
{
        DhWindowPriv  *priv;
        WebKitWebView *web_view;
        GtkWidget     *frame;

        priv = window->priv;

        frame = gtk_notebook_get_nth_page (
                GTK_NOTEBOOK (priv->web_view_notebook),
                gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->web_view_notebook)));
        web_view = g_object_get_data (G_OBJECT (frame), "web_view");

        webkit_web_view_go_back (web_view);
}

static void
window_activate_forward (GtkAction *action,
                         DhWindow  *window)
{
        DhWindowPriv  *priv;
        WebKitWebView *web_view;
        GtkWidget     *frame;

        priv = window->priv;

        frame = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->web_view_notebook),
                                           gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->web_view_notebook))
                                          );
        web_view = g_object_get_data (G_OBJECT (frame), "web_view");

        webkit_web_view_go_forward (web_view);
}

static void
window_activate_show_contents (GtkAction *action,
                               DhWindow  *window)
{
        DhWindowPriv *priv;

        priv = window->priv;

        gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->control_notebook), 0);
        gtk_widget_grab_focus (priv->book_tree);
}

static void
window_activate_show_search (GtkAction *action,
                             DhWindow  *window)
{
        DhWindowPriv *priv;

        priv = window->priv;

        gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->control_notebook), 1);
        dh_search_grab_focus (DH_SEARCH (priv->search));
}

static void
window_activate_about (GtkAction *action,
                       DhWindow  *window)
{
        const gchar  *authors[] = {
                "Mikael Hallendal <micke@imendio.com>",
                "Richard Hult <richard@imendio.com>",
                "Johan Dahlin <johan@gnome.org>",
                "Ross Burton <ross@burtonini.com>",
                NULL
        };
        const gchar **documenters = NULL;
        const gchar  *translator_credits = _("translator_credits");

        gtk_show_about_dialog (GTK_WINDOW (window),
                               "name",_("Devhelp"),
                               "version", PACKAGE_VERSION,
                               "comments", _("A developer's help browser for GNOME 2"),
                               "authors", authors,
                               "documenters", documenters,
                               "translator-credits",
                               strcmp (translator_credits, "translator_credits") != 0 ?
                               translator_credits : NULL,
                               "website", "http://developer.imendio.com/wiki/Devhelp",
                               "logo-icon-name", "devhelp",
                               NULL);
}

static void
window_save_state (DhWindow *window)
{
        DhWindowPriv   *priv;
        GdkWindowState  state;
        gboolean        maximized;

        priv = window->priv;

        state = gdk_window_get_state (GTK_WIDGET (window)->window);
        if (state & GDK_WINDOW_STATE_MAXIMIZED) {
                maximized = TRUE;
        } else {
                maximized = FALSE;
        }

        gconf_client_set_bool (priv->gconf_client,
                               GCONF_MAIN_WINDOW_MAXIMIZED, maximized,
                               NULL);

        /* If maximized don't save the size and position */
        if (!maximized) {
                gint width, height;
                gint x, y;

                gtk_window_get_size (GTK_WINDOW (window), &width, &height);
                gconf_client_set_int (priv->gconf_client,
                                      GCONF_MAIN_WINDOW_WIDTH, width,
                                      NULL);
                gconf_client_set_int (priv->gconf_client,
                                      GCONF_MAIN_WINDOW_HEIGHT, height,
                                      NULL);

                gtk_window_get_position (GTK_WINDOW (window), &x, &y);
                gconf_client_set_int (priv->gconf_client,
                                      GCONF_MAIN_WINDOW_POS_X, x,
                                      NULL);
                gconf_client_set_int (priv->gconf_client,
                                      GCONF_MAIN_WINDOW_POS_Y, y,
                                      NULL);
        }

        gconf_client_set_int (priv->gconf_client,
                              GCONF_PANED_LOCATION,
                              gtk_paned_get_position (GTK_PANED (priv->hpaned)),
                              NULL);

        if (gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->control_notebook)) == 0) {
                gconf_client_set_string (priv->gconf_client,
                                         GCONF_SELECTED_TAB, "content",
                                         NULL);
        } else {
                gconf_client_set_string (priv->gconf_client,
                                         GCONF_SELECTED_TAB, "search",
                                         NULL);
        }
}

static void
window_restore_state (DhWindow *window)
{
        DhWindowPriv *priv;
        gboolean      maximized;
        int           width, height;
        int           x, y;
        gchar        *tab;

        priv = window->priv;

        width = gconf_client_get_int (priv->gconf_client,
                                      GCONF_MAIN_WINDOW_WIDTH,
                                      NULL);

        if (width <= 0) {
                width = DEFAULT_WIDTH;
        }

        height = gconf_client_get_int (priv->gconf_client,
                                       GCONF_MAIN_WINDOW_HEIGHT,
                                       NULL);

        if (height <= 0) {
                height = DEFAULT_HEIGHT;
        }

        gtk_window_set_default_size (GTK_WINDOW (window),
                                     width, height);

        x = gconf_client_get_int (priv->gconf_client,
                                  GCONF_MAIN_WINDOW_POS_X,
                                  NULL);
        y = gconf_client_get_int (priv->gconf_client,
                                  GCONF_MAIN_WINDOW_POS_Y,
                                  NULL);

        gtk_window_move (GTK_WINDOW (window), x, y);

        maximized = gconf_client_get_bool (priv->gconf_client,
                                           GCONF_MAIN_WINDOW_MAXIMIZED,
                                           NULL);
        if (maximized) {
                gtk_window_maximize (GTK_WINDOW (window));
        }

        tab = gconf_client_get_string (priv->gconf_client,
                                       GCONF_SELECTED_TAB,
                                       NULL);
        if (!tab || strcmp (tab, "") == 0 || strcmp (tab, "content") == 0) {
                gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->control_notebook), 0);
                gtk_widget_grab_focus (priv->book_tree);
        } else {
                gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->control_notebook), 1);
                dh_search_grab_focus (DH_SEARCH (priv->search));
        }
        g_free (tab);
}

static gboolean
window_delete_cb (GtkWidget   *widget,
                  GdkEventAny *event,
                  gpointer     user_data)
{
        window_save_state (DH_WINDOW (widget));

        return FALSE;
}

static void
window_tree_link_selected_cb (GObject  *ignored,
                              DhLink   *link,
                              DhWindow *window)
{
        DhWindowPriv  *priv;
        WebKitWebView *web_view;

        priv = window->priv;

        web_view = window_get_active_web_view (window);

        /* Block so we don't try to sync the tree when we have already clicked
         * in it.
         */
        g_signal_handlers_block_by_func (web_view,
                                         window_web_view_open_uri_cb,
                                         window);

        webkit_web_view_open (web_view, dh_link_get_uri (link));

        g_signal_handlers_unblock_by_func (web_view,
                                           window_web_view_open_uri_cb,
                                           window);

        window_check_history (window, web_view);
}

static void
window_search_link_selected_cb (GObject  *ignored,
                                DhLink   *link,
                                DhWindow *window)
{
        DhWindowPriv  *priv;
        WebKitWebView *web_view;

        priv = window->priv;

        web_view = window_get_active_web_view (window);

        webkit_web_view_open (web_view, dh_link_get_uri (link));

        window_check_history (window, web_view);
}

static void
window_manager_add_widget (GtkUIManager *manager,
                           GtkWidget    *widget,
                           DhWindow     *window)
{
        DhWindowPriv *priv;

        priv = window->priv;

        gtk_box_pack_start (GTK_BOX (priv->menu_box), widget,
                            FALSE, FALSE, 0);

        gtk_widget_show (widget);
}

static void
window_check_history (DhWindow      *window,
                      WebKitWebView *web_view)
{
        DhWindowPriv *priv;
        GtkAction    *action;

        priv = window->priv;

        action = gtk_action_group_get_action (priv->action_group, "Forward");
        g_object_set (action,
                      "sensitive", web_view ? webkit_web_view_can_go_forward (web_view) : FALSE,
                      NULL);

        action = gtk_action_group_get_action (priv->action_group, "Back");
        g_object_set (action,
                      "sensitive", web_view ? webkit_web_view_can_go_back (web_view) : FALSE,
                      NULL);
}

#if 0
static void
window_web_view_location_changed_cb (WebKitWebView *web_view,
                                     const gchar   *location,
                                     DhWindow      *window)
{
        DhWindowPriv *priv;

        priv = window->priv;

        if (web_view == window_get_active_web_view (window)) {
                window_check_history (window, web_view);
        }
}
#endif

static gboolean
window_web_view_open_uri_cb (WebKitWebView *web_view,
                             const gchar   *uri,
                             DhWindow      *window)
{
        DhWindowPriv *priv;

        priv = window->priv;

        if (web_view == window_get_active_web_view (window)) {
                dh_book_tree_select_uri (DH_BOOK_TREE (priv->book_tree), uri);
        }

        return FALSE;
}

static void
window_web_view_title_changed_cb (WebKitWebView  *web_view,
                                  WebKitWebFrame *web_frame,
                                  const gchar    *title,
                                  DhWindow       *window)
{
        window_update_title (window,
                             window_get_active_web_view (window),
                             title);
}

static void
window_findbar_search_changed_cb (GObject    *object,
                                  GParamSpec *pspec,
                                  DhWindow   *window)
{
        DhWindowPriv  *priv;
        WebKitWebView *web_view;

        priv = window->priv;
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
}

static void
window_findbar_case_sensitive_changed_cb (GObject    *object,
                                          GParamSpec *pspec,
                                          DhWindow   *window)
{
        DhWindowPriv  *priv;
        WebKitWebView *web_view;

        priv = window->priv;
        web_view = window_get_active_web_view (window);

        webkit_web_view_unmark_text_matches (web_view);
        webkit_web_view_mark_text_matches (
                web_view, egg_find_bar_get_search_string (
                        EGG_FIND_BAR (priv->findbar)), 
                egg_find_bar_get_case_sensitive (EGG_FIND_BAR (priv->findbar)), 0);
        webkit_web_view_set_highlight_text_matches (web_view, TRUE);
}

static void
window_find_next_cb (GtkEntry *entry,
                     DhWindow *window)
{
        DhWindowPriv  *priv;
        WebKitWebView *web_view;

        priv = window->priv;
        web_view = window_get_active_web_view (window);

        gtk_widget_show (priv->findbar);

        webkit_web_view_search_text (
                web_view, egg_find_bar_get_search_string (EGG_FIND_BAR (priv->findbar)),
                egg_find_bar_get_case_sensitive (EGG_FIND_BAR (priv->findbar)), TRUE, TRUE);
}

static void
window_find_previous_cb (GtkEntry *entry,
                         DhWindow *window)
{
        DhWindowPriv  *priv;
        WebKitWebView *web_view;

        priv = window->priv;
        web_view = window_get_active_web_view (window);

        gtk_widget_show (priv->findbar);

        webkit_web_view_search_text (
                web_view, egg_find_bar_get_search_string (EGG_FIND_BAR (priv->findbar)),
                egg_find_bar_get_case_sensitive (EGG_FIND_BAR (priv->findbar)), FALSE, TRUE);
}

static void
window_findbar_close_cb (GtkWidget *widget,
                         DhWindow  *window)
{
        DhWindowPriv  *priv;
        WebKitWebView *web_view;

        priv = window->priv;
        web_view = window_get_active_web_view (window);

        gtk_widget_hide (priv->findbar);

        webkit_web_view_set_highlight_text_matches (web_view, FALSE);
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
                        GTK_NOTEBOOK (priv->web_view_notebook), num);
        }
}

static void
window_open_new_tab (DhWindow    *window,
                     const gchar *location)
{
        DhWindowPriv *priv;
        GtkWidget    *web_view;
        GtkWidget    *scrolled_window;
        GtkWidget    *label;
        gint          num;

        priv = window->priv;

        web_view = webkit_web_view_new ();
        gtk_widget_show (web_view);

        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_AUTOMATIC,
                                        GTK_POLICY_AUTOMATIC);
        gtk_widget_show (scrolled_window);

        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                             GTK_SHADOW_IN);
        gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
        gtk_container_add (GTK_CONTAINER (scrolled_window), web_view);

        g_object_set_data (G_OBJECT (scrolled_window), "web_view", web_view);

        label = window_new_tab_label (window, _("Empty Page"));
        gtk_widget_show_all (label);

        g_signal_connect (web_view, "title-changed",
                          G_CALLBACK (window_web_view_title_changed_cb),
                          window);
    /*
        g_signal_connect (web_view, "open-uri",
                          G_CALLBACK (window_web_view_open_uri_cb),
                          window);
        g_signal_connect (web_view, "location-changed",
                          G_CALLBACK (window_web_view_location_changed_cb),
                          window);
        g_signal_connect (web_view, "open-new-tab",
                          G_CALLBACK (window_web_view_open_new_tab_cb),
                          window);
              */

        num = gtk_notebook_append_page (GTK_NOTEBOOK (priv->web_view_notebook),
                                        scrolled_window, NULL);

        gtk_notebook_set_tab_label (GTK_NOTEBOOK (priv->web_view_notebook),
                                    scrolled_window, label);

        gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (priv->web_view_notebook),
                                            scrolled_window,
                                            TRUE, TRUE,
                                            GTK_PACK_START);

        if (location) {
                webkit_web_view_open (WEBKIT_WEB_VIEW (web_view), location);
        } else {
                webkit_web_view_open (WEBKIT_WEB_VIEW (web_view), "about:blank");
        }
        gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->web_view_notebook), num);

}

static void
close_button_clicked_cb (GtkButton *button,
                         DhWindow  *window)
{
        GtkAction *action_close;

        action_close = gtk_action_group_get_action (window->priv->action_group,
                                                    "Close");
        window_activate_close (action_close, window);
}

static void
tab_label_style_set_cb (GtkWidget *hbox,
                        GtkStyle  *previous_style,
                        gpointer   user_data)
{
        PangoFontMetrics *metrics;
        PangoContext     *context;
        GtkWidget        *button;
        gint              char_width;
        gint              h, w;

        context = gtk_widget_get_pango_context (hbox);
        metrics = pango_context_get_metrics (context,
                                             hbox->style->font_desc,
                                             pango_context_get_language (context));

        char_width = pango_font_metrics_get_approximate_digit_width (metrics);
        pango_font_metrics_unref (metrics);

        gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (hbox),
                                           GTK_ICON_SIZE_MENU, &w, &h);

        gtk_widget_set_size_request (hbox, 15 * PANGO_PIXELS (char_width) + 2 * w, -1);

        button = g_object_get_data (G_OBJECT (hbox), "close-button");
        gtk_widget_set_size_request (button, w + 2, h + 2);
}

static GtkWidget*
window_new_tab_label (DhWindow    *window,
                      const gchar *str)
{
        GtkWidget  *hbox, *label, *close_button, *image;

        hbox = gtk_hbox_new (FALSE, 4);

        label = gtk_label_new (str);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_label_set_single_line_mode (GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_misc_set_padding (GTK_MISC (label), 0, 0);
        gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

        close_button = gtk_button_new ();
        gtk_button_set_relief (GTK_BUTTON (close_button),
                               GTK_RELIEF_NONE);
        gtk_button_set_focus_on_click (GTK_BUTTON (close_button), FALSE);

        gtk_widget_set_name (close_button, "devhelp-tab-close-button");

        image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
        /*gtk_widget_set_tooltip_text (close_button, _("Close tab"));*/
        g_signal_connect (close_button, "clicked",
                          G_CALLBACK (close_button_clicked_cb), window);

        gtk_container_add (GTK_CONTAINER (close_button), image);

        gtk_box_pack_start (GTK_BOX (hbox), close_button, FALSE, FALSE, 0);

        /* Set minimal size */
        g_signal_connect (hbox, "style-set",
                          G_CALLBACK (tab_label_style_set_cb), NULL);

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

        page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->web_view_notebook));
        if (page_num == -1) {
                return NULL;
        }

        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->web_view_notebook), page_num);

        return g_object_get_data (G_OBJECT (page), "web_view");
}

static void
window_update_title (DhWindow      *window,
                     WebKitWebView *web_view,
                     const gchar   *web_view_title)
{
        DhWindowPriv *priv;
        const gchar  *book_title;

        priv = window->priv;

        if (!web_view_title) {
                WebKitWebFrame *web_frame = webkit_web_view_get_main_frame (web_view);
                web_view_title = webkit_web_frame_get_title (web_frame);
        }

        window_tab_set_title (window, web_view, web_view_title);

        if (web_view_title && *web_view_title == '\0') {
                web_view_title = NULL;
        }

        book_title = dh_book_tree_get_selected_book_title (DH_BOOK_TREE (priv->book_tree));

        /* Don't use both titles if they are the same. */
        if (book_title && web_view_title && strcmp (book_title, web_view_title) == 0) {
                web_view_title = NULL;
        }

        if (!book_title) {
                book_title = "Devhelp";
        }

        if (web_view_title) {
                gchar *full_title;
                full_title = g_strdup_printf ("%s : %s", book_title, web_view_title);
                gtk_window_set_title (GTK_WINDOW (window), full_title);
                g_free (full_title);
        } else {
                gtk_window_set_title (GTK_WINDOW (window), book_title);
        }
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

        priv = window->priv;

        if (!title || title[0] == '\0') {
                title = _("Empty Page");
        }

        num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->web_view_notebook));
        for (i = 0; i < num_pages; i++) {
                page = gtk_notebook_get_nth_page (
                        GTK_NOTEBOOK (priv->web_view_notebook), i);

                /* The web_view widget is inside a frame. */
                if (gtk_bin_get_child (GTK_BIN (page)) == GTK_WIDGET (web_view)) {
                        hbox = gtk_notebook_get_tab_label (
                                GTK_NOTEBOOK (priv->web_view_notebook), page);

                        if (hbox) {
                                label = g_object_get_data (G_OBJECT (hbox), "label");
                                gtk_label_set_text (GTK_LABEL (label), title);
                        }
                        break;
                }
        }
}

GtkWidget *
dh_window_new (DhBase *base)
{
        DhWindow     *window;
        DhWindowPriv *priv;

        window = g_object_new (DH_TYPE_WINDOW, NULL);
        priv = window->priv;

        priv->base = g_object_ref (base);

        g_signal_connect (window,
                          "delete-event",
                          G_CALLBACK (window_delete_cb),
                          NULL);

        window_populate (window);
        window_restore_state (window);

        gtk_window_set_icon_name (GTK_WINDOW (window), "devhelp");

        return GTK_WIDGET (window);
}

void
dh_window_search (DhWindow    *window,
                  const gchar *str)
{
        DhWindowPriv *priv;

        g_return_if_fail (DH_IS_WINDOW (window));

        priv = window->priv;

        gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->control_notebook), 1);

        dh_search_set_search_string (DH_SEARCH (priv->search), str);
}

void
dh_window_focus_search (DhWindow *window)
{
        DhWindowPriv *priv;

        g_return_if_fail (DH_IS_WINDOW (window));

        priv = window->priv;

        gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->control_notebook), 1);

        dh_search_grab_focus (DH_SEARCH (priv->search));
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
        webkit_web_view_open (web_view, uri);
        dh_book_tree_select_uri (DH_BOOK_TREE (priv->book_tree), uri);
}
