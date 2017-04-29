/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2005-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
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
#include "dh-sidebar.h"

#include <string.h>

#include "dh-keyword-model.h"
#include "dh-book.h"
#include "dh-book-tree.h"

typedef struct {
        DhBookManager           *book_manager;

        DhBookTree              *book_tree;
        GtkScrolledWindow       *sw_book_tree;

        GtkEntry                *entry;
        DhKeywordModel          *hitlist_model;
        GtkTreeView             *hitlist_view;
        GtkScrolledWindow       *sw_hitlist;

        GCompletion             *completion;
        guint                    idle_complete_id;
        guint                    idle_filter_id;
} DhSidebarPrivate;

enum {
        PROP_0,
        PROP_BOOK_MANAGER
};

enum {
        LINK_SELECTED,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (DhSidebar, dh_sidebar, GTK_TYPE_BOX)

/******************************************************************************/

static gboolean
sidebar_filter_idle_cb (DhSidebar *sidebar)
{
        DhSidebarPrivate *priv;
        const gchar *search_text;
        const gchar *book_id;
        DhLink      *book_link;
        DhLink      *link;

        priv = dh_sidebar_get_instance_private (sidebar);

        priv->idle_filter_id = 0;

        search_text = gtk_entry_get_text (priv->entry);

        book_link = dh_sidebar_get_selected_book (sidebar);
        book_id = book_link != NULL ? dh_link_get_book_id (book_link) : NULL;

        /* Disconnect the model during the filter, for:
         * 1. better performances.
         * 2. clearing the selection.
         */
        gtk_tree_view_set_model (priv->hitlist_view, NULL);

        link = dh_keyword_model_filter (priv->hitlist_model,
                                        search_text,
                                        book_id,
                                        NULL);

        gtk_tree_view_set_model (priv->hitlist_view,
                                 GTK_TREE_MODEL (priv->hitlist_model));

        if (link != NULL)
                g_signal_emit (sidebar, signals[LINK_SELECTED], 0, link);

        return G_SOURCE_REMOVE;
}

static void
sidebar_search_run_idle (DhSidebar *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);

        if (priv->idle_filter_id == 0)
                priv->idle_filter_id =
                        g_idle_add ((GSourceFunc) sidebar_filter_idle_cb, sidebar);
}

/******************************************************************************/

static void
sidebar_completion_add_book (DhSidebar *sidebar,
                             DhBook    *book)
{
        GList *completions;
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);

        if (G_UNLIKELY (priv->completion == NULL))
                priv->completion = g_completion_new (NULL);

        completions = dh_book_get_completions (book);
        if (completions != NULL)
                g_completion_add_items (priv->completion, completions);
}

static void
sidebar_completion_delete_book (DhSidebar *sidebar,
                                DhBook    *book)
{
        GList *completions;
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);

        if (G_UNLIKELY (priv->completion == NULL))
                return;

        completions = dh_book_get_completions (book);
        if (completions != NULL)
                g_completion_remove_items (priv->completion, completions);
}

static void
sidebar_book_created_or_enabled_cb (DhBookManager *book_manager,
                                    DhBook        *book,
                                    DhSidebar     *sidebar)
{
        sidebar_completion_add_book (sidebar, book);

        /* Update current search if any */
        sidebar_search_run_idle (sidebar);
}

static void
sidebar_book_deleted_or_disabled_cb (DhBookManager *book_manager,
                                     DhBook        *book,
                                     DhSidebar     *sidebar)
{
        sidebar_completion_delete_book (sidebar, book);

        /* Update current search if any */
        sidebar_search_run_idle (sidebar);
}

static void
sidebar_completion_populate (DhSidebar *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);
        GList *books;
        GList *l;

        books = dh_book_manager_get_books (priv->book_manager);

        for (l = books; l != NULL; l = l->next)
                sidebar_completion_add_book (sidebar, DH_BOOK (l->data));
}

/******************************************************************************/

