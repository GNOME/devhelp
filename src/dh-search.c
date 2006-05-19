/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2005-2006 Imendio AB
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

#include "dh-marshal.h"
#include "dh-keyword-model.h"
#include "dh-search.h"
#include "dh-preferences.h"
#include "dh-base.h"

#define d(x)

struct _DhSearchPriv {
	DhKeywordModel *model;

	DhLink         *selected_link;
	
	GtkWidget      *advanced_box;

	GtkWidget      *book;
	GtkWidget      *page;
	GtkWidget      *entry;
	GtkWidget      *hitlist;

	GCompletion    *completion;

	guint           idle_complete;
	guint           idle_filter;

	gboolean        first;

	guint           advanced_options_id;
	
	GString        *book_str;
	GString        *page_str;
	GString        *entry_str;
};


static void  search_init                       (DhSearch         *search);
static void  search_class_init                 (DhSearchClass    *klass);
static void  search_finalize                   (GObject          *object);
static void  search_advanced_options_setup     (DhSearch         *search);
static void  search_advanced_options_notify_cb (GConfClient      *client,
						guint             cnxn_id,
						GConfEntry       *entry,
						gpointer          user_data);
static void  search_selection_changed_cb       (GtkTreeSelection *selection,
						DhSearch         *content);
static gboolean search_tree_button_press_cb    (GtkTreeView      *view,
						GdkEventButton   *event,
						DhSearch         *search);
static gboolean search_entry_key_press_event_cb (GtkEntry        *entry,
						 GdkEventKey     *event,
						 DhSearch        *search);
static void  search_entry_changed_cb           (GtkEntry         *entry,
						DhSearch         *search);
static void  search_entry_activated_cb         (GtkEntry         *entry,
						DhSearch         *search);
static void  search_entry_text_inserted_cb     (GtkEntry         *entry,
						const gchar      *text,
						gint              length,
						gint             *position,
						DhSearch         *search);
static gboolean search_complete_idle           (DhSearch         *search);
static gboolean search_filter_idle             (DhSearch         *search);
static gchar *  search_complete_func           (DhLink           *link);
static gchar *  search_get_search_string       (DhSearch         *search);

enum {
        LINK_SELECTED,
        LAST_SIGNAL
};

static GtkVBox *parent_class;
static gint     signals[LAST_SIGNAL] = { 0 };

GType
dh_search_get_type (void)
{
        static GType type = 0;

        if (!type) {
                static const GTypeInfo info =
                        {
                                sizeof (DhSearchClass),
                                NULL,
                                NULL,
                                (GClassInitFunc) search_class_init,
                                NULL,
                                NULL,
                                sizeof (DhSearch),
                                0,
                                (GInstanceInitFunc) search_init,
                        };
                
                type = g_type_register_static (GTK_TYPE_VBOX,
					       "DhSearch", 
					       &info, 0);
        }
        
        return type;
}

static void
search_class_init (DhSearchClass *klass)
{
        GObjectClass   *object_class;
        GtkWidgetClass *widget_class;
	
        parent_class = g_type_class_peek_parent (klass);

        object_class = (GObjectClass *) klass;
        widget_class = (GtkWidgetClass *) klass;
	
	object_class->finalize = search_finalize;

        signals[LINK_SELECTED] =
                g_signal_new ("link_selected",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (DhSearchClass, link_selected),
			      NULL, NULL,
			      dh_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);
}

static void
search_init (DhSearch *search)
{
	DhSearchPriv *priv;

	priv = g_new0 (DhSearchPriv, 1);
	search->priv = priv;
	
	priv->book_str = g_string_new ("");
	priv->page_str = g_string_new ("");
	priv->entry_str = g_string_new ("");

	priv->idle_complete = 0;
	priv->idle_filter   = 0;

	priv->completion = 
		g_completion_new ((GCompletionFunc) search_complete_func);

	priv->hitlist = gtk_tree_view_new ();
	priv->model   = dh_keyword_model_new ();

	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->hitlist),
				 GTK_TREE_MODEL (priv->model));

	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (priv->hitlist), FALSE);

	gtk_box_set_spacing (GTK_BOX (search), 2);
}

