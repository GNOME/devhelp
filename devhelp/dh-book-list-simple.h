/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_BOOK_LIST_SIMPLE_H
#define DH_BOOK_LIST_SIMPLE_H

#include <glib-object.h>
#include "dh-book-list.h"
#include "dh-settings.h"

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
DhBookList *    _dh_book_list_simple_new        (GList      *sub_book_lists,
                                                 DhSettings *settings);

G_END_DECLS

#endif /* DH_BOOK_LIST_SIMPLE_H */
