/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2008 Imendio AB
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

struct _DhKeywordModelPriv {
        DhBookManager *book_manager;

        GList *keyword_words;
        gint   keyword_words_length;

        gint   stamp;
};

#define G_LIST(x) ((GList *) x)
#define MAX_HITS 100

static void dh_keyword_model_init            (DhKeywordModel      *list_store);
static void dh_keyword_model_class_init      (DhKeywordModelClass *class);
static void dh_keyword_model_tree_model_init (GtkTreeModelIface   *iface);

G_DEFINE_TYPE_WITH_CODE (DhKeywordModel, dh_keyword_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                dh_keyword_model_tree_model_init));

static void
keyword_model_dispose (GObject *object)
{
        DhKeywordModel     *model = DH_KEYWORD_MODEL (object);
        DhKeywordModelPriv *priv = model->priv;

        if (priv->book_manager) {
                g_object_unref (priv->book_manager);
                priv->book_manager = NULL;
        }

        G_OBJECT_CLASS (dh_keyword_model_parent_class)->dispose (object);
}

static void
keyword_model_finalize (GObject *object)
{
        DhKeywordModel     *model = DH_KEYWORD_MODEL (object);
        DhKeywordModelPriv *priv = model->priv;

        g_list_free (priv->keyword_words);

        g_free (model->priv);

        G_OBJECT_CLASS (dh_keyword_model_parent_class)->finalize (object);
}

static void
dh_keyword_model_class_init (DhKeywordModelClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);;

        object_class->finalize = keyword_model_finalize;
        object_class->dispose = keyword_model_dispose;
}

static void
dh_keyword_model_init (DhKeywordModel *model)
{
        DhKeywordModelPriv *priv;

        priv = g_new0 (DhKeywordModelPriv, 1);
        model->priv = priv;

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
                break;
        case DH_KEYWORD_MODEL_COL_LINK:
                return G_TYPE_POINTER;
        default:
                return G_TYPE_INVALID;
        }
}

static gboolean
keyword_model_get_iter (GtkTreeModel *tree_model,
                        GtkTreeIter  *iter,
                        GtkTreePath  *path)
{
        DhKeywordModel     *model;
        DhKeywordModelPriv *priv;
        GList              *node;
        const gint         *indices;

        model = DH_KEYWORD_MODEL (tree_model);
        priv = model->priv;

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
        DhKeywordModel     *model = DH_KEYWORD_MODEL (tree_model);
        DhKeywordModelPriv *priv;
        GtkTreePath        *path;
        gint                i = 0;

        g_return_val_if_fail (iter->stamp == model->priv->stamp, NULL);

        priv = model->priv;

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
        DhLink *link;

        link = G_LIST (iter->user_data)->data;

        switch (column) {
        case DH_KEYWORD_MODEL_COL_NAME:
                g_value_init (value, G_TYPE_STRING);
                g_value_set_string (value, dh_link_get_name (link));
                break;
        case DH_KEYWORD_MODEL_COL_LINK:
                g_value_init (value, G_TYPE_POINTER);
                g_value_set_pointer (value, link);
                break;
        default:
                g_warning ("Bad column %d requested", column);
        }
}

static gboolean
keyword_model_iter_next (GtkTreeModel *tree_model,
                         GtkTreeIter  *iter)
{
        DhKeywordModel *model = DH_KEYWORD_MODEL (tree_model);

        g_return_val_if_fail (model->priv->stamp == iter->stamp, FALSE);

        iter->user_data = G_LIST (iter->user_data)->next;

        return (iter->user_data != NULL);
}

