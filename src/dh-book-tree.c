/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2003      CodeFactory AB
 * Copyright (C) 2008      Imendio AB
 * Copyright (C) 2010      Lanedo GmbH
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
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "dh-book-tree.h"
#include "dh-book.h"

typedef struct {
        const gchar *uri;
        gboolean     found;
        GtkTreeIter  iter;
        GtkTreePath *path;
} FindURIData;

typedef struct {
        GtkTreeStore  *store;
        DhBookManager *book_manager;
        DhLink        *selected_link;

        /* Signals */
        guint book_created_id;
        guint book_deleted_id;
        guint book_enabled_id;
        guint book_disabled_id;
        guint group_by_language_id;
} DhBookTreePrivate;

static void dh_book_tree_class_init        (DhBookTreeClass  *klass);
static void dh_book_tree_init              (DhBookTree       *tree);
static void book_tree_add_columns          (DhBookTree       *tree);
static void book_tree_setup_selection      (DhBookTree       *tree);
static void book_tree_insert_node          (DhBookTree       *tree,
                                            GNode            *node,
                                            GtkTreeIter      *current_iter,
                                            DhBook           *book);
static void book_tree_selection_changed_cb (GtkTreeSelection *selection,
                                            DhBookTree       *tree);

enum {
        LINK_SELECTED,
        LAST_SIGNAL
};

enum {
	COL_TITLE,
	COL_LINK,
        COL_BOOK,
	COL_WEIGHT,
        COL_UNDERLINE,
	N_COLUMNS
};

enum {
        PROP_0,
        PROP_BOOK_MANAGER
};

G_DEFINE_TYPE_WITH_PRIVATE (DhBookTree, dh_book_tree, GTK_TYPE_TREE_VIEW);

static gint signals[LAST_SIGNAL] = { 0 };

static void
dh_book_tree_dispose (GObject *object)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (DH_BOOK_TREE (object));

        /* Disconnect signals */
        if (priv->book_created_id != 0 &&
            g_signal_handler_is_connected (priv->book_manager, priv->book_created_id)) {
                g_signal_handler_disconnect (priv->book_manager, priv->book_created_id);
                priv->book_created_id = 0;
        }

        if (priv->book_deleted_id &&
            g_signal_handler_is_connected (priv->book_manager, priv->book_deleted_id)) {
                g_signal_handler_disconnect (priv->book_manager, priv->book_deleted_id);
                priv->book_deleted_id = 0;
        }

        if (priv->book_enabled_id != 0 &&
            g_signal_handler_is_connected (priv->book_manager, priv->book_enabled_id)) {
                g_signal_handler_disconnect (priv->book_manager, priv->book_enabled_id);
                priv->book_enabled_id = 0;
        }

        if (priv->book_disabled_id != 0 &&
            g_signal_handler_is_connected (priv->book_manager, priv->book_disabled_id)) {
                g_signal_handler_disconnect (priv->book_manager, priv->book_disabled_id);
                priv->book_disabled_id = 0;
        }

        if (priv->group_by_language_id != 0 &&
            g_signal_handler_is_connected (priv->book_manager, priv->group_by_language_id)) {
                g_signal_handler_disconnect (priv->book_manager, priv->group_by_language_id);
                priv->group_by_language_id = 0;
        }

        g_clear_object (&priv->store);
        g_clear_object (&priv->book_manager);

        G_OBJECT_CLASS (dh_book_tree_parent_class)->dispose (object);
}

