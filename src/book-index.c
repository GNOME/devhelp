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

/* FIXES:
 * 
 * Destroy
 */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkctree.h>
#include <gtk/gtksignal.h>
#include "book.h"
#include "book-index.h"

static void book_index_class_init         (BookIndexClass     *klass);
static void book_index_init               (BookIndex          *index);

static void book_index_populate_tree      (BookIndex          *index);
static void book_index_insert_book_node   (BookIndex          *index,
                                           GtkCTreeNode       *parent,
                                           BookNode           *book_node);

static void book_index_create_pixmaps     (BookIndex          *index);

static void book_index_open_node          (BookIndex          *index,
                                           BookNode           *node);

static void book_index_select_row         (GtkCTree           *ctree,
                                           GtkCTreeNode       *node,
                                           gint                column);

static GtkCTreeClass *parent_class = NULL;

enum {
        URI_SELECTED,
        LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

typedef struct {
	GdkPixmap   *pixmap_opened;
	GdkPixmap   *pixmap_closed;
	GdkPixmap   *pixmap_helpdoc;
	GdkBitmap   *mask_opened;
	GdkBitmap   *mask_closed;
	GdkBitmap   *mask_helpdoc;
} BookIndexPixmaps;

struct _BookIndexPriv {
        Bookshelf          *bookshelf;

	gboolean            emit_uri_select;

