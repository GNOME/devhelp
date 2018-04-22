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

#include "dh-book-list.h"

/**
 * SECTION:dh-book-list
 * @Title: DhBookList
 * @Short_description: Abstract class for a list of #DhBook's
 *
 * #DhBookList is an abstract class for a list of #DhBook's.
 */

struct _DhBookListPrivate {
};

enum {
        SIGNAL_BOOK_ADDED,
        SIGNAL_BOOK_REMOVED,
        N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE (DhBookList, dh_book_list, G_TYPE_OBJECT)

static GList *
dh_book_list_get_books_default (DhBookList *book_list)
{
        return NULL;
}

static void
dh_book_list_class_init (DhBookListClass *klass)
{
        klass->get_books = dh_book_list_get_books_default;

        /**
         * DhBookList::book-added:
         * @book_list: the #DhBookList emitting the signal.
         * @book: the added #DhBook.
         *
         * Since: 3.30
         */
        signals[SIGNAL_BOOK_ADDED] =
                g_signal_new ("book-added",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DhBookListClass, book_added),
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1, DH_TYPE_BOOK);

        /**
         * DhBookList::book-removed:
         * @book_list: the #DhBookList emitting the signal.
         * @book: the removed #DhBook.
         *
         * Since: 3.30
         */
        signals[SIGNAL_BOOK_REMOVED] =
                g_signal_new ("book-removed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DhBookListClass, book_removed),
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1, DH_TYPE_BOOK);
}

static void
dh_book_list_init (DhBookList *book_list)
{
}

/**
 * dh_book_list_get_books:
 * @book_list: a #DhBookList.
 *
 * Returns: (transfer none) (element-type DhBook): the #GList of #DhBook's part
 * of @book_list.
 * Since: 3.30
 */
GList *
dh_book_list_get_books (DhBookList *book_list)
{
        g_return_val_if_fail (DH_IS_BOOK_LIST (book_list), NULL);

        return DH_BOOK_LIST_GET_CLASS (book_list)->get_books (book_list);
}

/**
 * dh_book_list_book_added:
 * @book_list: a #DhBookList.
 * @book: a #DhBook.
 *
 * Emits the #DhBookList::book-added signal.
 *
 * This function is intended to be used by #DhBookList subclasses.
 *
 * Since: 3.30
 */
void
dh_book_list_book_added (DhBookList *book_list,
                         DhBook     *book)
{
        g_return_if_fail (DH_IS_BOOK_LIST (book_list));
        g_return_if_fail (DH_IS_BOOK (book));

        g_signal_emit (book_list,
                       signals[SIGNAL_BOOK_ADDED],
                       0,
                       book);
}

/**
 * dh_book_list_book_removed:
 * @book_list: a #DhBookList.
 * @book: a #DhBook.
 *
 * Emits the #DhBookList::book-removed signal.
 *
 * This function is intended to be used by #DhBookList subclasses.
 *
 * Since: 3.30
 */
void
dh_book_list_book_removed (DhBookList *book_list,
                           DhBook     *book)
{
        g_return_if_fail (DH_IS_BOOK_LIST (book_list));
        g_return_if_fail (DH_IS_BOOK (book));

        g_signal_emit (book_list,
                       signals[SIGNAL_BOOK_REMOVED],
                       0,
                       book);
}
