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

#ifndef __DH_BOOKSHELF_H__
#define __DH_BOOKSHELF_H__

#include <glib-object.h>
#include "book.h"

#define DH_TYPE_BOOKSHELF        (dh_bookshelf_get_type ())
#define DH_BOOKSHELF(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), DH_TYPE_BOOKSHELF, DhBookshelf))
#define DH_BOOKSHELF_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), DH_TYPE_BOOKSHELF, DhBookshelfClass))
#define DH_IS_BOOKSHELF(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), DH_TYPE_BOOKSHELF))
#define DH_IS_BOOKSHELF_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), DH_TYPE_BOOKSHELF))

typedef struct _DhBookshelf      DhBookshelf;
typedef struct _DhBookshelfClass DhBookshelfClass;
typedef struct _DhBookshelfPriv  DhBookshelfPriv;
typedef struct _XMLBook        XMLBook;

struct _DhBookshelf {
	GObject          parent;
	
	DhBookshelfPriv  *priv;
};

struct _DhBookshelfClass 
{
	GObjectClass    parent_class;
	
	void (*book_added)    (DhBookshelf     *bookshelf,
			       Book          *book);
	void (*book_removed)  (DhBookshelf     *bookshelf,
			       Book          *book);
};

struct _XMLBook {
	const gchar    *spec_path;
	const gchar    *name;
	const gchar    *version;
	gboolean  visible;
};

GType         dh_bookshelf_get_type         (void);

DhBookshelf * dh_bookshelf_new              (FunctionDatabase  *fd);

FunctionDatabase *
dh_bookshelf_get_function_database          (DhBookshelf       *bookshelf);

void          dh_bookshelf_write_xml        (DhBookshelf           *bookshelf);

gboolean      dh_bookshelf_add_book         (DhBookshelf           *bookshelf,
					     Book                *book);

gboolean      dh_bookshelf_have_book        (DhBookshelf           *bookshelf,
					     Book                *book);

void          dh_bookshelf_add_directory    (DhBookshelf           *bookshelf,
					     const gchar         *directory);

GSList *      dh_bookshelf_get_books        (DhBookshelf           *bookshelf);

GSList *      dh_bookshelf_get_hidden_books (DhBookshelf           *bookshelf);

void          dh_bookshelf_show_book        (DhBookshelf           *bookshelf,
					     XMLBook             *xml_book);

void          dh_bookshelf_hide_book        (DhBookshelf           *bookshelf,
					     Book                *book);

Book *
dh_bookshelf_find_book_by_title             (DhBookshelf           *bookshelf,
					     const gchar         *title);

Book *        dh_bookshelf_find_book_by_uri (DhBookshelf           *bookshelf,
					     const GnomeVFSURI   *uri);

Book * 
dh_bookshelf_find_book_by_name              (DhBookshelf           *bookshelf,
					     const gchar         *name);

void          dh_bookshelf_remove_book      (DhBookshelf           *bookshelf,
					     Book                *book);

Document *    dh_bookshelf_find_document    (DhBookshelf           *bookshelf,
					     const gchar         *url,
					     gchar              **anchor);

BookNode *    dh_bookshelf_find_node        (DhBookshelf           *bookshelf,
					     const Document      *document,
					     const gchar         *anchor);

void          dh_bookshelf_open_document    (DhBookshelf          *bookshelf,
					     const Document     *document);

#endif /* __DH_BOOKSHELF_H__ */
