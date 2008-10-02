/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
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

#include "config.h"
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

typedef enum {
	SEARCH_API    = 0,
	SEARCH_ENTRY
} DhSearchSource;

typedef struct {
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

	DhSearchSource  search_source;
} DhSearchPriv;

static void  dh_search_init                    (DhSearch         *search);
static void  dh_search_class_init              (DhSearchClass    *klass);
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

G_DEFINE_TYPE (DhSearch, dh_search, GTK_TYPE_VBOX);

#define GET_PRIVATE(instance) G_TYPE_INSTANCE_GET_PRIVATE \
  (instance, DH_TYPE_SEARCH, DhSearchPriv);

static gint signals[LAST_SIGNAL] = { 0 };

static void
dh_search_class_init (DhSearchClass *klass)
{
        GObjectClass   *object_class;
        GtkWidgetClass *widget_class;
	
        object_class = (GObjectClass *) klass;
        widget_class = (GtkWidgetClass *) klass;
	
	object_class->finalize = search_finalize;

        signals[LINK_SELECTED] =
                g_signal_new ("link_selected",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (DhSearchClass, link_selected),
			      NULL, NULL,
			      _dh_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);

	g_type_class_add_private (klass, sizeof (DhSearchPriv));
}

static void
dh_search_init (DhSearch *search)
{
	DhSearchPriv *priv;

	priv = GET_PRIVATE (search);
	
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
	
	priv = GET_PRIVATE (object);

	g_string_free (priv->book_str, TRUE);
	g_string_free (priv->page_str, TRUE);
	g_string_free (priv->entry_str, TRUE);

	g_completion_free (priv->completion);

	gconf_client = dh_base_get_gconf_client (dh_base_get ());	
	gconf_client_notify_remove (gconf_client, priv->advanced_options_id);

	G_OBJECT_CLASS (dh_search_parent_class)->finalize (object);
}

static void
search_advanced_options_setup (DhSearch *search)
{
	DhSearchPriv *priv = GET_PRIVATE (search);
	gboolean      advanced_options;
	GConfClient  *gconf_client;

	gconf_client = dh_base_get_gconf_client (dh_base_get ());
	
	advanced_options = gconf_client_get_bool (gconf_client,
						  GCONF_ADVANCED_OPTIONS,
						  NULL);
	if (advanced_options) {
		gtk_widget_show (priv->advanced_box);

		g_signal_handlers_block_by_func (priv->book, search_entry_changed_cb, search);
		g_signal_handlers_block_by_func (priv->page, search_entry_changed_cb, search);

		gtk_entry_set_text (GTK_ENTRY (priv->book), 
				    priv->book_str->len > 5 ? 
				    priv->book_str->str + 5 : "");
		gtk_entry_set_text (GTK_ENTRY (priv->page), 
				    priv->page_str->len > 5 ? 
				    priv->page_str->str + 5 : "");

		g_signal_handlers_unblock_by_func (priv->book, search_entry_changed_cb, search);
		g_signal_handlers_unblock_by_func (priv->page, search_entry_changed_cb, search);

	} else {
		gtk_widget_hide (priv->advanced_box);
	}		
}

static void
search_advanced_options_notify_cb (GConfClient *client,
				   guint        cnxn_id,
				   GConfEntry  *entry,
				   gpointer     user_data)
{
	DhSearch     *search;
	DhSearchPriv *priv;

	search = DH_SEARCH (user_data);
	priv = GET_PRIVATE (search);

	search_advanced_options_setup (search);

	/* Simulate a new search to update. */
	search_entry_activated_cb (GTK_ENTRY (priv->entry), search);
}

static void
search_selection_changed_cb (GtkTreeSelection *selection,
                             DhSearch         *search)
{
	DhSearchPriv *priv;
 	GtkTreeIter   iter;
	
	priv = GET_PRIVATE (search);

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

	priv = GET_PRIVATE (search);
	
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
	DhSearchPriv *priv = GET_PRIVATE (search);
	
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
	DhSearchPriv *priv = GET_PRIVATE (search);;
	GString      *string;

	string = g_string_new ("");

	if (GTK_WIDGET_VISIBLE (priv->advanced_box) ||
	    priv->search_source == SEARCH_API) {

		if (priv->book_str->len > 0) {
			g_string_append (string, priv->book_str->str);
			g_string_append (string, " ");
		}
		
		if (priv->page_str->len > 0) {
			g_string_append (string, priv->page_str->str);
			g_string_append (string, " ");
		}
	}

	if (priv->entry_str->len > 0) {
		g_string_append (string, priv->entry_str->str);
		g_string_append (string, " ");
	}
	
	return g_string_free (string, FALSE);
}

