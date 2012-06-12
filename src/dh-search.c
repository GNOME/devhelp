/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2005-2008 Imendio AB
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
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "dh-marshal.h"
#include "dh-keyword-model.h"
#include "dh-search.h"
#include "dh-preferences.h"
#include "dh-util.h"
#include "dh-book-manager.h"
#include "dh-book.h"
#include "dh-language.h"

typedef struct {
        DhKeywordModel *model;

        DhBookManager  *book_manager;

        DhLink         *selected_link;

        GtkWidget      *book_combo;
        GtkWidget      *entry;
        GtkWidget      *hitlist;

        GCompletion    *completion;

        guint           idle_complete;
        guint           idle_filter;
} DhSearchPriv;

static void         dh_search_init                  (DhSearch         *search);
static void         dh_search_class_init            (DhSearchClass    *klass);
static void         search_grab_focus               (GtkWidget        *widget);
static void         search_selection_changed_cb     (GtkTreeSelection *selection,
                                                     DhSearch         *content);
static gboolean     search_tree_button_press_cb     (GtkTreeView      *view,
                                                     GdkEventButton   *event,
                                                     DhSearch         *search);
static gboolean     search_entry_key_press_event_cb (GtkEntry         *entry,
                                                     GdkEventKey      *event,
                                                     DhSearch         *search);
static void         search_combo_add_book           (DhSearch         *search,
                                                     DhBook           *book);
static void         search_combo_delete_book        (DhSearch         *search,
                                                     DhBook           *book);
static void         search_combo_add_language       (DhSearch         *search,
                                                     const gchar      *language_name);
static void         search_combo_delete_language    (DhSearch         *search,
                                                     const gchar      *language_name);
static void         search_combo_changed_cb         (GtkComboBox      *combo,
                                                     DhSearch         *search);
static void         search_entry_changed_cb         (GtkEntry         *entry,
                                                     DhSearch         *search);
static void         search_entry_activated_cb       (GtkEntry         *entry,
                                                     DhSearch         *search);
static void         search_entry_text_inserted_cb   (GtkEntry         *entry,
                                                     const gchar      *text,
                                                     gint              length,
                                                     gint             *position,
                                                     DhSearch         *search);
static gboolean     search_complete_idle            (DhSearch         *search);
static gboolean     search_filter_idle              (DhSearch         *search);

enum {
        LINK_SELECTED,
        LAST_SIGNAL
};

enum {
        ROW_TYPE_SEPARATOR,
        ROW_TYPE_ALL,
        ROW_TYPE_LANGUAGE,
        ROW_TYPE_BOOK
};

enum {
        COL_TITLE,
        COL_BOOK_ID,
        COL_BOOK,
        COL_ROW_TYPE,
        N_COLUMNS
};

G_DEFINE_TYPE (DhSearch, dh_search, GTK_TYPE_VBOX);

#define GET_PRIVATE(instance) G_TYPE_INSTANCE_GET_PRIVATE \
  (instance, DH_TYPE_SEARCH, DhSearchPriv);

static gint signals[LAST_SIGNAL] = { 0 };

static void
search_finalize (GObject *object)
{
        DhSearchPriv *priv;

        priv = GET_PRIVATE (object);

        g_completion_free (priv->completion);
        g_object_unref (priv->book_manager);

        G_OBJECT_CLASS (dh_search_parent_class)->finalize (object);
}

static void
dh_search_class_init (DhSearchClass *klass)
{
        GObjectClass   *object_class = (GObjectClass *) klass;;
        GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;;

        object_class->finalize = search_finalize;

        widget_class->grab_focus = search_grab_focus;

        signals[LINK_SELECTED] =
                g_signal_new ("link_selected",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DhSearchClass, link_selected),
                              NULL, NULL,
                              _dh_marshal_VOID__POINTER,
                              G_TYPE_NONE,
                              1, G_TYPE_POINTER);

        g_type_class_add_private (klass, sizeof (DhSearchPriv));
}

static void
dh_search_init (DhSearch *search)
{
        DhSearchPriv *priv = GET_PRIVATE (search);

        priv->completion = NULL;

        priv->hitlist = gtk_tree_view_new ();
        priv->model = dh_keyword_model_new ();

        gtk_tree_view_set_model (GTK_TREE_VIEW (priv->hitlist),
                                 GTK_TREE_MODEL (priv->model));

        gtk_tree_view_set_enable_search (GTK_TREE_VIEW (priv->hitlist), FALSE);

        gtk_box_set_spacing (GTK_BOX (search), 4);
}

