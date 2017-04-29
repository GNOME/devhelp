/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "dh-book.h"

#include <glib/gi18n-lib.h>

#include "dh-link.h"
#include "dh-parser.h"
#include "dh-util.h"

/* Timeout to wait for new events in the book so that
 * they are merged and we don't spam unneeded signals */
#define EVENT_MERGE_TIMEOUT_SECS 2

/* Signals managed by the DhBook */
enum {
        BOOK_ENABLED,
        BOOK_DISABLED,
        BOOK_UPDATED,
        BOOK_DELETED,
        BOOK_LAST_SIGNAL
};

typedef enum {
        BOOK_MONITOR_EVENT_NONE,
        BOOK_MONITOR_EVENT_UPDATED,
        BOOK_MONITOR_EVENT_DELETED
} DhBookMonitorEvent;

/* Structure defining basic contents to store about every book */
typedef struct {
        gchar        *path;
        gchar        *name;
        gchar        *title;
        gchar        *language;

        /* The book tree of DhLink* */
        GNode        *tree;

        /* List of DhLink* */
        GList        *keywords;

        /* Generated list of keyword completions (gchar*) in the book */
        GList        *completions;

        /* Monitor of this specific book */
        GFileMonitor *monitor;

        /* Last received event */
        DhBookMonitorEvent monitor_event;

        /* ID of the event source */
        guint         monitor_event_timeout_id;

        guint         enabled : 1;
} DhBookPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhBook, dh_book, G_TYPE_OBJECT);

static guint signals[BOOK_LAST_SIGNAL] = { 0 };

static void
dh_book_dispose (GObject *object)
{
        DhBookPrivate *priv;

        priv = dh_book_get_instance_private (DH_BOOK (object));

        g_clear_object (&priv->monitor);

        if (priv->monitor_event_timeout_id != 0) {
                g_source_remove (priv->monitor_event_timeout_id);
                priv->monitor_event_timeout_id = 0;
        }

        G_OBJECT_CLASS (dh_book_parent_class)->dispose (object);
}

static gboolean
unref_node_link (GNode    *node,
                 gpointer  data)
{
        dh_link_unref (node->data);
        return FALSE;
}

static void
dh_book_finalize (GObject *object)
{
        DhBookPrivate *priv;

        priv = dh_book_get_instance_private (DH_BOOK (object));

        if (priv->tree != NULL) {
                g_node_traverse (priv->tree,
                                 G_IN_ORDER,
                                 G_TRAVERSE_ALL,
                                 -1,
                                 unref_node_link,
                                 NULL);
                g_node_destroy (priv->tree);
        }

        g_list_free_full (priv->keywords, (GDestroyNotify)dh_link_unref);
        g_list_free_full (priv->completions, g_free);
        g_free (priv->language);
        g_free (priv->title);
        g_free (priv->name);
        g_free (priv->path);

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
         * @book: the book on which the signal is emitted.
         */
        signals[BOOK_ENABLED] =
                g_signal_new ("enabled",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              0);

        /**
         * DhBook::disabled:
         * @book: the book on which the signal is emitted.
         */
        signals[BOOK_DISABLED] =
                g_signal_new ("disabled",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              0);

        /**
         * DhBook::updated:
         * @book: the book on which the signal is emitted.
         */
        signals[BOOK_UPDATED] =
                g_signal_new ("updated",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              0);

        /**
         * DhBook::deleted:
         * @book: the book on which the signal is emitted.
         */
        signals[BOOK_DELETED] =
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
        priv->monitor_event = BOOK_MONITOR_EVENT_NONE;
}

