/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002      CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2003      Richard Hult <richard@imendio.com>
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
#include <gtk/gtkhpaned.h>
#include <gtk/gtklabel.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkvbox.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-about.h>
#include <libgnomeui/gnome-href.h>
#include <libgnomeui/gnome-stock-icons.h>
#include <libegg/menu/egg-menu-merge.h>

#include "dh-book-tree.h"
#include "dh-history.h"
#include "dh-html.h"
#include "dh-search.h"
#include "dh-window.h"

extern gchar *geometry;

struct _DhWindowPriv {
	DhBase         *base;
	DhHistory      *history;

	GtkWidget      *main_box;
	GtkWidget      *menu_box;
	GtkWidget      *hpaned;
        GtkWidget      *notebook;
        GtkWidget      *book_tree;
	GtkWidget      *search;
	GtkWidget      *html_view;

	DhHtml         *html;

	EggMenuMerge   *merge;
	EggActionGroup *action_group;
};

static void window_class_init                (DhWindowClass      *klass);
static void window_init                      (DhWindow           *window);
 
static void window_finalize                  (GObject            *object);

static void window_populate                  (DhWindow           *window);

static void window_activate_action           (EggAction          *action,
					      DhWindow           *window);
static void window_delete_cb                 (GtkWidget          *widget,
					      GdkEventAny        *event,
					      gpointer            user_data);

static gboolean window_open_url              (DhWindow           *window,
					      const gchar        *url);

static void window_link_selected_cb          (GObject            *ignored,
					      DhLink             *link,
					      DhWindow           *window);

static void window_merge_add_widget          (EggMenuMerge       *merge,
					      GtkWidget          *widget,
					      DhWindow           *window);
static void window_back_exists_changed_cb    (DhHistory          *history,
					      gboolean            exists,
					      DhWindow           *window);
static void window_forward_exists_changed_cb (DhHistory          *history,
					      gboolean            exists,
					      DhWindow           *window);
static gboolean window_key_press_event_cb    (GtkWidget          *widget,
					      GdkEventKey        *event,
					      DhWindow           *window);



static GtkWindowClass *parent_class = NULL;

static EggActionGroupEntry actions[] = {
	{ "StockFileMenuAction", N_("_File"), NULL, NULL, NULL, NULL, NULL },
	{ "StockGoMenuAction", N_("_Go"), NULL, NULL, NULL, NULL, NULL },
	{ "StockHelpMenuAction", N_("_Help"), NULL, NULL, NULL, NULL, NULL },

	/* File menu */
	{ "QuitAction", NULL, GTK_STOCK_QUIT, "<control>Q", NULL,
	  G_CALLBACK (window_activate_action), NULL },

	/* Go menu */
	{ "BackAction", NULL, GTK_STOCK_GO_BACK, NULL, NULL,
	  G_CALLBACK (window_activate_action), NULL },
	{ "ForwardAction", NULL, GTK_STOCK_GO_FORWARD, NULL, NULL,
	  G_CALLBACK (window_activate_action), NULL },

	/* About menu */
	{ "AboutAction", NULL, GNOME_STOCK_ABOUT, NULL, NULL,
	  G_CALLBACK (window_activate_action), NULL }
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
        
        object_class = (GObjectClass *) klass;
        
        object_class->finalize = window_finalize;
}

static void
window_init (DhWindow *window)
{
        DhWindowPriv *priv;
	gint          i;
	EggAction    *action;
	
        priv          = g_new0 (DhWindowPriv, 1);
	priv->history = dh_history_new ();

	g_signal_connect (priv->history, "forward_exists_changed",
			  G_CALLBACK (window_forward_exists_changed_cb),
			  window);
	g_signal_connect (priv->history, "back_exists_changed",
			  G_CALLBACK (window_back_exists_changed_cb),
			  window);

	g_signal_connect (window, "key_press_event",
			  G_CALLBACK (window_key_press_event_cb),
			  window);
	
	priv->merge = egg_menu_merge_new ();

	gtk_window_add_accel_group (GTK_WINDOW (window),
				    priv->merge->accel_group);
	
	priv->main_box = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->main_box);
	
	priv->menu_box = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->menu_box);
	gtk_container_set_border_width (GTK_CONTAINER (priv->menu_box), 0);
	gtk_box_pack_start (GTK_BOX (priv->main_box), priv->menu_box, 
			    FALSE, TRUE, 0);
	
	gtk_container_add (GTK_CONTAINER (window), priv->main_box);

	g_signal_connect (priv->merge,
			  "add_widget",
			  G_CALLBACK (window_merge_add_widget),
			  window);

	for (i = 0; i < G_N_ELEMENTS (actions); ++i) {
		actions[i].user_data = window;
	}

	priv->action_group = egg_action_group_new ("MainWindow");
	
	egg_action_group_add_actions (priv->action_group,
				      actions,
				      G_N_ELEMENTS (actions));

	egg_menu_merge_insert_action_group (priv->merge,
					    priv->action_group,
					    0);

	action = egg_action_group_get_action (priv->action_group, 
					      "BackAction");
	g_object_set (action, "sensitive", FALSE, NULL);
	
	action = egg_action_group_get_action (priv->action_group,
					      "ForwardAction");
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

	g_signal_handlers_block_by_func (priv->book_tree, window_link_selected_cb, window);
}

static void
window_switch_page_after_cb (GtkWidget       *notebook,
			     GtkNotebookPage *page,
			     guint            page_num,
			     DhWindow        *window)
{
	DhWindowPriv *priv;

	priv = window->priv;
	
	g_signal_handlers_unblock_by_func (priv->book_tree, window_link_selected_cb, window);
}

