/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#include "dh-book-tree.h"
#include "dh-html.h"
#include "dh-preferences.h"
#include "dh-search.h"
#include "dh-window.h"

struct _DhWindowPriv {
	DhBase         *base;

	GConfClient    *gconf_client;

	GtkWidget      *main_box;
	GtkWidget      *menu_box;
	GtkWidget      *hpaned;
        GtkWidget      *control_notebook;
        GtkWidget      *book_tree;
	GtkWidget      *search;
	GtkWidget      *html_notebook;

	GtkUIManager   *manager;
	GtkActionGroup *action_group;
};

static guint tab_accel_keys[] = {
	GDK_1, GDK_2, GDK_3, GDK_4, GDK_5,
	GDK_6, GDK_7, GDK_8, GDK_9, GDK_0
};

/* People have reported problems with the default values in GConf so I'm
 * adding this to make sure that the window isn't started 1x1 pixels or the
 * paned having size 0
 */
#define DEFAULT_WIDTH     700
#define DEFAULT_HEIGHT    500
#define DEFAULT_PANED_LOC 250

static void       window_class_init               (DhWindowClass   *klass);
static void       window_init                     (DhWindow        *window);
static void       window_finalize                 (GObject         *object);
static void       window_populate                 (DhWindow        *window);
static void       window_activate_new_window      (GtkAction       *action,
						   DhWindow        *window);
static void       window_activate_new_tab         (GtkAction       *action,
						   DhWindow        *window);
static void       window_activate_close           (GtkAction       *action,
						   DhWindow        *window);
static void       window_activate_copy            (GtkAction       *action,
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
static void       window_save_state               (DhWindow        *window);
static void       window_restore_state            (DhWindow        *window);
static gboolean   window_delete_cb                (GtkWidget       *widget,
						   GdkEventAny     *event,
						   gpointer         user_data);
static gboolean   window_key_press_event_cb       (GtkWidget       *widget,
						   GdkEventKey     *event,
						   DhWindow        *window);
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
						   DhHtml          *html);
static void       window_html_location_changed_cb (DhHtml          *html,
						   const gchar     *location,
						   DhWindow        *window);
static void       window_html_title_changed_cb    (DhHtml          *html,
						   const gchar     *location,
						   DhWindow        *window);
static gboolean   window_html_open_uri_cb         (DhHtml          *html,
						   const gchar     *uri,
						   DhWindow        *window);
static void       window_html_open_new_tab_cb     (DhHtml          *html,
						   const gchar     *location,
						   DhWindow        *window);
static void       window_html_tab_accel_cb        (GtkAccelGroup   *accel_group,
						   GObject         *object,
						   guint            key,
						   GdkModifierType  mod,
						   DhWindow        *window);
static GtkWidget *window_new_tab_label            (DhWindow        *window,
						   const gchar     *label);
static void       window_open_new_tab             (DhWindow        *window,
						   const gchar     *location);
static DhHtml *   window_get_active_html          (DhWindow        *window);
static void       window_update_title             (DhWindow        *window,
						   DhHtml          *html);
static void       window_tab_set_title            (DhWindow        *window,
						   DhHtml          *html,
						   const gchar     *title);


static GtkWindowClass *parent_class = NULL;

static const GtkActionEntry actions[] = {
	{ "FileMenu", NULL, N_("_File") },
	{ "EditMenu", NULL, N_("_Edit") },
	{ "GoMenu",   NULL, N_("_Go") },
	{ "HelpMenu", NULL, N_("_Help") },

	/* File menu */
	{ "NewWindow", GTK_STOCK_NEW, N_("_New Window"), "<control>N", NULL,
	  G_CALLBACK (window_activate_new_window) },
	{ "NewTab", GTK_STOCK_NEW, N_("New _Tab"), "<control>T", NULL,
	  G_CALLBACK (window_activate_new_tab) },
	{ "Close", GTK_STOCK_CLOSE, NULL, NULL, NULL,
	  G_CALLBACK (window_activate_close) },

	/* Edit menu */
	{ "Copy", GTK_STOCK_COPY, NULL, "<control>C", NULL,
	  G_CALLBACK (window_activate_copy) },
	{ "Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL,
	  G_CALLBACK (window_activate_preferences) },

	/* Go menu */
	{ "Back", GTK_STOCK_GO_BACK, NULL, "<alt>Left", NULL,
	  G_CALLBACK (window_activate_back) },
	{ "Forward", GTK_STOCK_GO_FORWARD, NULL, "<alt>Right", NULL,
	  G_CALLBACK (window_activate_forward) },

	{ "ShowContentsTab", NULL, N_("Browse Contents"), "<ctrl>B", NULL,
	  G_CALLBACK (window_activate_show_contents) },

	{ "ShowSearchTab", NULL, N_("Search"), "<ctrl>S", NULL,
	  G_CALLBACK (window_activate_show_search) },

	/* About menu */
	{ "About", GTK_STOCK_ABOUT, NULL, NULL, NULL,
	  G_CALLBACK (window_activate_about) }
};

