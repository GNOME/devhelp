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
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <string.h>

#include "dh-link.h"
#include "dh-parser.h"
#include "dh-book.h"
#include "dh-marshal.h"

/* Timeout to wait for new events in the book so that
 * they are merged and we don't spam unneeded signals */
#define EVENT_MERGE_TIMEOUT_SECS 2

/* Signals managed by the DhBook */
enum {
	BOOK_UPDATED,
        BOOK_DELETED,
	BOOK_LAST_SIGNAL
};

/* Structure defining basic contents to store about every book */
typedef struct {
        /* File path of the book */
        gchar        *path;
        /* Enable or disabled? */
        gboolean      enabled;
        /* Book name */
        gchar        *name;
        /* Book title */
        gchar        *title;
        /* Generated book tree */
        GNode        *tree;
        /* Generated list of keywords in the book */
        GList        *keywords;

        /* Monitor of this specific book */
        GFileMonitor *monitor;
        /* Last received events */
        gboolean      is_deleted;
        gboolean      is_updated;
        /* ID of the event source */
        guint         monitor_event_timeout_id;
} DhBookPriv;

G_DEFINE_TYPE (DhBook, dh_book, G_TYPE_OBJECT);

#define GET_PRIVATE(instance) G_TYPE_INSTANCE_GET_PRIVATE       \
        (instance, DH_TYPE_BOOK, DhBookPriv)

static void    dh_book_init          (DhBook            *book);
static void    dh_book_class_init    (DhBookClass       *klass);
static void    book_monitor_event_cb (GFileMonitor      *file_monitor,
                                      GFile             *file,
                                      GFile             *other_file,
                                      GFileMonitorEvent  event_type,
                                      gpointer	         user_data);
static void    unref_node_link       (GNode             *node,
                                      gpointer           data);

static guint signals[BOOK_LAST_SIGNAL] = { 0 };

static void
book_finalize (GObject *object)
{
        DhBookPriv *priv;

        priv = GET_PRIVATE (object);

        if (priv->tree) {
                g_node_traverse (priv->tree,
                                 G_IN_ORDER,
                                 G_TRAVERSE_ALL,
                                 -1,
                                 (GNodeTraverseFunc)unref_node_link,
                                 NULL);
                g_node_destroy (priv->tree);
        }

        if (priv->keywords) {
                g_list_foreach (priv->keywords, (GFunc)dh_link_unref, NULL);
                g_list_free (priv->keywords);
        }

        if (priv->monitor) {
                g_object_unref (priv->monitor);
        }

        g_free (priv->title);

        g_free (priv->path);

        G_OBJECT_CLASS (dh_book_parent_class)->finalize (object);
}

static void
dh_book_class_init (DhBookClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = book_finalize;

	signals[BOOK_UPDATED] =
		g_signal_new ("updated",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
                              _dh_marshal_VOID__VOID,
		              G_TYPE_NONE,
                              0);

	signals[BOOK_DELETED] =
		g_signal_new ("deleted",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
                              _dh_marshal_VOID__VOID,
		              G_TYPE_NONE,
		              0);

	g_type_class_add_private (klass, sizeof (DhBookPriv));
}

static void
dh_book_init (DhBook *book)
{
        DhBookPriv *priv = GET_PRIVATE (book);

        priv->name = NULL;
        priv->path = NULL;
        priv->title = NULL;
        priv->enabled = TRUE;
        priv->tree = NULL;
        priv->keywords = NULL;
        priv->monitor = NULL;
        priv->is_deleted = FALSE;
        priv->is_updated = FALSE;
        priv->monitor_event_timeout_id = 0;
}

static void
unref_node_link (GNode    *node,
                 gpointer  data)
{
        dh_link_unref (node->data);
}

