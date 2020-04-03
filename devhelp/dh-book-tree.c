/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2001-2003 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2003 CodeFactory AB
 * SPDX-FileCopyrightText: 2008 Imendio AB
 * SPDX-FileCopyrightText: 2010 Lanedo GmbH
 * SPDX-FileCopyrightText: 2015, 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"
#include "dh-book-tree.h"
#include <glib/gi18n-lib.h>
#include "dh-book.h"
#include "dh-book-list.h"
#include "dh-settings.h"

/**
 * SECTION:dh-book-tree
 * @Title: DhBookTree
 * @Short_description: A #GtkTreeView containing the tree structure of a
 * #DhBookList
 *
 * #DhBookTree is a #GtkTreeView (showing a tree, not a list) containing the
 * general tree structure of the #DhBook's contained in a #DhBookList (the
 * #DhBookList part of the provided #DhProfile).
 *
 * #DhBookTree calls the dh_book_get_tree() function to get the tree structure
 * of a #DhBook. As such the tree contains only #DhLink's of type
 * %DH_LINK_TYPE_BOOK or %DH_LINK_TYPE_PAGE.
 *
 * When an element is selected, the #DhBookTree::link-selected signal is
 * emitted. Only one element can be selected at a time.
 */

typedef struct {
        DhProfile *profile;
        GtkTreeStore *store;
        DhLink *selected_link;
        GtkMenu *context_menu;
} DhBookTreePrivate;

typedef struct {
        const gchar *uri;
        GtkTreeIter iter;
        GtkTreePath *path;
        guint found : 1;
} FindURIData;

enum {
        LINK_SELECTED,
        N_SIGNALS
};

enum {
        PROP_0,
        PROP_PROFILE,
        N_PROPERTIES
};

enum {
        COL_TITLE,
        COL_LINK,
        COL_BOOK,
        COL_WEIGHT,
        COL_UNDERLINE,
        N_COLUMNS
};

G_DEFINE_TYPE_WITH_PRIVATE (DhBookTree, dh_book_tree, GTK_TYPE_TREE_VIEW);

static guint signals[N_SIGNALS] = { 0 };
static GParamSpec *properties[N_PROPERTIES];

static gboolean
get_group_books_by_language (DhBookTree *tree)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        DhSettings *settings;

        settings = dh_profile_get_settings (priv->profile);
        return dh_settings_get_group_books_by_language (settings);
}

static void
book_tree_selection_changed_cb (GtkTreeSelection *selection,
                                DhBookTree       *tree)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        DhLink *link;

        link = dh_book_tree_get_selected_link (tree);

        if (link != NULL &&
            link != priv->selected_link) {
                if (priv->selected_link != NULL)
                        dh_link_unref (priv->selected_link);

                priv->selected_link = dh_link_ref (link);
                g_signal_emit (tree, signals[LINK_SELECTED], 0, link);
        }

        if (link != NULL)
                dh_link_unref (link);
}

static void
book_tree_setup_selection (DhBookTree *tree)
{
        GtkTreeSelection *selection;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

        gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

        g_signal_connect_object (selection,
                                 "changed",
                                 G_CALLBACK (book_tree_selection_changed_cb),
                                 tree,
                                 0);
}

