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

struct _DhBookListSimplePrivate {
        /* List of DhBookList*. */
        GList *book_lists;
};

G_DEFINE_TYPE_WITH_PRIVATE (DhBookListSimple, _dh_book_list_simple, DH_TYPE_BOOK_LIST)

static void
dh_book_list_simple_dispose (GObject *object)
{
        DhBookListSimple *list_simple = DH_BOOK_LIST_SIMPLE (object);

        g_list_free_full (list_simple->priv->book_lists, g_object_unref);
        list_simple->priv->book_lists = NULL;

        G_OBJECT_CLASS (_dh_book_list_simple_parent_class)->dispose (object);
}

static void
_dh_book_list_simple_class_init (DhBookListSimpleClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_book_list_simple_dispose;
}

static void
_dh_book_list_simple_init (DhBookListSimple *list_simple)
{
        list_simple->priv = _dh_book_list_simple_get_instance_private (list_simple);
}

/* Returns: (transfer full) (element-type DhBook). */
static GList *
generate_list (DhBookListSimple *list_simple)
{
        GList *ret = NULL;
        GList *book_list_node;

        for (book_list_node = list_simple->priv->book_lists;
             book_list_node != NULL;
             book_list_node = book_list_node->next) {
                DhBookList *book_list = DH_BOOK_LIST (book_list_node->data);
                GList *books;
                GList *book_node;

                books = dh_book_list_get_books (book_list);

                /* First DhBookList, take all DhBook's. */
                if (book_list_node == list_simple->priv->book_lists) {
                        g_assert (ret == NULL);
                        ret = g_list_copy_deep (books, (GCopyFunc) g_object_ref, NULL);
                        continue;
                }

                for (book_node = books; book_node != NULL; book_node = book_node->next) {
                        DhBook *book = DH_BOOK (book_node->data);

                        /* Ensure to have unique book IDs. */
                        if (g_list_find_custom (ret, book, (GCompareFunc)dh_book_cmp_by_id) == NULL)
                                ret = g_list_prepend (ret, g_object_ref (book));
                }
        }

        return ret;
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
        old_list_copy = g_list_copy_deep (old_list, (GCopyFunc) g_object_ref, NULL);

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
set_book_lists (DhBookListSimple *list_simple,
                GList            *book_lists)
{
        GList *l;

        g_assert (list_simple->priv->book_lists == NULL);

        for (l = book_lists; l != NULL; l = l->next) {
                DhBookList *book_list;

                if (!DH_IS_BOOK_LIST (l->data)) {
                        g_warn_if_reached ();
                        continue;
                }

                book_list = l->data;
                list_simple->priv->book_lists = g_list_prepend (list_simple->priv->book_lists,
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

        list_simple->priv->book_lists = g_list_reverse (list_simple->priv->book_lists);

        repopulate (list_simple);
}

DhBookList *
_dh_book_list_simple_new (GList *book_lists)
{
        DhBookListSimple *list_simple;

        list_simple = g_object_new (DH_TYPE_BOOK_LIST_SIMPLE, NULL);
        set_book_lists (list_simple, book_lists);

        return DH_BOOK_LIST (list_simple);
}
