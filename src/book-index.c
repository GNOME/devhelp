/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
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
 *
 * Author: Mikael Hallendal <micke@codefactory.se>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreestore.h>
#include <libgnomevfs/gnome-vfs.h>
#include "book.h"
#include "book-index.h"

static void book_index_class_init         (BookIndexClass       *klass);
static void book_index_init               (BookIndex            *index);

static void book_index_destroy            (GtkObject            *object);

static void book_index_populate_tree      (BookIndex            *index);
static void book_index_insert_book_node   (BookIndex            *index,
					   GtkTreeIter          *parent_iter,
                                           BookNode             *book_node);
static void book_index_create_pixbufs     (BookIndex            *index);
static void book_index_selection_changed_cb (GtkTreeSelection *selection,
					     BookIndex *index);
static GtkTreeViewClass *parent_class = NULL;

typedef struct {
        GdkPixbuf   *pixbuf_opened;
        GdkPixbuf   *pixbuf_closed;
        GdkPixbuf   *pixbuf_helpdoc;
} BookIndexPixbufs;

enum {
        URI_SELECTED,
        LAST_SIGNAL
};

enum {
	COL_OPEN_PIXBUF,
	COL_CLOSED_PIXBUF,
	COL_TITLE,
	COL_NODE,
	N_COLUMNS
};

static gint signals[LAST_SIGNAL] = { 0 };

struct _BookIndexPriv {
        Bookshelf           *bookshelf;

	GtkTreeStore        *store;

	BookIndexPixbufs    *pixbufs;
	GHashTable          *node_rows;
};

GType
book_index_get_type (void)
{
        static GType book_index_type = 0;

        if (!book_index_type) {
                static const GTypeInfo book_index_info = {
                        sizeof (BookIndexClass),
			NULL,
			NULL,
                        (GClassInitFunc) book_index_class_init,
			NULL,
			NULL,
                        sizeof (BookIndex),
			0,
                        (GInstanceInitFunc) book_index_init,
                };
		
		book_index_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
							  "BookIndex",
							  &book_index_info, 0);
        }

        return book_index_type;
}

static void
book_index_class_init (BookIndexClass *klass)
{
        GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
        object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;
	
        parent_class = g_type_class_peek_parent (klass);
	
	object_class->destroy         = book_index_destroy;
	
        signals[URI_SELECTED] =
                g_signal_new ("uri_selected",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (BookIndexClass,
					       uri_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);

}

static void
book_index_add_columns (BookIndex *index)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	column = gtk_tree_view_column_new ();
	
	renderer = GTK_CELL_RENDERER (gtk_cell_renderer_pixbuf_new ());
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes
		(column, renderer,
		 "pixbuf", COL_OPEN_PIXBUF,
		 "pixbuf-expander-open", COL_OPEN_PIXBUF,
		 "pixbuf-expander-closed", COL_CLOSED_PIXBUF,
		 NULL);
	
	renderer = GTK_CELL_RENDERER (gtk_cell_renderer_text_new ());
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", COL_TITLE,
					     NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (index), column);
}

static void
book_index_setup_selection (BookIndex *index)
{
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (index));
	
	g_signal_connect (selection, "changed",
			  G_CALLBACK (book_index_selection_changed_cb),
			  index);
}

static void
book_index_init (BookIndex *index)
{
        BookIndexPriv   *priv;
        
        priv                  = g_new0 (BookIndexPriv, 1);
        priv->bookshelf       = NULL;
	priv->store           = gtk_tree_store_new (N_COLUMNS, 
						    GDK_TYPE_PIXBUF,
						    GDK_TYPE_PIXBUF,
						    G_TYPE_STRING,
						    G_TYPE_POINTER);
	priv->node_rows       = g_hash_table_new (g_direct_hash,
						  g_direct_equal);
        index->priv           = priv;

	gtk_tree_view_set_model (GTK_TREE_VIEW (index), 
				 GTK_TREE_MODEL (priv->store));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (index), FALSE);
	
	book_index_create_pixbufs (index);

	book_index_add_columns (index);

	book_index_setup_selection (index);
}