/* Tries to find:
 *  - An exact match of the language group
 *  - Or the language group which should be just after our given language group.
 *  - Or both.
 *
 * FIXME: not great code. Maybe have a DhLanguage object, and add a new column
 * in the GtkTreeModel storing a DhLanguage object instead of a string.
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
        GtkTreeIter loop_iter;

        g_assert ((exact_iter != NULL && exact_found != NULL) ||
                  (next_iter != NULL && next_found != NULL));

        /* Reset all flags to not found */
        if (exact_found != NULL)
                *exact_found = FALSE;
        if (next_found != NULL)
                *next_found = FALSE;

        /* If we're not doing language grouping, return not found */
        if (!get_group_books_by_language (tree))
                return;

        if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store),
                                            &loop_iter)) {
                /* Store is empty, not found */
                return;
        }

        do {
                gchar *title = NULL;
                DhLink *link;

                /* Look for language titles, which are those where there
                 * is no book object associated in the row */
                gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
                                    &loop_iter,
                                    COL_TITLE, &title,
                                    COL_LINK, &link,
                                    -1);

                if (link != NULL) {
                        /* Not a language */
                        g_free (title);
                        dh_link_unref (link);
                        g_return_if_reached ();
                }

                if (exact_iter != NULL &&
                    g_ascii_strcasecmp (title, language) == 0) {
                        /* Exact match found! */
                        *exact_iter = loop_iter;
                        *exact_found = TRUE;
                        if (next_iter == NULL) {
                                /* If we were not requested to look for the next one, end here */
                                g_free (title);
                                return;
                        }
                } else if (next_iter != NULL &&
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
 *  - Or the book which should be just after our given book
 *  - Or both.
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
        GtkTreeIter loop_iter;

        g_assert ((exact_iter != NULL && exact_found != NULL) ||
                  (next_iter != NULL && next_found != NULL));

        /* Reset all flags to not found */
        if (exact_found != NULL)
                *exact_found = FALSE;
        if (next_found != NULL)
                *next_found = FALSE;

        /* Setup iteration start */
        if (first == NULL) {
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

                g_return_if_fail (DH_IS_BOOK (in_tree_book));

                /* We can compare pointers directly as we're playing with references
                 * of the same object */
                if (exact_iter != NULL &&
                    in_tree_book == book) {
                        *exact_iter = loop_iter;
                        *exact_found = TRUE;
                        if (next_iter == NULL) {
                                /* If we were not requested to look for the next one, end here */
                                g_object_unref (in_tree_book);
                                return;
                        }
                } else if (next_iter != NULL &&
                           dh_book_cmp_by_title (in_tree_book, book) > 0) {
                        *next_iter = loop_iter;
                        *next_found = TRUE;
                        g_object_unref (in_tree_book);
                        return;
                }

                g_object_unref (in_tree_book);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store),
                                           &loop_iter));
}

static void
book_tree_insert_node (DhBookTree  *tree,
                       GNode       *node,
                       GtkTreeIter *current_iter,
                       DhBook      *book)

{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        DhLink *link;
        PangoWeight weight;
        GNode *child;

        link = node->data;
        g_assert (link != NULL);

        if (dh_link_get_link_type (link) == DH_LINK_TYPE_BOOK)
                weight = PANGO_WEIGHT_BOLD;
        else
                weight = PANGO_WEIGHT_NORMAL;

        gtk_tree_store_set (priv->store,
                            current_iter,
                            COL_TITLE, dh_link_get_name (link),
                            COL_LINK, link,
                            COL_BOOK, book,
                            COL_WEIGHT, weight,
                            COL_UNDERLINE, PANGO_UNDERLINE_NONE,
                            -1);

        for (child = g_node_first_child (node);
             child != NULL;
             child = g_node_next_sibling (child)) {
                GtkTreeIter iter;

                /* Append new iter */
                gtk_tree_store_append (priv->store, &iter, current_iter);
                book_tree_insert_node (tree, child, &iter, NULL);
        }
}

