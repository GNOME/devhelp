/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2008 Imendio AB
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
#include <gtk/gtk.h>
#include <string.h>

#include "dh-link.h"
#include "dh-book.h"
#include "dh-keyword-model.h"

typedef struct {
        DhBookManager *book_manager;

        gchar *current_book_id;
        GList *keyword_words;
        gint   keyword_words_length;

        gint   stamp;
} DhKeywordModelPrivate;

/* Store a keyword as well as glob
 * patterns that match at the start of a word
 * and one that matches in any position of a
 * word */
struct _DhKeywordGlobPattern {
        gchar *keyword;
        gboolean has_glob;
        GPatternSpec *glob_pattern_start;
        GPatternSpec *glob_pattern_any;
};

#define G_LIST(x) ((GList *) x)
#define MAX_HITS 100

static void dh_keyword_model_init            (DhKeywordModel      *list_store);
static void dh_keyword_model_class_init      (DhKeywordModelClass *class);
static void dh_keyword_model_tree_model_init (GtkTreeModelIface   *iface);
static GList *dh_globbed_keywords_new        (GStrv                keywords);
static void dh_globbed_keywords_free         (GList               *keyword_globs);

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
        g_list_free (priv->keyword_words);

        G_OBJECT_CLASS (dh_keyword_model_parent_class)->finalize (object);
}

static void
dh_keyword_model_class_init (DhKeywordModelClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);;

        object_class->finalize = dh_keyword_model_finalize;
        object_class->dispose = dh_keyword_model_dispose;
}

static void
dh_keyword_model_init (DhKeywordModel *model)
{
        DhKeywordModelPrivate *priv = dh_keyword_model_get_instance_private (model);

        do {
                priv->stamp = g_random_int ();
        } while (priv->stamp == 0);
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

        if (indices[0] >= priv->keyword_words_length) {
                return FALSE;
        }

        node = g_list_nth (priv->keyword_words, indices[0]);

        iter->stamp     = priv->stamp;
        iter->user_data = node;

        return TRUE;
}

static GtkTreePath *
keyword_model_get_path (GtkTreeModel *tree_model,
                        GtkTreeIter  *iter)
{
        DhKeywordModelPrivate *priv;
        GtkTreePath           *path;
        gint                   i = 0;

        priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (tree_model));

        g_return_val_if_fail (iter->stamp == priv->stamp, NULL);

        i = g_list_position (priv->keyword_words, iter->user_data);
        if (i < 0) {
                return NULL;
        }

        path = gtk_tree_path_new ();
        gtk_tree_path_append_index (path, i);

        return path;
}

static void
keyword_model_get_value (GtkTreeModel *tree_model,
                         GtkTreeIter  *iter,
                         gint          column,
                         GValue       *value)
{
        DhKeywordModelPrivate *priv;
        DhLink *link;

        link = G_LIST (iter->user_data)->data;
        priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (tree_model));

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
                g_value_init (value, G_TYPE_BOOLEAN);
                g_value_set_boolean (value, (g_strcmp0 (dh_link_get_book_id (link), priv->current_book_id) == 0));
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

        priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (tree_model));

        g_return_val_if_fail (priv->stamp == iter->stamp, FALSE);

        iter->user_data = G_LIST (iter->user_data)->next;

        return (iter->user_data != NULL);
}

