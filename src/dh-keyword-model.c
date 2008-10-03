/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
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
#include "dh-keyword-model.h"

struct _DhKeywordModelPriv {
        GList *original_list;

        GList *keys_list;
        GList *book_list;
        GList *page_list;

        GList *keyword_words;

        gint   stamp;
};

#define G_LIST(x) ((GList *) x)
#define MAX_HITS 100

static void     keyword_model_init              (DhKeywordModel       *list_store);
static void     keyword_model_class_init        (DhKeywordModelClass  *class);
static void     keyword_model_tree_model_init   (GtkTreeModelIface  *iface);


static void     keyword_model_finalize          (GObject            *object);
static GtkTreeModelFlags keyword_model_get_flags (GtkTreeModel *tree_model);
static gint     keyword_model_get_n_columns     (GtkTreeModel       *tree_model);
static GType    keyword_model_get_column_type   (GtkTreeModel       *tree_model,
                                                 gint                keyword);

static gboolean keyword_model_get_iter          (GtkTreeModel       *tree_model,
                                                 GtkTreeIter        *iter,
                                                 GtkTreePath        *path);
static GtkTreePath *
keyword_model_get_path                          (GtkTreeModel       *tree_model,
                                                 GtkTreeIter        *iter);
static void     keyword_model_get_value         (GtkTreeModel       *tree_model,
                                                 GtkTreeIter        *iter,
                                                 gint                column,
                                                 GValue             *value);
static gboolean keyword_model_iter_next         (GtkTreeModel       *tree_model,
                                                 GtkTreeIter        *iter);
static gboolean keyword_model_iter_children     (GtkTreeModel       *tree_model,
                                                 GtkTreeIter        *iter,
                                                 GtkTreeIter        *parent);
static gboolean keyword_model_iter_has_child    (GtkTreeModel       *tree_model,
                                                 GtkTreeIter        *iter);
static gint     keyword_model_iter_n_children   (GtkTreeModel       *tree_model,
                                                 GtkTreeIter        *iter);
static gboolean keyword_model_iter_nth_child    (GtkTreeModel       *tree_model,
                                                 GtkTreeIter        *iter,
                                                 GtkTreeIter        *parent,
                                                 gint                n);
static gboolean keyword_model_iter_parent       (GtkTreeModel       *tree_model,
                                                 GtkTreeIter        *iter,
                                                 GtkTreeIter        *child);

static GObjectClass *parent_class = NULL;


GtkType
dh_keyword_model_get_type (void)
{
        static GType type = 0;

        if (!type) {
                static const GTypeInfo info =
                        {
                                sizeof (DhKeywordModelClass),
                                NULL,           /* base_init */
                                NULL,           /* base_finalize */
                                (GClassInitFunc) keyword_model_class_init,
                                NULL,           /* class_finalize */
                                NULL,           /* class_data */
                                sizeof (DhKeywordModel),
                                0,
                                (GInstanceInitFunc) keyword_model_init,
                        };

                static const GInterfaceInfo tree_model_info =
                        {
                                (GInterfaceInitFunc) keyword_model_tree_model_init,
                                NULL,
                                NULL
                        };

                type = g_type_register_static (G_TYPE_OBJECT,
                                               "DhKeywordModel",
                                               &info, 0);

                g_type_add_interface_static (type,
                                             GTK_TYPE_TREE_MODEL,
                                             &tree_model_info);
        }

        return type;
}

static void
keyword_model_class_init (DhKeywordModelClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);;

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = keyword_model_finalize;
}

static void
keyword_model_tree_model_init (GtkTreeModelIface *iface)
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

static void
keyword_model_init (DhKeywordModel *model)
{
        DhKeywordModelPriv *priv;

        priv = g_new0 (DhKeywordModelPriv, 1);
        model->priv = priv;

        do {
                priv->stamp = g_random_int ();
        } while (priv->stamp == 0);
}

