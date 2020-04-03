/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_BOOK_LIST_BUILDER_H
#define DH_BOOK_LIST_BUILDER_H

#include <glib-object.h>
#include <devhelp/dh-book-list.h>
#include <devhelp/dh-settings.h>

G_BEGIN_DECLS

#define DH_TYPE_BOOK_LIST_BUILDER             (dh_book_list_builder_get_type ())
#define DH_BOOK_LIST_BUILDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_BOOK_LIST_BUILDER, DhBookListBuilder))
#define DH_BOOK_LIST_BUILDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_BOOK_LIST_BUILDER, DhBookListBuilderClass))
#define DH_IS_BOOK_LIST_BUILDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_BOOK_LIST_BUILDER))
#define DH_IS_BOOK_LIST_BUILDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_BOOK_LIST_BUILDER))
#define DH_BOOK_LIST_BUILDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_BOOK_LIST_BUILDER, DhBookListBuilderClass))

typedef struct _DhBookListBuilder         DhBookListBuilder;
typedef struct _DhBookListBuilderClass    DhBookListBuilderClass;
typedef struct _DhBookListBuilderPrivate  DhBookListBuilderPrivate;

struct _DhBookListBuilder {
        GObject parent;

        DhBookListBuilderPrivate *priv;
};

struct _DhBookListBuilderClass {
        GObjectClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_book_list_builder_get_type                           (void);

DhBookListBuilder *
                dh_book_list_builder_new                                (void);

void            dh_book_list_builder_add_sub_book_list                  (DhBookListBuilder *builder,
                                                                         DhBookList        *sub_book_list);

void            dh_book_list_builder_add_default_sub_book_lists         (DhBookListBuilder *builder);

void            dh_book_list_builder_read_books_disabled_setting        (DhBookListBuilder *builder,
                                                                         DhSettings        *settings);

DhBookList *    dh_book_list_builder_create_object                      (DhBookListBuilder *builder);

G_END_DECLS

#endif /* DH_BOOK_LIST_BUILDER_H */
