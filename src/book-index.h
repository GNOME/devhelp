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

#ifndef __BOOK_INDEX_H__
#define __BOOK_INDEX_H__

#include <glib.h>
#include <gtk/gtkctree.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtktypeutils.h>
#include "bookshelf.h"

#define TYPE_BOOK_INDEX			(book_index_get_type ())
#define BOOK_INDEX(obj)			(GTK_CHECK_CAST ((obj), TYPE_BOOK_INDEX, BookIndex))
#define BOOK_INDEX_CLASS(klass)		(GTK_CHECK_CLASS_CAST ((klass), TYPE_BOOK_INDEX, BookIndexClass))
#define IS_BOOK_INDEX(obj)		(GTK_CHECK_TYPE ((obj), TYPE_BOOK_INDEX))
#define IS_BOOK_INDEX_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), TYPE_BOOK_INDEX))

typedef struct _BookIndex       BookIndex;
typedef struct _BookIndexClass  BookIndexClass;
typedef struct _BookIndexPriv   BookIndexPriv;

struct _BookIndex
{
        GtkCTree         parent;
        
        BookIndexPriv   *priv;
};

struct _BookIndexClass
{
        GtkCTreeClass parent_class;

        /* Signals */
        
        void (*uri_selected)  (BookIndex           *index,
                               const GnomeVFSURI   *uri);
};

GtkType      book_index_get_type      (void);
GtkWidget *  book_index_new           (Bookshelf     *bookshelf);

void         book_index_open          (BookIndex     *index,
                                       const gchar   *uri);

void         book_index_add_book      (BookIndex     *index,
                                       Book          *book);

#endif /* __BOOK_INDEX_H__ */
