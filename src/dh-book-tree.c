/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@codefactory.se>
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
#include "dh-bookshelf.h"
#include "dh-book-tree.h"

static void book_tree_class_init           (DhBookTreeClass       *klass);
static void book_tree_init                 (DhBookTree            *tree);

static void book_tree_destroy              (GtkObject             *object);

static void book_tree_add_columns          (DhBookTree            *tree);
static void book_tree_setup_selection      (DhBookTree            *tree);
static void book_tree_populate_tree        (DhBookTree            *tree);
static void book_tree_insert_book_node     (DhBookTree            *tree,
					    GtkTreeIter           *parent_iter,
					    BookNode              *book_node);
static void book_tree_create_pixbufs       (DhBookTree            *tree);
static void book_tree_selection_changed_cb (GtkTreeSelection      *selection,
					    DhBookTree            *tree);

static GtkTreeViewClass *parent_class = NULL;

typedef struct {
        GdkPixbuf *pixbuf_opened;
        GdkPixbuf *pixbuf_closed;
        GdkPixbuf *pixbuf_helpdoc;
} DhBookTreePixbufs;

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

struct _DhBookTreePriv {
        DhBookshelf       *bookshelf;
	GtkTreeStore      *store;

	DhBookTreePixbufs *pixbufs;
	GHashTable        *node_rows;
};

GType
dh_book_tree_get_type (void)
{
        static GType type = 0;

        if (!type) {
                static const GTypeInfo info = {
                        sizeof (DhBookTreeClass),
			NULL,
			NULL,
                        (GClassInitFunc) book_tree_class_init,
			NULL,
			NULL,
                        sizeof (DhBookTree),
			0,
                        (GInstanceInitFunc) book_tree_init,
                };
		
		type = g_type_register_static (GTK_TYPE_TREE_VIEW,
					       "DhBookTree",
					       &info, 0);
        }

        return type;
}

static void
book_tree_class_init (DhBookTreeClass *klass)
{
        GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
        object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;
	
        parent_class = g_type_class_peek_parent (klass);
	
	object_class->destroy = book_tree_destroy;
	
        signals[URI_SELECTED] =
                g_signal_new ("uri_selected",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (DhBookTreeClass,
					       uri_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);

}

static void
book_tree_init (DhBookTree *tree)
{
        DhBookTreePriv *priv;
        
        priv            = g_new0 (DhBookTreePriv, 1);
        priv->bookshelf = NULL;
	priv->store     = gtk_tree_store_new (N_COLUMNS, 
					      GDK_TYPE_PIXBUF,
					      GDK_TYPE_PIXBUF,
					      G_TYPE_STRING,
					      G_TYPE_POINTER);

	priv->node_rows = g_hash_table_new (g_direct_hash,
					    g_direct_equal);
        tree->priv      = priv;

	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), 
				 GTK_TREE_MODEL (priv->store));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
	
	book_tree_create_pixbufs (tree);

	book_tree_add_columns (tree);

	book_tree_setup_selection (tree);
}

static void
book_tree_destroy (GtkObject *object)
{
	DhBookTree     *tree;
	DhBookTreePriv *priv;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (DH_IS_BOOK_TREE (object));
	
	tree = DH_BOOK_TREE (object);
	priv  = tree->priv;

	if (priv) {
		g_object_unref (priv->store);

		g_object_unref (priv->pixbufs->pixbuf_opened);
		g_object_unref (priv->pixbufs->pixbuf_closed);
		g_object_unref (priv->pixbufs->pixbuf_helpdoc);
		g_free (priv->pixbufs);
			
		g_free (priv);
		
		tree->priv = NULL;
	}
}

static void
book_tree_add_columns (DhBookTree *tree)
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

	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
}

static void
book_tree_setup_selection (DhBookTree *tree)
{
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	
	g_signal_connect (selection, "changed",
			  G_CALLBACK (book_tree_selection_changed_cb),
			  tree);
}

static void
book_tree_populate_tree (DhBookTree *tree)
{
        DhBookTreePriv *priv;
	GSList         *books;
	Book           *book;
	GSList         *chapters;
	gchar          *text[1];
	
	g_return_if_fail (tree != NULL);
        g_return_if_fail (DH_IS_BOOK_TREE (tree));

        priv  = tree->priv;
	books = dh_bookshelf_get_books (priv->bookshelf);

	for (; books; books = books->next) {
		book = BOOK (books->data);

		book_tree_insert_book_node (tree, NULL, book_get_root (book));
	}
}

static void
book_tree_insert_book_node (DhBookTree   *tree, 
			     GtkTreeIter *parent_iter,
                             BookNode    *book_node)
{
        DhBookTreePriv       *priv;
	GSList              *chapters;
	gchar               *text;
	Book                *book;
	Document            *document;
	GtkTreeIter          iter;
	GtkTreePath         *path;
	GtkTreeRowReference *row;
        
        g_return_if_fail (tree != NULL);
        g_return_if_fail (DH_IS_BOOK_TREE (tree));
	g_return_if_fail (book_node != NULL);

        priv    = tree->priv;

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
		book_tree_insert_book_node (tree, &iter,
					    (BookNode *) chapters->data);
	}
}

