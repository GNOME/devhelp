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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "dh-keyword-model.h"

#include <gtk/gtk.h>
#include <string.h>

#include "dh-book.h"
#include "dh-util.h"

typedef struct {
        DhBookManager *book_manager;

        gchar *current_book_id;

        /* List of DhLink* */
        GQueue keywords;

        gint   stamp;
} DhKeywordModelPrivate;

/* Store a keyword as well as glob
 * patterns that match at the start of a word
 * and one that matches in any position of a
 * word */
typedef struct {
        gchar *keyword;
        GPatternSpec *glob_pattern_start;
        GPatternSpec *glob_pattern_any;
        guint has_glob : 1;
} DhKeywordGlobPattern;

typedef struct {
        const gchar *search_string;
        GStrv keywords;
        GList *keyword_globs;
        const gchar *book_id;
        const gchar *skip_book_id;
        const gchar *page_id;
        gchar *page_filename_prefix;
        const gchar *language;
        guint case_sensitive : 1;
        guint prefix : 1;
} SearchSettings;

#define MAX_HITS 100

static void dh_keyword_model_tree_model_init (GtkTreeModelIface   *iface);

G_DEFINE_TYPE_WITH_CODE (DhKeywordModel, dh_keyword_model, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (DhKeywordModel)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                dh_keyword_model_tree_model_init));

static void
dh_keyword_model_dispose (GObject *object)
{
        DhKeywordModelPrivate *priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (object));

        g_clear_object (&priv->book_manager);

        G_OBJECT_CLASS (dh_keyword_model_parent_class)->dispose (object);
}

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
        GObjectClass *object_class = G_OBJECT_CLASS (klass);;

        object_class->dispose = dh_keyword_model_dispose;
        object_class->finalize = dh_keyword_model_finalize;
}

static void
dh_keyword_model_init (DhKeywordModel *model)
{
        DhKeywordModelPrivate *priv = dh_keyword_model_get_instance_private (model);

        priv->stamp = g_random_int_range (1, G_MAXINT32);
}

static GtkTreeModelFlags
keyword_model_get_flags (GtkTreeModel *tree_model)
{
        return GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY;
}

static gint
keyword_model_get_n_columns (GtkTreeModel *tree_model)
{
        return DH_KEYWORD_MODEL_NUM_COLS;
}

static GType
keyword_model_get_column_type (GtkTreeModel *tree_model,
                               gint          column)
{
        switch (column) {
        case DH_KEYWORD_MODEL_COL_NAME:
                return G_TYPE_STRING;

        case DH_KEYWORD_MODEL_COL_LINK:
                return G_TYPE_POINTER;

        case DH_KEYWORD_MODEL_COL_CURRENT_BOOK_FLAG:
                return G_TYPE_BOOLEAN;

        default:
                return G_TYPE_INVALID;
        }
}

static gboolean
keyword_model_get_iter (GtkTreeModel *tree_model,
                        GtkTreeIter  *iter,
                        GtkTreePath  *path)
{
        DhKeywordModelPrivate *priv;
        GList                 *node;
        const gint            *indices;

        priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (tree_model));

        indices = gtk_tree_path_get_indices (path);

        if (indices == NULL) {
                return FALSE;
        }

        if (indices[0] >= priv->keywords.length) {
                return FALSE;
        }

        node = g_queue_peek_nth_link (&priv->keywords, indices[0]);

        iter->stamp     = priv->stamp;
        iter->user_data = node;

        return TRUE;
}