static void
window_populate (DhWindow *window)
{
        DhWindowPriv *priv;
	GtkWidget    *html_sw;
	GtkWidget    *frame;
	GtkWidget    *book_tree_sw;
	GNode        *contents_tree;
	GList        *keywords = NULL;
	GError       *error = NULL;
	 
        g_return_if_fail (window != NULL);
        g_return_if_fail (DH_IS_WINDOW (window));
        
        priv = window->priv;
	
	egg_menu_merge_add_ui_from_file (priv->merge,
					 DATADIR "/devhelp/ui/window.ui",
					 &error);

        priv->hpaned    = gtk_hpaned_new ();
        priv->notebook  = gtk_notebook_new ();
	priv->html      = dh_html_new ();
	priv->html_view = dh_html_get_widget (priv->html);

	g_signal_connect (priv->notebook,
			  "switch_page",
			  G_CALLBACK (window_switch_page_cb),
			  window);

	g_signal_connect_after (priv->notebook,
				"switch_page",
				G_CALLBACK (window_switch_page_after_cb),
				window);

	html_sw         = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (html_sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	book_tree_sw      = gtk_scrolled_window_new (NULL, NULL);
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
	
/*  	gtk_container_add (GTK_CONTAINER (html_sw), priv->html_view); */
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (html_sw),
					       priv->html_view);
	
	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (frame), html_sw);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

 	gtk_paned_add2 (GTK_PANED(priv->hpaned), frame);

 	gtk_paned_set_position (GTK_PANED (priv->hpaned), 250);

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

 	g_signal_connect_swapped (priv->html, 
				  "uri_selected", 
				  G_CALLBACK (window_open_url),
				  window);
}

static void
window_activate_action (EggAction *action, DhWindow *window)
{
	DhWindowPriv *priv;
	const gchar  *name = action->name;
	
	g_return_if_fail (DH_IS_WINDOW (window));

	priv = window->priv;
	
	if (strcmp (name, "QuitAction") == 0) {
		gtk_main_quit ();
		/* Quit */
	}
	else if (strcmp (name, "BackAction") == 0) {
		gchar *uri = dh_history_go_back (priv->history);
		if (uri) {
			dh_html_open_uri (priv->html, uri);
			g_free (uri);
		}
	}
	else if (strcmp (name, "ForwardAction") == 0) {
		gchar *uri = dh_history_go_forward (priv->history);
		if (uri) {
			dh_html_open_uri (priv->html, uri);
			g_free (uri);
		}
	}
	else if (strcmp (name, "AboutAction") == 0) {
		static GtkWidget *about = NULL;

		const gchar *authors[] = {
			"Mikael Hallendal <micke@imendio.com>",
			"Richard Hult <richard@imendio.com>",
			"Johan Dahlin <jdahlin@telia.com>",
			"Ross Burton <ross@burtonini.com>",
			NULL
		};

		if (!about) {
			GtkWidget *hbox;
			GtkWidget *href;
			
			about = gnome_about_new ("Devhelp", VERSION,
						 "",
						 _("A developer's help browser for GNOME 2"),
						 authors,
						 NULL,
						 NULL,
						 NULL);

					
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
		} else {
			gtk_window_present (GTK_WINDOW (about));
		}
	} else {
		g_message ("Unhandled action '%s'", name);
	}
}

static void
window_delete_cb (GtkWidget   *widget,
		  GdkEventAny *event,
		  gpointer     user_data)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (DH_IS_WINDOW (widget));
	
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

	return TRUE;
}

static void
window_link_selected_cb (GObject *ignored, DhLink *link, DhWindow *window)
{
	DhWindowPriv   *priv;

	g_return_if_fail (link != NULL);
	g_return_if_fail (DH_IS_WINDOW (window));
	
	priv = window->priv;

	if (window_open_url (window, link->uri)) {
		dh_history_goto (priv->history, link->uri);
	}
}

static void
window_merge_add_widget (EggMenuMerge *merge,
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

static void
window_back_exists_changed_cb (DhHistory *history, 
			       gboolean   exists,
			       DhWindow  *window)
{
	DhWindowPriv *priv;
	EggAction *action;
		
	g_return_if_fail (DH_IS_HISTORY (history));
	g_return_if_fail (DH_IS_WINDOW (window));
	
	priv = window->priv;
	
	action = egg_action_group_get_action (priv->action_group, 
					      "BackAction");
	
	g_object_set (action, "sensitive", exists, NULL);
}

static void
window_forward_exists_changed_cb (DhHistory *history, 
				  gboolean   exists, 
				  DhWindow  *window)
{
	DhWindowPriv *priv;
	EggAction *action;
		
	g_return_if_fail (DH_IS_HISTORY (history));
	g_return_if_fail (DH_IS_WINDOW (window));
	
	priv = window->priv;
	
	action = egg_action_group_get_action (priv->action_group, 
					      "ForwardAction");
	
	g_object_set (action, "sensitive", exists, NULL);
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
	gtk_window_set_wmclass (GTK_WINDOW (window), "devhelp", "devhelp");

	if (geometry) {
		gtk_window_parse_geometry (GTK_WINDOW (window), geometry);
	} else {
		gtk_window_set_default_size (GTK_WINDOW (window), 700, 500);
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
dh_window_search (DhWindow *window, const gchar *str)
{
	DhWindowPriv *priv;
	
	g_return_if_fail (window != NULL);
	g_return_if_fail (DH_IS_WINDOW (window));
	
	priv = window->priv;

	dh_search_set_search_string (DH_SEARCH (priv->search), str);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), 1);
}