GType
dh_window_get_type (void)
{
        static GType type = 0;

        if (!type) {
                static const GTypeInfo info = {
			sizeof (DhWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) window_class_init,
			NULL,
			NULL,
			sizeof (DhWindow),
			0,
			(GInstanceInitFunc) window_init
		};

                type = g_type_register_static (GTK_TYPE_WINDOW,
					       "DhWindow",
					       &info, 0);
        }

        return type;
}

static void
window_class_init (DhWindowClass *klass)
{
        GObjectClass *object_class;

        parent_class = g_type_class_peek_parent (klass);

        object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = window_finalize;
}

static void
window_init (DhWindow *window)
{
	DhWindowPriv  *priv;
	GtkAction     *action;
	GtkAccelGroup *accel_group;
	GClosure      *closure;
	gint           i;

	priv = g_new0 (DhWindowPriv, 1);
        window->priv = priv;

	g_signal_connect (window,
			  "key-press-event",
			  G_CALLBACK (window_key_press_event_cb),
			  window);

	priv->manager = gtk_ui_manager_new ();

	priv->gconf_client = gconf_client_get_default ();

	accel_group = gtk_ui_manager_get_accel_group (priv->manager);
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

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
		closure =  g_cclosure_new (G_CALLBACK (window_html_tab_accel_cb),
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
window_finalize (GObject *object)
{
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		G_OBJECT_CLASS (parent_class)->finalize (object);
	}
}

/* The ugliest hack. When switching tabs, the selection and cursor is changed
 * for the tree view so the html content is changed. Block the signal during
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
window_html_switch_page_cb (GtkNotebook     *notebook,
			    GtkNotebookPage *page,
			    guint            new_page_num,
			    DhWindow        *window)
{
	DhWindowPriv *priv;
	GtkWidget    *new_page;

	priv = window->priv;

	new_page = gtk_notebook_get_nth_page (notebook, new_page_num);
	if (new_page) {
		DhHtml *new_html;
		gchar  *title, *location;

		new_html = g_object_get_data (G_OBJECT (new_page), "html");

		window_update_title (window, new_html);
		return;


		
		title = dh_html_get_title (new_html);
		gtk_window_set_title (GTK_WINDOW (window), title);
		g_free (title);

		/* Sync the book tree. */
		location = dh_html_get_location (new_html);
		if (location) {
			dh_book_tree_select_uri (DH_BOOK_TREE (priv->book_tree),
						 location);
			g_free (location);
		}

		window_check_history (window, new_html);
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

        priv->hpaned   = gtk_hpaned_new ();

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

	/* HTML tabs notebook. */
 	priv->html_notebook = gtk_notebook_new ();
	gtk_paned_add2 (GTK_PANED (priv->hpaned), priv->html_notebook);

 	g_signal_connect (priv->html_notebook,
			  "switch-page",
			  G_CALLBACK (window_html_switch_page_cb),
			  window);

	dh_preferences_setup_fonts ();

	gtk_widget_show_all (priv->hpaned);
	window_open_new_tab (window, NULL);
}

static void
window_activate_new_window (GtkAction *action, DhWindow *window)
{
	DhWindowPriv *priv;
	GtkWidget    *new_window;

	priv = window->priv;

	new_window = dh_base_new_window (priv->base);
	gtk_widget_show (new_window);
}

static void
window_activate_new_tab (GtkAction *action, DhWindow *window)
{
	DhWindowPriv *priv;

	priv = window->priv;

	window_open_new_tab (window, NULL);
}

static void
window_activate_close (GtkAction *action, DhWindow *window)
{
	DhWindowPriv *priv;
	gint          page_num;

	priv = window->priv;

	page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->html_notebook));
        gtk_notebook_remove_page (GTK_NOTEBOOK (priv->html_notebook), page_num);

	if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->html_notebook)) == 0) {
		window_save_state (window);
		gtk_widget_destroy (GTK_WIDGET (window));
	}
}

static void
window_activate_copy (GtkAction *action, DhWindow *window)
{
	GtkWidget *widget;

	widget = gtk_window_get_focus (GTK_WINDOW (window));

	if (GTK_IS_EDITABLE (widget)) {
		gtk_editable_copy_clipboard (GTK_EDITABLE (widget));
	} else {
		DhHtml *html;

		html = window_get_active_html (window);
		dh_html_copy_selection (html);
	}
}

