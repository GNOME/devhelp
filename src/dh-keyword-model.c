/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
 * Copyright (C) 2015 Sébastien Wilmet <swilmet@gnome.org>
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

#include "dh-keyword-model.h"
#include <gtk/gtk.h>
#include "dh-book.h"
#include "dh-book-manager.h"
#include "dh-search-context.h"
#include "dh-util.h"

/**
 * SECTION:dh-keyword-model
 * @Title: DhKeywordModel
 * @Short_description: A custom #GtkTreeModel implementation for searching a
 * keyword
 *
 * #DhKeywordModel is a custom #GtkTreeModel implementation (as a list, not a
 * tree) for searching a keyword.
 *
 * The dh_keyword_model_filter() function is used to set the search criteria.
 */

typedef struct {
        gchar *current_book_id;

        /* List of DhLink* */
        /* FIXME ref the DhLinks, in case a DhBook is destroyed while the
         * DhLinks are still stored here.
         */
        GQueue keywords;

        gint stamp;
} DhKeywordModelPrivate;

typedef struct {
        DhSearchContext *search_context;
        const gchar *book_id;
        const gchar *skip_book_id;
        guint prefix : 1;
} SearchSettings;

#define MAX_HITS 1000

static void dh_keyword_model_tree_model_init (GtkTreeModelIface *iface);

G_DEFINE_TYPE_WITH_CODE (DhKeywordModel, dh_keyword_model, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (DhKeywordModel)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                dh_keyword_model_tree_model_init));

static void
dh_keyword_model_finalize (GObject *object)
{
        DhKeywordModelPrivate *priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (object));

        g_free (priv->current_book_id);
        g_queue_clear (&priv->keywords);

        G_OBJECT_CLASS (dh_keyword_model_parent_class)->finalize (object);
}

static void
dh_keyword_model_class_init (DhKeywordModelClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = dh_keyword_model_finalize;
}

static void
dh_keyword_model_init (DhKeywordModel *model)
{
        DhKeywordModelPrivate *priv = dh_keyword_model_get_instance_private (model);

        priv->stamp = g_random_int_range (1, G_MAXINT32);
}

static GtkTreeModelFlags
dh_keyword_model_get_flags (GtkTreeModel *tree_model)
{
        /* FIXME: check if GTK_TREE_MODEL_ITERS_PERSIST is correct. */
        return GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY;
}

static gint
dh_keyword_model_get_n_columns (GtkTreeModel *tree_model)
{
        return DH_KEYWORD_MODEL_NUM_COLS;
}

static GType
dh_keyword_model_get_column_type (GtkTreeModel *tree_model,
                                  gint          column)
{
        switch (column) {
        case DH_KEYWORD_MODEL_COL_NAME:
                return G_TYPE_STRING;

        case DH_KEYWORD_MODEL_COL_LINK:
                /* FIXME: use DH_TYPE_LINK boxed type, to take advantage of ref
                 * counting, to have safer code in case a DhLink is freed when
                 * still stored in the GtkTreeModel.
                 */
                return G_TYPE_POINTER;

        case DH_KEYWORD_MODEL_COL_CURRENT_BOOK_FLAG:
                return G_TYPE_BOOLEAN;

        default:
                return G_TYPE_INVALID;
        }
}

static gboolean
dh_keyword_model_get_iter (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter,
                           GtkTreePath  *path)
{
        DhKeywordModelPrivate *priv;
        GList *node;
        const gint *indices;

        priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (tree_model));

        indices = gtk_tree_path_get_indices (path);

        if (indices == NULL) {
                return FALSE;
        }

        if (indices[0] >= (gint)priv->keywords.length) {
                return FALSE;
        }

        node = g_queue_peek_nth_link (&priv->keywords, indices[0]);

        iter->stamp = priv->stamp;
        iter->user_data = node;

        return TRUE;
}

