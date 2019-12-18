/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
 * Copyright (C) 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
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

#include "config.h"
#include "dh-book.h"
#include <glib/gi18n-lib.h>
#include "dh-link.h"
#include "dh-parser.h"
#include "dh-util-lib.h"

/**
 * SECTION:dh-book
 * @Title: DhBook
 * @Short_description: A book, usually the documentation for one library
 *
 * A #DhBook usually contains the documentation for one library (or
 * application), for example GLib or GTK. A #DhBook corresponds to one index
 * file. An index file is a file with the extension `*.devhelp`, `*.devhelp2`,
 * `*.devhelp.gz` or `*.devhelp2.gz`.
 *
 * #DhBook creates a #GFileMonitor on the index file, and emits the
 * #DhBook::updated or #DhBook::deleted signal in case the index file has
 * changed on the filesystem. #DhBookListDirectory listens to those #DhBook
 * signals, and emits in turn the #DhBookList #DhBookList::remove-book and
 * #DhBookList::add-book signals.
 */

/* Timeout to wait for new events on the index file so that they are merged and
 * we don't spam unneeded signals.
 */
#define EVENT_MERGE_TIMEOUT_SECS (2)

enum {
        SIGNAL_UPDATED,
        SIGNAL_DELETED,
        N_SIGNALS
};

typedef enum {
        BOOK_MONITOR_EVENT_NONE,
        BOOK_MONITOR_EVENT_UPDATED,
        BOOK_MONITOR_EVENT_DELETED
} BookMonitorEvent;

typedef struct {
        GFile *index_file;

        gchar *id;
        gchar *title;
        gchar *language;

        /* The book tree of DhLink*. */
        GNode *tree;

        /* List of DhLink*. */
        GList *links;

        DhCompletion *completion;

        GFileMonitor *index_file_monitor;
        BookMonitorEvent last_monitor_event;
        guint monitor_event_timeout_id;
} DhBookPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhBook, dh_book, G_TYPE_OBJECT);

static guint signals[N_SIGNALS] = { 0 };

static void
dh_book_dispose (GObject *object)
{
        DhBookPrivate *priv;

        priv = dh_book_get_instance_private (DH_BOOK (object));

        g_clear_object (&priv->completion);
        g_clear_object (&priv->index_file_monitor);

        if (priv->monitor_event_timeout_id != 0) {
                g_source_remove (priv->monitor_event_timeout_id);
                priv->monitor_event_timeout_id = 0;
        }

        G_OBJECT_CLASS (dh_book_parent_class)->dispose (object);
}

static void
dh_book_finalize (GObject *object)
{
        DhBookPrivate *priv;

        priv = dh_book_get_instance_private (DH_BOOK (object));

        g_clear_object (&priv->index_file);
        g_free (priv->id);
        g_free (priv->title);
        g_free (priv->language);
        _dh_util_free_book_tree (priv->tree);
        g_list_free_full (priv->links, (GDestroyNotify)dh_link_unref);

        G_OBJECT_CLASS (dh_book_parent_class)->finalize (object);
}

static void
dh_book_class_init (DhBookClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_book_dispose;
        object_class->finalize = dh_book_finalize;

        /**
         * DhBook::updated:
         * @book: the #DhBook emitting the signal.
         *
         * The ::updated signal is emitted when the index file has been
         * modified (but the file still exists).
         */
        signals[SIGNAL_UPDATED] =
                g_signal_new ("updated",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              0);

        /**
         * DhBook::deleted:
         * @book: the #DhBook emitting the signal.
         *
         * The ::deleted signal is emitted when the index file has been deleted
         * from the filesystem.
         */
        signals[SIGNAL_DELETED] =
                g_signal_new ("deleted",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              0);
}

static void
dh_book_init (DhBook *book)
{
        DhBookPrivate *priv = dh_book_get_instance_private (book);

        priv->last_monitor_event = BOOK_MONITOR_EVENT_NONE;
}