static void
search_finalize (GObject *object)
{
	DhSearchPriv *priv;
	GConfClient  *gconf_client;
	
	priv = DH_SEARCH (object)->priv;

	g_string_free (priv->book_str, TRUE);
	g_string_free (priv->page_str, TRUE);
	g_string_free (priv->entry_str, TRUE);

	gconf_client = dh_base_get_gconf_client (dh_base_get ());	
	gconf_client_notify_remove (gconf_client, priv->advanced_options_id);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
search_advanced_options_setup (DhSearch *search)
{
	DhSearchPriv *priv;
	gboolean      advanced_options;
	GConfClient  *gconf_client;

	priv = search->priv;

	gconf_client = dh_base_get_gconf_client (dh_base_get ());
	
	advanced_options = gconf_client_get_bool (gconf_client,
						  GCONF_ADVANCED_OPTIONS,
						  NULL);
	if (advanced_options) {
		gtk_widget_show (priv->advanced_box);
	} else {
		gtk_widget_hide (priv->advanced_box);
	}		
}

static void
search_advanced_options_notify_cb (GConfClient     *client,
				   guint            cnxn_id,
				   GConfEntry      *entry,
				   gpointer         user_data)
{
	DhSearch     *search;
	DhSearchPriv *priv;

	search = DH_SEARCH (user_data);
	priv = search->priv;

	search_advanced_options_setup (search);

	/* Simulate a new search to update. */
	search_entry_activated_cb (GTK_ENTRY (priv->entry), search);
}

static void
search_selection_changed_cb (GtkTreeSelection *selection, DhSearch *search)
{
	DhSearchPriv *priv;
 	GtkTreeIter   iter;
	
	priv = search->priv;

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		DhLink *link;
		
		gtk_tree_model_get (GTK_TREE_MODEL (priv->model), &iter,
				    DH_KEYWORD_MODEL_COL_LINK, &link,
				    -1);

		if (link != priv->selected_link) {
			priv->selected_link = link;
		
			d(g_print ("Emiting signal with link to: %s (%s)\n",
				   link->name, link->uri));
		
			g_signal_emit (search, signals[LINK_SELECTED], 0, link);
		}
	}
}

/* Make it possible to jump back to the currently selected item, useful when the
 * html view has been scrolled away.
 */
static gboolean
search_tree_button_press_cb (GtkTreeView    *view,
			     GdkEventButton *event,
			     DhSearch       *search)
{
	GtkTreePath  *path;
	GtkTreeIter   iter;
	DhSearchPriv *priv;
	DhLink       *link;

	priv = search->priv;
	
	gtk_tree_view_get_path_at_pos (view, event->x, event->y, &path,
				       NULL, NULL, NULL);
	if (!path) {
		return FALSE;
	}
	
	gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->model), &iter, path);
	gtk_tree_path_free (path);
	
	gtk_tree_model_get (GTK_TREE_MODEL (priv->model),
			    &iter,
			    DH_KEYWORD_MODEL_COL_LINK, &link,
			    -1);

	priv->selected_link = link;
	
	g_signal_emit (search, signals[LINK_SELECTED], 0, link);
	
	/* Always return FALSE so the tree view gets the event and can update
	 * the selection etc.
	 */
	return FALSE;
}

static gboolean
search_entry_key_press_event_cb (GtkEntry    *entry,
				 GdkEventKey *event,
				 DhSearch    *search)
{
	DhSearchPriv *priv;
	
	priv = search->priv;
	
	if (event->keyval == GDK_Tab) {
		if (event->state & GDK_CONTROL_MASK) {
			gtk_widget_grab_focus (priv->hitlist);
		} else {
			gtk_editable_set_position (GTK_EDITABLE (entry), -1);
			gtk_editable_select_region (GTK_EDITABLE (entry), -1, -1);
		}
		return TRUE;
	}

	if (event->keyval == GDK_Return ||
	    event->keyval == GDK_KP_Enter) {
		GtkTreeIter  iter;
		DhLink      *link;
		gchar       *name;
		
		/* Get the first entry found. */
		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->model), &iter)) {
			gtk_tree_model_get (GTK_TREE_MODEL (priv->model),
					    &iter,
					    DH_KEYWORD_MODEL_COL_LINK, &link,
					    DH_KEYWORD_MODEL_COL_NAME, &name,
					    -1);
			
			gtk_entry_set_text (GTK_ENTRY (entry), name);
			g_free (name);
			
			gtk_editable_set_position (GTK_EDITABLE (entry), -1);
			gtk_editable_select_region (GTK_EDITABLE (entry), -1, -1);
			
			g_signal_emit (search, signals[LINK_SELECTED], 0, link);
			
			return TRUE;
		}
	}
	
	return FALSE;
}