static gboolean
book_monitor_event_timeout_cb (gpointer data)
{
        DhBook *book = data;
        DhBookPrivate *priv = dh_book_get_instance_private (book);
        DhBookMonitorEvent monitor_event = priv->monitor_event;

        /* Reset event */
        priv->monitor_event = BOOK_MONITOR_EVENT_NONE;
        priv->monitor_event_timeout_id = 0;

        /* We'll get either is_deleted OR is_updated,
         * not possible to have both or none */
        switch (monitor_event)
        {
        case BOOK_MONITOR_EVENT_DELETED:
                /* Emit the signal, but make sure we hold a reference
                 * while doing it */
                g_object_ref (book);
                g_signal_emit (book, signals[BOOK_DELETED], 0);
                g_object_unref (book);
                break;
        case BOOK_MONITOR_EVENT_UPDATED:
                /* Emit the signal, but make sure we hold a reference
                 * while doing it */
                g_object_ref (book);
                g_signal_emit (book, signals[BOOK_UPDATED], 0);
                g_object_unref (book);
                break;
        case BOOK_MONITOR_EVENT_NONE:
        default:
                break;
        }

        /* book can be destroyed here */

        return G_SOURCE_REMOVE;
}

static void
book_monitor_event_cb (GFileMonitor      *file_monitor,
                       GFile             *file,
                       GFile             *other_file,
                       GFileMonitorEvent  event_type,
                       gpointer           user_data)
{
        DhBook     *book = user_data;
        DhBookPrivate *priv = dh_book_get_instance_private (book);
        gboolean    reset_timer = FALSE;

        /* CREATED may happen if the file is deleted and then created right
         * away, as we're merging events.  Treat in the same way as a
         * CHANGES_DONE_HINT.
         */
        if (event_type == G_FILE_MONITOR_EVENT_CREATED ||
            event_type == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
                priv->monitor_event = BOOK_MONITOR_EVENT_UPDATED;
                reset_timer = TRUE;
        } else if (event_type == G_FILE_MONITOR_EVENT_DELETED) {
                priv->monitor_event = BOOK_MONITOR_EVENT_DELETED;
                reset_timer = TRUE;
        }

        /* Reset timer if any of the flags changed */
        if (reset_timer) {
                if (priv->monitor_event_timeout_id != 0) {
                        g_source_remove (priv->monitor_event_timeout_id);
                }
                priv->monitor_event_timeout_id = g_timeout_add_seconds (EVENT_MERGE_TIMEOUT_SECS,
                                                                        book_monitor_event_timeout_cb,
                                                                        book);
        }
}

/**
 * dh_book_new:
 * @book_path: the path of the book
 *
 * Returns: a new #DhBook object.
 */
DhBook *
dh_book_new (const gchar *book_path)
{
        DhBookPrivate *priv;
        DhBook     *book;
        GError     *error = NULL;
        GFile      *book_path_file;
        gchar      *language;

        g_return_val_if_fail (book_path, NULL);

        book = g_object_new (DH_TYPE_BOOK, NULL);
        priv = dh_book_get_instance_private (book);

        /* Parse file storing contents in the book struct */
        if (!dh_parser_read_file  (book_path,
                                   &priv->title,
                                   &priv->name,
                                   &language,
                                   &priv->tree,
                                   &priv->keywords,
                                   &error)) {
                g_warning ("Failed to read '%s': %s",
                           book_path, error->message);
                g_error_free (error);

                /* Deallocate the book, as we are not going to add it
                 *  in the manager */
                g_object_unref (book);
                return NULL;
        }

        /* Store path */
        priv->path = g_strdup (book_path);

        /* Rewrite language, if any, including the prefix we want
         * to use when seeing it. It is pretty ugly to do it here,
         * but it's the only way of making sure we standarize how
         * the language group is shown */
        dh_util_ascii_strtitle (language);
        priv->language = (language ?
                          g_strdup_printf (_("Language: %s"), language) :
                          g_strdup (_("Language: Undefined")));
        g_free (language);

        /* Setup monitor for changes */
        book_path_file = g_file_new_for_path (book_path);
        priv->monitor = g_file_monitor_file (book_path_file,
                                             G_FILE_MONITOR_NONE,
                                             NULL,
                                             NULL);
        if (priv->monitor) {
                /* Setup changed signal callback */
                g_signal_connect (priv->monitor,
                                  "changed",
                                  G_CALLBACK (book_monitor_event_cb),
                                  book);
        } else {
                g_warning ("Couldn't setup monitoring of changes in book '%s'",
                           priv->title);
        }
        g_object_unref (book_path_file);

        return book;
}

