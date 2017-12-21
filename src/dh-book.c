/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
 * Copyright (C) 2017 Sébastien Wilmet <swilmet@gnome.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "dh-book.h"
#include <glib/gi18n-lib.h>
#include "dh-link.h"
#include "dh-parser.h"
#include "dh-util.h"

/**
 * SECTION:dh-book
 * @Title: DhBook
 * @Short_description: A book, usually the documentation for one library
 *
 * A #DhBook usually contains the documentation for one library (or
 * application), for example GLib or GTK+. A #DhBook corresponds to one index
 * file. An index file is a file with the extension `*.devhelp`, `*.devhelp2`,
 * `*.devhelp.gz` or `*.devhelp2.gz`.
 *
 * #DhBook creates a #GFileMonitor on the index file, and emits the
 * #DhBook::updated or #DhBook::deleted signal in case the index file has
 * changed on the filesystem. #DhBookManager listens to those #DhBook signals,
 * and emits in turn the #DhBookManager::book-deleted and
 * #DhBookManager::book-created signals.
 */

/* Timeout to wait for new events on the index file so that they are merged and
 * we don't spam unneeded signals.
 */
#define EVENT_MERGE_TIMEOUT_SECS (2)

enum {
        /* FIXME: a boolean property would be a better API instead of the
         * ::enabled and ::disabled signals. Or this whole concept can be
         * removed from DhBook, by introducing DhBookSelection, see:
         * https://bugzilla.gnome.org/show_bug.cgi?id=784491#c3
         */
        SIGNAL_ENABLED,
        SIGNAL_DISABLED,

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

        /* Generated list of completions (gchar*) in the book. */
        GList *completions;

        GFileMonitor *index_file_monitor;
        BookMonitorEvent last_monitor_event;
        guint monitor_event_timeout_id;

        guint enabled : 1;
} DhBookPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhBook, dh_book, G_TYPE_OBJECT);

static guint signals[N_SIGNALS] = { 0 };

static void
dh_book_dispose (GObject *object)
{
        DhBookPrivate *priv;

        priv = dh_book_get_instance_private (DH_BOOK (object));

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
        g_list_free_full (priv->completions, g_free);

        G_OBJECT_CLASS (dh_book_parent_class)->finalize (object);
}

static void
dh_book_class_init (DhBookClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_book_dispose;
        object_class->finalize = dh_book_finalize;

        /**
         * DhBook::enabled:
         * @book: the #DhBook emitting the signal.
         */
        signals[SIGNAL_ENABLED] =
                g_signal_new ("enabled",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              0);

        /**
         * DhBook::disabled:
         * @book: the #DhBook emitting the signal.
         */
        signals[SIGNAL_DISABLED] =
                g_signal_new ("disabled",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              0);

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

        priv->enabled = TRUE;
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
        if (!dh_parser_read_file (priv->index_file,
                                  &priv->title,
                                  &priv->id,
                                  &language,
                                  &priv->tree,
                                  &priv->links,
                                  &error)) {
                if (error != NULL) {
                        gchar *parse_name;

                        parse_name = g_file_get_parse_name (priv->index_file);

                        g_warning ("Failed to read “%s”: %s",
                                   parse_name,
                                   error->message);

                        g_free (parse_name);
                        g_clear_error (&error);
                }

                /* Deallocate the book, as we are not going to add it in the
                 * manager.
                 */
                g_object_unref (book);
                return NULL;
        }

        /* Rewrite language, if any, including the prefix we want to use when
         * seeing it, to standarize how the language group is shown.
         * FIXME: do this in DhLanguage instead (this may require to implement
         * DhLanguage differently).
         */
        dh_util_ascii_strtitle (language);
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
 * dh_book_get_links:
 * @book: a #DhBook.
 *
 * Returns: (element-type DhLink) (transfer none) (nullable): the list of
 * <emphasis>all</emphasis> #DhLink's part of @book, or %NULL if the book is
 * disabled.
 */
GList *
dh_book_get_links (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->enabled ? priv->links : NULL;
}

/**
 * dh_book_get_completions:
 * @book: a #DhBook.
 *
 * Returns: (element-type utf8) (transfer none) (nullable): the completions
 * associated with the book.
 */
GList *
dh_book_get_completions (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        if (!priv->enabled)
                return NULL;

        if (priv->completions == NULL) {
                GList *l;

                for (l = priv->links; l != NULL; l = l->next) {
                        DhLink *link = l->data;
                        gchar *str;

                        /* Add additional "page:" and "book:" completions.
                         * FIXME: broken, doesn't match what DhKeywordModel
                         * does. But maybe the feature will be removed, see:
                         * https://bugzilla.gnome.org/show_bug.cgi?id=791442
                         */
                        if (dh_link_get_link_type (link) == DH_LINK_TYPE_BOOK) {
                                str = g_strdup_printf ("book:%s", dh_link_get_name (link));
                                priv->completions = g_list_prepend (priv->completions, str);
                        }
                        else if (dh_link_get_link_type (link) == DH_LINK_TYPE_PAGE) {
                                str = g_strdup_printf ("page:%s", dh_link_get_name (link));
                                priv->completions = g_list_prepend (priv->completions, str);
                        }

                        str = g_strdup (dh_link_get_name (link));
                        priv->completions = g_list_prepend (priv->completions, str);
                }
        }

        return priv->completions;
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
 * Returns: (transfer none) (nullable): the tree of #DhLink's part of the @book,
 * or %NULL if the book is disabled.
 */
GNode *
dh_book_get_tree (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->enabled ? priv->tree : NULL;
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
 * Returns: the book language.
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
 * dh_book_get_enabled:
 * @book: a #DhBook.
 *
 * Returns: whether the book is enabled.
 */
gboolean
dh_book_get_enabled (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), FALSE);

        priv = dh_book_get_instance_private (book);

        return priv->enabled;
}

/**
 * dh_book_set_enabled:
 * @book: a #DhBook.
 * @enabled: the new value.
 *
 * Enables or disables the book.
 */
void
dh_book_set_enabled (DhBook   *book,
                     gboolean  enabled)
{
        DhBookPrivate *priv;

        g_return_if_fail (DH_IS_BOOK (book));

        priv = dh_book_get_instance_private (book);

        enabled = enabled != FALSE;

        if (priv->enabled != enabled) {
                priv->enabled = enabled;
                g_signal_emit (book,
                               enabled ? signals[SIGNAL_ENABLED] : signals[SIGNAL_DISABLED],
                               0);
        }
}

/**
 * dh_book_cmp_by_path:
 * @a: a #DhBook.
 * @b: a #DhBook.
 *
 * Compares the #DhBook's by their path to the index file.
 *
 * Returns: an integer less than, equal to, or greater than zero, if @a is <, ==
 * or > than @b.
 */
gint
dh_book_cmp_by_path (DhBook *a,
                     DhBook *b)
{
        DhBookPrivate *priv_a;
        DhBookPrivate *priv_b;
        gchar *path_a;
        gchar *path_b;
        gint diff;

        if (a == NULL || b == NULL)
                return -1;

        priv_a = dh_book_get_instance_private (a);
        priv_b = dh_book_get_instance_private (b);

        path_a = g_file_get_path (priv_a->index_file);
        path_b = g_file_get_path (priv_b->index_file);

        diff = g_strcmp0 (path_a, path_b);

        g_free (path_a);
        g_free (path_b);
        return diff;
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