static GtkTreePath *
keyword_model_get_path (GtkTreeModel *tree_model,
                        GtkTreeIter  *iter)
{
        DhKeywordModelPrivate *priv;
        GList                 *node;
        GtkTreePath           *path;
        gint                   pos;

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
keyword_model_get_value (GtkTreeModel *tree_model,
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
keyword_model_iter_next (GtkTreeModel *tree_model,
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
keyword_model_iter_children (GtkTreeModel *tree_model,
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
keyword_model_iter_has_child (GtkTreeModel *tree_model,
                              GtkTreeIter  *iter)
{
        return FALSE;
}

static gint
keyword_model_iter_n_children (GtkTreeModel *tree_model,
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
keyword_model_iter_nth_child (GtkTreeModel *tree_model,
                              GtkTreeIter  *iter,
                              GtkTreeIter  *parent,
                              gint          n)
{
        DhKeywordModelPrivate *priv;
        GList                 *child;

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
keyword_model_iter_parent (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter,
                           GtkTreeIter  *child)
{
        return FALSE;
}

static void
dh_keyword_model_tree_model_init (GtkTreeModelIface *iface)
{
        iface->get_flags       = keyword_model_get_flags;
        iface->get_n_columns   = keyword_model_get_n_columns;
        iface->get_column_type = keyword_model_get_column_type;
        iface->get_iter        = keyword_model_get_iter;
        iface->get_path        = keyword_model_get_path;
        iface->get_value       = keyword_model_get_value;
        iface->iter_next       = keyword_model_iter_next;
        iface->iter_children   = keyword_model_iter_children;
        iface->iter_has_child  = keyword_model_iter_has_child;
        iface->iter_n_children = keyword_model_iter_n_children;
        iface->iter_nth_child  = keyword_model_iter_nth_child;
        iface->iter_parent     = keyword_model_iter_parent;
}

/**
 * dh_keyword_model_new:
 *
 * Create a new #DhKeywordModel object.
 *
 * Returns: a new #DhKeywordModel object
 */
DhKeywordModel *
dh_keyword_model_new (void)
{
        return g_object_new (DH_TYPE_KEYWORD_MODEL, NULL);
}

/**
 * dh_keyword_model_set_words:
 * @model: a #DhKeywordModel object
 * @book_manager: a #DhBookManager to analyze
 *
 * Set the #DhBookManager object on which words are analyzed.
 */
void
dh_keyword_model_set_words (DhKeywordModel *model,
                            DhBookManager  *book_manager)
{
        DhKeywordModelPrivate *priv;

        g_return_if_fail (DH_IS_KEYWORD_MODEL (model));
        g_return_if_fail (DH_IS_BOOK_MANAGER (book_manager));

        priv = dh_keyword_model_get_instance_private (model);

        priv->book_manager = g_object_ref (book_manager);
}

/* For each keyword, creates a DhKeywordGlobPattern with GPatternSpec's
 * allocated if there are any special glob characters ('*', '?') in the keyword.
 */
static GList *
dh_globbed_keywords_new (GStrv keywords)
{
        GList *list = NULL;
        gint i;

        if (keywords == NULL) {
                return NULL;
        }

        for (i = 0; keywords[i] != NULL; i++) {
                DhKeywordGlobPattern *data;

                data = g_slice_new (DhKeywordGlobPattern);
                data->keyword = keywords[i];

                if (strchr (keywords[i], '*') != NULL ||
                    strchr (keywords[i], '?') != NULL) {
                        gchar *glob;

                        data->has_glob = TRUE;

                        /* g_pattern_match matches whole strings only, so
                         * for globs, we need to end with a star for partial matches */
                        glob = g_strdup_printf ("%s*", keywords[i]);
                        data->glob_pattern_start = g_pattern_spec_new (glob);
                        g_free (glob);

                        glob = g_strdup_printf ("*%s*", keywords[i]);
                        data->glob_pattern_any = g_pattern_spec_new (glob);
                        g_free (glob);
                } else {
                        data->has_glob = FALSE;
                }

                list = g_list_prepend (list, data);
        }

        return g_list_reverse (list);
}

/* Frees all the datastructures and patterns associated with
 * keyword_globs as well as keyword_globs itself.  It does not free
 * DhKeywordGlobPattern->keyword however (only the pattern specs).
 */
static void
dh_globbed_keywords_free (GList *keyword_globs)
{
        GList *l;

        for (l = keyword_globs; l != NULL; l = l->next) {
                DhKeywordGlobPattern *data = l->data;

                if (data->has_glob) {
                        g_pattern_spec_free (data->glob_pattern_start);
                        g_pattern_spec_free (data->glob_pattern_any);
                }

                g_slice_free (DhKeywordGlobPattern, data);
        }

        g_list_free (keyword_globs);
}

static GQueue *
keyword_model_search_book (DhBook          *book,
                           SearchSettings  *settings,
                           guint            max_hits,
                           DhLink         **exact_link)
{
        GQueue *ret;
        GList *l;

        ret = g_queue_new ();

        for (l = dh_book_get_keywords (book);
             l != NULL && ret->length < max_hits;
             l = g_list_next (l)) {
                DhLink   *link;
                gboolean  found;

                link = l->data;
                found = FALSE;

                /* Filter by page? */
                if (settings->page_id != NULL) {
                        gchar *file_name;

                        file_name = (settings->case_sensitive ?
                                     g_strdup (dh_link_get_file_name (link)) :
                                     g_ascii_strdown (dh_link_get_file_name (link), -1));

                        /* First, filter out all keywords not belonging
                         * to this given page. */
                        if (!g_str_has_prefix (file_name, settings->page_filename_prefix)) {
                                /* No need of this keyword. */
                                g_free (file_name);
                                continue;
                        }
                        g_free (file_name);

                        /* This means we got no keywords to look for. */
                        if (settings->keywords == NULL) {
                                /* Show all in the page */
                                found = TRUE;
                        }
                }

                if (!found && settings->keywords != NULL) {
                        gboolean  all_found;
                        gboolean  prefix_found;
                        gchar    *name;
                        GList    *list;

                        name = (settings->case_sensitive ?
                                g_strdup (dh_link_get_name (link)) :
                                g_ascii_strdown (dh_link_get_name (link), -1));

                        all_found = TRUE;
                        prefix_found = FALSE;
                        for (list = settings->keyword_globs; list != NULL; list = g_list_next (list)) {
                                DhKeywordGlobPattern *data = list->data;

                                /* If our keyword is a glob pattern, use
                                 * it.  Otherwise, do more efficient string searching */
                                if (data->has_glob) {
                                        if (g_pattern_match_string (data->glob_pattern_start, name)) {
                                                prefix_found = TRUE;
                                                /* If we get a prefix match and we're not
                                                 * looking for prefix, stop. */
                                                if (!settings->prefix)
                                                        break;
                                        } else if (!g_pattern_match_string (data->glob_pattern_any, name)) {
                                                all_found = FALSE;
                                                break;
                                        }
                                } else {
                                        if (g_str_has_prefix (name, data->keyword)) {
                                                prefix_found = TRUE;
                                                if (!settings->prefix)
                                                        break;
                                        } else if (g_strrstr (name, data->keyword) == NULL) {
                                                all_found = FALSE;
                                                break;
                                        }
                                }
                        }

                        g_free (name);

                        found = all_found &&
                                ((settings->prefix && prefix_found) ||
                                 (!settings->prefix && !prefix_found));
                }

                if (found) {
                        g_queue_push_tail (ret, link);

                        if (exact_link == NULL || dh_link_get_name (link) == NULL)
                                continue;

                        /* Look for an exact link match. If the link is a PAGE,
                         * we can overwrite any previous exact link set. For
                         * example, when looking for GFile, we want the page,
                         * not the struct. */
                        if (dh_link_get_link_type (link) == DH_LINK_TYPE_PAGE &&
                            ((settings->page_id != NULL &&
                              strcmp (dh_link_get_name (link), settings->page_id) == 0) ||
                             (strcmp (dh_link_get_name (link), settings->search_string) == 0))) {
                                *exact_link = link;
                        } else if (*exact_link == NULL &&
                                   strcmp (dh_link_get_name (link), settings->search_string) == 0) {
                                *exact_link = link;
                        }
                }
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
keyword_model_search_books (DhKeywordModel  *model,
                            SearchSettings  *settings,
                            guint            max_hits,
                            DhLink         **exact_link)
{
        DhKeywordModelPrivate *priv;
        GQueue *ret;
        GList *l;

        priv = dh_keyword_model_get_instance_private (model);

        ret = g_queue_new ();

        for (l = dh_book_manager_get_books (priv->book_manager);
             l != NULL && ret->length < max_hits;
             l = l->next) {
                DhBook *book;
                GQueue *book_result;

                book = DH_BOOK (l->data);

                /* Filtering by book? */
                if (settings->book_id != NULL) {
                        if (g_strcmp0 (settings->book_id, dh_book_get_name (book)) != 0) {
                                continue;
                        }

                        /* Looking only for some specific book, without page or
                         * keywords? Return only the match of the first book page.
                         */
                        if (settings->page_id == NULL && settings->keywords == NULL) {
                                GNode *node;

                                node = dh_book_get_tree (book);
                                if (node != NULL) {
                                        if (exact_link != NULL)
                                                *exact_link = node->data;

                                        g_queue_clear (ret);
                                        g_queue_push_tail (ret, node->data);
                                        return ret;
                                }
                        }
                }

                /* Skipping a given book? */
                if (settings->skip_book_id != NULL &&
                    g_strcmp0 (settings->skip_book_id, dh_book_get_name (book)) == 0) {
                        continue;
                }

                /* Filtering by language? */
                if (settings->language != NULL &&
                    g_strcmp0 (settings->language, dh_book_get_language (book)) != 0) {
                        continue;
                }

                book_result = keyword_model_search_book (book,
                                                         settings,
                                                         max_hits - ret->length,
                                                         exact_link);

                dh_util_queue_concat (ret, book_result);
        }

        g_queue_sort (ret, (GCompareDataFunc) dh_link_compare, NULL);
        return ret;
}

static GQueue *
keyword_model_search (DhKeywordModel  *model,
                      const gchar     *search_string,
                      const GStrv      keywords,
                      const gchar     *book_id,
                      const gchar     *page_id,
                      const gchar     *language,
                      gboolean         case_sensitive,
                      DhLink         **exact_link)
{
        SearchSettings settings;
        gint max_hits = MAX_HITS;
        GQueue *in_book = NULL;
        GQueue *other_books = NULL;
        DhLink *in_book_exact_link = NULL;
        DhLink *other_books_exact_link = NULL;
        GQueue *out = g_queue_new ();

        settings.search_string = search_string;
        settings.keywords = keywords;
        settings.keyword_globs = dh_globbed_keywords_new (keywords);
        settings.book_id = book_id;
        settings.skip_book_id = NULL;
        settings.page_id = page_id;
        settings.page_filename_prefix = NULL;
        settings.language = language;
        settings.case_sensitive = case_sensitive;
        settings.prefix = TRUE;

        if (page_id != NULL) {
                settings.page_filename_prefix = g_strdup_printf ("%s.", page_id);

                /* If filtering per page, increase the maximum number of
                 * hits. This is due to the fact that a page may have
                 * more than MAX_HITS keywords, and the page link may be
                 * the last one in the list, but we always want to get it.
                 */
                max_hits = G_MAXINT;
        }

        /* If book_id given; first look for prefixed items in the given book id */
        if (book_id != NULL) {
                in_book = keyword_model_search_books (model,
                                                      &settings,
                                                      max_hits,
                                                      &in_book_exact_link);
        }

        /* Next, always check other books as well, as the exact match may be in there */
        settings.book_id = NULL;
        settings.skip_book_id = book_id;
        other_books = keyword_model_search_books (model,
                                                  &settings,
                                                  max_hits,
                                                  &other_books_exact_link);


        /* Now that we got prefix searches in current and other books, decide
         * which the preferred exact link is. If the exact match is in other
         * books; prefer those to the current book. */
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
                goto out;

        /* Look for non-prefixed matches in current book */
        settings.prefix = FALSE;

        if (book_id != NULL) {
                settings.book_id = book_id;
                settings.skip_book_id = NULL;

                in_book = keyword_model_search_books (model,
                                                      &settings,
                                                      max_hits - out->length,
                                                      NULL);

                dh_util_queue_concat (out, in_book);
                if (out->length >= max_hits)
                        goto out;
        }

        /* If still room for more items; look for non-prefixed items in other books */
        settings.book_id = NULL;
        settings.skip_book_id = book_id;
        other_books = keyword_model_search_books (model,
                                                  &settings,
                                                  max_hits - out->length,
                                                  NULL);
        dh_util_queue_concat (out, other_books);

out:
        dh_globbed_keywords_free (settings.keyword_globs);
        g_free (settings.page_filename_prefix);

        return out;
}

/* Process the input search string and extract:
 *  - If "book:" prefix given, a book_id
 *  - If "page:" prefix given, a page_id
 *  - All remaining keywords
 *
 * Returns TRUE when any of the output parameters are set.
 */
static gboolean
keyword_model_process_search_string (const gchar  *string,
                                     gchar       **book_id,
                                     gchar       **page_id,
                                     GStrv        *keywords)
{
        gchar *processed = NULL;
        gchar *aux;
        GStrv  tokens = NULL;
        gint   token_num;
        gint   keyword_num;
        gboolean ret = TRUE;

        *book_id = NULL;
        *page_id = NULL;
        *keywords = NULL;

        /* First, remove all leading and trailing whitespaces in
         * the search string */
        processed = g_strdup (string);
        g_strstrip (processed);

        /* Also avoid words being separated by more than one whitespace,
         * or g_strsplit() will give us empty strings. */
        aux = processed;
        while ((aux = strchr (aux, ' ')) != NULL) {
                g_strchug (++aux);
        }

        /* If after all this we get an empty string, nothing else to do */
        if (processed[0] == '\0') {
                ret = FALSE;
                goto out;
        }

        /* Split the input string into tokens */
        tokens = g_strsplit (processed, " ", 0);

        /* Allocate output keywords */
        *keywords = g_new0 (gchar *, g_strv_length (tokens) + 1);
        keyword_num = 0;

        for (token_num = 0; tokens[token_num] != NULL; token_num++) {
                gchar *cur_token;
                const gchar *prefix;
                gint prefix_len;

                cur_token = tokens[token_num];

                /* Book prefix? */
                prefix = "book:";
                if (g_str_has_prefix (cur_token, prefix)) {
                        prefix_len = strlen (prefix);

                        /* If keyword given but no content, skip it. */
                        if (cur_token[prefix_len] == '\0') {
                                continue;
                        }

                        /* We got a second request of book, don't allow
                         * this. */
                        if (*book_id != NULL) {
                                ret = FALSE;
                                goto out;
                        }

                        *book_id = g_strdup (cur_token + prefix_len);
                        continue;
                }

                /* Page prefix? */
                prefix = "page:";
                if (g_str_has_prefix (cur_token, prefix)) {
                        prefix_len = strlen (prefix);

                        /* If keyword given but no content, skip it. */
                        if (cur_token[prefix_len] == '\0') {
                                continue;
                        }

                        /* We got a second request of page, don't allow
                         * this. */
                        if (*page_id != NULL) {
                                ret = FALSE;
                                goto out;
                        }

                        *page_id = g_strdup (cur_token + prefix_len);
                        continue;
                }

                /* Then, a new keyword to look for */
                (*keywords)[keyword_num] = g_strdup (cur_token);
                keyword_num++;
        }

        if (keyword_num == 0) {
                g_free (*keywords);
                *keywords = NULL;
        }

out:
        if (ret == FALSE) {
                g_free (*book_id);
                g_free (*page_id);
                g_strfreev (*keywords);

                *book_id = NULL;
                *page_id = NULL;
                *keywords = NULL;
        }

        g_free (processed);
        g_strfreev (tokens);

        return ret;
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
 * @model: a #DhKeywordModel object
 * @search_string: a search query
 * @book_id: (nullable): the id of a specific book or %NULL for all books
 * @language: (nullable): the name of a language or %NULL for all languages
 *
 * Find the book matching the given criteria.
 *
 * Returns: (nullable) (transfer none): the corresponding #DhLink or %NULL if
 * no link corresponding to the criteria is found
 */
DhLink *
dh_keyword_model_filter (DhKeywordModel *model,
                         const gchar    *search_string,
                         const gchar    *book_id,
                         const gchar    *language)
{
        DhKeywordModelPrivate *priv;
        GQueue *new_list = NULL;
        gchar *book_id_in_search_string = NULL;
        gchar *page_id_in_search_string = NULL;
        GStrv keywords = NULL;
        DhLink *exact_link = NULL;

        g_return_val_if_fail (DH_IS_KEYWORD_MODEL (model), NULL);
        g_return_val_if_fail (search_string != NULL, NULL);

        priv = dh_keyword_model_get_instance_private (model);

        g_free (priv->current_book_id);
        priv->current_book_id = NULL;

        if (keyword_model_process_search_string (search_string,
                                                 &book_id_in_search_string,
                                                 &page_id_in_search_string,
                                                 &keywords)) {
                gboolean case_sensitive;
                gint i;

                /* Searches are case sensitive when any uppercase
                 * letter is used in the search terms, matching vim
                 * smartcase behaviour.
                 */
                case_sensitive = FALSE;
                for (i = 0; search_string[i] != '\0'; i++) {
                        if (g_ascii_isupper (search_string[i])) {
                                case_sensitive = TRUE;
                                break;
                        }
                }

                if (book_id_in_search_string != NULL) {
                        priv->current_book_id = book_id_in_search_string;
                        book_id_in_search_string = NULL;
                } else {
                        priv->current_book_id = g_strdup (book_id);
                }

                new_list = keyword_model_search (model,
                                                 search_string,
                                                 keywords,
                                                 priv->current_book_id,
                                                 page_id_in_search_string,
                                                 language,
                                                 case_sensitive,
                                                 &exact_link);
        } else {
                new_list = g_queue_new ();
        }

        set_keywords_list (model, new_list);
        new_list = NULL;

        g_free (book_id_in_search_string);
        g_free (page_id_in_search_string);
        g_strfreev (keywords);

        /* One hit */
        if (priv->keywords.length == 1) {
                return g_queue_peek_head (&priv->keywords);
        }

        return exact_link;
}