static void
search_grab_focus (GtkWidget *widget)
{
        DhSearchPriv *priv = GET_PRIVATE (widget);

        gtk_widget_grab_focus (priv->entry);
}

static void
search_selection_changed_cb (GtkTreeSelection *selection,
                             DhSearch         *search)
{
        DhSearchPriv *priv;
        GtkTreeIter   iter;

        priv = GET_PRIVATE (search);

        if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
                DhLink *link;

                gtk_tree_model_get (GTK_TREE_MODEL (priv->model), &iter,
                                    DH_KEYWORD_MODEL_COL_LINK, &link,
                                    -1);

                if (link != priv->selected_link) {
                        priv->selected_link = link;
                        g_signal_emit (search, signals[LINK_SELECTED], 0, link);
                }
        }
}

/* Make it possible to jump back to the currently selected item, useful when the
 * html view has been scrolled away.
 */
static gboolean
search_tree_button_press_cb (GtkTreeView    *view,
                             GdkEventButton *event,
                             DhSearch       *search)
{
        GtkTreePath  *path;
        GtkTreeIter   iter;
        DhSearchPriv *priv;
        DhLink       *link;

        priv = GET_PRIVATE (search);

        gtk_tree_view_get_path_at_pos (view, event->x, event->y, &path,
                                       NULL, NULL, NULL);
        if (!path) {
                return FALSE;
        }

        gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->model), &iter, path);
        gtk_tree_path_free (path);

        gtk_tree_model_get (GTK_TREE_MODEL (priv->model),
                            &iter,
                            DH_KEYWORD_MODEL_COL_LINK, &link,
                            -1);

        priv->selected_link = link;

        g_signal_emit (search, signals[LINK_SELECTED], 0, link);

        /* Always return FALSE so the tree view gets the event and can update
         * the selection etc.
         */
        return FALSE;
}

static gboolean
search_entry_key_press_event_cb (GtkEntry    *entry,
                                 GdkEventKey *event,
                                 DhSearch    *search)
{
        DhSearchPriv *priv = GET_PRIVATE (search);

        if (event->keyval == GDK_KEY_Tab) {
                if (event->state & GDK_CONTROL_MASK) {
                        gtk_widget_grab_focus (priv->hitlist);
                } else {
                        gtk_editable_set_position (GTK_EDITABLE (entry), -1);
                        gtk_editable_select_region (GTK_EDITABLE (entry), -1, -1);
                }
                return TRUE;
        }

        if (event->keyval == GDK_KEY_Return ||
            event->keyval == GDK_KEY_KP_Enter) {
                GtkTreeIter  iter;
                DhLink      *link;
                gchar       *name;

                /* Get the first entry found. */
                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->model), &iter)) {
                        gtk_tree_model_get (GTK_TREE_MODEL (priv->model),
                                            &iter,
                                            DH_KEYWORD_MODEL_COL_LINK, &link,
                                            DH_KEYWORD_MODEL_COL_NAME, &name,
                                            -1);

                        gtk_entry_set_text (GTK_ENTRY (entry), name);
                        g_free (name);

                        gtk_editable_set_position (GTK_EDITABLE (entry), -1);
                        gtk_editable_select_region (GTK_EDITABLE (entry), -1, -1);

                        g_signal_emit (search, signals[LINK_SELECTED], 0, link);

                        return TRUE;
                }
        }

        return FALSE;
}

static void
search_combo_get_active (DhSearch  *search,
                         gchar    **book_id,
                         gchar    **language)
{
        DhSearchPriv *priv = GET_PRIVATE (search);
        GtkTreeModel *model;
        GtkTreeIter   iter;
        gint          row_type;

        if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->book_combo),
                                            &iter)) {
                return;
        }

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->book_combo));
        gtk_tree_model_get (model,
                            &iter,
                            COL_ROW_TYPE, &row_type,
                            -1);
        switch (row_type) {
        case ROW_TYPE_LANGUAGE:
                gtk_tree_model_get (model,
                                    &iter,
                                    COL_TITLE, language,
                                    -1);
                *book_id = NULL;
                break;
        case ROW_TYPE_BOOK:
                gtk_tree_model_get (model,
                                    &iter,
                                    COL_BOOK_ID, book_id,
                                    -1);
                *language = NULL;
                break;
        default:
                *book_id = NULL;
                *language = NULL;
                break;
        }
}

