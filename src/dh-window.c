/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2004 Imendio HB
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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkactiongroup.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkhpaned.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkuimanager.h>
#include <gconf/gconf-client.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-about.h>
#include <libgnomeui/gnome-href.h>
#include <libgnomeui/gnome-stock-icons.h>

#include "dh-book-tree.h"
#include "dh-html.h"
#include "dh-preferences.h"
#include "dh-search.h"
#include "dh-window.h"

extern gchar *geometry;
extern GConfClient *gconf_client;

struct _DhWindowPriv {
	DhBase         *base;

	GtkWidget      *main_box;
	GtkWidget      *menu_box;
	GtkWidget      *hpaned;
        GtkWidget      *notebook;
        GtkWidget      *book_tree;
	GtkWidget      *search;
	GtkWidget      *html_view;

	DhHtml         *html;

	GtkUIManager   *manager;
	GtkActionGroup *action_group;
};

/* People have reported problems with the default values in GConf so I'm
 * adding this to make sure that the window isn't started 1x1 pixels or the
 * paned having size 0
 */
#define DEFAULT_WIDTH     700
#define DEFAULT_HEIGHT    500
#define DEFAULT_PANED_LOC 250 

static void window_class_init                (DhWindowClass      *klass);
static void window_init                      (DhWindow           *window);
 
static void window_finalize                  (GObject            *object);

static void window_populate                  (DhWindow           *window);

static void window_activate_quit             (GtkAction          *action,
					      DhWindow           *window);
static void window_activate_copy             (GtkAction          *action,
					      DhWindow           *window);
static void window_activate_preferences      (GtkAction          *action,
					      DhWindow           *window);
static void window_activate_back             (GtkAction          *action,
					      DhWindow           *window);
static void window_activate_forward          (GtkAction          *action,
					      DhWindow           *window);
static void window_activate_about            (GtkAction          *action,
					      DhWindow           *window);
static void window_save_state                (DhWindow           *window);
static void window_restore_state             (DhWindow           *window);
static void window_delete_cb                 (GtkWidget          *widget,
					      GdkEventAny        *event,
					      gpointer            user_data);

static gboolean window_open_url              (DhWindow           *window,
					      const gchar        *url);

static void window_link_selected_cb          (GObject            *ignored,
					      DhLink             *link,
					      DhWindow           *window);

static void window_manager_add_widget          (GtkUIManager        *manager,
					      GtkWidget          *widget,
					      DhWindow           *window);
static gboolean window_key_press_event_cb    (GtkWidget          *widget,
					      GdkEventKey        *event,
					      DhWindow           *window);
static void window_check_history             (DhWindow           *window);
static void window_location_changed_cb       (DhHtml             *html,
					      const gchar        *location,
					      DhWindow           *window);

static GtkWindowClass *parent_class = NULL;