static GtkTreePath *
dh_keyword_model_get_path (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter)
{
        DhKeywordModelPrivate *priv;
        GList *node;
        GtkTreePath *path;
        gint pos;

        priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (tree_model));

        g_return_val_if_fail (iter->stamp == priv->stamp, NULL);

        node = iter->user_data;
        pos = g_queue_link_index (&priv->keywords, node);

        if (pos < 0) {
                return NULL;
        }

        path = gtk_tree_path_new ();
        gtk_tree_path_append_index (path, pos);

        return path;
}

static void
dh_keyword_model_get_value (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter,
                            gint          column,
                            GValue       *value)
{
        DhKeywordModelPrivate *priv;
        GList *node;
        DhLink *link;
        gboolean in_current_book;

        priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (tree_model));

        g_return_if_fail (iter->stamp == priv->stamp);

        node = iter->user_data;
        link = node->data;

        switch (column) {
        case DH_KEYWORD_MODEL_COL_NAME:
                g_value_init (value, G_TYPE_STRING);
                g_value_set_string (value, dh_link_get_name (link));
                break;

        case DH_KEYWORD_MODEL_COL_LINK:
                g_value_init (value, G_TYPE_POINTER);
                g_value_set_pointer (value, link);
                break;

        case DH_KEYWORD_MODEL_COL_CURRENT_BOOK_FLAG:
                in_current_book = g_strcmp0 (dh_link_get_book_id (link), priv->current_book_id) == 0;
                g_value_init (value, G_TYPE_BOOLEAN);
                g_value_set_boolean (value, in_current_book);
                break;

        default:
                g_warning ("Bad column %d requested", column);
        }
}

static gboolean
dh_keyword_model_iter_next (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter)
{
        DhKeywordModelPrivate *priv;
        GList *node;

        priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (tree_model));

        g_return_val_if_fail (priv->stamp == iter->stamp, FALSE);

        node = iter->user_data;
        iter->user_data = node->next;

        return iter->user_data != NULL;
}

static gboolean
dh_keyword_model_iter_children (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter,
                                GtkTreeIter  *parent)
{
        DhKeywordModelPrivate *priv;

        priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (tree_model));

        /* This is a list, nodes have no children. */
        if (parent != NULL) {
                return FALSE;
        }

        /* But if parent == NULL we return the list itself as children of
         * the "root".
         */
        if (priv->keywords.head != NULL) {
                iter->stamp = priv->stamp;
                iter->user_data = priv->keywords.head;
                return TRUE;
        }

        return FALSE;
}

static gboolean
dh_keyword_model_iter_has_child (GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter)
{
        return FALSE;
}

static gint
dh_keyword_model_iter_n_children (GtkTreeModel *tree_model,
                                  GtkTreeIter  *iter)
{
        DhKeywordModelPrivate *priv;

        priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (tree_model));

        if (iter == NULL) {
                return priv->keywords.length;
        }

        g_return_val_if_fail (priv->stamp == iter->stamp, -1);

        return 0;
}

static gboolean
dh_keyword_model_iter_nth_child (GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter,
                                 GtkTreeIter  *parent,
                                 gint          n)
{
        DhKeywordModelPrivate *priv;
        GList *child;

        priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (tree_model));

        if (parent != NULL) {
                return FALSE;
        }

        child = g_queue_peek_nth_link (&priv->keywords, n);

        if (child != NULL) {
                iter->stamp = priv->stamp;
                iter->user_data = child;
                return TRUE;
        }

        return FALSE;
}

static gboolean
dh_keyword_model_iter_parent (GtkTreeModel *tree_model,
                              GtkTreeIter  *iter,
                              GtkTreeIter  *child)
{
        return FALSE;
}

