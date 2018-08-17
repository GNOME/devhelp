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

#include "dh-book-list-simple.h"

typedef struct {
        /* List of DhBookList*. */
        GList *sub_book_lists;

        /* For reading the "books-disabled" GSettings key. */
        DhSettings *settings;
} DhBookListSimplePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhBookListSimple, dh_book_list_simple, DH_TYPE_BOOK_LIST)

static gpointer
book_copy_func (gconstpointer src,
                gpointer      data)
{
        return g_object_ref ((gpointer) src);
}

static void
dh_book_list_simple_dispose (GObject *object)
{
        DhBookListSimple *list_simple = DH_BOOK_LIST_SIMPLE (object);
        DhBookListSimplePrivate *priv = dh_book_list_simple_get_instance_private (list_simple);

        g_list_free_full (priv->sub_book_lists, g_object_unref);
        priv->sub_book_lists = NULL;

        g_clear_object (&priv->settings);

        G_OBJECT_CLASS (dh_book_list_simple_parent_class)->dispose (object);
}

static void
dh_book_list_simple_class_init (DhBookListSimpleClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_book_list_simple_dispose;
}

static void
dh_book_list_simple_init (DhBookListSimple *list_simple)
{
}

/* Returns: the new start of the list. */
static GList *
filter_by_books_disabled (DhBookListSimple *list_simple,
                          GList            *list)
{
        DhBookListSimplePrivate *priv = dh_book_list_simple_get_instance_private (list_simple);
        GList *new_list = NULL;
        GList *l;

        if (priv->settings == NULL)
                return list;

        for (l = list; l != NULL; l = l->next) {
                DhBook *book = DH_BOOK (l->data);

                if (dh_settings_is_book_enabled (priv->settings, book))
                        new_list = g_list_prepend (new_list, g_object_ref (book));
        }

        g_list_free_full (list, g_object_unref);
        return new_list;
}

/* Returns: (transfer full) (element-type DhBook). */
static GList *
generate_list (DhBookListSimple *list_simple)
{
        GList *ret = NULL;
        GList *book_list_node;
        DhBookListSimplePrivate *priv = dh_book_list_simple_get_instance_private (list_simple);

        for (book_list_node = priv->sub_book_lists;
             book_list_node != NULL;
             book_list_node = book_list_node->next) {
                DhBookList *book_list = DH_BOOK_LIST (book_list_node->data);
                GList *books;
                GList *book_node;

                books = dh_book_list_get_books (book_list);

                /* First DhBookList, take all DhBook's. */
                if (book_list_node == priv->sub_book_lists) {
                        g_assert (ret == NULL);
                        ret = g_list_copy_deep (books, book_copy_func, NULL);
                        continue;
                }

                for (book_node = books; book_node != NULL; book_node = book_node->next) {
                        DhBook *book = DH_BOOK (book_node->data);

                        /* Ensure to have unique book IDs. */
                        if (g_list_find_custom (ret, book, (GCompareFunc)dh_book_cmp_by_id) == NULL)
                                ret = g_list_prepend (ret, g_object_ref (book));
                }
        }

        return filter_by_books_disabled (list_simple, ret);
}

static void
repopulate (DhBookListSimple *list_simple)
{
        GList *old_list;
        GList *old_list_copy;
        GList *new_list;
        GList *old_node;
        GList *new_node;

        old_list = dh_book_list_get_books (DH_BOOK_LIST (list_simple));
        old_list_copy = g_list_copy_deep (old_list, book_copy_func, NULL);

        new_list = generate_list (list_simple);

        for (old_node = old_list_copy; old_node != NULL; old_node = old_node->next) {
                DhBook *old_book = DH_BOOK (old_node->data);

                if (g_list_find (new_list, old_book) == NULL)
                        dh_book_list_remove_book (DH_BOOK_LIST (list_simple), old_book);
        }

        for (new_node = new_list; new_node != NULL; new_node = new_node->next) {
                DhBook *new_book = DH_BOOK (new_node->data);

                if (g_list_find (old_list_copy, new_book) == NULL)
                        dh_book_list_add_book (DH_BOOK_LIST (list_simple), new_book);
        }

        g_list_free_full (old_list_copy, g_object_unref);
        g_list_free_full (new_list, g_object_unref);
}

static void
book_list_add_book_cb (DhBookList       *book_list,
                       DhBook           *book,
                       DhBookListSimple *list_simple)
{
        repopulate (list_simple);
}

static void
book_list_remove_book_cb (DhBookList       *book_list,
                          DhBook           *book,
                          DhBookListSimple *list_simple)
{
        repopulate (list_simple);
}

static void
set_sub_book_lists (DhBookListSimple *list_simple,
                    GList            *sub_book_lists)
{
        DhBookListSimplePrivate *priv = dh_book_list_simple_get_instance_private (list_simple);
        GList *l;

        g_assert (priv->sub_book_lists == NULL);

        for (l = sub_book_lists; l != NULL; l = l->next) {
                DhBookList *book_list;

                if (!DH_IS_BOOK_LIST (l->data)) {
                        g_warn_if_reached ();
                        continue;
                }

                book_list = l->data;
                priv->sub_book_lists = g_list_prepend (priv->sub_book_lists,
                                                       g_object_ref (book_list));

                g_signal_connect_object (book_list,
                                         "add-book",
                                         G_CALLBACK (book_list_add_book_cb),
                                         list_simple,
                                         G_CONNECT_AFTER);

                g_signal_connect_object (book_list,
                                         "remove-book",
                                         G_CALLBACK (book_list_remove_book_cb),
                                         list_simple,
                                         G_CONNECT_AFTER);
        }

        priv->sub_book_lists = g_list_reverse (priv->sub_book_lists);
}

static void
books_disabled_changed_cb (DhSettings       *settings,
                           DhBookListSimple *list_simple)
{
        repopulate (list_simple);
}

/* @settings is for reading the "books-disabled" GSettings key. */
DhBookList *
_dh_book_list_simple_new (GList      *sub_book_lists,
                          DhSettings *settings)
{
        DhBookListSimple *list_simple;
        DhBookListSimplePrivate *priv;

        g_return_val_if_fail (settings == NULL || DH_IS_SETTINGS (settings), NULL);

        list_simple = g_object_new (DH_TYPE_BOOK_LIST_SIMPLE, NULL);
        priv = dh_book_list_simple_get_instance_private (list_simple);
        set_sub_book_lists (list_simple, sub_book_lists);

        if (settings != NULL) {
                priv->settings = g_object_ref (settings);

                g_signal_connect_object (settings,
                                         "books-disabled-changed",
                                         G_CALLBACK (books_disabled_changed_cb),
                                         list_simple,
                                         0);
        }

        repopulate (list_simple);

        return DH_BOOK_LIST (list_simple);
}