static gchar *
search_get_search_string (DhSearch *search)
{
	DhSearchPriv *priv;
	GString      *string;

	priv = search->priv;

	string = g_string_new ("");

	if (priv->book_str->len > 0) {
		g_string_append (string, priv->book_str->str);
		g_string_append (string, " ");
	}

	if (priv->page_str->len > 0) {
		g_string_append (string, priv->page_str->str);
		g_string_append (string, " ");
	}

	if (priv->entry_str->len > 0) {
		g_string_append (string, priv->entry_str->str);
		g_string_append (string, " ");
	}

	return g_string_free (string, FALSE);
}

static void
search_update_string (DhSearch *search, GtkEntry *entry)
{
	const gchar  *str = gtk_entry_get_text (entry);
	DhSearchPriv *priv;

	priv = search->priv;
	
	if (GTK_WIDGET (entry) == priv->book) {
		if (str && str[0]) {
			g_string_printf (priv->book_str, "book:%s", str ? str : "");
		} else {
			g_string_set_size (priv->book_str, 0);
		}
	} else if (GTK_WIDGET (entry) == priv->page) {
		if (str && str[0]) {
			g_string_printf (priv->page_str, "page:%s", str ? str : "");
		} else {
			g_string_set_size (priv->page_str, 0);
		}
	} else {
		g_string_set_size (priv->entry_str, 0);
		g_string_append (priv->entry_str, str);
	}
}

static void
search_entry_changed_cb (GtkEntry *entry, DhSearch *search)
{
	DhSearchPriv *priv;
	
	priv = search->priv;
 
	d(g_print ("Entry changed\n"));

	search_update_string (search, entry);

	if (!priv->idle_filter) {
		priv->idle_filter =
			g_idle_add ((GSourceFunc) search_filter_idle, search);
	}
}

static void
search_entry_activated_cb (GtkEntry *entry, DhSearch *search)
{
	DhSearchPriv *priv;
	DhLink       *link;
	gchar        *str;
	
	priv = search->priv;
	
	str = search_get_search_string (search);

	link = dh_keyword_model_filter (priv->model, str);

	g_free (str);
}

static void
search_entry_text_inserted_cb (GtkEntry    *entry,
			       const gchar *text,
			       gint         length,
			       gint        *position,
			       DhSearch    *search)
{
	DhSearchPriv *priv;
	
	priv = search->priv;
	
	if (!priv->idle_complete) {
		priv->idle_complete = 
			g_idle_add ((GSourceFunc) search_complete_idle, 
				    search);
	}
}

static gboolean
search_complete_idle (DhSearch *search)
{
	DhSearchPriv *priv;
	const gchar  *text;
	gchar        *completed = NULL;
	GList        *list;
	gint          text_length;
	
	priv = search->priv;
	
	text = gtk_entry_get_text (GTK_ENTRY (priv->entry));

	list = g_completion_complete (priv->completion,
				      (gchar *)text,
				      &completed);

	if (completed) {
		text_length = strlen (text);
		
		gtk_entry_set_text (GTK_ENTRY (priv->entry), completed);
 		gtk_editable_set_position (GTK_EDITABLE (priv->entry),
 					   text_length);
		gtk_editable_select_region (GTK_EDITABLE (priv->entry),
					    text_length, -1);
	}
	
	priv->idle_complete = 0;

	return FALSE;
}