static void
dh_keyword_model_tree_model_init (GtkTreeModelIface *iface)
{
        iface->get_flags = dh_keyword_model_get_flags;
        iface->get_n_columns = dh_keyword_model_get_n_columns;
        iface->get_column_type = dh_keyword_model_get_column_type;
        iface->get_iter = dh_keyword_model_get_iter;
        iface->get_path = dh_keyword_model_get_path;
        iface->get_value = dh_keyword_model_get_value;
        iface->iter_next = dh_keyword_model_iter_next;
        iface->iter_children = dh_keyword_model_iter_children;
        iface->iter_has_child = dh_keyword_model_iter_has_child;
        iface->iter_n_children = dh_keyword_model_iter_n_children;
        iface->iter_nth_child = dh_keyword_model_iter_nth_child;
        iface->iter_parent = dh_keyword_model_iter_parent;
}

/**
 * dh_keyword_model_new:
 *
 * Returns: a new #DhKeywordModel object.
 */
DhKeywordModel *
dh_keyword_model_new (void)
{
        return g_object_new (DH_TYPE_KEYWORD_MODEL, NULL);
}

static GQueue *
search_single_book (DhBook          *book,
                    SearchSettings  *settings,
                    guint            max_hits,
                    DhLink         **exact_link)
{
        GQueue *ret;
        GList *l;

        ret = g_queue_new ();

        for (l = dh_book_get_links (book);
             l != NULL && ret->length < max_hits;
             l = l->next) {
                DhLink *link = l->data;

                if (!_dh_search_context_match_link (settings->search_context,
                                                    link,
                                                    settings->prefix)) {
                        continue;
                }

                g_queue_push_tail (ret, link);

                if (exact_link == NULL || !settings->prefix)
                        continue;

                /* Look for an exact link match. If the link is a PAGE, we can
                 * overwrite any previous exact link set. For example, when
                 * looking for GFile, we want the page, not the struct.
                 */
                if ((*exact_link == NULL || dh_link_get_link_type (link) == DH_LINK_TYPE_PAGE) &&
                    _dh_search_context_is_exact_link (settings->search_context, link)) {
                        *exact_link = link;
                }
        }

        return ret;
}

static GQueue *
search_books (SearchSettings  *settings,
              guint            max_hits,
              DhLink         **exact_link)
{
        DhBookManager *book_manager;
        GList *books;
        GList *l;
        GQueue *ret;

        ret = g_queue_new ();

        book_manager = dh_book_manager_get_singleton ();
        books = dh_book_manager_get_books (book_manager);

        for (l = books;
             l != NULL && ret->length < max_hits;
             l = l->next) {
                DhBook *book = DH_BOOK (l->data);
                GQueue *book_result;

                if (!_dh_search_context_match_book (settings->search_context, book))
                        continue;

                /* Filtering by book? */
                if (settings->book_id != NULL &&
                    g_strcmp0 (settings->book_id, dh_book_get_id (book)) != 0) {
                        continue;
                }

                /* Skipping a given book? */
                if (settings->skip_book_id != NULL &&
                    g_strcmp0 (settings->skip_book_id, dh_book_get_id (book)) == 0) {
                        continue;
                }

                book_result = search_single_book (book,
                                                  settings,
                                                  max_hits - ret->length,
                                                  exact_link);

                dh_util_queue_concat (ret, book_result);
        }

        g_queue_sort (ret, (GCompareDataFunc) dh_link_compare, NULL);
        return ret;
}

static GQueue *
handle_book_id_only (DhSearchContext  *search_context,
                     DhLink          **exact_link)
{
        DhBookManager *book_manager;
        GList *books;
        GList *l;
        GQueue *ret;

        if (_dh_search_context_get_book_id (search_context) == NULL ||
            _dh_search_context_get_page_id (search_context) != NULL ||
            _dh_search_context_get_keywords (search_context) != NULL) {
                return NULL;
        }

        ret = g_queue_new ();

        book_manager = dh_book_manager_get_singleton ();
        books = dh_book_manager_get_books (book_manager);

        for (l = books; l != NULL; l = l->next) {
                DhBook *book = DH_BOOK (l->data);
                GNode *node;

                if (!_dh_search_context_match_book (search_context, book))
                        continue;

                /* Return only the top-level book page. */
                node = dh_book_get_tree (book);
                if (node != NULL) {
                        if (exact_link != NULL)
                                *exact_link = node->data;

                        g_queue_push_tail (ret, node->data);
                }

                break;
        }

        return ret;
}

