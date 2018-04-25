/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
 *
 * Devhelp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Devhelp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Devhelp.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DH_BOOK_LIST_SIMPLE_H
#define DH_BOOK_LIST_SIMPLE_H

#include <glib-object.h>
#include "dh-book-list.h"

G_BEGIN_DECLS

#define DH_TYPE_BOOK_LIST_SIMPLE             (_dh_book_list_simple_get_type ())
#define DH_BOOK_LIST_SIMPLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_BOOK_LIST_SIMPLE, DhBookListSimple))
#define DH_BOOK_LIST_SIMPLE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_BOOK_LIST_SIMPLE, DhBookListSimpleClass))
#define DH_IS_BOOK_LIST_SIMPLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_BOOK_LIST_SIMPLE))
#define DH_IS_BOOK_LIST_SIMPLE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_BOOK_LIST_SIMPLE))
#define DH_BOOK_LIST_SIMPLE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_BOOK_LIST_SIMPLE, DhBookListSimpleClass))

typedef struct _DhBookListSimple         DhBookListSimple;
typedef struct _DhBookListSimpleClass    DhBookListSimpleClass;
typedef struct _DhBookListSimplePrivate  DhBookListSimplePrivate;

struct _DhBookListSimple {
        DhBookList parent;

        DhBookListSimplePrivate *priv;
};

struct _DhBookListSimpleClass {
        DhBookListClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

G_GNUC_INTERNAL
GType           _dh_book_list_simple_get_type   (void);

G_GNUC_INTERNAL
DhBookList *    _dh_book_list_simple_new        (GList *sub_book_lists);

G_END_DECLS

#endif /* DH_BOOK_LIST_SIMPLE_H */