static gboolean
search_filter_idle (DhSearch *search)
{
	DhSearchPriv *priv;
	gchar        *str;
	DhLink       *link;
	
	priv = search->priv;

	d(g_print ("Filter idle\n"));
	
	str = search_get_search_string (search);
	link = dh_keyword_model_filter (priv->model, str);
	g_free (str);

	priv->idle_filter = 0;

	if (link) {
		g_signal_emit (search, signals[LINK_SELECTED], 0, link);
	}

	return FALSE;
}

static gchar *
search_complete_func (DhLink *link)
{
	return link->name;
}

GtkWidget *
dh_search_new (GList *keywords)
{
	DhSearch         *search;
	DhSearchPriv     *priv;
	GtkTreeSelection *selection;
        GtkWidget        *list_sw;
	GtkWidget        *frame;
	GtkWidget        *hbox;
	GtkWidget        *book_label, *page_label;
	GtkSizeGroup     *group;
	GtkCellRenderer  *cell;
	GConfClient      *gconf_client;
		
	search = g_object_new (DH_TYPE_SEARCH, NULL);

	priv = search->priv;

	gtk_container_set_border_width (GTK_CONTAINER (search), 2);

	/* Setup the book box */
	priv->book = gtk_entry_new ();
	g_signal_connect (priv->book, "changed", 
			  G_CALLBACK (search_entry_changed_cb),
			  search);
	g_signal_connect (priv->book, "activate",
			  G_CALLBACK (search_entry_activated_cb),
			  search);

	book_label = gtk_label_new_with_mnemonic (_("_Book:"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (book_label), priv->book);

	priv->advanced_box = gtk_vbox_new (FALSE, 2);
 	gtk_box_pack_start (GTK_BOX (search), priv->advanced_box, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), book_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->book, TRUE, TRUE, 0);
 	gtk_box_pack_start (GTK_BOX (priv->advanced_box), hbox, FALSE, FALSE, 0);

	/* Setup the page box */
	priv->page = gtk_entry_new ();
	g_signal_connect (priv->page, "changed", 
			  G_CALLBACK (search_entry_changed_cb),
			  search);
	g_signal_connect (priv->page, "activate",
			  G_CALLBACK (search_entry_activated_cb),
			  search);

	page_label = gtk_label_new_with_mnemonic (_("_Page:"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (page_label), priv->page);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), page_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->page, TRUE, TRUE, 0);
 	gtk_box_pack_start (GTK_BOX (priv->advanced_box), hbox, FALSE, FALSE, 0);
	
	/* Align the labels */
	group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (group, book_label);
	gtk_size_group_add_widget (group, page_label);
	g_object_unref (G_OBJECT (group));

	gtk_widget_show_all (priv->advanced_box);
	gtk_widget_set_no_show_all (priv->advanced_box, TRUE);

	/* Setup the keyword box */
	priv->entry = gtk_entry_new ();
	g_signal_connect (priv->entry, "key_press_event",
			  G_CALLBACK (search_entry_key_press_event_cb),
			  search);

	g_signal_connect (priv->hitlist, "button_press_event",
			  G_CALLBACK (search_tree_button_press_cb),
			  search);

	g_signal_connect (priv->entry, "changed", 
			  G_CALLBACK (search_entry_changed_cb),
			  search);

	g_signal_connect (priv->entry, "activate",
			  G_CALLBACK (search_entry_activated_cb),
			  search);
	
	g_signal_connect (priv->entry, "insert_text",
			  G_CALLBACK (search_entry_text_inserted_cb),
			  search);

	gtk_box_pack_start (GTK_BOX (search), priv->entry, FALSE, FALSE, 0);

	/* Setup the hitlist */
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	
        list_sw = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (list_sw),
                                        GTK_POLICY_NEVER, 
                                        GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (frame), list_sw);

	cell = gtk_cell_renderer_text_new ();
	g_object_set (cell,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      NULL);
	
	gtk_tree_view_insert_column_with_attributes (
		GTK_TREE_VIEW (priv->hitlist), -1,
		_("Section"), cell,
		"text", 0,
		NULL);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->hitlist),
					   FALSE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (priv->hitlist), FALSE);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->hitlist));

	g_signal_connect (selection, "changed",
			  G_CALLBACK (search_selection_changed_cb),
			  search);
	
	gtk_container_add (GTK_CONTAINER (list_sw), priv->hitlist);

	gtk_box_pack_end_defaults (GTK_BOX (search), frame);

	g_completion_add_items (priv->completion, keywords);
	dh_keyword_model_set_words (priv->model, keywords);

	gtk_widget_show_all (GTK_WIDGET (search));
	
	gconf_client = dh_base_get_gconf_client (dh_base_get ());
	priv->advanced_options_id = gconf_client_notify_add (gconf_client,
							     GCONF_ADVANCED_OPTIONS,
							     search_advanced_options_notify_cb,
							     search, NULL, NULL);

	search_advanced_options_setup (search);
	
	return GTK_WIDGET (search);
}