static void
search_run_idle (DhSearch *search)
{
        DhSearchPriv *priv = GET_PRIVATE (search);

        if (!priv->idle_filter) {
                priv->idle_filter =
                        g_idle_add ((GSourceFunc) search_filter_idle, search);
        }
}

static void
search_combo_changed_cb (GtkComboBox *combo,
                         DhSearch    *search)
{
        search_run_idle (search);
}

static void
search_entry_changed_cb (GtkEntry *entry,
                         DhSearch *search)
{
        search_run_idle (search);
}

static void
search_entry_activated_cb (GtkEntry *entry,
                           DhSearch *search)
{
        DhSearchPriv *priv = GET_PRIVATE (search);
        gchar        *book_id;
        gchar        *language;
        const gchar  *str;

        /* Always sets book_id and language */
        search_combo_get_active (search, &book_id, &language);
        str = gtk_entry_get_text (GTK_ENTRY (priv->entry));
        dh_keyword_model_filter (priv->model, str, book_id, language);
        g_free (book_id);
        g_free (language);
}

static void
search_entry_text_inserted_cb (GtkEntry    *entry,
                               const gchar *text,
                               gint         length,
                               gint        *position,
                               DhSearch    *search)
{
        DhSearchPriv *priv = GET_PRIVATE (search);

        if (!priv->idle_complete) {
                priv->idle_complete =
                        g_idle_add ((GSourceFunc) search_complete_idle,
                                    search);
        }
}

static gboolean
search_complete_idle (DhSearch *search)
{
        DhSearchPriv *priv = GET_PRIVATE (search);
        const gchar  *str;
        gchar        *completed = NULL;
        gsize         length;

        str = gtk_entry_get_text (GTK_ENTRY (priv->entry));

        g_completion_complete (priv->completion, str, &completed);
        if (completed) {
                length = strlen (str);

                gtk_entry_set_text (GTK_ENTRY (priv->entry), completed);
                gtk_editable_set_position (GTK_EDITABLE (priv->entry), length);
                gtk_editable_select_region (GTK_EDITABLE (priv->entry),
                                            length, -1);
                g_free (completed);
        }

        priv->idle_complete = 0;

        return FALSE;
}

static gboolean
search_filter_idle (DhSearch *search)
{
        DhSearchPriv *priv = GET_PRIVATE (search);
        const gchar  *str;
        gchar        *book_id;
        gchar        *language;
        DhLink       *link;

        /* Always sets book_id and language */
        search_combo_get_active (search, &book_id, &language);
        str = gtk_entry_get_text (GTK_ENTRY (priv->entry));
        link = dh_keyword_model_filter (priv->model, str, book_id, language);
        g_free (book_id);
        g_free (language);

        priv->idle_filter = 0;

        if (link) {
                g_signal_emit (search, signals[LINK_SELECTED], 0, link);
        }

        return FALSE;
}

static void
search_cell_data_func (GtkTreeViewColumn *tree_column,
                       GtkCellRenderer   *cell,
                       GtkTreeModel      *tree_model,
                       GtkTreeIter       *iter,
                       gpointer           data)
{
        DhLink       *link;
        PangoStyle    style;

        gtk_tree_model_get (tree_model, iter,
                            DH_KEYWORD_MODEL_COL_LINK, &link,
                            -1);

        style = PANGO_STYLE_NORMAL;

        if (dh_link_get_flags (link) & DH_LINK_FLAGS_DEPRECATED) {
                style |= PANGO_STYLE_ITALIC;
        }

        g_object_set (cell,
                      "text", dh_link_get_name (link),
                      "style", style,
                      NULL);
}

static gboolean
search_combo_row_separator_func (GtkTreeModel *model,
                                 GtkTreeIter  *iter,
                                 gpointer      data)
{
        guint row_type;

        gtk_tree_model_get (model, iter,
                            COL_ROW_TYPE, &row_type,
                            -1);
        return (row_type == ROW_TYPE_SEPARATOR);
}