static void
sidebar_hitlist_selection_changed_cb (GtkTreeSelection *selection,
                                      DhSidebar        *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);
        GtkTreeIter iter;

        if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
                DhLink *link;

                gtk_tree_model_get (GTK_TREE_MODEL (priv->hitlist_model), &iter,
                                    DH_KEYWORD_MODEL_COL_LINK, &link,
                                    -1);

                g_signal_emit (sidebar, signals[LINK_SELECTED], 0, link);
        }
}

/* Make it possible to jump back to the currently selected item, useful when the
 * html view has been scrolled away.
 */
static gboolean
sidebar_hitlist_button_press_cb (GtkTreeView    *hitlist_view,
                                 GdkEventButton *event,
                                 DhSidebar      *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);
        GtkTreePath  *path;
        GtkTreeIter   iter;
        DhLink       *link;

        gtk_tree_view_get_path_at_pos (hitlist_view, event->x, event->y, &path,
                                       NULL, NULL, NULL);
        if (path == NULL)
                return GDK_EVENT_PROPAGATE;

        gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->hitlist_model), &iter, path);
        gtk_tree_path_free (path);

        gtk_tree_model_get (GTK_TREE_MODEL (priv->hitlist_model),
                            &iter,
                            DH_KEYWORD_MODEL_COL_LINK, &link,
                            -1);

        g_signal_emit (sidebar, signals[LINK_SELECTED], 0, link);

        /* Always propagate the event so the tree view can update
         * the selection etc.
         */
        return GDK_EVENT_PROPAGATE;
}

static gboolean
sidebar_entry_key_press_event_cb (GtkEntry    *entry,
                                  GdkEventKey *event,
                                  DhSidebar   *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);

        if (event->keyval == GDK_KEY_Tab) {
                if (event->state & GDK_CONTROL_MASK) {
                        if (gtk_widget_is_visible (GTK_WIDGET (priv->hitlist_view)))
                                gtk_widget_grab_focus (GTK_WIDGET (priv->hitlist_view));
                } else {
                        gtk_editable_set_position (GTK_EDITABLE (entry), -1);
                        gtk_editable_select_region (GTK_EDITABLE (entry), -1, -1);
                }

                return GDK_EVENT_STOP;
        }

        if (event->keyval == GDK_KEY_Return ||
            event->keyval == GDK_KEY_KP_Enter) {
                GtkTreeIter  iter;
                DhLink      *link;
                gchar       *name;

                /* Get the first entry found. */
                if (gtk_widget_is_visible (GTK_WIDGET (priv->hitlist_view)) &&
                    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->hitlist_model), &iter)) {
                        gtk_tree_model_get (GTK_TREE_MODEL (priv->hitlist_model),
                                            &iter,
                                            DH_KEYWORD_MODEL_COL_LINK, &link,
                                            DH_KEYWORD_MODEL_COL_NAME, &name,
                                            -1);

                        gtk_entry_set_text (entry, name);
                        g_free (name);

                        gtk_editable_set_position (GTK_EDITABLE (entry), -1);
                        gtk_editable_select_region (GTK_EDITABLE (entry), -1, -1);

                        g_signal_emit (sidebar, signals[LINK_SELECTED], 0, link);

                        return GDK_EVENT_STOP;
                }
        }

        return GDK_EVENT_PROPAGATE;
}

static void
sidebar_entry_changed_cb (GtkEntry  *entry,
                          DhSidebar *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);
        const gchar *search_text;

        search_text = gtk_entry_get_text (entry);

        if (search_text == NULL || search_text[0] == '\0') {
                gtk_widget_hide (GTK_WIDGET (priv->sw_hitlist));
                gtk_widget_show (GTK_WIDGET (priv->sw_book_tree));
        } else {
                gtk_widget_hide (GTK_WIDGET (priv->sw_book_tree));
                gtk_widget_show (GTK_WIDGET (priv->sw_hitlist));
                sidebar_search_run_idle (sidebar);
        }
}

static gboolean
sidebar_complete_idle_cb (DhSidebar *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);
        const gchar  *search_text;
        gchar        *completed = NULL;

        search_text = gtk_entry_get_text (priv->entry);

        g_completion_complete (priv->completion, search_text, &completed);
        if (completed != NULL) {
                gsize length = strlen (search_text);

                gtk_entry_set_text (priv->entry, completed);
                gtk_editable_set_position (GTK_EDITABLE (priv->entry), length);
                gtk_editable_select_region (GTK_EDITABLE (priv->entry),
                                            length, -1);
                g_free (completed);
        }

        priv->idle_complete_id = 0;

        return G_SOURCE_REMOVE;
}