static GtkActionEntry actions[] = {
	{ "FileMenu", NULL, N_("_File") },
	{ "EditMenu", NULL, N_("_Edit") },
	{ "GoMenu", NULL, N_("_Go") },
	{ "HelpMenu", NULL, N_("_Help") },

	/* File menu */
	{ "Quit", GTK_STOCK_QUIT, NULL, "<control>Q", NULL,
	  G_CALLBACK (window_activate_quit) },

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

	/* About menu */
	{ "About", GNOME_STOCK_ABOUT, NULL, NULL, NULL,
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
	DhWindowPriv *priv;
	GtkAction    *action;
	
	priv = g_new0 (DhWindowPriv, 1);

	g_signal_connect (window, "key_press_event",
			  G_CALLBACK (window_key_press_event_cb),
			  window);

	priv->html = dh_html_new ();
	g_signal_connect (priv->html, "location-changed",
			  G_CALLBACK (window_location_changed_cb),
			  window);
	
	priv->manager = gtk_ui_manager_new ();

	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (priv->manager));
	
	priv->main_box = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->main_box);
	
	priv->menu_box = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->menu_box);
	gtk_container_set_border_width (GTK_CONTAINER (priv->menu_box), 0);
	gtk_box_pack_start (GTK_BOX (priv->main_box), priv->menu_box, 
			    FALSE, TRUE, 0);
	
	gtk_container_add (GTK_CONTAINER (window), priv->main_box);

	g_signal_connect (priv->manager,
			  "add_widget",
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

        window->priv = priv;
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
window_switch_page_cb (GtkWidget       *notebook,
		       GtkNotebookPage *page,
		       guint            page_num,
		       DhWindow        *window)
{
	DhWindowPriv *priv;

	priv = window->priv;

	g_signal_handlers_block_by_func (priv->book_tree, 
					 window_link_selected_cb, window);
}

static void
window_switch_page_after_cb (GtkWidget       *notebook,
			     GtkNotebookPage *page,
			     guint            page_num,
			     DhWindow        *window)
{
	DhWindowPriv *priv;

	priv = window->priv;
	
	g_signal_handlers_unblock_by_func (priv->book_tree, 
					   window_link_selected_cb, window);
}

static void
window_populate (DhWindow *window)
{
        DhWindowPriv *priv;
	GtkWidget    *frame;
	GtkWidget    *book_tree_sw;
	GNode        *contents_tree;
	GList        *keywords = NULL;
	GError       *error = NULL;
	gint          hpaned_position;
	
        g_return_if_fail (window != NULL);
        g_return_if_fail (DH_IS_WINDOW (window));
        
        priv = window->priv;
	
	gtk_ui_manager_add_ui_from_file (priv->manager,
					 DATADIR "/devhelp/ui/window.ui",
					 &error);
	if (error) {
		g_warning (_("Cannot set UI: %s"), error->message);
		g_error_free (error);
	}

	gtk_ui_manager_ensure_update (priv->manager);
	
        priv->hpaned    = gtk_hpaned_new ();
        priv->notebook  = gtk_notebook_new ();

	g_signal_connect (priv->notebook, "switch_page",
			  G_CALLBACK (window_switch_page_cb),
			  window);

	g_signal_connect_after (priv->notebook, "switch_page",
				G_CALLBACK (window_switch_page_after_cb),
				window);

	book_tree_sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (book_tree_sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (book_tree_sw),
					     GTK_SHADOW_IN);
	gtk_container_set_border_width (GTK_CONTAINER (book_tree_sw), 2);

	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (frame), priv->notebook);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

	gtk_paned_add1 (GTK_PANED (priv->hpaned), frame);
	
	priv->html_view = dh_html_get_widget (priv->html);
	
	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (frame), priv->html_view);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);

	gtk_paned_add2 (GTK_PANED(priv->hpaned), frame);

	hpaned_position = gconf_client_get_int (gconf_client,
						GCONF_PANED_LOCATION,
						NULL);

	if (hpaned_position <= 0) {
		hpaned_position = DEFAULT_PANED_LOC;
	}
	
 	gtk_paned_set_position (GTK_PANED (priv->hpaned), hpaned_position);

	contents_tree = dh_base_get_book_tree (priv->base);
	keywords      = dh_base_get_keywords (priv->base);
	
	if (contents_tree) {
		priv->book_tree = dh_book_tree_new (contents_tree);
	
		gtk_container_add (GTK_CONTAINER (book_tree_sw), 
				   priv->book_tree);

		gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
					  book_tree_sw,
					  gtk_label_new (_("Contents")));
		g_signal_connect (priv->book_tree, "link_selected", 
				  G_CALLBACK (window_link_selected_cb),
				  window);
	}
	
	if (keywords) {
		priv->search = dh_search_new (keywords);
		
		gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
					  priv->search,
					  gtk_label_new (_("Search")));

		g_signal_connect (priv->search, "link_selected",
				  G_CALLBACK (window_link_selected_cb),
				  window);
	}

	gtk_box_pack_start (GTK_BOX (priv->main_box), priv->hpaned,
			    TRUE, TRUE, 0);

	gtk_widget_show_all (priv->hpaned);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), 0);

	dh_preferences_setup_fonts ();
}

static void
window_activate_quit (GtkAction *action, DhWindow *window)
{
	DhWindowPriv *priv;
	
	g_return_if_fail (DH_IS_WINDOW (window));

	priv = window->priv;

	window_save_state (window);
	
	gtk_main_quit ();
}

static void
window_activate_copy (GtkAction *action, DhWindow *window)
{
	g_print ("Copy\n");
}

static void
window_activate_preferences (GtkAction *action, DhWindow *window)
{
	dh_preferences_show_dialog (GTK_WINDOW (window));
}

static void window_activate_back (GtkAction *action, DhWindow *window)
{
	DhWindowPriv *priv;

	priv = window->priv;

	dh_html_go_back (priv->html);
}