static gboolean
monitor_event_timeout_cb (gpointer data)
{
        DhBook *book = DH_BOOK (data);
        DhBookPrivate *priv = dh_book_get_instance_private (book);
        BookMonitorEvent last_monitor_event = priv->last_monitor_event;

        /* Reset event */
        priv->last_monitor_event = BOOK_MONITOR_EVENT_NONE;
        priv->monitor_event_timeout_id = 0;

        /* We'll get either is_deleted OR is_updated, not possible to have both
         * or none.
         */
        switch (last_monitor_event)
        {
        case BOOK_MONITOR_EVENT_DELETED:
                /* Emit the signal, but make sure we hold a reference while
                 * doing it.
                 */
                g_object_ref (book);
                g_signal_emit (book, signals[SIGNAL_DELETED], 0);
                g_object_unref (book);
                break;

        case BOOK_MONITOR_EVENT_UPDATED:
                /* Emit the signal, but make sure we hold a reference while
                 * doing it.
                 */
                g_object_ref (book);
                g_signal_emit (book, signals[SIGNAL_UPDATED], 0);
                g_object_unref (book);
                break;

        case BOOK_MONITOR_EVENT_NONE:
        default:
                break;
        }

        /* book can be destroyed here. */

        return G_SOURCE_REMOVE;
}

static void
index_file_changed_cb (GFileMonitor      *file_monitor,
                       GFile             *file,
                       GFile             *other_file,
                       GFileMonitorEvent  event_type,
                       DhBook            *book)
{
        DhBookPrivate *priv = dh_book_get_instance_private (book);
        gboolean reset_timeout = FALSE;

        /* CREATED may happen if the file is deleted and then created right
         * away, as we're merging events.
         */
        if (event_type == G_FILE_MONITOR_EVENT_CHANGED ||
            event_type == G_FILE_MONITOR_EVENT_CREATED) {
                priv->last_monitor_event = BOOK_MONITOR_EVENT_UPDATED;
                reset_timeout = TRUE;
        } else if (event_type == G_FILE_MONITOR_EVENT_DELETED) {
                priv->last_monitor_event = BOOK_MONITOR_EVENT_DELETED;
                reset_timeout = TRUE;
        }

        if (reset_timeout) {
                if (priv->monitor_event_timeout_id != 0)
                        g_source_remove (priv->monitor_event_timeout_id);

                priv->monitor_event_timeout_id = g_timeout_add_seconds (EVENT_MERGE_TIMEOUT_SECS,
                                                                        monitor_event_timeout_cb,
                                                                        book);
        }
}

/**
 * dh_book_new:
 * @index_file: the index file.
 *
 * Returns: (nullable): a new #DhBook object, or %NULL if parsing the index file
 * failed.
 */
DhBook *
dh_book_new (GFile *index_file)
{
        DhBookPrivate *priv;
        DhBook *book;
        gchar *language = NULL;
        GError *error = NULL;

        g_return_val_if_fail (G_IS_FILE (index_file), NULL);

        book = g_object_new (DH_TYPE_BOOK, NULL);
        priv = dh_book_get_instance_private (book);

        priv->index_file = g_object_ref (index_file);

        /* Parse file storing contents in the book struct. */
        if (!_dh_parser_read_file (priv->index_file,
                                   &priv->title,
                                   &priv->id,
                                   &language,
                                   &priv->tree,
                                   &priv->links,
                                   &error)) {
                /* It's fine if the file doesn't exist, because
                 * DhBookListDirectory tries to create a DhBook for each
                 * possible index file in a certain book directory.
                 */
                if (error != NULL &&
                    !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
                        gchar *parse_name;

                        parse_name = g_file_get_parse_name (priv->index_file);

                        g_warning ("Failed to read “%s”: %s",
                                   parse_name,
                                   error->message);

                        g_free (parse_name);
                }

                g_clear_error (&error);

                /* Deallocate the book, as we are not going to add it in the
                 * manager.
                 */
                g_object_unref (book);
                return NULL;
        }

        /* Rewrite language, if any, including the prefix we want to use when
         * seeing it, to standarize how the language group is shown.
         * FIXME: maybe instead of a string, have a DhLanguage object which
         * canonicalizes the string.
         */
        _dh_util_ascii_strtitle (language);
        priv->language = (language != NULL ?
                          g_strdup_printf (_("Language: %s"), language) :
                          g_strdup (_("Language: Undefined")));
        g_free (language);

        /* Setup monitor for changes */

        priv->index_file_monitor = g_file_monitor_file (priv->index_file,
                                                        G_FILE_MONITOR_NONE,
                                                        NULL,
                                                        &error);

        if (error != NULL) {
                gchar *parse_name;

                parse_name = g_file_get_parse_name (priv->index_file);

                g_warning ("Failed to create file monitor for file “%s”: %s",
                           parse_name,
                           error->message);

                g_free (parse_name);
                g_clear_error (&error);
        }

        if (priv->index_file_monitor != NULL) {
                g_signal_connect_object (priv->index_file_monitor,
                                         "changed",
                                         G_CALLBACK (index_file_changed_cb),
                                         book,
                                         0);
        }

        return book;
}