static void
search_update_string (DhSearch *search,
                      GtkEntry *entry)
{
	const gchar  *str = gtk_entry_get_text (entry);
	DhSearchPriv *priv = GET_PRIVATE (search);

	priv->search_source = SEARCH_ENTRY;
	
	if (GTK_WIDGET (entry) == priv->book) {
		if (str && str[0]) {
			g_string_printf (priv->book_str, "book:%s", str);
		} else {
			g_string_set_size (priv->book_str, 0);
		}
	} else if (GTK_WIDGET (entry) == priv->page) {
		if (str && str[0]) {
			g_string_printf (priv->page_str, "page:%s", str);
		} else {
			g_string_set_size (priv->page_str, 0);
		}
	} else {
		if (GTK_WIDGET_VISIBLE (priv->advanced_box) == FALSE) {
			g_string_set_size (priv->book_str, 0);
			g_string_set_size (priv->page_str, 0);
		}

		g_string_set_size (priv->entry_str, 0);
		if (str && str[0]) {
			g_string_append (priv->entry_str, str);
		}
	}
}

static void
search_entry_changed_cb (GtkEntry *entry,
                         DhSearch *search)
{
	DhSearchPriv *priv = GET_PRIVATE (search);
	
	d(g_print ("Entry changed\n"));

	search_update_string (search, entry);

	if (!priv->idle_filter) {
		priv->idle_filter =
			g_idle_add ((GSourceFunc) search_filter_idle, search);
	}
}

static void
search_entry_activated_cb (GtkEntry *entry,
                           DhSearch *search)
{
	DhSearchPriv *priv = GET_PRIVATE (search);
	DhLink       *link;
	gchar        *str;
	
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
	DhSearchPriv *priv = GET_PRIVATE (search);
	
	if (!priv->idle_complete) {
		priv->idle_complete = 
			g_idle_add ((GSourceFunc) search_complete_idle, 
				    search);
	}
}

static gboolean
search_complete_idle (DhSearch *search)
{
	DhSearchPriv *priv = GET_PRIVATE (search);
	const gchar  *text;
	gchar        *completed = NULL;
	GList        *list;
	gint          text_length;
	
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
	DhSearchPriv *priv = GET_PRIVATE (search);
	gchar        *str;
	DhLink       *link;
	
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

static void
search_cell_data_func (GtkTreeViewColumn *tree_column,
		       GtkCellRenderer   *cell,
		       GtkTreeModel      *tree_model,
		       GtkTreeIter       *iter,
		       gpointer           data)
{
	DhSearch     *search;
	DhSearchPriv *priv;
	gchar        *name;
	gboolean      is_deprecated;
	GdkColor     *color;

	search = data;
	priv = GET_PRIVATE (search);

	gtk_tree_model_get (tree_model, iter,
			    DH_KEYWORD_MODEL_COL_NAME, &name,
			    DH_KEYWORD_MODEL_COL_IS_DEPRECATED, &is_deprecated,
			    -1);

	if (is_deprecated) {
		color = &GTK_WIDGET (search)->style->text_aa[GTK_STATE_NORMAL];
	} else {
		color = NULL;
	}
	
	g_object_set (cell,
		      "text", name,
		      "foreground-gdk", color,
		      NULL);

	g_free (name);
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

	priv = GET_PRIVATE (search);

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

	gtk_tree_view_insert_column_with_data_func (
		GTK_TREE_VIEW (priv->hitlist),
		-1,
		NULL, 
		cell,
		search_cell_data_func,
		search, NULL);
	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->hitlist),
					   FALSE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (priv->hitlist),
					 DH_KEYWORD_MODEL_COL_NAME);

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
dh_search_set_search_string (DhSearch    *search,
                             const gchar *str)
{
	DhSearchPriv  *priv;
	gchar        **split, **leftover, *lower;
	gchar         *string = NULL;
	gint           i;

	g_return_if_fail (DH_IS_SEARCH (search));

	priv = GET_PRIVATE (search);

	priv->search_source = SEARCH_API;

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
				g_string_append (priv->book_str, split[i]);
				leftover++;
			} else if (!strncmp (lower, "page:", 5)) {
				g_string_append (priv->page_str, split[i]);
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

		if (string) {
			g_string_append (priv->entry_str, string);
		}

		if (string) {
			g_free (string);
		}

	} else if (str) {
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

	priv = GET_PRIVATE (search);

	gtk_widget_grab_focus (priv->entry);
}