static void
search_combo_populate (DhSearch *search)
{
        DhSearchPriv *priv;
        GtkListStore *store;
        GtkTreeIter   iter;
        GList        *l;

        priv = GET_PRIVATE (search);

        store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->book_combo)));

        gtk_list_store_clear (store);

        /* Add main title */
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            COL_TITLE, _("All books"),
                            COL_BOOK_ID, NULL,
                            COL_BOOK, NULL,
                            COL_ROW_TYPE, ROW_TYPE_ALL,
                            -1);

        /* Add a separator */
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            COL_TITLE, NULL,
                            COL_BOOK_ID, NULL,
                            COL_BOOK, NULL,
                            COL_ROW_TYPE, ROW_TYPE_SEPARATOR,
                            -1);

        /* Add language items */
        for (l = dh_book_manager_get_languages (priv->book_manager);
             l;
             l = g_list_next (l)) {
                search_combo_add_language (search,
                                           dh_language_get_name (l->data));
        }

        /* Add book items */
        for (l = dh_book_manager_get_books (priv->book_manager);
             l;
             l = g_list_next (l)) {
                search_combo_add_book (search, DH_BOOK (l->data));
        }

        gtk_combo_box_set_active (GTK_COMBO_BOX (priv->book_combo), 0);
}

static void
search_combo_find_language (DhSearch    *search,
                            const gchar *language_name,
                            GtkTreeIter *exact_iter,
                            gboolean    *exact_found,
                            GtkTreeIter *prev_iter,
                            gboolean    *prev_found,
                            GtkTreeIter *next_iter,
                            gboolean    *next_found)
{
        DhSearchPriv *priv = GET_PRIVATE (search);
        GtkListStore *store;
        GtkTreeIter   loop_iter;

        g_assert ((exact_iter && exact_found) || (next_iter && next_found));

        store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->book_combo)));

        /* Reset all flags to not found */
        if (exact_found)
                *exact_found = FALSE;
        if (prev_found)
                *prev_found = FALSE;
        if (next_found)
                *next_found = FALSE;

        /* Setup iteration start */
        if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &loop_iter)) {
                /* Store is empty, not found */
                return;
        }

        /* Skip 'All books' and first separator items, which are always there. */
        g_assert (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &loop_iter));
        if (prev_iter)
                *prev_iter = loop_iter;
        if (!gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &loop_iter)) {
                /* No languages defined yet */
                return;
        }

        do {
                gchar  *in_list_language_name = NULL;
                guint   row_type;

                gtk_tree_model_get (GTK_TREE_MODEL (store),
                                    &loop_iter,
                                    COL_TITLE,    &in_list_language_name,
                                    COL_ROW_TYPE, &row_type,
                                    -1);

                /* If book or separator found, stop loop as no more languages
                 * are available. But this would already be the place of the
                 * next item. */
                if (row_type != ROW_TYPE_LANGUAGE) {
                        if (next_iter) {
                                *next_iter = loop_iter;
                                *next_found = TRUE;
                        }
                        g_free (in_list_language_name);
                        return;
                }

                /* Exact found? */
                if (exact_iter &&
                    g_strcmp0 (in_list_language_name, language_name) == 0) {
                        *exact_iter = loop_iter;
                        *exact_found = TRUE;
                        if (prev_found)
                                *prev_found = TRUE;
                        if (!next_iter) {
                                /* If we were not requested to look for the next one, end here */
                                g_free (in_list_language_name);
                                return;
                        }
                } else if (next_iter &&
                           g_strcmp0 (in_list_language_name, language_name) > 0) {
                        *next_iter = loop_iter;
                        *next_found = TRUE;
                        g_free (in_list_language_name);
                        return;
                }

                g_free (in_list_language_name);
                if (prev_iter && prev_found && *prev_found == FALSE)
                        *prev_iter = loop_iter;
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &loop_iter));
}