void
dh_search_set_search_string (DhSearch *search, const gchar *str)
{
	DhSearchPriv  *priv;
	gchar        **split, **leftover, *lower;
	gchar         *string = NULL;
	gint           i;

	g_return_if_fail (DH_IS_SEARCH (search));

	priv = search->priv;

	g_string_set_size (priv->book_str, 0);
	g_string_set_size (priv->page_str, 0);
	g_string_set_size (priv->entry_str, 0);

	g_signal_handlers_block_by_func (priv->book, search_entry_changed_cb, search);
	g_signal_handlers_block_by_func (priv->page, search_entry_changed_cb, search);
	g_signal_handlers_block_by_func (priv->entry, search_entry_changed_cb, search);

	if ((leftover = split = g_strsplit (str, " ", -1)) != NULL) {

		for (i = 0; split[i] != NULL; i++) {

			lower = g_ascii_strdown (split[i], -1);
			
			/* Determine if there was a book or page specification
			 */
			if (!strncmp (lower, "book:", 5)) {
				g_string_append (priv->book_str, lower);
				leftover++;
			} else if (!strncmp (lower, "page:", 5)) {

				g_string_append (priv->page_str, lower);
				leftover++;
			} else {
				/* No more specifications */
				break;
			}
			
			g_free (lower);
		}

		/* Collect the search string */
		string = NULL;
		for (i = 0; leftover[i] != NULL; i++) {
			if (string == NULL) {
				string = g_strdup (leftover[i]);
			} else { 
				lower = g_strdup_printf ("%s %s", string, leftover[i]);
				g_free (string);
				string = lower;
			}
		}

		g_strfreev (split);
		
		g_string_append (priv->entry_str, string);

		if (string) {
			g_free (string);
		}

	} else {
		g_string_append (priv->entry_str, str);
	}
	
	gtk_entry_set_text (GTK_ENTRY (priv->entry), 
			    priv->entry_str->str);

	if (GTK_WIDGET_VISIBLE (priv->advanced_box)) {
		gtk_entry_set_text (GTK_ENTRY (priv->book), 
				    priv->book_str->len > 5 ? 
				    priv->book_str->str + 5 : "");
		gtk_entry_set_text (GTK_ENTRY (priv->page), 
				    priv->page_str->len > 5 ? 
				    priv->page_str->str + 5 : "");
	}

	gtk_editable_set_position (GTK_EDITABLE (priv->entry), -1);
	gtk_editable_select_region (GTK_EDITABLE (priv->entry), -1, -1);

	g_signal_handlers_unblock_by_func (priv->book, search_entry_changed_cb, search);
	g_signal_handlers_unblock_by_func (priv->page, search_entry_changed_cb, search);
	g_signal_handlers_unblock_by_func (priv->entry, search_entry_changed_cb, search);

	if (!priv->idle_filter) {
		priv->idle_filter =
			g_idle_add ((GSourceFunc) search_filter_idle, search);
	}
}

void
dh_search_grab_focus (DhSearch *search)
{
	DhSearchPriv *priv;
	
	g_return_if_fail (DH_IS_SEARCH (search));

	priv = search->priv;

	gtk_widget_grab_focus (priv->entry);
}