static void
dh_book_tree_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (DH_BOOK_TREE (object));

        switch (prop_id) {
                case PROP_BOOK_MANAGER:
                        g_value_set_object (value, priv->book_manager);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_book_tree_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (DH_BOOK_TREE (object));

        switch (prop_id) {
                case PROP_BOOK_MANAGER:
                        g_return_if_fail (priv->book_manager == NULL);
                        priv->book_manager = g_value_dup_object (value);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_book_tree_init (DhBookTree *tree)
{
        DhBookTreePrivate *priv;

        priv = dh_book_tree_get_instance_private (tree);

	priv->store = gtk_tree_store_new (N_COLUMNS,
					  G_TYPE_STRING,
					  G_TYPE_POINTER,
                                          G_TYPE_OBJECT,
                                          PANGO_TYPE_WEIGHT,
                                          PANGO_TYPE_UNDERLINE);
	priv->selected_link = NULL;
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree),
				 GTK_TREE_MODEL (priv->store));

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);

	book_tree_add_columns (tree);

	book_tree_setup_selection (tree);
}

static void
book_tree_add_columns (DhBookTree *tree)
{
	GtkCellRenderer   *cell;
	GtkTreeViewColumn *column;

	column = gtk_tree_view_column_new ();

	cell = gtk_cell_renderer_text_new ();
	g_object_set (cell,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      NULL);
	gtk_tree_view_column_pack_start (column, cell, TRUE);
	gtk_tree_view_column_set_attributes (column, cell,
					     "text", COL_TITLE,
                                             "weight", COL_WEIGHT,
                                             "underline", COL_UNDERLINE,
					     NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
}

static void
book_tree_setup_selection (DhBookTree *tree)
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	g_signal_connect (selection, "changed",
			  G_CALLBACK (book_tree_selection_changed_cb),
			  tree);
}

/* Tries to find:
 *  - An exact match of the language group
 *  - The language group which should be just after our given language group.
 *  - Both.
 */
static void
book_tree_find_language_group (DhBookTree  *tree,
                               const gchar *language,
                               GtkTreeIter *exact_iter,
                               gboolean    *exact_found,
                               GtkTreeIter *next_iter,
                               gboolean    *next_found)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        GtkTreeIter     loop_iter;

        g_assert ((exact_iter && exact_found) || (next_iter && next_found));

        /* Reset all flags to not found */
        if (exact_found)
                *exact_found = FALSE;
        if (next_found)
                *next_found = FALSE;

        /* If we're not doing language grouping, return not found */
        if (!dh_book_manager_get_group_by_language (priv->book_manager))
                return;

        if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store),
                                            &loop_iter)) {
                /* Store is empty, not found */
                return;
        }

        do {
                gchar  *title = NULL;

                /* Look for language titles, which are those where there
                 * is no book object associated in the row */
                gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
                                    &loop_iter,
                                    COL_TITLE, &title,
                                    -1);

                if (exact_iter &&
                    g_ascii_strcasecmp (title, language) == 0) {
                        /* Exact match found! */
                        *exact_iter = loop_iter;
                        *exact_found = TRUE;
                        if (!next_iter) {
                                /* If we were not requested to look for the next one, end here */
                                g_free (title);
                                return;
                        }
                } else if (next_iter &&
                           g_ascii_strcasecmp (title, language) > 0) {
                        *next_iter = loop_iter;
                        *next_found = TRUE;
                        /* There's no way to have an exact match after the next, so end here */
                        g_free (title);
                        return;
                }

                g_free (title);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store),
                                           &loop_iter));
}

/* Tries to find, starting at 'first' (if given), and always in the same
 * level of the tree:
 *  - An exact match of the book
 *  - The book which should be just after our given book
 *  - Both.
 */