static void
search_combo_add_language (DhSearch    *search,
                           const gchar *language_name)
{
        DhSearchPriv *priv = GET_PRIVATE (search);
        GtkListStore *store;
        GtkTreeIter   iter;
        GtkTreeIter   next_iter;
        gboolean      next_iter_found = FALSE;
        gboolean      add_separator = FALSE;

        store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->book_combo)));

        /* Look for the proper place to add the new item */
        search_combo_find_language (search,
                                    language_name,
                                    NULL, NULL,
                                    NULL, NULL,
                                    &next_iter, &next_iter_found);

        if (!next_iter_found) {
                gtk_list_store_append (store,
                                       &iter);
                add_separator = TRUE;
        } else {
                gint next_iter_type;

                gtk_tree_model_get (GTK_TREE_MODEL (store),
                                    &next_iter,
                                    COL_ROW_TYPE, &next_iter_type,
                                    -1);
                if (next_iter_type == ROW_TYPE_BOOK) {
                        add_separator = TRUE;
                }

                gtk_list_store_insert_before (store,
                                              &iter,
                                              &next_iter);
        }

        gtk_list_store_set (store,
                            &iter,
                            COL_TITLE,    language_name,
                            COL_BOOK_ID,  NULL,
                            COL_BOOK,     NULL,
                            COL_ROW_TYPE, ROW_TYPE_LANGUAGE,
                            -1);

        if (add_separator) {
                GtkTreeIter separator_iter;

                gtk_list_store_insert_after (store,
                                             &separator_iter,
                                             &iter);
                gtk_list_store_set (store,
                                    &separator_iter,
                                    COL_TITLE,    NULL,
                                    COL_BOOK_ID,  NULL,
                                    COL_BOOK,     NULL,
                                    COL_ROW_TYPE, ROW_TYPE_SEPARATOR,
                                    -1);
        }
}

static void
search_combo_delete_language (DhSearch    *search,
                              const gchar *language_name)
{
        DhSearchPriv *priv = GET_PRIVATE (search);
        GtkListStore *store;
        GtkTreeIter   prev_iter;
        GtkTreeIter   exact_iter;
        GtkTreeIter   next_iter;
        GtkTreeIter   active_iter;
        gboolean      prev_iter_found = FALSE;
        gboolean      exact_iter_found = FALSE;
        gboolean      next_iter_found = FALSE;
        gboolean      remove_separator = FALSE;

        store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->book_combo)));

        /* Deleting active iter? */
        if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->book_combo),
                                           &active_iter)) {
                gchar *active_title;
                gint   row_type;

                gtk_tree_model_get (GTK_TREE_MODEL (store),
                                    &active_iter,
                                    COL_TITLE,    &active_title,
                                    COL_ROW_TYPE, &row_type,
                                    -1);
                if (row_type == ROW_TYPE_LANGUAGE &&
                    active_title &&
                    strcmp (active_title, language_name) == 0) {
                        /* Reset active item */
                        g_signal_handlers_block_by_func (priv->book_combo,
                                                         search_combo_changed_cb,
                                                         search);
                        gtk_combo_box_set_active (GTK_COMBO_BOX (priv->book_combo), 0);
                        g_signal_handlers_unblock_by_func (priv->book_combo,
                                                           search_combo_changed_cb,
                                                           search);
                }
                g_free (active_title);
        }

        /* Look for the item, keeping next and prev items as well */
        search_combo_find_language (search,
                                    language_name,
                                    &exact_iter,
                                    &exact_iter_found,
                                    &prev_iter,
                                    &prev_iter_found,
                                    &next_iter,
                                    &next_iter_found);

        g_assert (prev_iter_found);
        g_assert (exact_iter_found);

        if (next_iter_found) {
                gint next_row_type;

                gtk_tree_model_get (GTK_TREE_MODEL (store),
                                    &next_iter,
                                    COL_ROW_TYPE, &next_row_type,
                                    -1);

                /* Is this language the last language in the list? */
                if (next_row_type == ROW_TYPE_SEPARATOR) {
                        gint prev_row_type;

                        gtk_tree_model_get (GTK_TREE_MODEL (store),
                                            &prev_iter,
                                            COL_ROW_TYPE, &prev_row_type,
                                            -1);

                        /* Is this language the first language in the list? */
                        if (prev_row_type == ROW_TYPE_SEPARATOR) {
                                /* If first and last, its the only one, so
                                 * remove next separator */
                                remove_separator = TRUE;
                        }
                }
        }

        /* If found, delete item from the store */
        gtk_list_store_remove (store, &exact_iter);

        if (remove_separator) {
                gtk_list_store_remove (store, &next_iter);
        }
}