/**
 * dh_book_get_index_file:
 * @book: a #DhBook.
 *
 * Returns: (transfer none): the index file.
 */
GFile *
dh_book_get_index_file (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->index_file;
}

/**
 * dh_book_get_id:
 * @book: a #DhBook.
 *
 * Gets the book ID. In the Devhelp index file format version 2, it is actually
 * the “name”, not the ID, but “book ID” is clearer, “book name” can be confused
 * with the title.
 *
 * Returns: the book ID.
 */
const gchar *
dh_book_get_id (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->id;
}

/**
 * dh_book_get_title:
 * @book: a #DhBook.
 *
 * Returns: the book title.
 */
const gchar *
dh_book_get_title (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->title;
}

/**
 * dh_book_get_language:
 * @book: a #DhBook.
 *
 * Returns: the programming language used in @book.
 */
const gchar *
dh_book_get_language (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->language;
}

/**
 * dh_book_get_links:
 * @book: a #DhBook.
 *
 * Returns: (element-type DhLink) (transfer none): the list of
 * <emphasis>all</emphasis> #DhLink's part of @book.
 */
GList *
dh_book_get_links (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->links;
}

/**
 * dh_book_get_tree:
 * @book: a #DhBook.
 *
 * Gets the general structure of the book, as a tree. The tree contains only
 * #DhLink's of type %DH_LINK_TYPE_BOOK or %DH_LINK_TYPE_PAGE. The other
 * #DhLink's are not contained in the tree. To have a list of
 * <emphasis>all</emphasis> #DhLink's part of the book, you need to call
 * dh_book_get_links().
 *
 * Returns: (transfer none): the tree of #DhLink's part of @book.
 */
GNode *
dh_book_get_tree (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->tree;
}

/**
 * dh_book_get_completion:
 * @book: a #DhBook.
 *
 * Returns: (transfer none): the #DhCompletion of @book.
 * Since: 3.28
 */
DhCompletion *
dh_book_get_completion (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        if (priv->completion == NULL) {
                GList *l;

                priv->completion = dh_completion_new ();

                for (l = priv->links; l != NULL; l = l->next) {
                        DhLink *link = l->data;
                        const gchar *str;

                        /* Do not provide completion for book titles. Normally
                         * the user doesn't need it, it's more convenient to
                         * choose a book with the DhBookTree.
                         */
                        if (dh_link_get_link_type (link) == DH_LINK_TYPE_BOOK)
                                continue;

                        str = dh_link_get_name (link);
                        dh_completion_add_string (priv->completion, str);
                }

                dh_completion_sort (priv->completion);
        }

        return priv->completion;
}

/**
 * dh_book_cmp_by_id:
 * @a: a #DhBook.
 * @b: a #DhBook.
 *
 * Compares the #DhBook's by their IDs, with g_ascii_strcasecmp().
 *
 * Returns: an integer less than, equal to, or greater than zero, if @a is <, ==
 * or > than @b.
 */
gint
dh_book_cmp_by_id (DhBook *a,
                   DhBook *b)
{
        DhBookPrivate *priv_a;
        DhBookPrivate *priv_b;

        if (a == NULL || b == NULL)
                return -1;

        priv_a = dh_book_get_instance_private (a);
        priv_b = dh_book_get_instance_private (b);

        if (priv_a->id == NULL || priv_b->id == NULL)
                return -1;

        return g_ascii_strcasecmp (priv_a->id, priv_b->id);
}

/**
 * dh_book_cmp_by_title:
 * @a: a #DhBook.
 * @b: a #DhBook.
 *
 * Compares the #DhBook's by their title.
 *
 * Returns: an integer less than, equal to, or greater than zero, if @a is <, ==
 * or > than @b.
 */
gint
dh_book_cmp_by_title (DhBook *a,
                      DhBook *b)
{
        DhBookPrivate *priv_a;
        DhBookPrivate *priv_b;

        if (a == NULL || b == NULL)
                return -1;

        priv_a = dh_book_get_instance_private (a);
        priv_b = dh_book_get_instance_private (b);

        if (priv_a->title == NULL || priv_b->title == NULL)
                return -1;

        return g_utf8_collate (priv_a->title, priv_b->title);
}