static void
book_tree_find_book (DhBookTree        *tree,
                     DhBook            *book,
                     const GtkTreeIter *first,
                     GtkTreeIter       *exact_iter,
                     gboolean          *exact_found,
                     GtkTreeIter       *next_iter,
                     gboolean          *next_found)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        GtkTreeIter     loop_iter;

        g_assert ((exact_iter && exact_found) || (next_iter && next_found));

        /* Reset all flags to not found */
        if (exact_found)
                *exact_found = FALSE;
        if (next_found)
                *next_found = FALSE;

        /* Setup iteration start */
        if (!first) {
                /* If no first given, start iterating from the start of the model */
                if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store),
                                                    &loop_iter)) {
                        /* Store is empty, not found */
                        return;
                }
        } else {
                loop_iter = *first;
        }

        do {
                DhBook *in_tree_book = NULL;

                gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
                                    &loop_iter,
                                    COL_BOOK, &in_tree_book,
                                    -1);

                /* We can compare pointers directly as we're playing with references
                 * of the same object */
                if (exact_iter &&
                    in_tree_book == book) {
                        *exact_iter = loop_iter;
                        *exact_found = TRUE;
                        if (!next_iter) {
                                /* If we were not requested to look for the next one, end here */
                                g_object_unref (in_tree_book);
                                return;
                        }
                } else if (next_iter &&
                           dh_book_cmp_by_title (in_tree_book, book) > 0) {
                        *next_iter = loop_iter;
                        *next_found = TRUE;
                        g_object_unref (in_tree_book);
                        return;
                }

                if (in_tree_book)
                        g_object_unref (in_tree_book);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store),
                                           &loop_iter));
}

static void
book_tree_add_book_to_store (DhBookTree *tree,
                             DhBook     *book,
                             gboolean    group_by_language)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        GtkTreeIter     book_iter;

        /* If grouping by language we need to add the language categories */
        if (group_by_language) {
                GtkTreeIter  language_iter;
                gboolean     language_iter_found;
                GtkTreeIter  next_language_iter;
                gboolean     next_language_iter_found;
                const gchar *language_title;
                gboolean     new_language = FALSE;

                language_title = dh_book_get_language (book);

                /* Look for the proper language group */
                book_tree_find_language_group (tree,
                                               language_title,
                                               &language_iter,
                                               &language_iter_found,
                                               &next_language_iter,
                                               &next_language_iter_found);
                /* New language group needs to be created? */
                if (!language_iter_found) {
                        if (!next_language_iter_found) {
                                gtk_tree_store_append (priv->store,
                                                       &language_iter,
                                                       NULL);
                        } else {
                                gtk_tree_store_insert_before (priv->store,
                                                              &language_iter,
                                                              NULL,
                                                              &next_language_iter);
                        }

                        gtk_tree_store_set (priv->store,
                                            &language_iter,
                                            COL_TITLE,      language_title,
                                            COL_LINK,       NULL,
                                            COL_BOOK,       NULL,
                                            COL_WEIGHT,     PANGO_WEIGHT_BOLD,
                                            COL_UNDERLINE,  PANGO_UNDERLINE_SINGLE,
                                            -1);

                        new_language = TRUE;
                }

                /* If we got to add first book in a given language group, just append it. */
                if (new_language) {
                        GtkTreePath *path;

                        gtk_tree_store_append (priv->store,
                                               &book_iter,
                                               &language_iter);

                        /* Make sure we start with the language row expanded */
                        path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store),
                                                        &language_iter);
                        gtk_tree_view_expand_row (GTK_TREE_VIEW (tree),
                                                  path,
                                                  FALSE);
                        gtk_tree_path_free (path);
                } else {
                        GtkTreeIter first_book_iter;
                        GtkTreeIter next_book_iter;
                        gboolean    next_book_iter_found;

                        /* The language will have at least one book, so we move iter to it */
                        gtk_tree_model_iter_children (GTK_TREE_MODEL (priv->store),
                                                      &first_book_iter,
                                                      &language_iter);

                        /* Find next possible book in language group */
                        book_tree_find_book (tree,
                                             book,
                                             &first_book_iter,
                                             NULL,
                                             NULL,
                                             &next_book_iter,
                                             &next_book_iter_found);
                        if (!next_book_iter_found) {
                                gtk_tree_store_append (priv->store,
                                                       &book_iter,
                                                       &language_iter);
                        } else {
                                gtk_tree_store_insert_before (priv->store,
                                                              &book_iter,
                                                              &language_iter,
                                                              &next_book_iter);
                        }
                }
        } else {
                /* No language grouping, just order by book title */
                GtkTreeIter next_book_iter;
                gboolean    next_book_iter_found;

                book_tree_find_book (tree,
                                     book,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &next_book_iter,
                                     &next_book_iter_found);
                if (!next_book_iter_found) {
                        gtk_tree_store_append (priv->store,
                                               &book_iter,
                                               NULL);
                } else {
                        gtk_tree_store_insert_before (priv->store,
                                                      &book_iter,
                                                      NULL,
                                                      &next_book_iter);
                }
        }

        /* Now book_iter contains the proper iterator where we'll add the whole
         * book tree. */
        book_tree_insert_node (tree,
                               dh_book_get_tree (book),
                               &book_iter,
                               book);
}

