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

#include <glib/gi18n.h>

#include "dh-link.h"
#include "dh-parser.h"
#include "dh-book.h"
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
        /* File path of the book */
        gchar        *path;
        /* Enable or disabled? */
        gboolean      enabled;
        /* Book name */
        gchar        *name;
        /* Book title */
        gchar        *title;
        /* Book language */
        gchar        *language;
        /* Generated book tree */
        GNode        *tree;
        /* Generated list of keywords in the book */
        GList        *keywords;
        /* Generated list of keyword completions in the book */
        GList        *completions;

        /* Monitor of this specific book */
        GFileMonitor *monitor;
        /* Last received event */
        DhBookMonitorEvent monitor_event;
        /* ID of the event source */
        guint         monitor_event_timeout_id;
} DhBookPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhBook, dh_book, G_TYPE_OBJECT);

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
dh_book_finalize (GObject *object)
{
        DhBookPrivate *priv;

        priv = dh_book_get_instance_private (DH_BOOK (object));

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

        if (priv->completions) {
                g_list_foreach (priv->completions, (GFunc)g_free, NULL);
                g_list_free (priv->completions);
        }

        if (priv->monitor) {
                g_object_unref (priv->monitor);
        }

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

        object_class->finalize = dh_book_finalize;

	signals[BOOK_ENABLED] =
		g_signal_new ("enabled",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE,
                              0);

	signals[BOOK_DISABLED] =
		g_signal_new ("disabled",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE,
                              0);


	signals[BOOK_UPDATED] =
		g_signal_new ("updated",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE,
                              0);

	signals[BOOK_DELETED] =
		g_signal_new ("deleted",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE,
		              0);
}

static void
dh_book_init (DhBook *book)
{
        DhBookPrivate *priv = dh_book_get_instance_private (book);

        priv->name = NULL;
        priv->path = NULL;
        priv->title = NULL;
        priv->enabled = TRUE;
        priv->tree = NULL;
        priv->keywords = NULL;
        priv->completions = NULL;
        priv->monitor = NULL;
        priv->monitor_event = BOOK_MONITOR_EVENT_NONE;
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
                           priv->path, error->message);
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

static gboolean
book_monitor_event_timeout_cb  (gpointer data)
{
        DhBook     *book = data;
        DhBookPrivate *priv = dh_book_get_instance_private (book);

        /* We'll get either is_deleted OR is_updated,
         * not possible to have both or none */
        switch (priv->monitor_event)
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
        default:
                break;
        }

        /* Reset event */
        priv->monitor_event = BOOK_MONITOR_EVENT_NONE;

        /* Destroy the reference we got in the timeout */
        g_object_unref (book);
        return FALSE;
}

static void
book_monitor_event_cb (GFileMonitor      *file_monitor,
                       GFile             *file,
                       GFile             *other_file,
                       GFileMonitorEvent  event_type,
                       gpointer	          user_data)
{
        DhBook     *book = user_data;
        DhBookPrivate *priv = dh_book_get_instance_private (book);
        gboolean    reset_timer = FALSE;

        switch (event_type) {
        case G_FILE_MONITOR_EVENT_CREATED:
                /* This may happen if the file is deleted and then
                 * created right away, as we're merging events.
                 * Treat in the same way as a CHANGES_DONE_HINT, so
                 * fall through the case.  */
        case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
                priv->monitor_event = BOOK_MONITOR_EVENT_UPDATED;
                reset_timer = TRUE;
                break;
        case G_FILE_MONITOR_EVENT_DELETED:
                priv->monitor_event = BOOK_MONITOR_EVENT_DELETED;
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
                                                                        g_object_ref (book));
        }
}

GList *
dh_book_get_keywords (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->enabled ? priv->keywords : NULL;
}

GList *
dh_book_get_completions (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        if (!priv->enabled)
                return NULL;

        if (!priv->completions) {
                GList *l;
                for (l = priv->keywords; l; l = g_list_next (l)) {
                        DhLink *link = l->data;

                        /* Add additional "page:" and "book:" completions */
                        if (dh_link_get_link_type (link) == DH_LINK_TYPE_BOOK) {
                                priv->completions =
                                        g_list_prepend (priv->completions,
                                                        g_strdup_printf ("book:%s",
                                                                         dh_link_get_name (link)));
                        }
                        else if (dh_link_get_link_type (link) == DH_LINK_TYPE_PAGE) {
                                priv->completions =
                                        g_list_prepend (priv->completions,
                                                        g_strdup_printf ("page:%s",
                                                                         dh_link_get_name (link)));
                        }

                        priv->completions =  g_list_prepend (priv->completions,
                                                          g_strdup (dh_link_get_name (link)));
                }
        }

        return priv->completions;
}

GNode *
dh_book_get_tree (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->enabled ? priv->tree : NULL;
}

const gchar *
dh_book_get_name (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->name;
}

const gchar *
dh_book_get_title (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->title;
}

const gchar *
dh_book_get_language (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->language;
}

const gchar *
dh_book_get_path (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), NULL);

        priv = dh_book_get_instance_private (book);

        return priv->path;
}

gboolean
dh_book_get_enabled (DhBook *book)
{
        DhBookPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK (book), FALSE);

        priv = dh_book_get_instance_private (book);

        return priv->enabled;
}

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

gint
dh_book_cmp_by_path (DhBook *a,
                     DhBook *b)
{
        DhBookPrivate *priv_a;
        DhBookPrivate *priv_b;

        priv_a = dh_book_get_instance_private (a);
        priv_b = dh_book_get_instance_private (b);

        return ((a && b) ?
                g_strcmp0 (priv_a->path, priv_b->path) :
                -1);
}

gint
dh_book_cmp_by_path_str (DhBook *a,
                         const gchar  *b_path)
{
        DhBookPrivate *priv_a;

        priv_a = dh_book_get_instance_private (a);

        return ((a && b_path) ?
                g_strcmp0 (priv_a->path, b_path) :
                -1);
}

gint
dh_book_cmp_by_name (DhBook *a,
                     DhBook *b)
{
        DhBookPrivate *priv_a;
        DhBookPrivate *priv_b;

        priv_a = dh_book_get_instance_private (a);
        priv_b = dh_book_get_instance_private (b);

        return ((a && b) ?
                g_ascii_strcasecmp (priv_a->name, priv_b->name) :
                -1);
}

gint
dh_book_cmp_by_name_str (DhBook *a,
                         const gchar  *b_name)
{
        DhBookPrivate *priv_a;

        priv_a = dh_book_get_instance_private (a);

        return ((a && b_name) ?
                g_ascii_strcasecmp (priv_a->name, b_name) :
                -1);
}

gint
dh_book_cmp_by_title (DhBook *a,
                      DhBook *b)
{
        DhBookPrivate *priv_a;
        DhBookPrivate *priv_b;

        priv_a = dh_book_get_instance_private (a);
        priv_b = dh_book_get_instance_private (b);

        return ((a && b) ?
                g_utf8_collate (priv_a->title, priv_b->title) :
                -1);
}