static void
window_activate_preferences (GtkAction *action, DhWindow *window)
{
	dh_preferences_show_dialog (GTK_WINDOW (window));
}

static void
window_activate_back (GtkAction *action, DhWindow *window)
{
	DhWindowPriv *priv;
	DhHtml       *html;
	GtkWidget    *frame;

	priv = window->priv;

	frame = gtk_notebook_get_nth_page (
		GTK_NOTEBOOK (priv->html_notebook),
		gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->html_notebook)));
	html = g_object_get_data (G_OBJECT (frame), "html");

	dh_html_go_back (html);
}

static void
window_activate_forward (GtkAction *action,
			 DhWindow  *window)
{
	DhWindowPriv *priv;
	DhHtml       *html;
	GtkWidget    *frame;

	priv = window->priv;

	frame = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->html_notebook),
	                                   gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->html_notebook))
	                                  );
	html = g_object_get_data (G_OBJECT (frame), "html");

	dh_html_go_forward (html);
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
			       "version", VERSION,
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
	const gchar  *tab;

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
}

static gboolean
window_delete_cb (GtkWidget   *widget,
		  GdkEventAny *event,
		  gpointer     user_data)
{
	window_save_state (DH_WINDOW (widget));

	return FALSE;
}

static gboolean
window_key_press_event_cb (GtkWidget   *widget,
			   GdkEventKey *event,
			   DhWindow    *window)
{
	if (event->keyval == GDK_Escape) {
		gtk_window_iconify (GTK_WINDOW (window));
		return TRUE;
	}

	return FALSE;
}

static void
window_tree_link_selected_cb (GObject  *ignored,
			      DhLink   *link,
			      DhWindow *window)
{
	DhWindowPriv *priv;
	DhHtml       *html;

	priv = window->priv;

	html = window_get_active_html (window);

	/* Block so we don't try to sync the tree when we have already clicked
	 * in it.
	 */
	g_signal_handlers_block_by_func (html,
					 window_html_open_uri_cb,
					 window);

	dh_html_open_uri (html, link->uri);

	g_signal_handlers_unblock_by_func (html,
					   window_html_open_uri_cb,
					   window);

	window_check_history (window, html);
}

static void
window_search_link_selected_cb (GObject  *ignored,
				DhLink   *link,
				DhWindow *window)
{
	DhWindowPriv *priv;
	DhHtml       *html;

	priv = window->priv;

	html = window_get_active_html (window);

	dh_html_open_uri (html, link->uri);

	window_check_history (window, html);
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
window_check_history (DhWindow *window, DhHtml *html)
{
	DhWindowPriv *priv;
	GtkAction    *action;

	priv = window->priv;

	action = gtk_action_group_get_action (priv->action_group, "Forward");
	g_object_set (action,
		      "sensitive", html ? dh_html_can_go_forward (html) : FALSE,
		      NULL);

	action = gtk_action_group_get_action (priv->action_group, "Back");
	g_object_set (action,
		      "sensitive", html ? dh_html_can_go_back (html) : FALSE,
		      NULL);
}

static void
window_html_location_changed_cb (DhHtml      *html,
				 const gchar *location,
				 DhWindow    *window)
{
	DhWindowPriv *priv;

	priv = window->priv;

	if (html == window_get_active_html (window)) {
		window_check_history (window, html);
	}
}

static gboolean
window_html_open_uri_cb (DhHtml      *html,
			 const gchar *uri,
			 DhWindow    *window)
{
	DhWindowPriv *priv;

	priv = window->priv;

	if (html == window_get_active_html (window)) {
		dh_book_tree_select_uri (DH_BOOK_TREE (priv->book_tree), uri);
	}

	return FALSE;
}

static void
window_html_title_changed_cb (DhHtml      *html,
			      const gchar *title,
			      DhWindow    *window)
{
	window_update_title (window, window_get_active_html (window));
}

static void
window_html_open_new_tab_cb (DhHtml      *html,
			     const gchar *location,
			     DhWindow    *window)
{
	window_open_new_tab (window, location);
}

static void
window_html_tab_accel_cb (GtkAccelGroup    *accel_group,
			  GObject          *object,
			  guint             key,
			  GdkModifierType   mod,
			  DhWindow         *window)
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
			GTK_NOTEBOOK (priv->html_notebook), num);
	}
}