static void
book_tree_populate_tree (DhBookTree *tree)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        GList          *l;
        gboolean        group_by_language;

        group_by_language = dh_book_manager_get_group_by_language (priv->book_manager);

        gtk_tree_store_clear (priv->store);

        /* This list comes in order, but we don't really mind */
        for (l = dh_book_manager_get_books (priv->book_manager);
             l;
             l = g_list_next (l)) {
                DhBook *book = DH_BOOK (l->data);

                /* Only add enabled books to the tree */
                if (dh_book_get_enabled (book)) {
                        book_tree_add_book_to_store (tree,
                                                     book,
                                                     group_by_language);
                }
        }
}

static void
book_tree_book_created_or_enabled_cb (DhBookManager *book_manager,
                                      GObject       *book_object,
                                      gpointer       user_data)
{
        DhBookTree     *tree = user_data;
        DhBook         *book = DH_BOOK (book_object);
        gboolean        group_by_language;

        if (!dh_book_get_enabled (book))
                return;

        group_by_language = dh_book_manager_get_group_by_language (book_manager);

        book_tree_add_book_to_store (tree,
                                     book,
                                     group_by_language);
}

static void
book_tree_book_deleted_or_disabled_cb (DhBookManager *book_manager,
                                       GObject       *book_object,
                                       gpointer       user_data)
{
        DhBookTree     *tree = user_data;
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        DhBook         *book = DH_BOOK (book_object);
        GtkTreeIter     exact_iter;
        gboolean        exact_iter_found = FALSE;
        GtkTreeIter     language_iter;
        gboolean        language_iter_found = FALSE;

        if (dh_book_manager_get_group_by_language (book_manager)) {
                GtkTreeIter first_book_iter;

                book_tree_find_language_group (tree,
                                               dh_book_get_language (book),
                                               &language_iter,
                                               &language_iter_found,
                                               NULL,
                                               NULL);

                if (language_iter_found &&
                    gtk_tree_model_iter_children (GTK_TREE_MODEL (priv->store),
                                                  &first_book_iter,
                                                  &language_iter)) {
                        book_tree_find_book (tree,
                                             book,
                                             &first_book_iter,
                                             &exact_iter,
                                             &exact_iter_found,
                                             NULL,
                                             NULL);
                }
        } else {
                book_tree_find_book (tree,
                                     book,
                                     NULL,
                                     &exact_iter,
                                     &exact_iter_found,
                                     NULL,
                                     NULL);
        }

        if (exact_iter_found) {
                /* Remove the book from the tree */
                gtk_tree_store_remove (priv->store, &exact_iter);
                /* If this book was inside a language group, check if the group
                 * is now empty and so removable */
                if (language_iter_found) {
                        GtkTreeIter first_book_iter;

                        if (!gtk_tree_model_iter_children (GTK_TREE_MODEL (priv->store),
                                                           &first_book_iter,
                                                           &language_iter)) {
                                /* Oh, well, no more books in this language... remove! */
                                gtk_tree_store_remove (priv->store, &language_iter);
                        }
                }
        }
}