static void window_activate_forward (GtkAction *action, DhWindow *window)
{
	DhWindowPriv *priv;

	priv = window->priv;

	dh_html_go_forward (priv->html);
}

static void window_activate_about            (GtkAction          *action,
					      DhWindow           *window)
{
	static GtkWidget *about = NULL;
	GtkWidget        *hbox;
	GtkWidget        *href;

	const gchar *authors[] = {
		"Mikael Hallendal <micke@imendio.com>",
		"Richard Hult <richard@imendio.com>",
		"Johan Dahlin <jdahlin@telia.com>",
		"Ross Burton <ross@burtonini.com>",
		NULL
	};
	
	if (about != NULL) {
		gtk_window_present (GTK_WINDOW (about));
		return;
	}
	
	about = gnome_about_new ("Devhelp", VERSION,
				 "",
				 _("A developer's help browser for GNOME 2"),
				 authors,
				 NULL,
				 NULL,
				 NULL);

	gtk_window_set_transient_for (GTK_WINDOW (about), GTK_WINDOW (window));

	g_signal_connect (about,
			  "destroy",
			  G_CALLBACK (gtk_widget_destroyed),
			  &about);
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about)->vbox),
			    hbox, FALSE, FALSE, 0);
	href = gnome_href_new ("http://www.imendio.com/projects/devhelp/",
			       _("Devhelp project page"));
	gtk_box_pack_start (GTK_BOX (hbox), href, 
			    TRUE, TRUE, 0);
	href = gnome_href_new ("http://bugzilla.gnome.org/",
			       _("Bug report Devhelp"));
	gtk_box_pack_start (GTK_BOX (hbox), href,
			    TRUE, TRUE, 0);

	gtk_widget_show_all (about);
}

static void
window_save_state (DhWindow *window)
{
	DhWindowPriv *priv;
	GdkWindowState state;
	gboolean       maximized;

	priv = window->priv;

	state = gdk_window_get_state (GTK_WIDGET (window)->window);
	if (state & GDK_WINDOW_STATE_MAXIMIZED) {
		maximized = TRUE;
	} else {
		maximized = FALSE;
	}

	gconf_client_set_bool (gconf_client,
			       GCONF_MAIN_WINDOW_MAXIMIZED,
			       maximized, NULL);

	/* If maximized don't save the size and position */
	if (!maximized) {
		int width, height;
		int x, y;

		gtk_window_get_size (GTK_WINDOW (window), &width, &height);
		gconf_client_set_int (gconf_client,
				      GCONF_MAIN_WINDOW_WIDTH,
				      width, NULL);
		gconf_client_set_int (gconf_client,
				      GCONF_MAIN_WINDOW_HEIGHT,
				      height, NULL);

		gtk_window_get_position (GTK_WINDOW (window), &x, &y);
		gconf_client_set_int (gconf_client,
				      GCONF_MAIN_WINDOW_POS_X,
				      x, NULL);
		gconf_client_set_int (gconf_client,
				      GCONF_MAIN_WINDOW_POS_Y,
				      y, NULL);
	}

	gconf_client_set_int (gconf_client,
			      GCONF_PANED_LOCATION,
			      gtk_paned_get_position (GTK_PANED (priv->hpaned)),
			      NULL);
}

static void
window_restore_state (DhWindow *window)
{
	gboolean maximized;
	int      width, height;
	int      x, y;

	width = gconf_client_get_int (gconf_client,
				      GCONF_MAIN_WINDOW_WIDTH,
				      NULL);

	if (width <= 0) {
		width = DEFAULT_WIDTH;
	}

	height = gconf_client_get_int (gconf_client,
				       GCONF_MAIN_WINDOW_HEIGHT,
				       NULL);

	if (height <= 0) {
		height = DEFAULT_HEIGHT;
	}

	gtk_window_set_default_size (GTK_WINDOW (window), 
				     width, height);

	x = gconf_client_get_int (gconf_client,
				  GCONF_MAIN_WINDOW_POS_X,
				  NULL);
	y = gconf_client_get_int (gconf_client,
				  GCONF_MAIN_WINDOW_POS_Y,
				  NULL);

	gtk_window_move (GTK_WINDOW (window), x, y);

	maximized = gconf_client_get_bool (gconf_client,
					   GCONF_MAIN_WINDOW_MAXIMIZED,
					   NULL);
	if (maximized) {
		gtk_window_maximize (GTK_WINDOW (window));
	}
}