static void
sidebar_entry_insert_text_cb (GtkEntry    *entry,
                              const gchar *text,
                              gint         length,
                              gint        *position,
                              DhSidebar   *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);

        if (priv->idle_complete_id == 0)
                priv->idle_complete_id =
                        g_idle_add ((GSourceFunc) sidebar_complete_idle_cb, sidebar);
}

/**
 * dh_sidebar_set_search_string:
 * @sidebar: a #DhSidebar.
 * @str: the string to search.
 */
void
dh_sidebar_set_search_string (DhSidebar   *sidebar,
                              const gchar *str)
{
        DhSidebarPrivate *priv;

        g_return_if_fail (DH_IS_SIDEBAR (sidebar));

        priv = dh_sidebar_get_instance_private (sidebar);

        gtk_entry_set_text (priv->entry, str);
        gtk_editable_set_position (GTK_EDITABLE (priv->entry), -1);
        gtk_editable_select_region (GTK_EDITABLE (priv->entry), -1, -1);
}

/**
 * dh_sidebar_set_search_focus:
 * @sidebar: a #DhSidebar.
 *
 * Gives the focus to the search entry.
 */
void
dh_sidebar_set_search_focus (DhSidebar *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);

        gtk_widget_grab_focus (GTK_WIDGET (priv->entry));
}

static void
hitlist_cell_data_func (GtkTreeViewColumn *tree_column,
                        GtkCellRenderer   *cell,
                        GtkTreeModel      *hitlist_model,
                        GtkTreeIter       *iter,
                        gpointer           data)
{
        DhLink       *link;
        DhLinkType    link_type;
        PangoStyle    style;
        PangoWeight   weight;
        gboolean      current_book_flag;
        gchar        *name;

        gtk_tree_model_get (hitlist_model, iter,
                            DH_KEYWORD_MODEL_COL_LINK, &link,
                            DH_KEYWORD_MODEL_COL_CURRENT_BOOK_FLAG, &current_book_flag,
                            -1);

        if (dh_link_get_flags (link) & DH_LINK_FLAGS_DEPRECATED)
                style = PANGO_STYLE_ITALIC;
        else
                style = PANGO_STYLE_NORMAL;

        /* Matches on the current book are given in bold. Note that we check the
         * current book as it was given to the DhKeywordModel. Do *not* rely on
         * the current book as given by the DhSidebar, as that will change
         * whenever a hit is clicked. */
        if (current_book_flag)
                weight = PANGO_WEIGHT_BOLD;
        else
                weight = PANGO_WEIGHT_NORMAL;

        link_type = dh_link_get_link_type (link);
        if (link_type == DH_LINK_TYPE_STRUCT || link_type == DH_LINK_TYPE_PROPERTY ||
            link_type == DH_LINK_TYPE_SIGNAL) {
                name = g_markup_printf_escaped (
                                "%s <i><small><span weight=\"normal\">(%s)</span></small></i>",
                                dh_link_get_name (link),
                                dh_link_get_type_as_string (link));
        } else {
                name = g_markup_printf_escaped ("%s", dh_link_get_name (link));
        }

        g_object_set (cell,
                      "markup", name,
                      "style", style,
                      "weight", weight,
                      NULL);
        g_free (name);
}

static void
sidebar_book_tree_link_selected_cb (DhBookTree *book_tree,
                                    DhLink     *link,
                                    DhSidebar  *sidebar)
{
        g_signal_emit (sidebar, signals[LINK_SELECTED], 0, link);
}

/**
 * dh_sidebar_get_selected_book:
 * @sidebar: a #DhSidebar.
 *
 * Returns: (nullable) (transfer none): the #DhLink of the selected book, or
 * %NULL if there is no selection.
 */