static void
book_tree_add_book_to_store (DhBookTree *tree,
                             DhBook     *book)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        GtkTreeIter book_iter;

        /* If grouping by language we need to add the language categories */
        if (get_group_books_by_language (tree)) {
                GtkTreeIter language_iter;
                gboolean language_iter_found;
                GtkTreeIter next_language_iter;
                gboolean next_language_iter_found;
                const gchar *language_title;
                gboolean new_language = FALSE;

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
                                            COL_TITLE, language_title,
                                            COL_LINK, NULL,
                                            COL_BOOK, NULL,
                                            COL_WEIGHT, PANGO_WEIGHT_BOLD,
                                            COL_UNDERLINE, PANGO_UNDERLINE_SINGLE,
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
                        gboolean next_book_iter_found;

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
                gboolean next_book_iter_found;

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
add_book_cb (DhBookList *book_list,
             DhBook     *book,
             DhBookTree *tree)
{
        book_tree_add_book_to_store (tree, book);
}

static void
remove_book_cb (DhBookList *book_list,
                DhBook     *book,
                DhBookTree *tree)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        GtkTreeIter exact_iter;
        gboolean exact_iter_found = FALSE;
        GtkTreeIter language_iter;
        gboolean language_iter_found = FALSE;

        if (get_group_books_by_language (tree)) {
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
book_tree_init_selection (DhBookTree *tree)
{
        DhBookTreePrivate *priv;
        GtkTreeSelection *selection;
        GtkTreeIter iter;
        gboolean iter_found = FALSE;

        priv = dh_book_tree_get_instance_private (tree);

        /* Mark the first item as selected, or it would get automatically
         * selected when the treeview will get focus (a behavior that we want to
         * avoid); but that's not even enough as a selection ::changed would
         * still be emitted when there is no change, hence the manual tracking
         * of selection with priv->selected_link.
         *
         * If there is no manual tracking with selected_link, there is this bug:
         * 1. Open Devhelp.
         * 2. The first book is initially selected (thanks to this function),
         *    OK.
         * 3. Click on the arrow of another book to expand it.
         * --> The other book gets correctly expanded (and not selected), but
         *     the selection ::changed signal is emitted for the *first* book
         *     (even though it was already selected, strange).
         *
         * https://bugzilla.gnome.org/show_bug.cgi?id=492206 - GtkTreeView bug
         * https://bugzilla.gnome.org/show_bug.cgi?id=603040 - Devhelp bug
         */
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
        g_signal_handlers_block_by_func (selection,
                                         book_tree_selection_changed_cb,
                                         tree);

        /* If grouping by languages, get first book in the first language */
        if (get_group_books_by_language (tree)) {
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
                DhLink *link;

                gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
                                    &iter,
                                    COL_LINK, &link,
                                    -1);

                if (link == NULL || dh_link_get_link_type (link) != DH_LINK_TYPE_BOOK)
                        g_warn_if_reached ();

                if (priv->selected_link != NULL)
                        dh_link_unref (priv->selected_link);

                priv->selected_link = link;
                gtk_tree_selection_select_iter (selection, &iter);
        }

        g_signal_handlers_unblock_by_func (selection,
                                           book_tree_selection_changed_cb,
                                           tree);
}

static void
book_tree_populate_tree (DhBookTree *tree)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        GList *books;
        GList *l;

        gtk_tree_view_set_model (GTK_TREE_VIEW (tree), NULL);
        gtk_tree_store_clear (priv->store);
        gtk_tree_view_set_model (GTK_TREE_VIEW (tree),
                                 GTK_TREE_MODEL (priv->store));

        books = dh_book_list_get_books (dh_profile_get_book_list (priv->profile));

        for (l = books; l != NULL; l = l->next) {
                DhBook *book = DH_BOOK (l->data);
                book_tree_add_book_to_store (tree, book);
        }

        book_tree_init_selection (tree);
}

static void
group_books_by_language_notify_cb (DhSettings *settings,
                                   GParamSpec *pspec,
                                   DhBookTree *tree)
{
        book_tree_populate_tree (tree);
}

static void
set_profile (DhBookTree *tree,
             DhProfile  *profile)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);

        g_return_if_fail (profile == NULL || DH_IS_PROFILE (profile));

        g_assert (priv->profile == NULL);
        g_set_object (&priv->profile, profile);
}

