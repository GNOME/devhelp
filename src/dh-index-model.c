/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <rhult@codefactory.se>
 * Copyright (C) 2002 Mikael Hallendal <micke@codefactory.se>
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

#include <gtk/gtktreemodel.h>
#include <libgnome/gnome-i18n.h>
#include <string.h>

#include "dh-link.h"
#include "dh-index-model.h"

struct _DhIndexModelPriv {
	GList *original_list;

        GList *index_words;

        gint   stamp;
};

#define G_LIST(x) ((GList *) x)

static void     index_model_init              (DhIndexModel       *list_store);
static void     index_model_class_init        (DhIndexModelClass  *class);
static void     index_model_tree_model_init   (GtkTreeModelIface  *iface);


static void     index_model_finalize          (GObject            *object);
static gint     index_model_get_n_columns     (GtkTreeModel       *tree_model);
static GType    index_model_get_column_type   (GtkTreeModel       *tree_model,
					       gint                index);

static gboolean index_model_get_iter          (GtkTreeModel       *tree_model,
					       GtkTreeIter        *iter,
					       GtkTreePath        *path);
static GtkTreePath * 
index_model_get_path                          (GtkTreeModel       *tree_model,
					       GtkTreeIter        *iter);
static void     index_model_get_value         (GtkTreeModel       *tree_model,
					       GtkTreeIter        *iter,
					       gint                column,
					       GValue             *value);
static gboolean index_model_iter_next         (GtkTreeModel       *tree_model,
					       GtkTreeIter        *iter);
static gboolean index_model_iter_children     (GtkTreeModel       *tree_model,
					       GtkTreeIter        *iter,
					       GtkTreeIter        *parent);
static gboolean index_model_iter_has_child    (GtkTreeModel       *tree_model,
					       GtkTreeIter        *iter);
static gint     index_model_iter_n_children   (GtkTreeModel       *tree_model,
					       GtkTreeIter        *iter);
static gboolean index_model_iter_nth_child    (GtkTreeModel       *tree_model,
					       GtkTreeIter        *iter,
					       GtkTreeIter        *parent,
					       gint                n);
static gboolean index_model_iter_parent       (GtkTreeModel       *tree_model,
					       GtkTreeIter        *iter,
					       GtkTreeIter        *child);

static GObjectClass *parent_class = NULL;


GtkType
dh_index_model_get_type (void)
{
        static GType type = 0;
        
        if (!type) {
                static const GTypeInfo info =
                        {
                                sizeof (DhIndexModelClass),
                                NULL,		/* base_init */
                                NULL,		/* base_finalize */
                                (GClassInitFunc) index_model_class_init,
                                NULL,		/* class_finalize */
                                NULL,		/* class_data */
                                sizeof (DhIndexModel),
                                0,
                                (GInstanceInitFunc) index_model_init,
                        };
                
                static const GInterfaceInfo tree_model_info =
                        {
                                (GInterfaceInitFunc) index_model_tree_model_init,
                                NULL,
                                NULL
                        };
                
                type = g_type_register_static (G_TYPE_OBJECT,
					       "DhIndexModel", 
					       &info, 0);
      
                g_type_add_interface_static (type,
                                             GTK_TYPE_TREE_MODEL,
                                             &tree_model_info);
        }
        
        return type;
}

static void
index_model_class_init (DhIndexModelClass *class)
{
        GObjectClass *object_class;

        parent_class = g_type_class_peek_parent (class);
        object_class = (GObjectClass*) class;

        object_class->finalize = index_model_finalize;
}

static void
index_model_tree_model_init (GtkTreeModelIface *iface)
{
        iface->get_n_columns   = index_model_get_n_columns;
        iface->get_column_type = index_model_get_column_type;
        iface->get_iter        = index_model_get_iter;
        iface->get_path        = index_model_get_path;
        iface->get_value       = index_model_get_value;
        iface->iter_next       = index_model_iter_next;
        iface->iter_children   = index_model_iter_children;
        iface->iter_has_child  = index_model_iter_has_child;
        iface->iter_n_children = index_model_iter_n_children;
        iface->iter_nth_child  = index_model_iter_nth_child;
        iface->iter_parent     = index_model_iter_parent;
}