static void
book_index_destroy (GtkObject *object)
{
	BookIndex       *index;
	BookIndexPriv   *priv;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_BOOK_INDEX (object));
	
	index = BOOK_INDEX (object);
	priv  = index->priv;

	if (priv) {
		g_object_unref (priv->store);

		g_object_unref (priv->pixbufs->pixbuf_opened);
		g_object_unref (priv->pixbufs->pixbuf_closed);
		g_object_unref (priv->pixbufs->pixbuf_helpdoc);
		g_free (priv->pixbufs);
			
		g_free (priv);
		
		index->priv = NULL;
	}
}

static void
book_index_populate_tree (BookIndex *index)
{
        BookIndexPriv   *priv;
	GSList          *books;
	Book            *book;
	GSList          *chapters;
	gchar           *text[1];
	
	g_return_if_fail (index != NULL);
        g_return_if_fail (IS_BOOK_INDEX (index));

        priv  = index->priv;
	books = bookshelf_get_books (priv->bookshelf);

	for (; books; books = books->next) {
		book = BOOK (books->data);

		book_index_insert_book_node (index, NULL,
                                             book_get_root (book));
	}
}

static void
book_index_insert_book_node (BookIndex      *index, 
			     GtkTreeIter    *parent_iter,
                             BookNode       *book_node)
{
        BookIndexPriv       *priv;
	GSList              *chapters;
	gchar               *text;
	Book                *book;
	Document            *document;
	GtkTreeIter          iter;
	GtkTreePath         *path;
	GtkTreeRowReference *row;
        
        g_return_if_fail (index != NULL);
        g_return_if_fail (IS_BOOK_INDEX (index));
	g_return_if_fail (book_node != NULL);

        priv    = index->priv;

	text = (gchar *) book_node_get_title (book_node);

	if (book_node_is_chapter (book_node)) {
		document = book_node_get_document (book_node);
		book = document_get_book (document);

		/* Is the book hidden, then skip */
		if (book_is_visible (book) == FALSE) {
			return;
		}

		gtk_tree_store_append (priv->store, &iter, parent_iter);
		gtk_tree_store_set (priv->store, &iter, 
				    COL_OPEN_PIXBUF, 
				    priv->pixbufs->pixbuf_opened,
				    COL_CLOSED_PIXBUF, 
				    priv->pixbufs->pixbuf_closed,
				    COL_TITLE, 
				    text, 
				    COL_NODE, 
				    book_node,
				    -1);
	} else {
		gtk_tree_store_append (priv->store, &iter, parent_iter);
		gtk_tree_store_set (priv->store, &iter, 
				    COL_OPEN_PIXBUF, 
				    priv->pixbufs->pixbuf_helpdoc,
				    COL_CLOSED_PIXBUF, 
				    priv->pixbufs->pixbuf_helpdoc,
				    COL_TITLE, 
				    text, 
				    COL_NODE,
				    book_node,
				    -1);
	}
	
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store), &iter);
	row = gtk_tree_row_reference_new (GTK_TREE_MODEL (priv->store), path);
	gtk_tree_path_free (path);

	g_hash_table_insert (priv->node_rows, book_node, row);

	chapters = book_node_get_contents (book_node);
	
	for (; chapters; chapters = chapters->next) {
		book_index_insert_book_node (index, &iter,
                                             (BookNode *) chapters->data);
	}
}

static void
book_index_create_pixbufs (BookIndex *index)
{
 	BookIndexPixbufs   *pixbufs;

        g_return_if_fail (index != NULL);
        g_return_if_fail (IS_BOOK_INDEX (index));
	
	pixbufs = g_new0 (BookIndexPixbufs, 1);
	
	pixbufs->pixbuf_closed = gdk_pixbuf_new_from_file (DATA_DIR "/images/devhelp/book_closed.png", NULL);
	pixbufs->pixbuf_opened = gdk_pixbuf_new_from_file (DATA_DIR "/images/devhelp/book_open.png", NULL);
	pixbufs->pixbuf_helpdoc = gdk_pixbuf_new_from_file (DATA_DIR "/images/devhelp/helpdoc.png", NULL);

        index->priv->pixbufs = pixbufs;
}