DhLink *
dh_sidebar_get_selected_book (DhSidebar *sidebar)
{
        DhSidebarPrivate *priv;

        g_return_val_if_fail (DH_IS_SIDEBAR (sidebar), NULL);

        priv = dh_sidebar_get_instance_private (sidebar);

        return dh_book_tree_get_selected_book (priv->book_tree);
}

/**
 * dh_sidebar_select_uri:
 * @sidebar: a #DhSidebar.
 * @uri: the URI to select.
 */
void
dh_sidebar_select_uri (DhSidebar   *sidebar,
                       const gchar *uri)
{
        DhSidebarPrivate *priv;

        g_return_if_fail (DH_IS_SIDEBAR (sidebar));

        priv = dh_sidebar_get_instance_private (sidebar);

        dh_book_tree_select_uri (priv->book_tree, uri);
}

/**
 * dh_sidebar_new:
 * @book_manager: a #DhBookManager.
 *
 * Returns: (transfer floating): a new #DhSidebar widget.
 */
GtkWidget *
dh_sidebar_new (DhBookManager *book_manager)
{
        return GTK_WIDGET (g_object_new (DH_TYPE_SIDEBAR,
                                         "orientation", GTK_ORIENTATION_VERTICAL,
                                         "book-manager", book_manager,
                                         NULL));
}

static void
dh_sidebar_dispose (GObject *object)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (DH_SIDEBAR (object));

        g_clear_object (&priv->book_manager);
        g_clear_object (&priv->hitlist_model);

        if (priv->idle_complete_id != 0) {
                g_source_remove (priv->idle_complete_id);
                priv->idle_complete_id = 0;
        }

        if (priv->idle_filter_id != 0) {
                g_source_remove (priv->idle_filter_id);
                priv->idle_filter_id = 0;
        }

        G_OBJECT_CLASS (dh_sidebar_parent_class)->dispose (object);
}

static void
dh_sidebar_finalize (GObject *object)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (DH_SIDEBAR (object));

        g_clear_pointer (&priv->completion, g_completion_free);

        G_OBJECT_CLASS (dh_sidebar_parent_class)->finalize (object);
}