static void
index_model_init (DhIndexModel *model)
{
        DhIndexModelPriv *priv;
        
        priv = g_new0 (DhIndexModelPriv, 1);
        
	do {
		priv->stamp = g_random_int ();
	} while (priv->stamp == 0);

	priv->original_list = NULL;
	priv->index_words = NULL;

        model->priv = priv;
}

static void
index_model_finalize (GObject *object)
{
	DhIndexModel *model = DH_INDEX_MODEL (object);
        
        if (model->priv) {
		if (model->priv->index_words) {
			g_list_free (model->priv->index_words);
		}

                g_free (model->priv);
                model->priv = NULL;
        }
        
        (* parent_class->finalize) (object);
}

static gint
index_model_get_n_columns (GtkTreeModel *tree_model)
{
	return DH_INDEX_MODEL_NR_OF_COLS;
}

static GType
index_model_get_column_type (GtkTreeModel *tree_model,
                     gint          column)
{
	switch (column) {
	case DH_INDEX_MODEL_COL_NAME:
		return G_TYPE_STRING;
		break;
	case DH_INDEX_MODEL_COL_LINK:
		return G_TYPE_POINTER;
		break;
	default:
		return G_TYPE_INVALID;
	}
}

static gboolean
index_model_get_iter (GtkTreeModel *tree_model,
              GtkTreeIter  *iter,
              GtkTreePath  *path)
{
        DhIndexModel     *model;
        DhIndexModelPriv *priv;
        GList               *node;
        gint                 i;
        
        g_return_val_if_fail (DH_IS_INDEX_MODEL (tree_model), FALSE);
        g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, FALSE);

        model = DH_INDEX_MODEL (tree_model);
        priv  = model->priv;
        
        i = gtk_tree_path_get_indices (path)[0];
        
        if (i >= g_list_length (priv->index_words)) {
                return FALSE;
        }
        
        node = g_list_nth (priv->index_words, i);
        
        iter->stamp     = priv->stamp;
        iter->user_data = node;
        
        return TRUE;
}

static GtkTreePath *
index_model_get_path (GtkTreeModel *tree_model,
              GtkTreeIter  *iter)
{
        DhIndexModel     *model = DH_INDEX_MODEL (tree_model);
        DhIndexModelPriv *priv;
        GtkTreePath         *path;
        GList               *node;
        gint                 i = 0;

        g_return_val_if_fail (DH_IS_INDEX_MODEL (tree_model), NULL);
        g_return_val_if_fail (iter->stamp == model->priv->stamp, NULL);

        priv = model->priv;

        for (node = priv->index_words; node; node = node->next)
        {
                if ((gpointer)node->data == (gpointer)iter->user_data)
                        break;
                i++;
        }

        if (node == NULL) {
                return NULL;
        }
        
        path = gtk_tree_path_new ();
        gtk_tree_path_append_index (path, i);

        return path;
}

static void
index_model_get_value (GtkTreeModel *tree_model,
		       GtkTreeIter  *iter,
		       gint          column,
		       GValue       *value)
{
        DhLink *link;
	
        g_return_if_fail (DH_IS_INDEX_MODEL (tree_model));
        g_return_if_fail (iter != NULL);

	link = DH_LINK (G_LIST(iter->user_data)->data);
	
        switch (column) {
	case DH_INDEX_MODEL_COL_NAME:
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, link->name);
		break;
	case DH_INDEX_MODEL_COL_LINK:
		g_value_init (value, G_TYPE_POINTER);
		g_value_set_pointer (value, link);
		break;
        default:
                g_warning ("Bad column %d requested", column);
        }
}

static gboolean
index_model_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	DhIndexModel *model = DH_INDEX_MODEL (tree_model);
        
        g_return_val_if_fail (DH_IS_INDEX_MODEL (tree_model), FALSE);
        g_return_val_if_fail (model->priv->stamp == iter->stamp, FALSE);

        iter->user_data = G_LIST(iter->user_data)->next;
        
        return (iter->user_data != NULL);
}