GtkWidget *
book_index_new (Bookshelf *bookshelf)
{
        BookIndex       *index;
	index = g_object_new (TYPE_BOOK_INDEX, NULL);

        index->priv->bookshelf = bookshelf;

        book_index_populate_tree (index);
	
        return GTK_WIDGET (index);
}

static void
book_index_selection_changed_cb (GtkTreeSelection *selection,
				 BookIndex *index)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	BookNode     *book_node;
	GnomeVFSURI  *uri;
	
	g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
	g_return_if_fail (IS_BOOK_INDEX (index));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (index->priv->store), 
				    &iter, COL_NODE, &book_node, -1);

		
		if (book_node) {
			g_signal_handlers_block_by_func
				(selection, 
				 book_index_selection_changed_cb,
				 index);
 
			bookshelf_open_document (index->priv->bookshelf, 
						 book_node_get_document (book_node));

			uri = book_node_get_uri (book_node, NULL);

			g_signal_emit (G_OBJECT (index),
				       signals[URI_SELECTED], 
				       0,
				       uri);
//			gnome_vfs_uri_unref (uri);
			g_signal_handlers_unblock_by_func
				(selection, 
				 book_index_selection_changed_cb,
				 index);
		}
	}
}

void
book_index_open_node (BookIndex *index, BookNode *book_node)
{
	GtkTreeSelection    *selection;
	GtkTreeRowReference *row;
	GtkTreePath         *path;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (index));

	row = g_hash_table_lookup (index->priv->node_rows, book_node);
	g_return_if_fail (row != NULL);

	path = gtk_tree_row_reference_get_path (row);
	g_return_if_fail (path != NULL);
	
	g_signal_handlers_block_by_func
		(selection, 
		 book_index_selection_changed_cb,
		 index);

	gtk_tree_selection_select_path (selection, path);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (index), path, NULL, 0);	

	g_signal_handlers_unblock_by_func
		(selection, 
		 book_index_selection_changed_cb,
		 index);

	gtk_tree_path_free (path);
}

void
book_index_add_book (BookIndex *index, Book *book)
{
	g_return_if_fail (index != NULL);
        g_return_if_fail (IS_BOOK_INDEX (index));
        g_return_if_fail (book != NULL);
        g_return_if_fail (IS_BOOK (book));

        book_index_insert_book_node (index, NULL, book_get_root (book));
}

void
book_index_remove_book (BookIndex *index, Book *book)
{
#if 0
        BookIndexPriv   *priv;
        GtkCTreeNode    *node;
   
	g_return_if_fail (index != NULL);
        g_return_if_fail (IS_BOOK_INDEX (index));
        g_return_if_fail (book != NULL);
        g_return_if_fail (IS_BOOK (book));

        priv = index->priv;
        
        gtk_clist_freeze (GTK_CLIST (index));
   
        node = gtk_ctree_find_by_row_data (GTK_CTREE (index),
					   NULL,
					   book_get_root (book));
   
        gtk_ctree_remove_node (GTK_CTREE (index), node);
					
        gtk_clist_sort (GTK_CLIST (index));
	
        gtk_clist_thaw (GTK_CLIST (index));
#endif
}

GtkWidget *
book_index_get_scrolled (BookIndex *index)
{
	BookIndexPriv   *priv;
 	GtkWidget       *sw;

 	g_return_val_if_fail (index != NULL, NULL);
 	g_return_val_if_fail (IS_BOOK_INDEX (index), NULL);

	priv = index->priv;
	
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
					     GTK_SHADOW_IN);

	gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (index));

	return sw;
}