static void
search_combo_find_book (DhSearch    *search,
                        DhBook      *book,
                        GtkTreeIter *exact_iter,
                        gboolean    *exact_found,
                        GtkTreeIter *next_iter,
                        gboolean    *next_found)
{
        DhSearchPriv *priv = GET_PRIVATE (search);
        GtkListStore *store;
        GtkTreeIter   loop_iter;

        g_assert ((exact_iter && exact_found) || (next_iter && next_found));

        store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->book_combo)));

        /* Reset all flags to not found */
        if (exact_found)
                *exact_found = FALSE;
        if (next_found)
                *next_found = FALSE;

        /* Setup iteration start */
        if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &loop_iter)) {
                /* Store is empty, not found */
                return;
        }

        do {
                DhBook *in_list_book = NULL;
                guint   row_type;

                gtk_tree_model_get (GTK_TREE_MODEL (store),
                                    &loop_iter,
                                    COL_BOOK,     &in_list_book,
                                    COL_ROW_TYPE, &row_type,
                                    -1);

                /* Until first book found, do nothing */
                if (row_type != ROW_TYPE_BOOK) {
                        if (in_list_book)
                                g_object_unref (in_list_book);
                        continue;
                }

                /* We can compare pointers directly as we're playing with references
                 * of the same object */
                if (in_list_book == book) {
                        *exact_iter = loop_iter;
                        *exact_found = TRUE;
                        if (!next_iter) {
                                /* If we were not requested to look for the next one, end here */
                                g_object_unref (in_list_book);
                                return;
                        }
                } else if (next_iter &&
                           dh_book_cmp_by_title (in_list_book, book) > 0) {
                        *next_iter = loop_iter;
                        *next_found = TRUE;
                        g_object_unref (in_list_book);
                        return;
                }

                g_object_unref (in_list_book);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &loop_iter));
}

static void
search_combo_add_book (DhSearch *search,
                       DhBook   *book)
{
        DhSearchPriv *priv = GET_PRIVATE (search);
        GtkListStore *store;
        GNode        *node;
        DhLink       *link;
        GtkTreeIter   book_iter;
        GtkTreeIter   next_book_iter;
        gboolean      next_book_iter_found;

        store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->book_combo)));

        node = dh_book_get_tree (book);
        if (!node)
                return;

        link = node->data;

        /* Look for the proper place to add the new item */
        search_combo_find_book (search,
                                book,
                                NULL,
                                NULL,
                                &next_book_iter,
                                &next_book_iter_found);

        if (!next_book_iter_found) {
                gtk_list_store_append (store,
                                       &book_iter);
        } else {
                gtk_list_store_insert_before (store,
                                              &book_iter,
                                              &next_book_iter);
        }

        gtk_list_store_set (store, &book_iter,
                            COL_TITLE,    dh_link_get_name (link),
                            COL_BOOK_ID,  dh_link_get_book_id (link),
                            COL_BOOK,     book,
                            COL_ROW_TYPE, ROW_TYPE_BOOK,
                            -1);
}

static void
search_combo_delete_book (DhSearch *search,
                          DhBook   *book)
{
        DhSearchPriv *priv = GET_PRIVATE (search);
        GtkListStore *store;
        GtkTreeIter   iter;
        gboolean      found;

        store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->book_combo)));

        /* Deleting active iter? */
        if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->book_combo),
                                           &iter)) {
                DhBook *active_book = NULL;

                gtk_tree_model_get (GTK_TREE_MODEL (store),
                                    &iter,
                                    COL_BOOK, &active_book,
                                    -1);
                if (active_book == book) {
                        /* Reset active item */
                        g_signal_handlers_block_by_func (priv->book_combo,
                                                         search_combo_changed_cb,
                                                         search);
                        gtk_combo_box_set_active (GTK_COMBO_BOX (priv->book_combo), 0);
                        g_signal_handlers_unblock_by_func (priv->book_combo,
                                                           search_combo_changed_cb,
                                                           search);
                        /* And remove it right here */
                        gtk_list_store_remove (store, &iter);
                        g_object_unref (active_book);
                        return;
                }
                if (active_book)
                        g_object_unref (active_book);
        }

        /* Look for the item */
        search_combo_find_book (search,
                                book,
                                &iter,
                                &found,
                                NULL,
                                NULL);

        /* Book may not be found in the combobox if it wasn't enabled */
        if (found) {
                gtk_list_store_remove (store, &iter);
        }
}