/* The Search rationale is as follows:
 *
 * - If 'book_id' is given, but no 'page_id' or 'keywords', the main page of
 *   the book will only be shown, giving as exact match this book link.
 * - If 'book_id' and 'page_id' are given, but no 'keywords', all the items
 *   in the given page of the given book will be shown.
 * - If 'book_id' and 'keywords' are given, but no 'page_id', up to MAX_HITS
 *   items matching the keywords in the given book will be shown.
 * - If 'book_id' and 'page_id' and 'keywords' are given, all the items
 *   matching the keywords in the given page of the given book will be shown.
 *
 * - If 'page_id' is given, but no 'book_id' or 'keywords', all the items
 *   in the given page will be shown, giving as exact match the page link.
 * - If 'page_id' and 'keywords' are given but no 'book_id', all the items
 *   matching the keywords in the given page will be shown.
 *
 * - If 'keywords' only are given, up to max_hits items matching the keywords
 *   will be shown. If keyword matches both a page link and a non-page one,
 *   the page link is the one given as exact match.
 */
static GQueue *
keyword_model_search (DhKeywordModel   *model,
                      DhSearchContext  *search_context,
                      DhLink          **exact_link)
{
        DhKeywordModelPrivate *priv = dh_keyword_model_get_instance_private (model);
        SearchSettings settings;
        guint max_hits = MAX_HITS;
        GQueue *in_book = NULL;
        GQueue *other_books = NULL;
        DhLink *in_book_exact_link = NULL;
        DhLink *other_books_exact_link = NULL;
        GQueue *out;

        out = handle_book_id_only (search_context, exact_link);
        if (out != NULL)
                return out;

        out = g_queue_new ();

        settings.search_context = search_context;
        settings.book_id = priv->current_book_id;
        settings.skip_book_id = NULL;
        settings.prefix = TRUE;

        if (_dh_search_context_get_page_id (search_context) != NULL) {
                /* If filtering per page, increase the maximum number of
                 * hits. This is due to the fact that a page may have
                 * more than MAX_HITS keywords, and the page link may be
                 * the last one in the list, but we always want to get it.
                 */
                max_hits = G_MAXUINT;
        }

        /* First look for prefixed items in the given book id. */
        if (priv->current_book_id != NULL) {
                in_book = search_books (&settings,
                                        max_hits,
                                        &in_book_exact_link);
        }

        /* Next, always check other books as well, as the exact match may be in
         * there.
         */
        settings.book_id = NULL;
        settings.skip_book_id = priv->current_book_id;
        other_books = search_books (&settings,
                                    max_hits,
                                    &other_books_exact_link);

        /* Now that we got prefix searches in current and other books, decide
         * which the preferred exact link is. If the exact match is in other
         * books, prefer those to the current book.
         */
        if (in_book_exact_link != NULL) {
                *exact_link = in_book_exact_link;
                dh_util_queue_concat (out, in_book);
                dh_util_queue_concat (out, other_books);
        } else if (other_books_exact_link != NULL) {
                *exact_link = other_books_exact_link;
                dh_util_queue_concat (out, other_books);
                dh_util_queue_concat (out, in_book);
        } else {
                *exact_link = NULL;
                dh_util_queue_concat (out, in_book);
                dh_util_queue_concat (out, other_books);
        }

        if (out->length >= max_hits)
                return out;

        /* Look for non-prefixed matches in current book. */
        settings.prefix = FALSE;

        if (priv->current_book_id != NULL) {
                settings.book_id = priv->current_book_id;
                settings.skip_book_id = NULL;

                in_book = search_books (&settings,
                                        max_hits - out->length,
                                        NULL);

                dh_util_queue_concat (out, in_book);
                if (out->length >= max_hits)
                        return out;
        }

        /* If still room for more items, look for non-prefixed items in other
         * books.
         */
        settings.book_id = NULL;
        settings.skip_book_id = priv->current_book_id;
        other_books = search_books (&settings,
                                    max_hits - out->length,
                                    NULL);
        dh_util_queue_concat (out, other_books);

        return out;
}