static void
book_tree_insert_node (DhBookTree  *tree,
		       GNode       *node,
                       GtkTreeIter *current_iter,
                       DhBook      *book)

{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
	DhLink         *link;
        PangoWeight     weight;
	GNode          *child;

	link = node->data;

	if (dh_link_get_link_type (link) == DH_LINK_TYPE_BOOK) {
                weight = PANGO_WEIGHT_BOLD;
	} else {
                weight = PANGO_WEIGHT_NORMAL;
        }

        gtk_tree_store_set (priv->store,
                            current_iter,
                            COL_TITLE, dh_link_get_name (link),
                            COL_LINK, link,
                            COL_BOOK, book,
                            COL_WEIGHT, weight,
                            COL_UNDERLINE, PANGO_UNDERLINE_NONE,
                            -1);

	for (child = g_node_first_child (node);
	     child;
	     child = g_node_next_sibling (child)) {
                GtkTreeIter iter;

                /* Append new iter */
                gtk_tree_store_append (priv->store, &iter, current_iter);
		book_tree_insert_node (tree, child, &iter, NULL);
	}
}

static void
book_tree_selection_changed_cb (GtkTreeSelection *selection,
				DhBookTree       *tree)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        GtkTreeIter     iter;

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
                DhLink *link;

		gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
				    &iter,
                                    COL_LINK, &link,
                                    -1);
                if (link) {
                        if (link != priv->selected_link) {
                                g_signal_emit (tree, signals[LINK_SELECTED], 0, link);
                        }
                        priv->selected_link = link;
                }
	}
}

static void
book_tree_group_by_language_cb (GObject    *object,
                                GParamSpec *pspec,
                                gpointer    user_data)
{
        book_tree_populate_tree (DH_BOOK_TREE (user_data));
}


static void
book_tree_init_selection (DhBookTree *tree)
{
        DhBookTreePrivate   *priv;
        GtkTreeSelection *selection;
        GtkTreeIter       iter;
        gboolean          iter_found = FALSE;
        DhLink           *link;

        priv = dh_book_tree_get_instance_private (tree);

	/* Mark the first item as selected, or it would get automatically
	 * selected when the treeview will get focus; but that's not even
	 * enough as a selection changed would still be emitted when there
	 * is no change, hence the manual tracking of selection in
	 * selected_link.
	 *   https://bugzilla.gnome.org/show_bug.cgi?id=492206
	 */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	g_signal_handlers_block_by_func	(selection,
					 book_tree_selection_changed_cb,
					 tree);

        /* If grouping by languages, get first book in the first language */
        if (dh_book_manager_get_group_by_language (priv->book_manager)) {
                GtkTreeIter language_iter;

                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store),
                                                   &language_iter)) {
                        iter_found = gtk_tree_model_iter_children (GTK_TREE_MODEL (priv->store),
                                                                   &iter,
                                                                   &language_iter);
                }
        } else {
                iter_found = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store),
                                                            &iter);
        }

        if (iter_found) {
                gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
                                    &iter,
                                    COL_LINK, &link,
                                    -1);
                priv->selected_link = link;
                gtk_tree_selection_select_iter (selection, &iter);
        }

	g_signal_handlers_unblock_by_func (selection,
					   book_tree_selection_changed_cb,
					   tree);
}