static void
window_delete_cb (GtkWidget   *widget,
		  GdkEventAny *event,
		  gpointer     user_data)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (DH_IS_WINDOW (widget));

	window_save_state (DH_WINDOW (widget));

	gtk_main_quit ();
}

static gboolean 
window_open_url (DhWindow *window, const gchar *url)
{
	DhWindowPriv *priv;
	
	g_return_val_if_fail (DH_IS_WINDOW (window), FALSE);
	g_return_val_if_fail (url != NULL, FALSE);

	priv = window->priv;

	dh_html_open_uri (priv->html, url);
	dh_book_tree_show_uri (DH_BOOK_TREE (priv->book_tree), url);

	window_check_history (window);
	
	return TRUE;
}

static void
window_link_selected_cb (GObject *ignored, DhLink *link, DhWindow *window)
{
	DhWindowPriv   *priv;

	g_return_if_fail (link != NULL);
	g_return_if_fail (DH_IS_WINDOW (window));
	
	priv = window->priv;

	window_open_url (window, link->uri);
}

static void
window_manager_add_widget (GtkUIManager *manager,
			 GtkWidget    *widget,
			 DhWindow     *window)
{
	DhWindowPriv *priv;
	
	g_return_if_fail (DH_IS_WINDOW (window));
	priv = window->priv;

	gtk_box_pack_start (GTK_BOX (priv->menu_box), widget,
			    FALSE, FALSE, 0);
	
	gtk_widget_show (widget);
}

static gboolean
window_key_press_event_cb (GtkWidget   *widget,
			   GdkEventKey *event,
			   DhWindow    *window)
{
	DhWindowPriv *priv;

	priv = window->priv;
	
	if ((event->state & GDK_CONTROL_MASK) &&
	    (event->keyval == GDK_l) &&
	    (gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)) == 1)) {
		dh_search_grab_focus (DH_SEARCH (priv->search));
		return TRUE;
	}
	
	return FALSE;
}

static void
window_check_history (DhWindow *window)
{
	DhWindowPriv *priv;
	GtkAction *action;
		
	priv = window->priv;
	
	action = gtk_action_group_get_action (priv->action_group, 
					      "Forward");
	
	g_object_set (action, "sensitive", 
		      dh_html_can_go_forward (priv->html), NULL);
	action = gtk_action_group_get_action (priv->action_group,
					      "Back");
	g_object_set (action, "sensitive",
		      dh_html_can_go_back (priv->html), NULL);
}

static void
window_location_changed_cb (DhHtml      *html,
			    const gchar *location, 
			    DhWindow    *window)
{
	window_check_history (window);
}

GtkWidget *
dh_window_new (DhBase *base)
{
        DhWindow     *window;
        DhWindowPriv *priv;
	GdkPixbuf    *icon;
	
        window = g_object_new (DH_TYPE_WINDOW, NULL);
        priv   = window->priv;

	priv->base = g_object_ref (base);

        gtk_window_set_policy (GTK_WINDOW (window), TRUE, TRUE, FALSE);
	gtk_window_set_title (GTK_WINDOW (window), "Devhelp");

	if (geometry) {
		gtk_window_parse_geometry (GTK_WINDOW (window), geometry);
	} else {
		window_restore_state (window);
	}

	g_signal_connect (window, 
			  "delete_event",
			  G_CALLBACK (window_delete_cb),
			  NULL);

	window_populate (window);

	icon = gdk_pixbuf_new_from_file (DATA_DIR "/pixmaps/devhelp.png", 
					 NULL);
	if (icon) {
		gtk_window_set_icon (GTK_WINDOW (window), icon);
		g_object_unref (icon);
	}

	return GTK_WIDGET (window);
}

void
dh_window_show (DhWindow *window)
{
	gtk_widget_show_all (GTK_WIDGET (window));

	/* Make sure that the HTML widget is realized before trying to 
	 * clear it. Solves bug #147343.
	 */
	while (g_main_context_pending (NULL)) {
		g_main_context_iteration (NULL, FALSE);
	}

	dh_html_clear (window->priv->html);
}

void
dh_window_search (DhWindow *window, const gchar *str)
{
	DhWindowPriv *priv;
	
	g_return_if_fail (window != NULL);
	g_return_if_fail (DH_IS_WINDOW (window));
	
	priv = window->priv;

	dh_search_set_search_string (DH_SEARCH (priv->search), str);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), 1);
}

