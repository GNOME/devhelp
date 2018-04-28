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