static void
search_combo_create (DhSearch *search)
{
        GtkListStore    *store;
        GtkCellRenderer *cell;
        DhSearchPriv    *priv;

        priv = GET_PRIVATE (search);

        store = gtk_list_store_new (4,
                                    G_TYPE_STRING, /* COL_TITLE    */
                                    G_TYPE_STRING, /* COL_BOOK_ID  */
                                    G_TYPE_OBJECT, /* COL_BOOK     */
                                    G_TYPE_INT);   /* COL_ROW_TYPE */
        priv->book_combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
        g_object_unref (store);

        gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (priv->book_combo),
                                              search_combo_row_separator_func,
                                              NULL, NULL);

        cell = gtk_cell_renderer_text_new ();
        g_object_set (cell, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->book_combo),
                                    cell,
                                    TRUE);
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (priv->book_combo),
                                       cell,
                                       "text", 0);

        search_combo_populate (search);
}

static void
search_completion_add_book (DhSearch *search,
                            DhBook   *book)
{
        DhSearchPriv *priv = GET_PRIVATE (search);
        GList        *completions;

        if (G_UNLIKELY (!priv->completion)) {
                priv->completion = g_completion_new (NULL);
        }

        completions = dh_book_get_completions (book);
        if (completions) {
                g_completion_add_items (priv->completion, completions);
        }
}

static void
search_completion_delete_book (DhSearch *search,
                               DhBook   *book)
{
        DhSearchPriv *priv = GET_PRIVATE (search);
        GList        *completions;

        if (G_UNLIKELY (!priv->completion)) {
                return;
        }

        completions = dh_book_get_completions (book);
        if (completions) {
                g_completion_remove_items (priv->completion, completions);
        }
}

static void
search_book_created_or_enabled_cb (DhBookManager *book_manager,
                                   DhBook        *book,
                                   gpointer       user_data)
{
        DhSearch *search = DH_SEARCH (user_data);

        search_completion_add_book (search, book);
        search_combo_add_book (search, book);
        /* Update current search if any */
        search_run_idle (search);
}

static void
search_book_deleted_or_disabled_cb (DhBookManager *book_manager,
                                    DhBook        *book,
                                    gpointer       user_data)
{
        DhSearch *search = DH_SEARCH (user_data);

        search_completion_delete_book (search, book);
        search_combo_delete_book (search, book);
        /* Update current search if any */
        search_run_idle (search);
}

static void
search_language_enabled_cb (DhBookManager *book_manager,
                            const gchar   *language_name,
                            gpointer       user_data)
{
        DhSearch *search = DH_SEARCH (user_data);

        search_combo_add_language (search, language_name);
}

static void
search_language_disabled_cb (DhBookManager *book_manager,
                             const gchar   *language_name,
                             gpointer       user_data)
{
        DhSearch *search = DH_SEARCH (user_data);

        search_combo_delete_language (search, language_name);
}

static void
search_completion_populate (DhSearch *search)
{
        DhSearchPriv *priv;
        GList        *l;

        priv = GET_PRIVATE (search);

        for (l = dh_book_manager_get_books (priv->book_manager);
             l;
             l = g_list_next (l)) {
                search_completion_add_book (search, DH_BOOK (l->data));
        }
}