static void
set_keywords_list (DhKeywordModel *model,
                   GQueue         *new_keywords)
{
        DhKeywordModelPrivate *priv;
        gint old_length;
        gint new_length;
        gint row_num;
        GList *node;
        GtkTreePath *path;

        priv = dh_keyword_model_get_instance_private (model);

        old_length = priv->keywords.length;
        new_length = new_keywords->length;

        g_queue_clear (&priv->keywords);
        dh_util_queue_concat (&priv->keywords, new_keywords);
        new_keywords = NULL;

        /* Update model: common rows */
        row_num = 0;
        node = priv->keywords.head;
        path = gtk_tree_path_new_first ();

        while (row_num < MIN (old_length, new_length)) {
                GtkTreeIter iter;

                g_assert (node != NULL);

                iter.stamp = priv->stamp;
                iter.user_data = node;

                gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);

                row_num++;
                node = node->next;
                gtk_tree_path_next (path);
        }

        gtk_tree_path_free (path);

        /* Insert new rows */
        if (old_length < new_length) {
                row_num = old_length;
                g_assert_cmpint (g_queue_link_index (&priv->keywords, node), ==, row_num);
                path = gtk_tree_path_new_from_indices (row_num, -1);

                while (row_num < new_length) {
                        GtkTreeIter iter;

                        g_assert (node != NULL);

                        iter.stamp = priv->stamp;
                        iter.user_data = node;

                        gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);

                        row_num++;
                        node = node->next;
                        gtk_tree_path_next (path);
                }

                gtk_tree_path_free (path);
        }

        /* Delete old rows */
        else if (old_length > new_length) {
                row_num = old_length - 1;
                path = gtk_tree_path_new_from_indices (row_num, -1);

                while (row_num >= new_length) {
                        gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);

                        row_num--;
                        gtk_tree_path_prev (path);
                }

                gtk_tree_path_free (path);
        }
}

/**
 * dh_keyword_model_filter:
 * @model: a #DhKeywordModel.
 * @search_string: a search query.
 * @current_book_id: (nullable): the ID of the book currently shown, or %NULL.
 * @language: (nullable): deprecated, must be %NULL.
 *
 * Searches in the #DhBookManager the list of #DhLink's that correspond to
 * @search_string, and fills the @model with that list.
 *
 * Returns: (nullable) (transfer none): the #DhLink that matches exactly
 * @search_string, or %NULL if there is no such #DhLink.
 */
DhLink *
dh_keyword_model_filter (DhKeywordModel *model,
                         const gchar    *search_string,
                         const gchar    *current_book_id,
                         const gchar    *language)
{
        DhKeywordModelPrivate *priv;
        DhSearchContext *search_context;
        GQueue *new_list = NULL;
        DhLink *exact_link = NULL;

        g_return_val_if_fail (DH_IS_KEYWORD_MODEL (model), NULL);
        g_return_val_if_fail (search_string != NULL, NULL);
        g_return_val_if_fail (language == NULL, NULL);

        priv = dh_keyword_model_get_instance_private (model);

        g_free (priv->current_book_id);
        priv->current_book_id = NULL;

        search_context = _dh_search_context_new (search_string);

        if (search_context != NULL) {
                const gchar *book_id_in_search_string;

                book_id_in_search_string = _dh_search_context_get_book_id (search_context);

                if (book_id_in_search_string != NULL)
                        priv->current_book_id = g_strdup (book_id_in_search_string);
                else
                        priv->current_book_id = g_strdup (current_book_id);

                new_list = keyword_model_search (model, search_context, &exact_link);
        } else {
                new_list = g_queue_new ();
        }

        set_keywords_list (model, new_list);
        new_list = NULL;

        _dh_search_context_free (search_context);

        /* One hit */
        if (priv->keywords.length == 1)
                return g_queue_peek_head (&priv->keywords);

        return exact_link;
}