static gboolean
index_model_iter_children (GtkTreeModel *tree_model,
                   GtkTreeIter  *iter,
                   GtkTreeIter  *parent)
{
        DhIndexModelPriv *priv;
        
        g_return_val_if_fail (DH_IS_INDEX_MODEL (tree_model), FALSE);
        
        priv = DH_INDEX_MODEL(tree_model)->priv;
        
        /* this is a list, nodes have no children */
        if (parent) {
                return FALSE;
        }

        /* but if parent == NULL we return the list itself as children of the
         * "root"
         */
        
        if (priv->index_words) {
                iter->stamp = priv->stamp;
                iter->user_data = priv->index_words;
                return TRUE;
        } 
        
        return FALSE;
}

static gboolean
index_model_iter_has_child (GtkTreeModel *tree_model,
                    GtkTreeIter  *iter)
{
        return FALSE;
}

static gint
index_model_iter_n_children (GtkTreeModel *tree_model,
                     GtkTreeIter  *iter)
{
        DhIndexModelPriv *priv;

        g_return_val_if_fail (DH_IS_INDEX_MODEL (tree_model), -1);

        priv = DH_INDEX_MODEL(tree_model)->priv;

        if (iter == NULL) {
                return g_list_length (priv->index_words);
        }
        
        g_return_val_if_fail (priv->stamp == iter->stamp, -1);

        return 0;
}

static gboolean
index_model_iter_nth_child (GtkTreeModel *tree_model,
                    GtkTreeIter  *iter,
                    GtkTreeIter  *parent,
                    gint          n)
{
        DhIndexModelPriv *priv;
        GList               *child;

        g_return_val_if_fail (DH_IS_INDEX_MODEL (tree_model), FALSE);

        priv = DH_INDEX_MODEL(tree_model)->priv;
        
        if (parent) {
                return FALSE;
        }
        
        child = g_list_nth (priv->index_words, n);
        
        if (child) {
                iter->stamp     = priv->stamp;
                iter->user_data = child;
                return TRUE;
        }

        return FALSE;
}

static gboolean
index_model_iter_parent (GtkTreeModel *tree_model,
                 GtkTreeIter  *iter,
                 GtkTreeIter  *child)
{
        return FALSE;
}

DhIndexModel *
dh_index_model_new (void)
{
        DhIndexModel     *model;
        
        model = g_object_new (DH_TYPE_INDEX_MODEL, NULL);
        
        return model;
}

void
dh_index_model_set_words (DhIndexModel *model, GList *index_words)
{
	DhIndexModelPriv *priv;

	g_return_if_fail (DH_IS_INDEX_MODEL (model));

	priv = model->priv;
		
	priv->original_list = g_list_copy (index_words);
}

void
dh_index_model_filter (DhIndexModel *model, const gchar *string)
{
	DhIndexModelPriv *priv;
	DhLink           *link;
	GList              *node;
	GList              *new_list = NULL;
	gint                new_length, old_length;
	gint                i;
	GtkTreePath        *path;
 	GtkTreeIter         iter;
      
	g_return_if_fail (DH_IS_INDEX_MODEL (model));
	g_return_if_fail (string != NULL);

	priv = model->priv;

	/* here we want to change the contents of index_words,
	   call update on all rows that is included in the new 
	   list and remove on all outside it */
	
	old_length = g_list_length (priv->index_words);

	if (!strcmp ("", string)) {
		new_list = NULL;
	} else {
		for (node = priv->original_list; node; node = node->next) {
			link = DH_LINK (node->data);
			
			if (!strncmp (link->name, string, strlen (string))) {
				/* Include in the new list */
				new_list = g_list_prepend (new_list, link);
			}
		}
		
		new_list = g_list_sort (new_list, dh_link_compare);
	}

	new_length = g_list_length (new_list);
	
	if (priv->index_words != priv->original_list) {
		/* Only remove the old list if it's not pointing at the 
		   original list */
 		g_list_free (priv->index_words);
	}

	priv->index_words = new_list;

	/* Update rows 0 - new_length */
	for (i = 0; i < new_length; ++i) {
		path = gtk_tree_path_new ();
		gtk_tree_path_append_index (path, i);
		
		index_model_get_iter (GTK_TREE_MODEL (model), &iter, path);
		
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
			
			index_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

			gtk_tree_model_row_inserted (GTK_TREE_MODEL (model),
						     path, &iter);
			
			gtk_tree_path_free (path);
		}
		
	}
}