GtkWidget *
dh_search_new (DhBookManager *book_manager)
{
        DhSearch         *search;
        DhSearchPriv     *priv;
        GtkTreeSelection *selection;
        GtkWidget        *list_sw;
        GtkWidget        *hbox;
        GtkWidget        *book_label;
        GtkCellRenderer  *cell;

        search = g_object_new (DH_TYPE_SEARCH, NULL);

        priv = GET_PRIVATE (search);

        priv->book_manager = g_object_ref (book_manager);

        g_signal_connect (priv->book_manager,
                          "book-created",
                          G_CALLBACK (search_book_created_or_enabled_cb),
                          search);
        g_signal_connect (priv->book_manager,
                          "book-deleted",
                          G_CALLBACK (search_book_deleted_or_disabled_cb),
                          search);
        g_signal_connect (priv->book_manager,
                          "book-enabled",
                          G_CALLBACK (search_book_created_or_enabled_cb),
                          search);
        g_signal_connect (priv->book_manager,
                          "book-disabled",
                          G_CALLBACK (search_book_deleted_or_disabled_cb),
                          search);
        g_signal_connect (priv->book_manager,
                          "language-enabled",
                          G_CALLBACK (search_language_enabled_cb),
                          search);
        g_signal_connect (priv->book_manager,
                          "language-disabled",
                          G_CALLBACK (search_language_disabled_cb),
                          search);

        gtk_container_set_border_width (GTK_CONTAINER (search), 2);

        search_combo_create (search);
        g_signal_connect (priv->book_combo, "changed",
                          G_CALLBACK (search_combo_changed_cb),
                          search);

        book_label = gtk_label_new_with_mnemonic (_("Search in:"));
        gtk_label_set_mnemonic_widget (GTK_LABEL (book_label), priv->book_combo);

        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_box_pack_start (GTK_BOX (hbox), book_label, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), priv->book_combo, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (search), hbox, FALSE, FALSE, 0);

        /* Setup the keyword box. */
        priv->entry = gtk_entry_new ();
        g_signal_connect (priv->entry, "key-press-event",
                          G_CALLBACK (search_entry_key_press_event_cb),
                          search);

        g_signal_connect (priv->hitlist, "button-press-event",
                          G_CALLBACK (search_tree_button_press_cb),
                          search);

        g_signal_connect (priv->entry, "changed",
                          G_CALLBACK (search_entry_changed_cb),
                          search);

        g_signal_connect (priv->entry, "activate",
                          G_CALLBACK (search_entry_activated_cb),
                          search);

        g_signal_connect (priv->entry, "insert-text",
                          G_CALLBACK (search_entry_text_inserted_cb),
                          search);

        gtk_box_pack_start (GTK_BOX (search), priv->entry, FALSE, FALSE, 0);

        /* Setup the hitlist */
        list_sw = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (list_sw), GTK_SHADOW_IN);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (list_sw),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);

        cell = gtk_cell_renderer_text_new ();
        g_object_set (cell,
                      "ellipsize", PANGO_ELLIPSIZE_END,
                      NULL);

        gtk_tree_view_insert_column_with_data_func (
                GTK_TREE_VIEW (priv->hitlist),
                -1,
                NULL,
                cell,
                search_cell_data_func,
                search, NULL);

        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->hitlist),
                                           FALSE);
        gtk_tree_view_set_search_column (GTK_TREE_VIEW (priv->hitlist),
                                         DH_KEYWORD_MODEL_COL_NAME);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->hitlist));

        g_signal_connect (selection, "changed",
                          G_CALLBACK (search_selection_changed_cb),
                          search);

        gtk_container_add (GTK_CONTAINER (list_sw), priv->hitlist);

        gtk_box_pack_end (GTK_BOX (search), list_sw, TRUE, TRUE, 0);

        search_completion_populate (search);

        dh_keyword_model_set_words (priv->model, book_manager);

        gtk_widget_show_all (GTK_WIDGET (search));

        return GTK_WIDGET (search);
}

void
dh_search_set_search_string (DhSearch    *search,
                             const gchar *str,
                             const gchar *unused)
{
        DhSearchPriv *priv;

        g_return_if_fail (DH_IS_SEARCH (search));

        priv = GET_PRIVATE (search);

        /* Mark "All books" as active */
        gtk_combo_box_set_active (GTK_COMBO_BOX (priv->book_combo), 0);

        g_signal_handlers_block_by_func (priv->entry,
                                         search_entry_changed_cb,
                                         search);

        gtk_entry_set_text (GTK_ENTRY (priv->entry), str);

        gtk_editable_set_position (GTK_EDITABLE (priv->entry), -1);
        gtk_editable_select_region (GTK_EDITABLE (priv->entry), -1, -1);

        g_signal_handlers_unblock_by_func (priv->entry,
                                           search_entry_changed_cb,
                                           search);

        if (!priv->idle_filter) {
                priv->idle_filter =
                        g_idle_add ((GSourceFunc) search_filter_idle, search);
        }
}
