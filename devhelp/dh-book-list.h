/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
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

GType           dh_book_list_get_type           (void);

DhBookList *    dh_book_list_new                (void);

DhBookList *    dh_book_list_get_default        (void);

G_GNUC_INTERNAL
void            _dh_book_list_unref_default     (void);

GList *         dh_book_list_get_books          (DhBookList *book_list);

void            dh_book_list_add_book           (DhBookList *book_list,
                                                 DhBook     *book);

void            dh_book_list_remove_book        (DhBookList *book_list,
                                                 DhBook     *book);

G_END_DECLS

#endif /* DH_BOOK_LIST_H */