/**
 * dh_book_get_keywords:
 * @book: a #DhBook.
 *
 * Returns: (element-type DhLink) (transfer none) (nullable): the list of
 * #DhLink's part of @book.
 */
GList *
dh_book_get_keywords (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->enabled ? priv->keywords : NULL;
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

                for (l = priv->keywords; l != NULL; l = l->next) {
                        DhLink *link = l->data;
                        gchar *str;

                        /* Add additional "page:" and "book:" completions */
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
 * Returns: (transfer none) (nullable): the tree of #DhLink's part of the @book.
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
 * dh_book_get_name:
 * @book: a #DhBook.
 *
 * Returns: the book name.
 */
const gchar *
dh_book_get_name (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->name;
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
 * dh_book_get_path:
 * @book: a #DhBook.
 *
 * Returns: the book path.
 */
const gchar *
dh_book_get_path (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->path;
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
        if (priv->enabled != enabled) {
                priv->enabled = enabled;
                g_signal_emit (book,
                               enabled ? signals[BOOK_ENABLED] : signals[BOOK_DISABLED],
                               0);
        }
}

/**
 * dh_book_cmp_by_path:
 * @a: a #DhBook.
 * @b: a #DhBook.
 *
 * Compares the #DhBook's by their paths.
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

        if (a == NULL || b == NULL)
                return -1;

        priv_a = dh_book_get_instance_private (a);
        priv_b = dh_book_get_instance_private (b);

        return g_strcmp0 (priv_a->path, priv_b->path);
}

/**
 * dh_book_cmp_by_path_str:
 * @a: a #DhBook.
 * @b_path: a path to a book.
 *
 * Compares the paths of the #DhBook @a with @b_path.
 *
 * Returns: an integer less than, equal to, or greater than zero, if the path of
 * @a is <, == or > than @b_path.
 * Deprecated: Use dh_book_cmp_by_path() instead.
 */
gint
dh_book_cmp_by_path_str (DhBook      *a,
                         const gchar *b_path)
{
        DhBookPrivate *priv_a;

        if (a == NULL)
                return -1;

        priv_a = dh_book_get_instance_private (a);

        return g_strcmp0 (priv_a->path, b_path);
}

/**
 * dh_book_cmp_by_name:
 * @a: a #DhBook.
 * @b: a #DhBook.
 *
 * Compares the #DhBook's by their name.
 *
 * Returns: an integer less than, equal to, or greater than zero, if @a is <, ==
 * or > than @b.
 */
gint
dh_book_cmp_by_name (DhBook *a,
                     DhBook *b)
{
        DhBookPrivate *priv_a;
        DhBookPrivate *priv_b;

        if (a == NULL || b == NULL)
                return -1;

        priv_a = dh_book_get_instance_private (a);
        priv_b = dh_book_get_instance_private (b);

        if (priv_a->name == NULL || priv_b->name == NULL)
                return -1;

        return g_ascii_strcasecmp (priv_a->name, priv_b->name);
}

/**
 * dh_book_cmp_by_name_str:
 * @a: a #DhBook object
 * @b_name: a path to a book
 *
 * Compare the name of the #DhBook a with @b_name.
 *
 * Returns: an integer less than, equal to, or greater than zero, if the name of
 * @a is <, == or > than @b_name.
 * Deprecated: Use dh_book_cmp_by_name() instead.
 */
gint
dh_book_cmp_by_name_str (DhBook      *a,
                         const gchar *b_name)
{
        DhBookPrivate *priv_a;

        if (a == NULL)
                return -1;

        priv_a = dh_book_get_instance_private (a);

        if (priv_a->name == NULL || b_name == NULL)
                return -1;

        return g_ascii_strcasecmp (priv_a->name, b_name);
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