static void
book_tree_create_pixbufs (DhBookTree *tree)
{
 	DhBookTreePixbufs *pixbufs;
	
        g_return_if_fail (tree != NULL);
        g_return_if_fail (DH_IS_BOOK_TREE (tree));
	
	pixbufs = g_new0 (DhBookTreePixbufs, 1);
	
	pixbufs->pixbuf_closed = gdk_pixbuf_new_from_file (DATA_DIR "/images/devhelp/book_closed.png", NULL);
	pixbufs->pixbuf_opened = gdk_pixbuf_new_from_file (DATA_DIR "/images/devhelp/book_open.png", NULL);
	pixbufs->pixbuf_helpdoc = gdk_pixbuf_new_from_file (DATA_DIR "/images/devhelp/helpdoc.png", NULL);

        tree->priv->pixbufs = pixbufs;
}

static void
book_tree_selection_changed_cb (GtkTreeSelection *selection, DhBookTree *tree)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	BookNode     *book_node;
	GnomeVFSURI  *uri;
	
	g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
	g_return_if_fail (DH_IS_BOOK_TREE (tree));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (tree->priv->store), 
				    &iter, COL_NODE, &book_node, -1);

		
		if (book_node) {
			g_signal_handlers_block_by_func
				(selection, 
				 book_tree_selection_changed_cb,
				 tree);
 
			dh_bookshelf_open_document (tree->priv->bookshelf, 
						    book_node_get_document (book_node));

			uri = book_node_get_uri (book_node, NULL);

			g_signal_emit (G_OBJECT (tree),
				       signals[URI_SELECTED], 
				       0,
				       uri);
//			gnome_vfs_uri_unref (uri);
			g_signal_handlers_unblock_by_func
				(selection, 
				 book_tree_selection_changed_cb,
				 tree);
		}
	}
}

GtkWidget *
dh_book_tree_new (DhBookshelf *bookshelf)
{
        DhBookTree *tree;

	tree = g_object_new (DH_TYPE_BOOK_TREE, NULL);

        tree->priv->bookshelf = bookshelf;

        book_tree_populate_tree (tree);
	
        return GTK_WIDGET (tree);
}

void
dh_book_tree_open_node (DhBookTree *tree, BookNode *book_node)
{
	GtkTreeSelection    *selection;
	GtkTreeRowReference *row;
	GtkTreePath         *path;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

	row = g_hash_table_lookup (tree->priv->node_rows, book_node);
	g_return_if_fail (row != NULL);

	path = gtk_tree_row_reference_get_path (row);
	g_return_if_fail (path != NULL);
	
	g_signal_handlers_block_by_func
		(selection, 
		 book_tree_selection_changed_cb,
		 tree);

	gtk_tree_selection_select_path (selection, path);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree), path, NULL, 0);	

	g_signal_handlers_unblock_by_func
		(selection, 
		 book_tree_selection_changed_cb,
		 tree);

	gtk_tree_path_free (path);
}

void
dh_book_tree_add_book (DhBookTree *tree, Book *book)
{
	g_return_if_fail (tree != NULL);
        g_return_if_fail (DH_IS_BOOK_TREE (tree));
        g_return_if_fail (book != NULL);
        g_return_if_fail (IS_BOOK (book));

        book_tree_insert_book_node (tree, NULL, book_get_root (book));
}

void
dh_book_tree_remove_book (DhBookTree *tree, Book *book)
{
#if 0
        DhBookTreePriv   *priv;
        GtkCTreeNode    *node;
   
	g_return_if_fail (tree != NULL);
        g_return_if_fail (DH_IS_BOOK_TREE (tree));
        g_return_if_fail (book != NULL);
        g_return_if_fail (IS_BOOK (book));

        priv = tree->priv;
        
        gtk_clist_freeze (GTK_CLIST (tree));
   
        node = gtk_ctree_find_by_row_data (GTK_CTREE (tree),
					   NULL,
					   book_get_root (book));
   
        gtk_ctree_remove_node (GTK_CTREE (tree), node);
					
        gtk_clist_sort (GTK_CLIST (tree));
	
        gtk_clist_thaw (GTK_CLIST (tree));
#endif
}

GtkWidget *
dh_book_tree_get_scrolled (DhBookTree *tree)
{
	DhBookTreePriv   *priv;
 	GtkWidget       *sw;

 	g_return_val_if_fail (tree != NULL, NULL);
 	g_return_val_if_fail (DH_IS_BOOK_TREE (tree), NULL);

	priv = tree->priv;
	
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
					     GTK_SHADOW_IN);

	gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (tree));

	return sw;
}

