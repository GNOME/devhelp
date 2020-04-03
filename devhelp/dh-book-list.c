/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dh-book-list.h"
#include "dh-book-list-builder.h"
#include "dh-settings.h"

/**
 * SECTION:dh-book-list
 * @Title: DhBookList
 * @Short_description: Base class for a list of #DhBook's
 *
 * #DhBookList is a base class for a list of #DhBook's.
 *
 * The default implementation maintains an internal #GList when books are added
 * and removed with the #DhBookList::add-book and #DhBookList::remove-book
 * signals, and returns that #GList in dh_book_list_get_books().
 *
 * The #DhBookList base class doesn't listen to the #DhBook #DhBook::deleted and
 * #DhBook::updated signals. It is for example handled by #DhBookListDirectory.
 */

struct _DhBookListPrivate {
        /* The list of DhBook's. */
        GList *books;
};

enum {
        SIGNAL_ADD_BOOK,
        SIGNAL_REMOVE_BOOK,
        N_SIGNALS
};

static DhBookList *default_instance = NULL;
static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (DhBookList, dh_book_list, G_TYPE_OBJECT)

static gboolean
book_id_present_in_list (DhBookList *book_list,
                         DhBook     *book)
{
        return g_list_find_custom (book_list->priv->books,
                                   book,
                                   (GCompareFunc) dh_book_cmp_by_id) != NULL;
}

static void
dh_book_list_dispose (GObject *object)
{
        DhBookList *book_list = DH_BOOK_LIST (object);

        g_list_free_full (book_list->priv->books, g_object_unref);
        book_list->priv->books = NULL;

        G_OBJECT_CLASS (dh_book_list_parent_class)->dispose (object);
}

static void
dh_book_list_finalize (GObject *object)
{
        if (default_instance == DH_BOOK_LIST (object))
                default_instance = NULL;

        G_OBJECT_CLASS (dh_book_list_parent_class)->finalize (object);
}

static void
dh_book_list_add_book_default (DhBookList *book_list,
                               DhBook     *book)
{
        g_return_if_fail (!book_id_present_in_list (book_list, book));

        book_list->priv->books = g_list_prepend (book_list->priv->books,
                                                 g_object_ref (book));
}

static void
dh_book_list_remove_book_default (DhBookList *book_list,
                                  DhBook     *book)
{
        GList *node;

        node = g_list_find (book_list->priv->books, book);
        g_return_if_fail (node != NULL);

        book_list->priv->books = g_list_delete_link (book_list->priv->books, node);

        if (g_list_find (book_list->priv->books, book) != NULL)
                g_warning ("The same DhBook was inserted several times.");

        g_object_unref (book);
}

static GList *
dh_book_list_get_books_default (DhBookList *book_list)
{
        return book_list->priv->books;
}

static void
dh_book_list_class_init (DhBookListClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_book_list_dispose;
        object_class->finalize = dh_book_list_finalize;

        klass->add_book = dh_book_list_add_book_default;
        klass->remove_book = dh_book_list_remove_book_default;
        klass->get_books = dh_book_list_get_books_default;

        /**
         * DhBookList::add-book:
         * @book_list: the #DhBookList emitting the signal.
         * @book: the #DhBook being added.
         *
         * The ::add-book signal is emitted when a #DhBook is added to a
         * #DhBookList.
         *
         * The default object method handler adds @book to the internal #GList
         * of @book_list after verifying that @book is not already present in
         * the list.
         *
         * Since: 3.30
         */
        signals[SIGNAL_ADD_BOOK] =
                g_signal_new ("add-book",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DhBookListClass, add_book),
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1, DH_TYPE_BOOK);

        /**
         * DhBookList::remove-book:
         * @book_list: the #DhBookList emitting the signal.
         * @book: the #DhBook being removed.
         *
         * The ::remove-book signal is emitted when a #DhBook is removed from a
         * #DhBookList.
         *
         * The default object method handler removes @book from the internal
         * #GList of @book_list, and verifies that @book was present in the list
         * and that @book was not inserted several times.
         *
         * Since: 3.30
         */
        signals[SIGNAL_REMOVE_BOOK] =
                g_signal_new ("remove-book",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DhBookListClass, remove_book),
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1, DH_TYPE_BOOK);
}

static void
dh_book_list_init (DhBookList *book_list)
{
        book_list->priv = dh_book_list_get_instance_private (book_list);
}

/**
 * dh_book_list_new:
 *
 * Returns: (transfer full): a new empty #DhBookList object.
 * Since: 3.30
 */
DhBookList *
dh_book_list_new (void)
{
        return g_object_new (DH_TYPE_BOOK_LIST, NULL);
}

/**
 * dh_book_list_get_default:
 *
 * Gets the default #DhBookList object. It is created with #DhBookListBuilder,
 * dh_book_list_builder_add_default_sub_book_lists() is called, and
 * dh_book_list_builder_read_books_disabled_setting() is called with the default
 * #DhSettings object as returned by dh_settings_get_default().
 *
 * Returns: (transfer none): the default #DhBookList object.
 * Since: 3.30
 */
DhBookList *
dh_book_list_get_default (void)
{
        if (default_instance == NULL) {
                DhSettings *default_settings;
                DhBookListBuilder *builder;

                default_settings = dh_settings_get_default ();

                builder = dh_book_list_builder_new ();
                dh_book_list_builder_add_default_sub_book_lists (builder);
                dh_book_list_builder_read_books_disabled_setting (builder, default_settings);
                default_instance = dh_book_list_builder_create_object (builder);
                g_object_unref (builder);
        }

        return default_instance;
}

void
_dh_book_list_unref_default (void)
{
        if (default_instance != NULL)
                g_object_unref (default_instance);

        /* default_instance is not set to NULL here, it is set to NULL in
         * dh_book_list_finalize() (i.e. when we are sure that the ref count
         * reaches 0).
         */
}

/**
 * dh_book_list_get_books:
 * @book_list: a #DhBookList.
 *
 * Gets the list of #DhBook's part of @book_list, in no particular order. Each
 * book ID in the list is unique (see dh_book_get_id()).
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
 * dh_book_list_add_book:
 * @book_list: a #DhBookList.
 * @book: a #DhBook.
 *
 * Emits the #DhBookList::add-book signal.
 *
 * It is a programmer error to call this function if @book is already inserted
 * in @book_list.
 *
 * Since: 3.30
 */
void
dh_book_list_add_book (DhBookList *book_list,
                       DhBook     *book)
{
        g_return_if_fail (DH_IS_BOOK_LIST (book_list));
        g_return_if_fail (DH_IS_BOOK (book));

        g_signal_emit (book_list,
                       signals[SIGNAL_ADD_BOOK],
                       0,
                       book);
}

/**
 * dh_book_list_remove_book:
 * @book_list: a #DhBookList.
 * @book: a #DhBook.
 *
 * Emits the #DhBookList::remove-book signal.
 *
 * It is a programmer error to call this function if @book is not present in
 * @book_list.
 *
 * Since: 3.30
 */
void
dh_book_list_remove_book (DhBookList *book_list,
                          DhBook     *book)
{
        g_return_if_fail (DH_IS_BOOK_LIST (book_list));
        g_return_if_fail (DH_IS_BOOK (book));

        /* Keep the DhBook alive during the whole signal emission. */
        g_object_ref (book);

        g_signal_emit (book_list,
                       signals[SIGNAL_REMOVE_BOOK],
                       0,
                       book);

        g_object_unref (book);
}