static void
dh_book_tree_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
        DhBookTree *tree = DH_BOOK_TREE (object);

        switch (prop_id) {
                case PROP_PROFILE:
                        g_value_set_object (value, dh_book_tree_get_profile (tree));
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
        DhBookTree *tree = DH_BOOK_TREE (object);

        switch (prop_id) {
                case PROP_PROFILE:
                        set_profile (tree, g_value_get_object (value));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_book_tree_constructed (GObject *object)
{
        DhBookTree *tree = DH_BOOK_TREE (object);
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);
        DhBookList *book_list;
        DhSettings *settings;

        if (G_OBJECT_CLASS (dh_book_tree_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (dh_book_tree_parent_class)->constructed (object);

        if (priv->profile == NULL)
                priv->profile = g_object_ref (dh_profile_get_default ());

        book_tree_setup_selection (tree);

        book_list = dh_profile_get_book_list (priv->profile);

        g_signal_connect_object (book_list,
                                 "add-book",
                                 G_CALLBACK (add_book_cb),
                                 tree,
                                 G_CONNECT_AFTER);

        g_signal_connect_object (book_list,
                                 "remove-book",
                                 G_CALLBACK (remove_book_cb),
                                 tree,
                                 G_CONNECT_AFTER);

        settings = dh_profile_get_settings (priv->profile);
        g_signal_connect_object (settings,
                                 "notify::group-books-by-language",
                                 G_CALLBACK (group_books_by_language_notify_cb),
                                 tree,
                                 0);

        book_tree_populate_tree (tree);
}

static void
dh_book_tree_dispose (GObject *object)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (DH_BOOK_TREE (object));

        g_clear_object (&priv->profile);
        g_clear_object (&priv->store);
        priv->context_menu = NULL;

        if (priv->selected_link != NULL) {
                dh_link_unref (priv->selected_link);
                priv->selected_link = NULL;
        }

        G_OBJECT_CLASS (dh_book_tree_parent_class)->dispose (object);
}

static void
collapse_all_activate_cb (GtkMenuItem *menu_item,
                          DhBookTree  *tree)
{
        gtk_tree_view_collapse_all (GTK_TREE_VIEW (tree));
}

static void
do_popup_menu (DhBookTree     *tree,
               GdkEventButton *event)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);

        if (priv->context_menu == NULL) {
                GtkWidget *menu_item;

                /* Create the menu only once. At first I wanted to create a new
                 * menu each time this function is called, connect to the
                 * GtkMenuShell::deactivate signal to call gtk_widget_destroy().
                 * But GtkMenuShell::deactivate is emitted before
                 * collapse_all_activate_cb(), so collapse_all_activate_cb() was
                 * never called... It's maybe a GTK bug.
                 */
                priv->context_menu = GTK_MENU (gtk_menu_new ());

                /* When tree is destroyed, the context menu is destroyed too. */
                gtk_menu_attach_to_widget (priv->context_menu, GTK_WIDGET (tree), NULL);

                menu_item = gtk_menu_item_new_with_mnemonic (_("_Collapse All"));
                gtk_menu_shell_append (GTK_MENU_SHELL (priv->context_menu), menu_item);
                gtk_widget_show (menu_item);

                g_signal_connect_object (menu_item,
                                         "activate",
                                         G_CALLBACK (collapse_all_activate_cb),
                                         tree,
                                         0);
        }

        if (event != NULL) {
                gtk_menu_popup_at_pointer (priv->context_menu, (GdkEvent *) event);
        } else {
                gtk_menu_popup_at_widget (priv->context_menu,
                                          GTK_WIDGET (tree),
                                          GDK_GRAVITY_NORTH_EAST,
                                          GDK_GRAVITY_NORTH_WEST,
                                          NULL);
        }
}

static gboolean
dh_book_tree_button_press_event (GtkWidget      *widget,
                                 GdkEventButton *event)
{
        DhBookTree *tree = DH_BOOK_TREE (widget);

        if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
            event->type == GDK_BUTTON_PRESS) {
                do_popup_menu (tree, event);
                return GDK_EVENT_STOP;
        }

        if (GTK_WIDGET_CLASS (dh_book_tree_parent_class)->button_press_event != NULL)
                return GTK_WIDGET_CLASS (dh_book_tree_parent_class)->button_press_event (widget, event);

        return GDK_EVENT_PROPAGATE;
}

static gboolean
dh_book_tree_popup_menu (GtkWidget *widget)
{
        if (GTK_WIDGET_CLASS (dh_book_tree_parent_class)->popup_menu != NULL)
                g_warning ("%s(): chain-up?", G_STRFUNC);

        do_popup_menu (DH_BOOK_TREE (widget), NULL);
        return TRUE;
}

