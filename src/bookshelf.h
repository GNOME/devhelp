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

#ifndef __BOOKSHELF_H__
#define __BOOKSHELF_H__

#include <gtk/gtkobject.h>
#include <gtk/gtktypeutils.h>
#include "book.h"

#define TYPE_BOOKSHELF        (bookshelf_get_type ())
#define BOOKSHELF(o)          (GTK_CHECK_CAST ((o), TYPE_BOOKSHELF, Bookshelf))
#define BOOKSHELF_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), TYPE_BOOKSHELF, BookshelfClass))
#define IS_BOOKSHELF(o)       (GTK_CHECK_TYPE ((o), TYPE_BOOKSHELF))
#define IS_BOOKSHELF_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), TYPE_BOOKSHELF))

typedef struct _Bookshelf      Bookshelf;
typedef struct _BookshelfClass BookshelfClass;
typedef struct _BookshelfPriv  BookshelfPriv;
typedef struct _XMLBook        XMLBook;

struct _Bookshelf {
	GtkObject         parent;
	
	BookshelfPriv    *priv;
};

struct _BookshelfClass 
{
	GtkObjectClass    parent_class;
	
	/* FIX: Add book_added and book_removed signals */

};

GtkType          bookshelf_get_type           (void);

Bookshelf *      bookshelf_new                (const char          *default_dir,
                                               FunctionDatabase    *fd);

FunctionDatabase * 
bookshelf_get_function_database               (Bookshelf           *bookshelf);

GList *          bookshelf_read_xml           (Bookshelf           *bookshelf,
					       const gchar         *filename);

void             bookshelf_write_xml          (Bookshelf           *bookshelf,
					       const gchar         *xml_filename);

gboolean         bookshelf_add_book           (Bookshelf           *bookshelf,
					       Book                *book);

void             bookshelf_add_directory      (Bookshelf           *bookshelf,
					       const gchar         *directory);

GSList *         bookshelf_get_books          (Bookshelf           *bookshelf);


Book *           bookshelf_find_book_by_title (Bookshelf           *bookshelf,
					       const gchar         *title);

Book *           bookshelf_find_book_by_uri   (Bookshelf           *bookshelf,
					       const GnomeVFSURI   *uri);

Book *           bookshelf_find_book_by_name  (Bookshelf           *bookshelf,
					       const gchar         *name);

void             bookshelf_remove_book        (Bookshelf           *bookshelf,
					       Book                *book);

Document *       bookshelf_find_document      (Bookshelf           *bookshelf,
					       const gchar         *url,
					       gchar              **anchor);

BookNode *       bookshelf_find_node          (Bookshelf           *bookshelf,
					       Document            *document,
					       const gchar         *anchor);

void             bookshelf_open_document      (Bookshelf          *bookshelf,
					       const Document     *document);

#endif /* __BOOKSHELF_H__ */