static void
dh_book_tree_constructed (GObject *object)
{
        DhBookTree *tree = DH_BOOK_TREE (object);
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);

        priv->book_created_id = g_signal_connect (priv->book_manager,
                                                  "book-created",
                                                  G_CALLBACK (book_tree_book_created_or_enabled_cb),
                                                  tree);
        priv->book_deleted_id = g_signal_connect (priv->book_manager,
                                                  "book-deleted",
                                                  G_CALLBACK (book_tree_book_deleted_or_disabled_cb),
                                                  tree);
        priv->book_enabled_id = g_signal_connect (priv->book_manager,
                                                  "book-enabled",
                                                  G_CALLBACK (book_tree_book_created_or_enabled_cb),
                                                  tree);
        priv->book_disabled_id = g_signal_connect (priv->book_manager,
                                                   "book-disabled",
                                                   G_CALLBACK (book_tree_book_deleted_or_disabled_cb),
                                                   tree);
        priv->group_by_language_id = g_signal_connect (priv->book_manager,
                                                       "notify::group-by-language",
                                                       G_CALLBACK (book_tree_group_by_language_cb),
                                                       tree);

        book_tree_populate_tree (tree);

        book_tree_init_selection (tree);

        G_OBJECT_CLASS (dh_book_tree_parent_class)->constructed (object);
}

static void
dh_book_tree_class_init (DhBookTreeClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_book_tree_dispose;
        object_class->get_property = dh_book_tree_get_property;
        object_class->set_property = dh_book_tree_set_property;
        object_class->constructed = dh_book_tree_constructed;

        g_object_class_install_property (object_class,
                                         PROP_BOOK_MANAGER,
                                         g_param_spec_object ("book-manager",
                                                              "Book Manager",
                                                              "The book maanger",
                                                              DH_TYPE_BOOK_MANAGER,
                                                              G_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT_ONLY));

        signals[LINK_SELECTED] =
                g_signal_new ("link-selected",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);
}

GtkWidget *
dh_book_tree_new (DhBookManager *book_manager)
{
        return GTK_WIDGET (g_object_new (DH_TYPE_BOOK_TREE, "book-manager", book_manager, NULL));
}

static gboolean
book_tree_find_uri_foreach (GtkTreeModel *model,
			    GtkTreePath  *path,
			    GtkTreeIter  *iter,
			    FindURIData  *data)
{
	DhLink *link;

	gtk_tree_model_get (model, iter,
			    COL_LINK, &link,
			    -1);
        if (link) {
                gchar *link_uri;

                link_uri = dh_link_get_uri (link);
                if (g_str_has_prefix (data->uri, link_uri)) {
                        data->found = TRUE;
                        data->iter = *iter;
                        data->path = gtk_tree_path_copy (path);
                }
                g_free (link_uri);
        }

	return data->found;
}

void
dh_book_tree_select_uri (DhBookTree  *tree,
			 const gchar *uri)
{
        DhBookTreePrivate   *priv = dh_book_tree_get_instance_private (tree);
	GtkTreeSelection *selection;
	FindURIData       data;

	data.found = FALSE;
	data.uri = uri;

	gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
				(GtkTreeModelForeachFunc) book_tree_find_uri_foreach,
				&data);

	if (!data.found) {
		return;
	}

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

        /* Do not re-select (which will expand current additionally) if already
         * there. */
        if (gtk_tree_selection_iter_is_selected (selection, &data.iter))
                return;

	g_signal_handlers_block_by_func	(selection,
					 book_tree_selection_changed_cb,
					 tree);

	gtk_tree_view_expand_to_path (GTK_TREE_VIEW (tree), data.path);
	gtk_tree_selection_select_iter (selection, &data.iter);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree), data.path, NULL, 0);

	g_signal_handlers_unblock_by_func (selection,
					   book_tree_selection_changed_cb,
					   tree);

	gtk_tree_path_free (data.path);
}

DhLink *
dh_book_tree_get_selected_book (DhBookTree *tree)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GtkTreePath      *path;
	DhLink           *link;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		return NULL;
	}

	path = gtk_tree_model_get_path (model, &iter);

	/* Get the book node for this link. */
	while (1) {
		if (gtk_tree_path_get_depth (path) <= 1) {
			break;
		}

		gtk_tree_path_up (path);
	}

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);

	gtk_tree_model_get (model, &iter,
			    COL_LINK, &link,
			    -1);

	return link;
}