static void
dh_book_tree_class_init (DhBookTreeClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        object_class->get_property = dh_book_tree_get_property;
        object_class->set_property = dh_book_tree_set_property;
        object_class->constructed = dh_book_tree_constructed;
        object_class->dispose = dh_book_tree_dispose;

        widget_class->button_press_event = dh_book_tree_button_press_event;
        widget_class->popup_menu = dh_book_tree_popup_menu;

        /**
         * DhBookTree::link-selected:
         * @tree: the #DhBookTree.
         * @link: the selected #DhLink.
         */
        signals[LINK_SELECTED] =
                g_signal_new ("link-selected",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1, DH_TYPE_LINK);

        /**
         * DhBookTree:profile:
         *
         * The #DhProfile. If set to %NULL, the default profile as returned by
         * dh_profile_get_default() is used.
         *
         * Since: 3.30
         */
        properties[PROP_PROFILE] =
                g_param_spec_object ("profile",
                                     "Profile",
                                     "",
                                     DH_TYPE_PROFILE,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
book_tree_add_columns (DhBookTree *tree)
{
        GtkCellRenderer *cell;
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
dh_book_tree_init (DhBookTree *tree)
{
        DhBookTreePrivate *priv = dh_book_tree_get_instance_private (tree);

        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
        gtk_tree_view_set_enable_search (GTK_TREE_VIEW (tree), FALSE);

        priv->store = gtk_tree_store_new (N_COLUMNS,
                                          G_TYPE_STRING, /* Title */
                                          DH_TYPE_LINK,
                                          DH_TYPE_BOOK,
                                          PANGO_TYPE_WEIGHT,
                                          PANGO_TYPE_UNDERLINE);

        gtk_tree_view_set_model (GTK_TREE_VIEW (tree),
                                 GTK_TREE_MODEL (priv->store));

        book_tree_add_columns (tree);
}

/**
 * dh_book_tree_new:
 * @profile: (nullable): a #DhProfile, or %NULL for the default profile.
 *
 * Returns: (transfer floating): a new #DhBookTree widget.
 */
DhBookTree *
dh_book_tree_new (DhProfile *profile)
{
        g_return_val_if_fail (profile == NULL || DH_IS_PROFILE (profile), NULL);

        return g_object_new (DH_TYPE_BOOK_TREE,
                             "profile", profile,
                             NULL);
}

/**
 * dh_book_tree_get_profile:
 * @tree: a #DhBookTree.
 *
 * Returns: (transfer none): the #DhProfile of @tree.
 * Since: 3.30
 */
DhProfile *
dh_book_tree_get_profile (DhBookTree *tree)
{
        DhBookTreePrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK_TREE (tree), NULL);

        priv = dh_book_tree_get_instance_private (tree);
        return priv->profile;
}

/**
 * dh_book_tree_get_selected_link:
 * @tree: a #DhBookTree.
 *
 * Returns: (transfer full) (nullable): the currently selected #DhLink in @tree,
 * or %NULL if the selection is empty or if a language group row is selected.
 * Unref with dh_link_unref() when no longer needed.
 * Since: 3.30
 */
DhLink *
dh_book_tree_get_selected_link (DhBookTree *tree)
{
        GtkTreeSelection *selection;
        GtkTreeModel *model;
        GtkTreeIter iter;
        DhLink *link;

        g_return_val_if_fail (DH_IS_BOOK_TREE (tree), NULL);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
        if (!gtk_tree_selection_get_selected (selection, &model, &iter))
                return NULL;

        gtk_tree_model_get (model, &iter,
                            COL_LINK, &link,
                            -1);

        return link;
}

static gboolean
book_tree_find_uri_foreach_func (GtkTreeModel *model,
                                 GtkTreePath  *path,
                                 GtkTreeIter  *iter,
                                 gpointer      _data)
{
        FindURIData *data = _data;
        DhLink *link;

        gtk_tree_model_get (model, iter,
                            COL_LINK, &link,
                            -1);

        if (link != NULL) {
                gchar *link_uri;

                link_uri = dh_link_get_uri (link);

                if (link_uri != NULL &&
                    g_str_has_prefix (data->uri, link_uri)) {
                        data->found = TRUE;
                        data->iter = *iter;
                        data->path = gtk_tree_path_copy (path);
                }

                g_free (link_uri);
                dh_link_unref (link);
        }

        return data->found;
}

/**
 * dh_book_tree_select_uri:
 * @tree: a #DhBookTree.
 * @uri: the URI to select.
 *
 * Selects the row corresponding to @uri. It searches in the tree a #DhLink
 * being at @uri (if it's an exact match), or containing @uri (if @uri contains
 * an anchor).
 */
void
dh_book_tree_select_uri (DhBookTree  *tree,
                         const gchar *uri)
{
        DhBookTreePrivate *priv;
        GtkTreeSelection *selection;
        FindURIData data;

        g_return_if_fail (DH_IS_BOOK_TREE (tree));
        g_return_if_fail (uri != NULL);

        priv = dh_book_tree_get_instance_private (tree);

        data.found = FALSE;
        data.uri = uri;

        gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
                                book_tree_find_uri_foreach_func,
                                &data);

        if (!data.found)
                return;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

        /* Do not re-expand/select/scroll if already there. */
        if (gtk_tree_selection_iter_is_selected (selection, &data.iter))
                goto out;

        /* The order is important here: select_iter() doesn't work if the row is
         * hidden.
         */
        gtk_tree_view_expand_to_path (GTK_TREE_VIEW (tree), data.path);
        gtk_tree_selection_select_iter (selection, &data.iter);

        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tree),
                                      data.path, NULL,
                                      FALSE, 0.0, 0.0);

out:
        gtk_tree_path_free (data.path);
}