static gboolean
keyword_model_iter_children (GtkTreeModel *tree_model,
                             GtkTreeIter  *iter,
                             GtkTreeIter  *parent)
{
        DhKeywordModelPriv *priv;

        priv = DH_KEYWORD_MODEL (tree_model)->priv;

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
        DhKeywordModelPriv *priv;

        priv = DH_KEYWORD_MODEL (tree_model)->priv;

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
        DhKeywordModelPriv *priv;
        GList              *child;

        priv = DH_KEYWORD_MODEL (tree_model)->priv;

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
        g_return_if_fail (DH_IS_KEYWORD_MODEL (model));

        model->priv->book_manager = g_object_ref (book_manager);
}

static GList *
keyword_model_search_books (DhKeywordModel  *model,
                            const gchar     *string,
                            gchar          **stringv,
                            const gchar     *book_id,
                            gboolean         case_sensitive,
                            gboolean         prefix,
                            guint            max_hits,
                            guint           *n_hits,
                            DhLink         **exact_link)
{
        DhKeywordModelPriv *priv;
        GList              *new_list = NULL, *b;
        gint                hits = 0;
        gchar              *page_id = NULL;
        gchar              *page_filename_prefix = NULL;

        priv = model->priv;

        /* The search string may be prefixed by a page:foobar qualifier, it
         * will be matched against the filenames of the hits to limit the
         * search to pages whose filename is prefixed by "foobar.
         */
        if (stringv && g_str_has_prefix(stringv[0], "page:")) {
                page_id = stringv[0] + 5;
                page_filename_prefix = g_strdup_printf("%s.", page_id);
                stringv++;
        }

        for (b = dh_book_manager_get_books (priv->book_manager);
             b && hits < max_hits;
             b = g_list_next (b)) {
                DhBook *book;
                GList *l;

                book = DH_BOOK (b->data);

                if (book_id &&
                    g_strcmp0 (book_id, dh_book_get_name (book)) != 0) {
                        continue;
                }

                for (l = dh_book_get_keywords (book);
                     l && hits < max_hits;
                     l = g_list_next (l)) {
                        DhLink   *link;
                        gboolean  found;
                        gchar    *name;
                        gchar    *file_name;

                        link = l->data;
                        found = FALSE;

                        file_name = (case_sensitive ?
                                     g_strdup (dh_link_get_file_name (link)) :
                                     g_ascii_strdown (dh_link_get_file_name (link), -1));

                        if (page_id &&
                            (dh_link_get_link_type (link) == DH_LINK_TYPE_PAGE ||
                             !g_str_has_prefix (file_name, page_filename_prefix))) {
                                g_free (file_name);
                                continue;
                        }

                        name = (case_sensitive ?
                                g_strdup (dh_link_get_name (link)) :
                                g_ascii_strdown (dh_link_get_name (link), -1));

                        if (stringv[0] == NULL) {
                                /* means only a page was specified, no keyword */
                                if (prefix) {
                                        if (g_str_has_prefix (file_name, page_id))
                                                found = TRUE;
                                } else {
                                        if (!g_str_has_prefix (file_name, page_id) &&
                                            g_strrstr (file_name, page_id))
                                                found = TRUE;
                                }
                        } else {
                                gboolean all_found;
                                gboolean prefix_found;
                                gint     i;

                                prefix_found = FALSE;
                                all_found = TRUE;

                                for (i = 0; stringv[i] != NULL; i++) {
                                        if (g_str_has_prefix (name, stringv[i])) {
                                                prefix_found = TRUE;
                                                /* If we get a prefix match and we're not
                                                 * looking for prefix, stop. */
                                                if (!prefix)
                                                        break;
                                        } else if (!g_strrstr (name, stringv[i])) {
                                                all_found = FALSE;
                                                break;
                                        }
                                }

                                found = (all_found &&
                                         ((prefix && prefix_found) ||
                                         (!prefix && !prefix_found)) ?
                                         TRUE : FALSE);
                        }

                        g_free (name);
                        g_free (file_name);

                        if (found) {
                                /* Include in the new list. */
                                new_list = g_list_prepend (new_list, link);
                                hits++;

                                if (exact_link &&
                                    !*exact_link &&
                                    dh_link_get_name (link) && (
                                            (dh_link_get_link_type (link) == DH_LINK_TYPE_PAGE &&
                                             page_id && strcmp (dh_link_get_name (link), page_id) == 0) ||
                                            (strcmp (dh_link_get_name (link), string) == 0))) {
                                        *exact_link = link;
                                }
                        }
                }
        }

        g_free (page_filename_prefix);

        if (n_hits)
                *n_hits = hits;
        return g_list_sort (new_list, dh_link_compare);
}

