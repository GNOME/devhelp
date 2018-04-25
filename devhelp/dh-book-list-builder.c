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

#include "dh-book-list-builder.h"
#include "dh-book-list-simple.h"

/**
 * SECTION:dh-book-list-builder
 * @Title: DhBookListBuilder
 * @Short_description: Builds #DhBookList objects
 *
 * #DhBookListBuilder permits to build #DhBookList objects.
 */

/* API design:
 *
 * It follows the builder pattern, see:
 * https://blogs.gnome.org/otte/2018/02/03/builders/
 * but it is implemented in a simpler way, to have less boilerplate.
 */

struct _DhBookListBuilderPrivate {
        /* List of DhBookList*. */
        GList *sub_book_lists;
};

G_DEFINE_TYPE_WITH_PRIVATE (DhBookListBuilder, dh_book_list_builder, G_TYPE_OBJECT)

static void
dh_book_list_builder_dispose (GObject *object)
{
        DhBookListBuilder *builder = DH_BOOK_LIST_BUILDER (object);

        g_list_free_full (builder->priv->sub_book_lists, g_object_unref);
        builder->priv->sub_book_lists = NULL;

        G_OBJECT_CLASS (dh_book_list_builder_parent_class)->dispose (object);
}

static void
dh_book_list_builder_class_init (DhBookListBuilderClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_book_list_builder_dispose;
}

static void
dh_book_list_builder_init (DhBookListBuilder *builder)
{
        builder->priv = dh_book_list_builder_get_instance_private (builder);
}

/**
 * dh_book_list_builder_new:
 *
 * Returns: (transfer full): a new #DhBookListBuilder.
 * Since: 3.30
 */
DhBookListBuilder *
dh_book_list_builder_new (void)
{
        return g_object_new (DH_TYPE_BOOK_LIST_BUILDER, NULL);
}

/**
 * dh_book_list_builder_add_sub_book_list:
 * @builder: a #DhBookListBuilder.
 * @sub_book_list: a #DhBookList.
 *
 * Adds @sub_book_list.
 *
 * The #DhBookList object that will be created with
 * dh_book_list_builder_create_object() will contain all the sub-#DhBookList's
 * added with this function (and it will listen to their signals). The
 * sub-#DhBookList's must be added in order of decreasing priority (the first
 * sub-#DhBookList added has the highest priority). The priority is used in case
 * of book ID conflicts (see dh_book_get_id()).
 *
 * Since: 3.30
 */
void
dh_book_list_builder_add_sub_book_list (DhBookListBuilder *builder,
                                        DhBookList        *sub_book_list)
{
        g_return_if_fail (DH_IS_BOOK_LIST_BUILDER (builder));
        g_return_if_fail (DH_IS_BOOK_LIST (sub_book_list));

        builder->priv->sub_book_lists = g_list_append (builder->priv->sub_book_lists,
                                                       g_object_ref (sub_book_list));
}

/**
 * dh_book_list_builder_create_object:
 * @builder: a #DhBookListBuilder.
 *
 * Creates the #DhBookList. It actually creates a subclass of #DhBookList, but
 * the subclass is not exposed to the public API.
 *
 * Returns: (transfer full): the newly created #DhBookList object.
 * Since: 3.30
 */
DhBookList *
dh_book_list_builder_create_object (DhBookListBuilder *builder)
{
        g_return_val_if_fail (DH_IS_BOOK_LIST_BUILDER (builder), NULL);

        return _dh_book_list_simple_new (builder->priv->sub_book_lists);
}