static void
keyword_model_finalize (GObject *object)
{
        DhKeywordModel     *model = DH_KEYWORD_MODEL (object);
        DhKeywordModelPriv *priv = model->priv;
        
        g_list_free (priv->keyword_words);
        g_list_free (priv->original_list);
        g_list_free (priv->keys_list);
        g_list_free (priv->book_list);
        g_list_free (priv->page_list);

        g_free (model->priv);

        parent_class->finalize (object);
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

        if (indices[0] >= g_list_length (priv->keyword_words)) {
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

        /* this is a list, nodes have no children */
        if (parent) {
                return FALSE;
        }

        /* but if parent == NULL we return the list itself as children of the
         * "root"
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
                return g_list_length (priv->keyword_words);
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

DhKeywordModel *
dh_keyword_model_new (void)
{
        DhKeywordModel     *model;

        model = g_object_new (DH_TYPE_KEYWORD_MODEL, NULL);

        return model;
}

void
dh_keyword_model_set_words (DhKeywordModel *model,
                            GList          *keyword_words)
{
        DhKeywordModelPriv *priv;
        DhLink             *link;
        GList              *list;

        g_return_if_fail (DH_IS_KEYWORD_MODEL (model));

        priv = model->priv;

        g_list_free (priv->original_list);
        g_list_free (priv->keys_list);
        g_list_free (priv->book_list);
        g_list_free (priv->page_list);

        priv->original_list = g_list_copy (keyword_words);
        priv->keys_list = priv->book_list = priv->page_list = NULL;

        /* Parse it into usable lists
         */
        for (list = priv->original_list;
             list; list = list->next) {
                link = list->data;
                switch (dh_link_get_link_type (link)) {
                case DH_LINK_TYPE_BOOK:
                        priv->book_list =
                                g_list_prepend (priv->book_list, link);
                        break;
                case DH_LINK_TYPE_PAGE:
                        priv->page_list =
                                g_list_prepend (priv->page_list, link);
                        break;
                case DH_LINK_TYPE_KEYWORD:
                case DH_LINK_TYPE_FUNCTION:
                case DH_LINK_TYPE_STRUCT:
                case DH_LINK_TYPE_MACRO:
                case DH_LINK_TYPE_ENUM:
                case DH_LINK_TYPE_TYPEDEF:
                        priv->keys_list =
                                g_list_prepend (priv->keys_list, link);
                        break;
                default:
                        g_assert_not_reached();
                }
        }
}

DhLink *
dh_keyword_model_filter (DhKeywordModel *model,
                         const gchar    *string)
{
        DhKeywordModelPriv  *priv;
        DhLink              *link;
        GList               *node;
        GList               *new_list = NULL;
        gint                 new_length, old_length;
        gint                 i;
        GtkTreePath         *path;
        GtkTreeIter          iter;
        gint                 hits = 0;
        DhLink              *exact_link = NULL;
        gboolean             found;
        gboolean             case_sensitive;
        gchar               *lower, *name;
        gchar              **stringv, **searchv, *search = NULL;
        gchar               *book_search, *page_search;

        g_return_val_if_fail (DH_IS_KEYWORD_MODEL (model), NULL);
        g_return_val_if_fail (string != NULL, NULL);

        priv = model->priv;

        /* here we want to change the contents of keyword_words,
           call update on all rows that is included in the new
           list and remove on all outside it */
        old_length = g_list_length (priv->keyword_words);

        if (!strcmp ("", string)) {
                new_list = NULL;
        } else {
                stringv = g_strsplit (string, " ", -1);

                book_search    = NULL;
                page_search    = NULL;
                case_sensitive = FALSE;
                searchv        = stringv;

                /* Search for any parameters and position search cursor
                 * to the next element in the search string, also collect
                 * a search string for exact matches.
                 */
                for (i = 0; stringv[i] != NULL; i++) {

                        if (stringv[i][0] == '\0')
                                continue;

                        /* Parse specifications insensitively
                         */
                        lower = g_ascii_strdown (stringv[i], -1);

                        /* Determine if there was a book or page specification
                         */
                        if (!strncmp (lower, "book:", 5)) {
                                book_search = g_strdup (stringv[i] + 5);
                                searchv++;
                        } else if (!strncmp (lower, "page:", 5)) {
                                page_search = g_strdup (stringv[i] + 5);
                                searchv++;
                        } else {

                                /* determine wether or not we should search with case
                                 * sensitivity, searches are case sensitive when upper
                                 * case is used in the search terms, matching vim
                                 * smartcase behaviour
                                 */
                                name = g_ascii_strdown (stringv[i], -1);
                                if (strcmp (name, stringv[i])) {
                                        case_sensitive = TRUE;
                                }
                                g_free (name);

                                /* Accumulate our search string
                                 */
                                if (search == NULL) {
                                        search = g_strdup (stringv[i]);
                                } else {
                                        name = g_strdup_printf ("%s %s", search, stringv[i]);
                                        g_free (search);
                                        search = name;
                                }
                        }
                        g_free (lower);
                }

                /* Only return book and page matches with empty
                 * search strings
                 */
                if (search == NULL) {

                        if (book_search && !page_search) {
                                /* Search books first */
                                for (node = priv->book_list;
                                     node && hits < MAX_HITS;
                                     node = node->next) {

                                        link = node->data;

                                        if (strcmp (dh_link_get_book (link), book_search))
                                                continue;

                                        /* Found our book */
                                        new_list = g_list_prepend (new_list, link);
                                        hits++;

                                        exact_link = link;

                                }

                                /* Currently, only searches that specify
                                 * page & book return page indexes.
                                 */
                        } else if (book_search && page_search) {

                                /* Now search pages */
                                for (node = priv->page_list;
                                     book_search && page_search &&
                                             node && hits < MAX_HITS; node = node->next) {

                                        link = node->data;

                                        if (strcmp (dh_link_get_book (link), book_search))
                                                continue;
                                        if (strcmp (dh_link_get_page (link), page_search))
                                                continue;

                                        /* Found our page */
                                        new_list  = g_list_prepend (new_list, link);
                                        exact_link = link;
                                        hits++;
                                }
                        }
                } else { /* if (search != NULL) */
                        /* Now search keywords */
                        for (node = priv->keys_list;
                             node && hits < MAX_HITS;
                             node = node->next) {

                                link = node->data;
                                found = FALSE;

                                if (book_search && strcmp (dh_link_get_book (link), book_search))
                                        continue;
                                if (page_search && strcmp (dh_link_get_page (link), page_search))
                                        continue;

                                if (!found) {
                                        found = TRUE;

                                        for (i = 0; searchv[i] != NULL; i++) {
                                                if (!case_sensitive) {
                                                        name = g_ascii_strdown (
                                                                dh_link_get_name (link), -1);
                                                } else {
                                                        name = g_strdup (dh_link_get_name (link));
                                                }

                                                if (!g_strrstr (name, searchv[i])) {
                                                        found = FALSE;
                                                        g_free (name);
                                                        break;
                                                }
                                                g_free (name);
                                        }
                                }

                                if (found) {
                                        /* Include in the new list */
                                        new_list = g_list_prepend (new_list, link);
                                        hits++;

                                        if (search && strcmp (dh_link_get_name (link), search) == 0) {
                                                exact_link = link;
                                        }
                                }
                        }
                }

                new_list = g_list_sort (new_list, dh_link_compare);
                g_strfreev (stringv);

                if (search) {
                        g_free (search);
                }

                if (book_search) {
                        g_free (book_search);
                }

                if (page_search) {
                        g_free (page_search);
                }
        }

        new_length = g_list_length (new_list);

        if (priv->keyword_words != priv->original_list) {
                /* Only remove the old list if it's not pointing at the
                   original list */
                g_list_free (priv->keyword_words);
        }

        priv->keyword_words = new_list;

        /* Update rows 0 - new_length */
        for (i = 0; i < new_length; ++i) {
                path = gtk_tree_path_new ();
                gtk_tree_path_append_index (path, i);

                keyword_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

                gtk_tree_model_row_changed (GTK_TREE_MODEL (model),
                                            path, &iter);
                gtk_tree_path_free (path);
        }

        if (old_length > new_length) {
                /* Remove rows new_length - old_length */
                for (i = old_length - 1; i >= new_length; --i) {
                        path = gtk_tree_path_new ();
                        gtk_tree_path_append_index (path, i);

                        gtk_tree_model_row_deleted (GTK_TREE_MODEL (model),
                                                    path);
                        gtk_tree_path_free (path);
                }
        }
        else if (old_length < new_length) {
                /* Add rows old_length - new_length */
                for (i = old_length; i < new_length; ++i) {
                        path = gtk_tree_path_new ();

                        gtk_tree_path_append_index (path, i);

                        keyword_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

                        gtk_tree_model_row_inserted (GTK_TREE_MODEL (model),
                                                     path, &iter);

                        gtk_tree_path_free (path);
                }

        }

        if (hits == 1) {
                return priv->keyword_words->data;
        }

        return exact_link;
}