DhBook *
dh_book_new (const gchar *book_path)
{
        DhBookPriv *priv;
        DhBook     *book;
        GError     *error = NULL;
        GFile      *book_path_file;

        g_return_val_if_fail (book_path, NULL);

        book = g_object_new (DH_TYPE_BOOK, NULL);
        priv = GET_PRIVATE (book);

        /* Parse file storing contents in the book struct */
        if (!dh_parser_read_file  (book_path,
                                   &priv->tree,
                                   &priv->keywords,
                                   &error)) {
                g_warning ("Failed to read '%s': %s",
                           priv->path, error->message);
                g_error_free (error);

                /* Deallocate the book, as we are not going to add it
                 *  in the manager */
                g_object_unref (book);
                return NULL;
        }

        /* Store path */
        priv->path = g_strdup (book_path);

        /* Setup title */
        priv->title = g_strdup (dh_link_get_name ((DhLink *)priv->tree->data));

        /* Setup name */
        priv->name = g_strdup (dh_link_get_book_id ((DhLink *)priv->tree->data));

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

static gboolean
book_monitor_event_timeout_cb  (gpointer data)
{
        DhBook *book = data;
        DhBookPriv *priv;

        priv = GET_PRIVATE (book);

        /* We'll get either is_deleted OR is_updated,
         * not possible to have both or none */
        if (priv->is_deleted) {
                /* Emit the signal, but make sure we hold a reference
                 * while doing it */
                g_object_ref (book);
		g_signal_emit (book, signals[BOOK_DELETED], 0);
                g_object_unref (book);
        } else if (priv->is_updated) {
		g_signal_emit (book, signals[BOOK_UPDATED], 0);
        } else {
                g_warn_if_reached ();
        }

        return FALSE;
}

static void
book_monitor_event_cb (GFileMonitor      *file_monitor,
                       GFile             *file,
                       GFile             *other_file,
                       GFileMonitorEvent  event_type,
                       gpointer	          user_data)
{
        DhBook *book = user_data;
        DhBookPriv *priv;
        gboolean reset_timer = FALSE;

        priv = GET_PRIVATE (book);

        switch (event_type) {
        case G_FILE_MONITOR_EVENT_CREATED:
                /* This may happen if the file is deleted and then
                 * created right away, as we're merging events.
                 * Treat in the same way as a CHANGES_DONE_HINT, so
                 * fall through the case.  */
        case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
                priv->is_deleted = FALSE; /* Reset any previous one */
                priv->is_updated = TRUE;
                reset_timer = TRUE;
                break;
        case G_FILE_MONITOR_EVENT_DELETED:
                priv->is_deleted = TRUE;
                priv->is_updated = FALSE; /* Reset any previous one */
                reset_timer = TRUE;
                break;
        default:
                /* Ignore all the other events */
                break;
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

GList *
dh_book_get_keywords (DhBook *book)
{
        DhBookPriv *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = GET_PRIVATE (book);

        return priv->enabled ? priv->keywords : NULL;
}

GNode *
dh_book_get_tree (DhBook *book)
{
        DhBookPriv *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = GET_PRIVATE (book);

        return priv->enabled ? priv->tree : NULL;
}

const gchar *
dh_book_get_name (DhBook *book)
{
        DhBookPriv *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = GET_PRIVATE (book);

        return priv->name;
}

const gchar *
dh_book_get_title (DhBook *book)
{
        DhBookPriv *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = GET_PRIVATE (book);

        return priv->title;
}

const gchar *
dh_book_get_path (DhBook *book)
{
        DhBookPriv *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = GET_PRIVATE (book);

        return priv->path;
}

gboolean
dh_book_get_enabled (DhBook *book)
{
        g_return_val_if_fail (DH_IS_BOOK (book), FALSE);

        return GET_PRIVATE (book)->enabled;
}

void
dh_book_set_enabled (DhBook *book,
                     gboolean enabled)
{
        g_return_if_fail (DH_IS_BOOK (book));

        GET_PRIVATE (book)->enabled = enabled;
}

gint
dh_book_cmp_by_path (const DhBook *a,
                     const DhBook *b)
{
        return ((a && b) ?
                g_strcmp0 (GET_PRIVATE (a)->path, GET_PRIVATE (b)->path) :
                -1);
}

gint
dh_book_cmp_by_path_str (const DhBook *a,
                         const gchar  *b_path)
{
        return ((a && b_path) ?
                g_strcmp0 (GET_PRIVATE (a)->path, b_path) :
                -1);
}

gint
dh_book_cmp_by_name (const DhBook *a,
                     const DhBook *b)
{
        return ((a && b) ?
                g_ascii_strcasecmp (GET_PRIVATE (a)->name, GET_PRIVATE (b)->name) :
                -1);
}

gint
dh_book_cmp_by_name_str (const DhBook *a,
                         const gchar  *b_name)
{
        return ((a && b_name) ?
                g_ascii_strcasecmp (GET_PRIVATE (a)->name, b_name) :
                -1);
}

gint
dh_book_cmp_by_title (const DhBook *a,
                      const DhBook *b)
{
        return ((a && b) ?
                g_utf8_collate (GET_PRIVATE (a)->title, GET_PRIVATE (b)->title) :
                -1);
}
