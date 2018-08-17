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

#pragma once

#include <glib-object.h>
#include <devhelp/dh-book.h>

G_BEGIN_DECLS

#define DH_TYPE_BOOK_LIST             (dh_book_list_get_type ())
G_DECLARE_DERIVABLE_TYPE (DhBookList, dh_book_list, DH, BOOK_LIST, GObject)

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

        /* Padding for future expansion */
        gpointer padding[12];
};

DhBookList *dh_book_list_new            (void);
DhBookList *dh_book_list_get_default    (void);
G_GNUC_INTERNAL
void        _dh_book_list_unref_default (void);
GList      *dh_book_list_get_books      (DhBookList *book_list);
void        dh_book_list_add_book       (DhBookList *book_list,
                                         DhBook     *book);
void        dh_book_list_remove_book    (DhBookList *book_list,
                                         DhBook     *book);

G_END_DECLS

