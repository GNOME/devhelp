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

#ifndef __BOOK_H__
#define __BOOK_H__

#include <gtk/gtkobject.h>
#include <gtk/gtktypeutils.h>
#include <libgnomevfs/gnome-vfs.h>
#include "book-node.h"
#include "function-database.h"

#define TYPE_BOOK        (book_get_type ())
#define BOOK(o)          (GTK_CHECK_CAST ((o), TYPE_BOOK, Book))
#define BOOK_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), TYPE_BOOK, BookClass))
#define IS_BOOK(o)       (GTK_CHECK_TYPE ((o), TYPE_BOOK))
#define IS_BOOK_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), TYPE_BOOK))

typedef struct _Book        Book;
typedef struct _BookClass   BookClass;
typedef struct _BookPriv    BookPriv;

struct _Book {
	GtkObject         parent;
	
	BookPriv         *priv;
};

struct _BookClass {
	GtkObjectClass    parent_class;
};

GtkType             book_get_type              (void);
Book *              book_new                   (GnomeVFSURI         *book_uri,
						FunctionDatabase    *fd);

const gchar *       book_get_name              (Book                *book);

const gchar *       book_get_name_full         (Book                *book);

const gchar *       book_get_title             (Book                *book);

const gchar *       book_get_author            (Book                *book);

const gchar *       book_get_version           (Book                *book);

gchar *             book_get_path              (Book                *book);

gboolean            book_contains              (Book                *book,
						const GnomeVFSURI   *uri);

void                book_set_base_url          (Book                *book,
						const gchar         *url);

BookNode *          book_get_root              (Book                *book);

void                book_set_current_document  (Book                *book,
						const Document      *document);

void                book_open_document         (Book                *book,
						const Document      *document);

Document *          book_find_document         (Book                *book,
						const gchar         *rel_url,
						gchar              **anchor);

BookNode *          book_find_node             (Book                *book,
						const Document      *document,
						const gchar         *anchor);

gint                book_get_current_depth     (Book                *book);

gint                book_compare_func          (gconstpointer        a,
						gconstpointer        b);

/* Has to be here */
Book *              document_get_book          (const Document      *document);
Book *              book_node_get_book         (const BookNode      *node);

#endif /* __BOOK_H__ */