static gboolean
keyword_model_iter_children (GtkTreeModel *tree_model,
                             GtkTreeIter  *iter,
                             GtkTreeIter  *parent)
{
        DhKeywordModelPrivate *priv;

        priv = dh_keyword_model_get_instance_private (DH_KEYWORD_MODEL (tree_model));

        /* This is a list, nodes have no children. */
        if (parent) {
                return FALSE;
        }

        /* But if parent == NULL we return the list itself as children of
         * the "root".
         */
        if (priv->keyword_words) {
                iter->stamp = priv->stamp;
                iter->user_data = priv->keyword_words;
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
                return priv->keyword_words_length;
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

        if (parent) {
                return FALSE;
        }

        child = g_list_nth (priv->keyword_words, n);

        if (child) {
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

DhKeywordModel *
dh_keyword_model_new (void)
{
        DhKeywordModel *model;

        model = g_object_new (DH_TYPE_KEYWORD_MODEL, NULL);

        return model;
}

void
dh_keyword_model_set_words (DhKeywordModel *model,
                            DhBookManager  *book_manager)
{
        DhKeywordModelPrivate *priv;

        g_return_if_fail (DH_IS_KEYWORD_MODEL (model));

        priv = dh_keyword_model_get_instance_private (model);

        priv->book_manager = g_object_ref (book_manager);
}

/* Returns a GList of struct _DhKeywordGlobPattern
 * with GPatternSpec's allocated if there are any
 * special glob characters ('*', '?') in a keyword in keywords.
 * The list returned is the same length as keywords */
static GList *
dh_globbed_keywords_new (GStrv keywords)
{
        gint i;
        gchar *glob;
        GList *list = NULL;
        struct _DhKeywordGlobPattern *glob_struct;

        if (keywords == NULL) {
                return NULL;
        }

        for (i = 0; keywords[i] != NULL; i++) {
                glob_struct = g_slice_new (struct _DhKeywordGlobPattern);
                glob_struct->keyword = keywords[i];
                if (g_strrstr (keywords[i], "*") || g_strrstr (keywords[i], "?")) {
                        glob_struct->has_glob = TRUE;
                        /* g_pattern_match matches whole strings only, so
                         * for globs, we need to end with a star for partial matches */
                        glob = g_strdup_printf ("%s*", keywords[i]);
                        glob_struct->glob_pattern_start = g_pattern_spec_new (glob);
                        g_free (glob);

                        glob = g_strdup_printf ("*%s*", keywords[i]);
                        glob_struct->glob_pattern_any = g_pattern_spec_new (glob);
                        g_free (glob);
                } else {
                        glob_struct->has_glob = FALSE;
                }

                list = g_list_append (list, (gpointer)glob_struct);
        }

        return list;
}

/* Frees all the datastructures and patterns associated with
 * keyword_globs as well as keyword_globs itself.  It does not free
 * _DhKeywordGlobPattern->keyword however (only the pattern spects) */
static void
dh_globbed_keywords_free (GList *keyword_globs)
{
        GList *list;
        for (list = keyword_globs; list != NULL; list = g_list_next (list)) {
                struct _DhKeywordGlobPattern *data = (struct _DhKeywordGlobPattern *)list->data;
                if (data->has_glob) {
                        g_pattern_spec_free (data->glob_pattern_start);
                        g_pattern_spec_free (data->glob_pattern_any);
                }
                g_slice_free (struct _DhKeywordGlobPattern, data);
        }
        g_list_free (keyword_globs);
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
 * - If 'keywords' only are given, up to MAX_HITS items matching the keywords
 *   will be shown. If keyword matches both a page link and a non-page one,
 *   the page link is the one given as exact match.
 */
static GList *
keyword_model_search_books (DhKeywordModel  *model,
                            const gchar     *string,
                            const GStrv      keywords,
                            const gchar     *book_id,
                            const gchar     *skip_book_id,
                            const gchar     *page_id,
                            const gchar     *language,
                            gboolean         case_sensitive,
                            gboolean         prefix,
                            guint            max_hits,
                            guint           *n_hits,
                            DhLink         **exact_link)
{
        DhKeywordModelPrivate *priv;
        GList                 *new_list = NULL, *b;
        gint                   hits = 0;
        gchar                 *page_filename_prefix = NULL;
        GList                 *keyword_globs;

        priv = dh_keyword_model_get_instance_private (model);

        /* Compile each keyword into a GPatternSpec if necessary */
        keyword_globs = dh_globbed_keywords_new (keywords);

        if (page_id) {
                page_filename_prefix = g_strdup_printf("%s.", page_id);
                /* If filtering per page, increase the maximum number of
                 * hits. This is due to the fact that a page may have
                 * more than MAX_HITS keywords, and the page link may be
                 * the last one in the list, but we always want to get it.
                 */
                max_hits = G_MAXINT;
        }

        for (b = dh_book_manager_get_books (priv->book_manager);
             b && hits < max_hits;
             b = g_list_next (b)) {
                DhBook *book;
                GList *l;

                book = DH_BOOK (b->data);

                /* Filtering by book? */
                if (book_id) {
                        if (g_strcmp0 (book_id, dh_book_get_name (book)) != 0) {
                                continue;
                        }

                        /* Looking only for some specific book, without page or
                         * keywords? Return only the match of the first book page.
                         */
                        if (!page_id && !keywords) {
                                GNode *node;

                                node = dh_book_get_tree (book);
                                if (node) {
                                        if (exact_link)
                                                *exact_link = node->data;
                                        return g_list_prepend (NULL, node->data);
                                }
                        }
                }

                /* Skipping a given book? */
                if (skip_book_id &&
                    g_strcmp0 (skip_book_id, dh_book_get_name (book)) == 0) {
                        continue;
                }

                /* Filtering by language? */
                if (language &&
                    g_strcmp0 (language, dh_book_get_language (book)) != 0) {
                        continue;
                }

                for (l = dh_book_get_keywords (book);
                     l && hits < max_hits;
                     l = g_list_next (l)) {
                        DhLink   *link;
                        gboolean  found;

                        link = l->data;
                        found = FALSE;

                        /* Filter by page? */
                        if (page_id) {
                                gchar *file_name;

                                file_name = (case_sensitive ?
                                             g_strdup (dh_link_get_file_name (link)) :
                                             g_ascii_strdown (dh_link_get_file_name (link), -1));

                                /* First, filter out all keywords not belonging
                                 * to this given page. */
                                if (!g_str_has_prefix (file_name, page_filename_prefix)) {
                                        /* No need of this keyword. */
                                        g_free (file_name);
                                        continue;
                                }
                                g_free (file_name);

                                /* This means we got no keywords to look for. */
                                if (!keywords) {
                                        /* Show all in the page */
                                        found = TRUE;
                                }
                        }

                        if (!found && keywords) {
                                gboolean  all_found;
                                gboolean  prefix_found;
                                gchar    *name;
                                GList    *list;

                                name = (case_sensitive ?
                                        g_strdup (dh_link_get_name (link)) :
                                        g_ascii_strdown (dh_link_get_name (link), -1));

                                all_found = TRUE;
                                prefix_found = FALSE;
                                for (list = keyword_globs; list != NULL; list = g_list_next (list)) {
                                        struct _DhKeywordGlobPattern *data = (struct _DhKeywordGlobPattern *)list->data;

                                        /* If our keyword is a glob pattern, use
                                         * it.  Otherwise, do more efficient string searching */
                                        if (data->has_glob) {
                                                if (g_pattern_match_string (data->glob_pattern_start, name)) {
                                                        prefix_found = TRUE;
                                                        /* If we get a prefix match and we're not
                                                         * looking for prefix, stop. */
                                                        if (!prefix)
                                                                break;
                                                } else if (!g_pattern_match_string (data->glob_pattern_any, name)) {
                                                        all_found = FALSE;
                                                        break;
                                                }
                                        } else {
                                                if (g_str_has_prefix (name, data->keyword)) {
                                                        prefix_found = TRUE;
                                                        if (!prefix)
                                                                break;
                                                } else if (!g_strrstr (name, data->keyword)) {
                                                        all_found = FALSE;
                                                        break;
                                                }
                                        }
                                }

                                g_free (name);

                                found = (all_found &&
                                         ((prefix && prefix_found) ||
                                          (!prefix && !prefix_found)) ?
                                         TRUE : FALSE);
                        }

                        if (found) {
                                /* Include in the new list. */
                                new_list = g_list_prepend (new_list, link);
                                hits++;

                                if (!exact_link || !dh_link_get_name (link))
                                    continue;

                                /* Look for an exact link match. If the link is a PAGE,
                                 * we can overwrite any previous exact link set. For
                                 * example, when looking for GFile, we want the page,
                                 * not the struct. */
                                if (dh_link_get_link_type (link) == DH_LINK_TYPE_PAGE &&
                                    ((page_id && strcmp (dh_link_get_name (link), page_id) == 0) ||
                                     (strcmp (dh_link_get_name (link), string) == 0))) {
                                        *exact_link = link;
                                } else if (!*exact_link &&
                                           strcmp (dh_link_get_name (link), string) == 0) {
                                        *exact_link = link;
                                }
                        }
                }
        }

        g_free (page_filename_prefix);

        dh_globbed_keywords_free (keyword_globs);

        if (n_hits)
                *n_hits = hits;
        return g_list_sort (new_list, dh_link_compare);
}

static GList *
keyword_model_search (DhKeywordModel  *model,
                      const gchar     *string,
                      const GStrv      keywords,
                      const gchar     *book_id,
                      const gchar     *page_id,
                      const gchar     *language,
                      gboolean         case_sensitive,
                      DhLink         **exact_link)
{
        guint n_hits_left = MAX_HITS;
        guint n_hits;
        GList *in_book_prefixed = NULL;
        GList *in_book_non_prefixed = NULL;
        GList *other_books_prefixed = NULL;
        GList *other_books_non_prefixed = NULL;
        DhLink *in_book_exact_link = NULL;
        DhLink *other_books_exact_link = NULL;

        /* If book_id given; first look for items in the given book id */
        if (book_id) {
                /* First, look for prefixed items */
                n_hits = 0;
                in_book_prefixed = keyword_model_search_books (model,
                                                               string,
                                                               keywords,
                                                               book_id, NULL,
                                                               page_id,
                                                               language,
                                                               case_sensitive,
                                                               TRUE,
                                                               n_hits_left,
                                                               &n_hits,
                                                               &in_book_exact_link);
                n_hits_left -= n_hits;

                /* If not enough hits, get non-prefixed ones */
                if (n_hits_left > 0) {
                        n_hits = 0;
                        in_book_non_prefixed = keyword_model_search_books (model,
                                                                           string,
                                                                           keywords,
                                                                           book_id, NULL,
                                                                           page_id,
                                                                           language,
                                                                           case_sensitive,
                                                                           FALSE,
                                                                           n_hits_left,
                                                                           &n_hits,
                                                                           NULL);
                        n_hits_left -= n_hits;

                }
        }

        /* If still room for more items; look for prefixed items in OTHER books */
        if (n_hits_left > 0) {
                /* First, look for prefixed items */
                n_hits = 0;
                other_books_prefixed = keyword_model_search_books (model,
                                                                   string,
                                                                   keywords,
                                                                   NULL, book_id,
                                                                   page_id,
                                                                   language,
                                                                   case_sensitive,
                                                                   TRUE,
                                                                   n_hits_left,
                                                                   &n_hits,
                                                                   &other_books_exact_link);
                n_hits_left -= n_hits;

                /* If not enough hits, get non-prefixed ones */
                if (n_hits_left > 0) {
                        other_books_non_prefixed = keyword_model_search_books (model,
                                                                               string,
                                                                               keywords,
                                                                               NULL, book_id,
                                                                               page_id,
                                                                               language,
                                                                               case_sensitive,
                                                                               FALSE,
                                                                               n_hits_left,
                                                                               NULL,
                                                                               NULL);
                }
        }

        /* Prefer in-book exact link */
        if (in_book_exact_link)
                *exact_link = in_book_exact_link;
        else if (other_books_exact_link)
                *exact_link = other_books_exact_link;
        else
                *exact_link = NULL;

        /* Build resulting list */
        return (g_list_concat (
                        g_list_concat (
                                g_list_concat (in_book_prefixed,
                                               in_book_non_prefixed),
                                other_books_prefixed),
                        other_books_non_prefixed));
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
        gchar *processed;
        gchar *aux;
        GStrv  strv;
        gint   i;
        gint   j;

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
                g_free (processed);
                return FALSE;
        }

        /* Split the input string into tokens */
        strv = g_strsplit (processed, " ", 0);
        g_free (processed);

        /* Allocate output keywords */
        *keywords = g_new0 (gchar *, g_strv_length (strv) + 1);

        for (i = 0, j = 0; strv[i]; i++) {
                /* Book prefix? */
                if (g_str_has_prefix (strv[i], "book:")) {
                        /* If keyword given but no content, skip it. */
                        if (strv[i][5] == '\0') {
                                continue;
                        }

                        /* We got a second request of book, don't allow
                         * this. */
                        if (*book_id) {
                                g_free (*book_id);
                                g_free (*page_id);
                                g_strfreev (*keywords);
                                return FALSE;
                        }

                        *book_id = g_strdup (&strv[i][5]);
                        continue;
                }

                /* Page prefix? */
                if (g_str_has_prefix (strv[i], "page:")) {
                        /* If keyword given but no content, skip it. */
                        if (strv[i][5] == '\0') {
                                continue;
                        }

                        /* We got a second request of page, don't allow
                         * this. */
                        if (*page_id) {
                                g_free (*book_id);
                                g_free (*page_id);
                                g_strfreev (*keywords);
                                return FALSE;
                        }

                        *page_id = g_strdup (&strv[i][5]);
                        continue;
                }

                /* Then, a new keyword to look for */
                (*keywords)[j++] = g_strdup (strv[i]);
        }

        if (j == 0) {
                g_free (*keywords);
                *keywords = NULL;
        }

        g_strfreev (strv);

        return TRUE;
}

DhLink *
dh_keyword_model_filter (DhKeywordModel *model,
                         const gchar    *string,
                         const gchar    *book_id,
                         const gchar    *language)
{
        DhKeywordModelPrivate *priv;
        GList                 *new_list = NULL;
        gint                   old_length;
        DhLink                *exact_link = NULL;
        gint                   hits;
        gint                   i;
        GtkTreePath           *path;
        GtkTreeIter            iter;
        gchar                 *book_id_in_string = NULL;
        gchar                 *page_id_in_string = NULL;
        GStrv                  keywords = NULL;

        g_return_val_if_fail (DH_IS_KEYWORD_MODEL (model), NULL);
        g_return_val_if_fail (string != NULL, NULL);

        priv = dh_keyword_model_get_instance_private (model);

        /* Do the minimum amount of work: call update on all rows that are
         * kept and remove the rest.
         */
        old_length = priv->keyword_words_length;
        new_list = NULL;
        hits = 0;

        g_free (priv->current_book_id);
        priv->current_book_id = NULL;

        /* Parse input search string */
        if (keyword_model_process_search_string (string,
                                                 &book_id_in_string,
                                                 &page_id_in_string,
                                                 &keywords)) {
                gboolean case_sensitive;

                /* Searches are case sensitive when any uppercase
                 * letter is used in the search terms, matching vim
                 * smartcase behaviour.
                 */
                case_sensitive = FALSE;
                for (i = 0; string[i] != '\0'; i++) {
                        if (g_ascii_isupper (string[i])) {
                                case_sensitive = TRUE;
                                break;
                        }
                }

                /* Keep new current book id */
                priv->current_book_id = g_strdup (book_id_in_string ? book_id_in_string : book_id);

                new_list = keyword_model_search (model,
                                                 string,
                                                 keywords,
                                                 priv->current_book_id,
                                                 page_id_in_string,
                                                 language,
                                                 case_sensitive,
                                                 &exact_link);
                hits = g_list_length (new_list);
        }

        g_free (book_id_in_string);
        g_free (page_id_in_string);
        g_strfreev (keywords);

        /* Update the list of hits. */
        g_list_free (priv->keyword_words);
        priv->keyword_words = new_list;
        priv->keyword_words_length = hits;

        /* Update model: rows 0 -> hits. */
        for (i = 0; i < hits; ++i) {
                path = gtk_tree_path_new ();
                gtk_tree_path_append_index (path, i);
                keyword_model_get_iter (GTK_TREE_MODEL (model), &iter, path);
                gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
                gtk_tree_path_free (path);
        }

        if (old_length > hits) {
                /* Update model: remove rows hits -> old_length. */
                for (i = old_length - 1; i >= hits; i--) {
                        path = gtk_tree_path_new ();
                        gtk_tree_path_append_index (path, i);
                        gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
                        gtk_tree_path_free (path);
                }
        }
        else if (old_length < hits) {
                /* Update model: add rows old_length -> hits. */
                for (i = old_length; i < hits; i++) {
                        path = gtk_tree_path_new ();
                        gtk_tree_path_append_index (path, i);
                        keyword_model_get_iter (GTK_TREE_MODEL (model), &iter, path);
                        gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
                        gtk_tree_path_free (path);
                }
        }

        if (hits == 1) {
                return priv->keyword_words->data;
        }

        return exact_link;
}