static void
dh_sidebar_init (DhSidebar *sidebar)
{
        DhSidebarPrivate *priv;
        GtkCellRenderer  *cell;

        priv = dh_sidebar_get_instance_private (sidebar);

        /* Setup the search entry */
        priv->entry = GTK_ENTRY (gtk_search_entry_new ());
        gtk_widget_set_hexpand (GTK_WIDGET (priv->entry), TRUE);
        g_object_set (priv->entry,
                      "margin", 6,
                      NULL);
        gtk_box_pack_start (GTK_BOX (sidebar), GTK_WIDGET (priv->entry), FALSE, FALSE, 0);

        g_signal_connect (priv->entry, "key-press-event",
                          G_CALLBACK (sidebar_entry_key_press_event_cb),
                          sidebar);
        g_signal_connect (priv->entry, "changed",
                          G_CALLBACK (sidebar_entry_changed_cb),
                          sidebar);
        g_signal_connect (priv->entry, "insert-text",
                          G_CALLBACK (sidebar_entry_insert_text_cb),
                          sidebar);

        /* Setup hitlist */
        priv->hitlist_model = dh_keyword_model_new ();
        priv->hitlist_view = GTK_TREE_VIEW (gtk_tree_view_new ());
        gtk_tree_view_set_model (priv->hitlist_view, GTK_TREE_MODEL (priv->hitlist_model));
        gtk_tree_view_set_headers_visible (priv->hitlist_view, FALSE);
        gtk_tree_view_set_enable_search (priv->hitlist_view, FALSE);
        gtk_widget_show (GTK_WIDGET (priv->hitlist_view));

        g_signal_connect (priv->hitlist_view, "button-press-event",
                          G_CALLBACK (sidebar_hitlist_button_press_cb),
                          sidebar);

        g_signal_connect (gtk_tree_view_get_selection (priv->hitlist_view),
                          "changed",
                          G_CALLBACK (sidebar_hitlist_selection_changed_cb),
                          sidebar);

        cell = gtk_cell_renderer_text_new ();
        g_object_set (cell,
                      "ellipsize", PANGO_ELLIPSIZE_END,
                      NULL);
        gtk_tree_view_insert_column_with_data_func (priv->hitlist_view,
                                                    -1,
                                                    NULL,
                                                    cell,
                                                    hitlist_cell_data_func,
                                                    sidebar,
                                                    NULL);

        /* Hitlist packing */
        priv->sw_hitlist = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
        gtk_widget_set_no_show_all (GTK_WIDGET (priv->sw_hitlist), TRUE);
        gtk_scrolled_window_set_policy (priv->sw_hitlist,
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_container_add (GTK_CONTAINER (priv->sw_hitlist),
                           GTK_WIDGET (priv->hitlist_view));
        gtk_box_pack_start (GTK_BOX (sidebar), GTK_WIDGET (priv->sw_hitlist), TRUE, TRUE, 0);

        /* Setup the book tree */
        priv->sw_book_tree = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
        gtk_widget_show (GTK_WIDGET (priv->sw_book_tree));
        gtk_widget_set_no_show_all (GTK_WIDGET (priv->sw_book_tree), TRUE);
        gtk_scrolled_window_set_policy (priv->sw_book_tree,
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);

        gtk_widget_show_all (GTK_WIDGET (sidebar));
}

static void
dh_sidebar_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (DH_SIDEBAR (object));

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
dh_sidebar_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (DH_SIDEBAR (object));

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
dh_sidebar_constructed (GObject *object)
{
        DhSidebar *sidebar = DH_SIDEBAR (object);
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);

        /* Setup book manager */
        g_signal_connect_object (priv->book_manager,
                                 "book-created",
                                 G_CALLBACK (sidebar_book_created_or_enabled_cb),
                                 sidebar,
                                 0);

        g_signal_connect_object (priv->book_manager,
                                 "book-enabled",
                                 G_CALLBACK (sidebar_book_created_or_enabled_cb),
                                 sidebar,
                                 0);

        g_signal_connect_object (priv->book_manager,
                                 "book-deleted",
                                 G_CALLBACK (sidebar_book_deleted_or_disabled_cb),
                                 sidebar,
                                 0);

        g_signal_connect_object (priv->book_manager,
                                 "book-disabled",
                                 G_CALLBACK (sidebar_book_deleted_or_disabled_cb),
                                 sidebar,
                                 0);

        priv->book_tree = DH_BOOK_TREE (dh_book_tree_new (priv->book_manager));
        gtk_widget_show (GTK_WIDGET (priv->book_tree));
        g_signal_connect (priv->book_tree,
                          "link-selected",
                          G_CALLBACK (sidebar_book_tree_link_selected_cb),
                          sidebar);
        gtk_container_add (GTK_CONTAINER (priv->sw_book_tree), GTK_WIDGET (priv->book_tree));
        gtk_box_pack_end (GTK_BOX (sidebar), GTK_WIDGET (priv->sw_book_tree), TRUE, TRUE, 0);

        sidebar_completion_populate (sidebar);

        dh_keyword_model_set_words (priv->hitlist_model, priv->book_manager);

        G_OBJECT_CLASS (dh_sidebar_parent_class)->constructed (object);
}

static void
dh_sidebar_class_init (DhSidebarClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_sidebar_dispose;
        object_class->finalize = dh_sidebar_finalize;
        object_class->get_property = dh_sidebar_get_property;
        object_class->set_property = dh_sidebar_set_property;
        object_class->constructed = dh_sidebar_constructed;

        /**
         * DhSidebar:book-manager:
         *
         * The #DhBookManager.
         */
        g_object_class_install_property (object_class,
                                         PROP_BOOK_MANAGER,
                                         g_param_spec_object ("book-manager",
                                                              "Book Manager",
                                                              "",
                                                              DH_TYPE_BOOK_MANAGER,
                                                              G_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT_ONLY |
                                                              G_PARAM_STATIC_STRINGS));

        /**
         * DhSidebar::link-selected:
         * @sidebar: a #DhSidebar.
         * @link: the selected #DhLink.
         */
        signals[LINK_SELECTED] =
                g_signal_new ("link-selected",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DhSidebarClass, link_selected),
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1, DH_TYPE_LINK);
}