static GList *
keyword_model_search (DhKeywordModel  *model,
                      const gchar     *string,
                      gchar          **stringv,
                      const gchar     *book_id,
                      gboolean         case_sensitive,
                      DhLink         **exact_link)
{
        guint max_hits = MAX_HITS;
        guint n_hits;
        GList *list;
        gint i;

        g_debug ("-------------------");
        g_debug ("string: %s", string);
        g_debug ("book_id: %s", book_id);
        for (i = 0; stringv[i]; i++) {
                g_debug ("stringv[%d]: '%s'", i, stringv[i]);
        }

        /* First, look for prefixed items */
        list = keyword_model_search_books (model,
                                           string,
                                           stringv,
                                           book_id,
                                           case_sensitive,
                                           TRUE,
                                           max_hits,
                                           &n_hits,
                                           exact_link);

        if (n_hits < max_hits) {
                GList *non_prefixed_list;

                /* If not enough hits, get non-prefixed ones */
                non_prefixed_list = keyword_model_search_books (model,
                                                                string,
                                                                stringv,
                                                                book_id,
                                                                case_sensitive,
                                                                FALSE,
                                                                max_hits - n_hits,
                                                                NULL,
                                                                NULL);
                list = g_list_concat (list, non_prefixed_list);
        }

        return list;
}

DhLink *
dh_keyword_model_filter (DhKeywordModel *model,
                         const gchar    *string,
                         const gchar    *book_id)
{
        DhKeywordModelPriv  *priv;
        GList               *new_list = NULL;
        gint                 old_length;
        DhLink              *exact_link = NULL;
        gint                 hits;
        gint                 i;
        GtkTreePath         *path;
        GtkTreeIter          iter;
        gchar               *processed_string;
        gchar               *aux_str;

        g_return_val_if_fail (DH_IS_KEYWORD_MODEL (model), NULL);
        g_return_val_if_fail (string != NULL, NULL);

        priv = model->priv;

        /* Do the minimum amount of work: call update on all rows that are
         * kept and remove the rest.
         */
        old_length = priv->keyword_words_length;
        new_list = NULL;
        hits = 0;

        /* Remove all leading and trailing whitespaces in the search string */
        processed_string = g_strdup (string);
        g_strstrip (processed_string);
        /* Avoid words being separated by more than one whitespace */
        aux_str = processed_string;
        while ((aux_str = strchr (aux_str, ' ')) != NULL) {
                g_strchug (++aux_str);
        }

        if (processed_string[0] != '\0') {
                gchar    **stringv;
                gboolean   case_sensitive;

                stringv = g_strsplit (processed_string, " ", -1);

                /* Searches are case sensitive when any uppercase
                 * letter is used in the search terms, matching vim
                 * smartcase behaviour.
                 */
                case_sensitive = FALSE;
                for (i = 0; processed_string[i] != '\0'; i++) {
                        if (g_ascii_isupper (processed_string[i])) {
                                case_sensitive = TRUE;
                                break;
                        }
                }

                new_list = keyword_model_search (model,
                                                 processed_string,
                                                 stringv,
                                                 book_id,
                                                 case_sensitive,
                                                 &exact_link);
                hits = g_list_length (new_list);

                g_strfreev (stringv);
        }

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

        g_free (processed_string);

        if (hits == 1) {
                return priv->keyword_words->data;
        }

        return exact_link;
}
