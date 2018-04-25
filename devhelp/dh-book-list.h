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

#ifndef DH_BOOK_LIST_H
#define DH_BOOK_LIST_H

#include <glib-object.h>
#include <devhelp/dh-book.h>

G_BEGIN_DECLS

#define DH_TYPE_BOOK_LIST             (dh_book_list_get_type ())
#define DH_BOOK_LIST(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_BOOK_LIST, DhBookList))
#define DH_BOOK_LIST_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_BOOK_LIST, DhBookListClass))
#define DH_IS_BOOK_LIST(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_BOOK_LIST))
#define DH_IS_BOOK_LIST_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_BOOK_LIST))
#define DH_BOOK_LIST_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_BOOK_LIST, DhBookListClass))

typedef struct _DhBookList         DhBookList;
typedef struct _DhBookListClass    DhBookListClass;
typedef struct _DhBookListPrivate  DhBookListPrivate;

struct _DhBookList {
        GObject parent;

        DhBookListPrivate *priv;
};

/**
 * DhBookListClass:
 * @parent_class: The parent class.
 * @add_book: Virtual function pointer for the #DhBookList::add-book signal.
 * @remove_book: Virtual function pointer for the #DhBookList::remove-book
 *   signal.
 * @get_books: Virtual function pointer for dh_book_list_get_books(). Returns
 *   the #DhBookList internal #GList by default. If you override this vfunc
 *   ensure that each book ID is unique in the returned list.
 */
struct _DhBookListClass {
        GObjectClass parent_class;

        /* Signals */
        void    (* add_book)            (DhBookList *book_list,
                                         DhBook     *book);

        void    (* remove_book)         (DhBookList *book_list,
                                         DhBook     *book);

        /* Vfuncs */
        GList * (* get_books)           (DhBookList *book_list);

        /*< private >*/

        /* Padding for future expansion */
        gpointer padding[12];
};

GType   dh_book_list_get_type           (void);

GList * dh_book_list_get_books          (DhBookList *book_list);

void    dh_book_list_add_book           (DhBookList *book_list,
                                         DhBook     *book);

void    dh_book_list_remove_book        (DhBookList *book_list,
                                         DhBook     *book);

G_END_DECLS

#endif /* DH_BOOK_LIST_H */