static void
window_open_new_tab (DhWindow    *window,
		     const gchar *location)
{
	DhWindowPriv *priv;
	DhHtml       *html;
	GtkWidget    *frame;
	GtkWidget    *view;
	GtkWidget    *label;
	gint          num;

	priv = window->priv;

	html = dh_html_new ();

	view = dh_html_get_widget (html);
	gtk_widget_show (view);

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);

	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
	gtk_container_add (GTK_CONTAINER (frame), view);

	g_object_set_data (G_OBJECT (frame), "html", html);

	label = window_new_tab_label (window, _("Empty Page"));
	gtk_widget_show (label);

	g_signal_connect (html, "title-changed",
			  G_CALLBACK (window_html_title_changed_cb),
			  window);
	g_signal_connect (html, "open-uri",
			  G_CALLBACK (window_html_open_uri_cb),
			  window);
	g_signal_connect (html, "location-changed",
			  G_CALLBACK (window_html_location_changed_cb),
			  window);
	g_signal_connect (html, "open-new-tab",
			  G_CALLBACK (window_html_open_new_tab_cb),
			  window);

	num = gtk_notebook_append_page (GTK_NOTEBOOK (priv->html_notebook),
				  frame, NULL);

	gtk_notebook_set_tab_label (GTK_NOTEBOOK (priv->html_notebook),
				    frame, label);

	gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (priv->html_notebook),
					    frame,
					    TRUE, TRUE,
					    GTK_PACK_START);

	/* Hack to get GtkMozEmbed to work properly. */
	gtk_widget_realize (view);

	if (location) {
		dh_html_open_uri (html, location);
	} else {
		dh_html_clear (html);
	}

	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->html_notebook), num);

}

static GtkWidget*
window_new_tab_label (DhWindow *window, const gchar *str)
{
	GtkWidget *label;

	label = gtk_label_new (str);
	gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
	gtk_label_set_single_line_mode (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label), 0, 0);

	return label;
}

static DhHtml *
window_get_active_html (DhWindow *window)
{
	DhWindowPriv *priv;
	gint          page_num;
	GtkWidget    *page;

	priv = window->priv;

	page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->html_notebook));
	if (page_num == -1) {
		return NULL;
	}

	page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->html_notebook), page_num);

	return g_object_get_data (G_OBJECT (page), "html");
}

static void
window_update_title (DhWindow *window,
		     DhHtml   *html)
{
	DhWindowPriv *priv;
	gchar        *html_title;
	const gchar  *book_title;
	gchar        *full_title;

	priv = window->priv;
	
	html_title = dh_html_get_title (html);

	window_tab_set_title (window, html, html_title);

	if (html_title && *html_title == '\0') {
		g_free (html_title);
		html_title = NULL;
	}

	book_title = dh_book_tree_get_selected_book_title (DH_BOOK_TREE (priv->book_tree));

	/* Don't use both titles if they are the same. */
	if (book_title && html_title && strcmp (book_title, html_title) == 0) {
		html_title = NULL;
	}
	
	if (!book_title) {
		book_title = "Devhelp";
	}
	
	if (html_title) {
		full_title = g_strdup_printf ("%s : %s", book_title, html_title);
	} else {
		full_title = g_strdup (book_title);
	}
	
	gtk_window_set_title (GTK_WINDOW (window), full_title);
	g_free (full_title);
}

static void
window_tab_set_title (DhWindow *window, DhHtml *html, const gchar *title)
{
	DhWindowPriv *priv;
	gint          num_pages, i;
	GtkWidget    *view;
	GtkWidget    *page;
	GtkWidget    *label;

	priv = window->priv;

	view = dh_html_get_widget (html);

	if (!title || title[0] == '\0') {
		title = _("Empty Page");
	}

	num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->html_notebook));
	for (i = 0; i < num_pages; i++) {
		page = gtk_notebook_get_nth_page (
			GTK_NOTEBOOK (priv->html_notebook), i);

		/* The html widget is inside a frame. */
		if (gtk_bin_get_child (GTK_BIN (page)) == view) {
			label = gtk_notebook_get_tab_label (
				GTK_NOTEBOOK (priv->html_notebook), page);

			if (label) {
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
	GdkPixbuf    *icon;

        window = g_object_new (DH_TYPE_WINDOW, NULL);
        priv = window->priv;

	priv->base = g_object_ref (base);

	g_signal_connect (window,
			  "delete-event",
			  G_CALLBACK (window_delete_cb),
			  NULL);

	window_populate (window);
	window_restore_state (window);

	icon = gdk_pixbuf_new_from_file (DATA_DIR "/pixmaps/devhelp.png",
					 NULL);
	if (icon) {
		gtk_window_set_icon (GTK_WINDOW (window), icon);
		g_object_unref (icon);
	}

	return GTK_WIDGET (window);
}

void
dh_window_search (DhWindow *window, const gchar *str)
{
	DhWindowPriv *priv;

	g_return_if_fail (DH_IS_WINDOW (window));

	priv = window->priv;

	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->control_notebook), 1);

	dh_search_set_search_string (DH_SEARCH (priv->search), str);
}