        BookIndexPixmaps   *pixmaps;
};

GtkType
book_index_get_type (void)
{
        static GtkType book_index_type = 0;

        if (!book_index_type) {
                static const GtkTypeInfo book_index_info = {
                        "BookIndex",
                        sizeof (BookIndex),
                        sizeof (BookIndexClass),
                        (GtkClassInitFunc) book_index_class_init,
                        (GtkObjectInitFunc) book_index_init,
                        /* reserved_1 */ NULL,
                        /* reserved_2 */ NULL,
                        (GtkClassInitFunc) NULL,
                };

                book_index_type = gtk_type_unique (gtk_ctree_get_type (), 
                                                   &book_index_info);
        }

        return book_index_type;
}

static void
book_index_class_init (BookIndexClass *klass)
{
        GtkObjectClass *object_class;
        GtkCTreeClass  *ctree_class;

        object_class = (GtkObjectClass *) klass;
        ctree_class  = (GtkCTreeClass *)  klass;

        parent_class = gtk_type_class (gtk_ctree_get_type ());

        ctree_class->tree_select_row = book_index_select_row;

        signals[URI_SELECTED] =
                gtk_signal_new ("uri_selected",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (BookIndexClass,
                                                   uri_selected),
                                gtk_marshal_NONE__POINTER,
                                GTK_TYPE_NONE,
                                1, GTK_TYPE_POINTER);
        
        gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
}

static void
book_index_init (BookIndex *index)
{
        BookIndexPriv   *priv;
        
        priv                  = g_new0 (BookIndexPriv, 1);
        priv->bookshelf       = NULL;
        priv->pixmaps         = NULL;
	priv->emit_uri_select = TRUE;
        index->priv           = priv;

        book_index_create_pixmaps (index);
}

static void
book_index_populate_tree (BookIndex *index)
{
        BookIndexPriv   *priv;
	GSList          *books;
	Book            *book;
	GSList          *chapters;
	gchar           *text[1];
	GtkCTreeNode    *ctree_node;
	
	g_return_if_fail (index != NULL);
        g_return_if_fail (IS_BOOK_INDEX (index));

        priv  = index->priv;
	books = bookshelf_get_books (priv->bookshelf);

	gtk_clist_freeze (GTK_CLIST (index));
	
	for (; books; books = books->next) {
		book = BOOK (books->data);

		book_index_insert_book_node (index, NULL,
                                             book_get_root (book));
	}

	gtk_clist_thaw (GTK_CLIST (index));
}

static void
book_index_insert_book_node (BookIndex      *index, 
                             GtkCTreeNode   *parent, 
                             BookNode       *book_node)
{
        BookIndexPriv      *priv;
	GSList             *chapters;
	gchar              *text;
	GtkCTreeNode       *ctree_node = NULL;
	Book               *book;
	Document           *document;
        BookIndexPixmaps   *pixmaps;
        
        g_return_if_fail (index != NULL);
        g_return_if_fail (IS_BOOK_INDEX (index));
	g_return_if_fail (book_node != NULL);

        priv    = index->priv;
        pixmaps = priv->pixmaps;

	text = (gchar *) book_node_get_title (book_node);

	if (book_node_is_chapter (book_node)) {
		document = book_node_get_document (book_node);
		book = document_get_book (document);

		/* Is the book hidden, then skip */
		if (book_is_visible (book) == FALSE) {
			return;
		}
	
		ctree_node = gtk_ctree_insert_node (GTK_CTREE (index),
						    parent,
						    NULL,
						    &text,
						    5,
						    pixmaps->pixmap_closed, 
						    pixmaps->mask_closed,
						    pixmaps->pixmap_opened, 
						    pixmaps->mask_opened, 
						    FALSE,
						    FALSE);
	} else {
		ctree_node = gtk_ctree_insert_node (GTK_CTREE (index),
						    parent,
						    NULL,
						    &text,
						    5,
						    pixmaps->pixmap_helpdoc, 
						    pixmaps->mask_helpdoc,
						    pixmaps->pixmap_helpdoc, 
						    pixmaps->mask_helpdoc,
						    FALSE,
						    FALSE);
	}
	
	gtk_ctree_node_set_row_data (GTK_CTREE (index), ctree_node, book_node);
	
	chapters = book_node_get_contents (book_node);
	
	for (; chapters; chapters = chapters->next) {
		book_index_insert_book_node (index, ctree_node,
                                             (BookNode *) chapters->data);
	}
}

static void
book_index_create_pixmaps (BookIndex *index)
{
 	BookIndexPixmaps   *pixmaps;
	GdkPixbuf          *pixbuf;

        g_return_if_fail (index != NULL);
        g_return_if_fail (IS_BOOK_INDEX (index));
	
	pixmaps = g_new0 (BookIndexPixmaps, 1);
	
	pixbuf = gdk_pixbuf_new_from_file (DATA_DIR "/images/devhelp/book_red.xpm");
	gdk_pixbuf_render_pixmap_and_mask (pixbuf,
					   &pixmaps->pixmap_closed,
					   &pixmaps->mask_closed,
					   127);
					   
	pixbuf = gdk_pixbuf_new_from_file (DATA_DIR "/images/devhelp/book_open.xpm");
	gdk_pixbuf_render_pixmap_and_mask (pixbuf,
					   &pixmaps->pixmap_opened,
					   &pixmaps->mask_opened,
					   127);
	
	pixbuf = gdk_pixbuf_new_from_file (DATA_DIR "/images/devhelp/helpdoc.xpm");
	gdk_pixbuf_render_pixmap_and_mask (pixbuf,
					   &pixmaps->pixmap_helpdoc,
					   &pixmaps->mask_helpdoc,
					   127);
        index->priv->pixmaps = pixmaps;
}

GtkWidget *
book_index_new (Bookshelf *bookshelf)
{
        BookIndex   *index;
        
        index = gtk_type_new (TYPE_BOOK_INDEX);
        
        gtk_ctree_construct (GTK_CTREE (index), 1, 0, NULL);
        
        index->priv->bookshelf = bookshelf;

        book_index_populate_tree (index);
        
        return GTK_WIDGET (index);
}

static void
book_index_open_node (BookIndex *index, BookNode *book_node)
{
	BookIndexPriv   *priv;
	GtkCTreeNode    *node;
	GtkCTreeRow     *row;
	GtkCTreeNode    *parent;
	
	g_return_if_fail (index != NULL);
        g_return_if_fail (IS_BOOK_INDEX (index));

	priv = index->priv;
	
	node = gtk_ctree_find_by_row_data (GTK_CTREE (index), NULL, book_node);
	
	if (node) {
		row = GTK_CTREE_ROW (node);

		while (parent = row->parent) {
			gtk_ctree_expand (GTK_CTREE (index), row->parent);
			row = GTK_CTREE_ROW (parent);
		}

		/* Have to do this workaround to only emit when user click */
		priv->emit_uri_select = FALSE;
		gtk_ctree_select (GTK_CTREE(index), node);
		priv->emit_uri_select = TRUE;
		
		gtk_ctree_node_moveto (GTK_CTREE(index), node, 0, 0.5, 0.5);
	}
}

static void
book_index_select_row (GtkCTree *ctree, GtkCTreeNode *node, gint column)
{
        BookIndex       *index;
        BookIndexPriv   *priv;
        BookNode        *book_node;
        GnomeVFSURI     *uri;
        
        g_return_if_fail (ctree != NULL);
        g_return_if_fail (IS_BOOK_INDEX (ctree));
        g_return_if_fail (node != NULL);
        
        index = BOOK_INDEX (ctree);
        priv  = index->priv;

        /* Chaining up */
        if (parent_class->tree_select_row) {
                parent_class->tree_select_row (ctree, node, column);
        }

	if (priv->emit_uri_select) {

		book_node = (BookNode *) gtk_ctree_node_get_row_data (ctree, 
								      node);
        
		if (book_node) {
			bookshelf_open_document (priv->bookshelf, 
						 book_node_get_document (book_node));

			uri = book_node_get_uri (book_node);
			
			gtk_signal_emit (GTK_OBJECT (index),
					 signals[URI_SELECTED],
					 uri);
                
			gnome_vfs_uri_unref (uri);
		}
	}
}

void
book_index_open (BookIndex *index, const gchar *uri)
{
        BookIndexPriv   *priv;
        Document        *document;
        BookNode        *node;
        gchar           *anchor = NULL;

        g_return_if_fail (index != NULL);
        g_return_if_fail (IS_BOOK_INDEX (index));
        
        priv = index->priv;
        
        g_print ("Trying to find document for: [%s]\n", uri);

        document = bookshelf_find_document (priv->bookshelf, uri, &anchor);
        
        if (document) {
                g_print ("Flirp\n");
                node = bookshelf_find_node (priv->bookshelf, document, anchor);

                if (node) {
                        book_index_open_node (index, node);
                }
        }
        
        if (anchor) {
                g_free (anchor);
        }
}

void
book_index_add_book (BookIndex *index, Book *book)
{
        BookIndexPriv   *priv;
        
	g_return_if_fail (index != NULL);
        g_return_if_fail (IS_BOOK_INDEX (index));
        g_return_if_fail (book != NULL);
        g_return_if_fail (IS_BOOK (book));

        priv = index->priv;
        
        gtk_clist_freeze (GTK_CLIST (index));

        book_index_insert_book_node (index, NULL, book_get_root (book));
        
        gtk_clist_thaw (GTK_CLIST (index));
}
