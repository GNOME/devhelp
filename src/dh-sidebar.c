/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2005-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
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
#include "dh-sidebar.h"
#include "dh-util.h"
#include "dh-book-manager.h"
#include "dh-book.h"
#include "dh-book-tree.h"

G_DEFINE_TYPE (DhSidebar, dh_sidebar, GTK_TYPE_VBOX)

enum {
        LINK_SELECTED,
        LAST_SIGNAL
};

struct _DhSidebarPrivate {
        DhKeywordModel *model;

        DhBookManager  *book_manager;

        DhLink         *selected_link;

        GtkWidget      *search_all_button;
        GtkWidget      *search_current_button;
        GtkWidget      *entry;
        GtkWidget      *hitlist;
        GtkWidget      *sw_hitlist;
        GtkWidget      *book_tree;
        GtkWidget      *sw_book_tree;

        GCompletion    *completion;
        guint           idle_complete;
        guint           idle_filter;
};

static gint signals[LAST_SIGNAL] = { 0 };

/******************************************************************************/

static gboolean
sidebar_filter_idle (DhSidebar *self)
{
        const gchar *str;
        DhLink      *link;
        DhLink      *book_link;

        self->priv->idle_filter = 0;

        str = gtk_entry_get_text (GTK_ENTRY (self->priv->entry));

        book_link = (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->search_all_button)) ?
                     NULL :
                     dh_sidebar_get_selected_book (self));

        link = dh_keyword_model_filter (self->priv->model,
                                        str,
                                        book_link ? dh_link_get_book_id (book_link) : NULL,
                                        NULL);

        if (link)
                g_signal_emit (self, signals[LINK_SELECTED], 0, link);

        return FALSE;
}

static void
sidebar_search_run_idle (DhSidebar *self)
{
        if (!self->priv->idle_filter)
                self->priv->idle_filter =
                        g_idle_add ((GSourceFunc) sidebar_filter_idle, self);
}

/******************************************************************************/

static void
sidebar_completion_add_book (DhSidebar *self,
                             DhBook    *book)
{
        GList *completions;

        if (G_UNLIKELY (!self->priv->completion))
                self->priv->completion = g_completion_new (NULL);

        completions = dh_book_get_completions (book);
        if (completions)
                g_completion_add_items (self->priv->completion, completions);
}

static void
sidebar_completion_delete_book (DhSidebar *self,
                                DhBook    *book)
{
        GList *completions;

        if (G_UNLIKELY (!self->priv->completion))
                return;

        completions = dh_book_get_completions (book);
        if (completions)
                g_completion_remove_items (self->priv->completion, completions);
}

static void
sidebar_book_created_or_enabled_cb (DhBookManager *book_manager,
                                    DhBook        *book,
                                    DhSidebar     *self)
{
        sidebar_completion_add_book (self, book);
        /* Update current search if any */
        sidebar_search_run_idle (self);
}

static void
sidebar_book_deleted_or_disabled_cb (DhBookManager *book_manager,
                                     DhBook        *book,
                                     DhSidebar     *self)
{
        sidebar_completion_delete_book (self, book);
        /* Update current search if any */
        sidebar_search_run_idle (self);
}

static void
sidebar_completion_populate (DhSidebar *self)
{
        GList        *l;

        for (l = dh_book_manager_get_books (self->priv->book_manager);
             l;
             l = g_list_next (l)) {
                sidebar_completion_add_book (self, DH_BOOK (l->data));
        }
}

/******************************************************************************/

static void
sidebar_selection_changed_cb (GtkTreeSelection *selection,
                              DhSidebar        *self)
{
        GtkTreeIter   iter;

        if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
                DhLink *link;

                gtk_tree_model_get (GTK_TREE_MODEL (self->priv->model), &iter,
                                    DH_KEYWORD_MODEL_COL_LINK, &link,
                                    -1);

                if (link != self->priv->selected_link) {
                        self->priv->selected_link = link;
                        g_signal_emit (self, signals[LINK_SELECTED], 0, link);
                }
        }
}

/* Make it possible to jump back to the currently selected item, useful when the
 * html view has been scrolled away.
 */
static gboolean
sidebar_tree_button_press_cb (GtkTreeView    *view,
                              GdkEventButton *event,
                              DhSidebar      *self)
{
        GtkTreePath  *path;
        GtkTreeIter   iter;
        DhLink       *link;

        gtk_tree_view_get_path_at_pos (view, event->x, event->y, &path,
                                       NULL, NULL, NULL);
        if (!path)
                return FALSE;

        gtk_tree_model_get_iter (GTK_TREE_MODEL (self->priv->model), &iter, path);
        gtk_tree_path_free (path);

        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->model),
                            &iter,
                            DH_KEYWORD_MODEL_COL_LINK, &link,
                            -1);

        self->priv->selected_link = link;

        g_signal_emit (self, signals[LINK_SELECTED], 0, link);

        /* Always return FALSE so the tree view gets the event and can update
         * the selection etc.
         */
        return FALSE;
}

static gboolean
sidebar_entry_key_press_event_cb (GtkEntry    *entry,
                                  GdkEventKey *event,
                                  DhSidebar   *self)
{
        if (event->keyval == GDK_KEY_Tab) {
                if (event->state & GDK_CONTROL_MASK) {
                        gtk_widget_grab_focus (self->priv->hitlist);
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
                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->model), &iter)) {
                        gtk_tree_model_get (GTK_TREE_MODEL (self->priv->model),
                                            &iter,
                                            DH_KEYWORD_MODEL_COL_LINK, &link,
                                            DH_KEYWORD_MODEL_COL_NAME, &name,
                                            -1);

                        gtk_entry_set_text (GTK_ENTRY (entry), name);
                        g_free (name);

                        gtk_editable_set_position (GTK_EDITABLE (entry), -1);
                        gtk_editable_select_region (GTK_EDITABLE (entry), -1, -1);

                        g_signal_emit (self, signals[LINK_SELECTED], 0, link);

                        return TRUE;
                }
        }

        return FALSE;
}

static void
sidebar_entry_changed_cb (GtkEntry  *entry,
                          DhSidebar *self)
{
        /* If search entry is empty, hide the hitlist */
        if (strcmp (gtk_entry_get_text (entry), "") == 0) {
                gtk_widget_hide (self->priv->sw_hitlist);
                gtk_widget_show (self->priv->sw_book_tree);
                return;
        }

        gtk_widget_hide (self->priv->sw_book_tree);
        gtk_widget_show (self->priv->sw_hitlist);
        sidebar_search_run_idle (self);
}

static gboolean
sidebar_complete_idle (DhSidebar *self)
{
        const gchar  *str;
        gchar        *completed = NULL;
        gsize         length;

        str = gtk_entry_get_text (GTK_ENTRY (self->priv->entry));

        g_completion_complete (self->priv->completion, str, &completed);
        if (completed) {
                length = strlen (str);

                gtk_entry_set_text (GTK_ENTRY (self->priv->entry), completed);
                gtk_editable_set_position (GTK_EDITABLE (self->priv->entry), length);
                gtk_editable_select_region (GTK_EDITABLE (self->priv->entry),
                                            length, -1);
                g_free (completed);
        }

        self->priv->idle_complete = 0;

        return FALSE;
}

static void
sidebar_entry_text_inserted_cb (GtkEntry    *entry,
                                const gchar *text,
                                gint         length,
                                gint        *position,
                                DhSidebar   *self)
{
        if (!self->priv->idle_complete)
                self->priv->idle_complete =
                        g_idle_add ((GSourceFunc) sidebar_complete_idle, self);
}

/******************************************************************************/

void
dh_sidebar_set_search_string (DhSidebar   *self,
                              const gchar *str)
{
        g_return_if_fail (DH_IS_SIDEBAR (self));

        /* Mark "All books" as active */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->search_all_button), TRUE);

        g_signal_handlers_block_by_func (self->priv->entry,
                                         sidebar_entry_changed_cb,
                                         self);

        gtk_entry_set_text (GTK_ENTRY (self->priv->entry), str);
        gtk_editable_set_position (GTK_EDITABLE (self->priv->entry), -1);
        gtk_editable_select_region (GTK_EDITABLE (self->priv->entry), -1, -1);

        g_signal_handlers_unblock_by_func (self->priv->entry,
                                           sidebar_entry_changed_cb,
                                           self);

        sidebar_search_run_idle (self);
}

/******************************************************************************/

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

        if (dh_link_get_flags (link) & DH_LINK_FLAGS_DEPRECATED)
                style |= PANGO_STYLE_ITALIC;

        g_object_set (cell,
                      "text", dh_link_get_name (link),
                      "style", style,
                      NULL);
}

/******************************************************************************/

static void
search_filter_button_toggled (GtkToggleButton *button,
                              DhSidebar       *self)
{
        sidebar_search_run_idle (self);
}

/******************************************************************************/

static void
sidebar_book_tree_link_selected_cb (GObject   *ignored,
                                    DhLink    *link,
                                    DhSidebar *self)
{
        if (link != self->priv->selected_link) {
                self->priv->selected_link = link;
                g_signal_emit (self, signals[LINK_SELECTED], 0, link);
        }
}

DhLink *
dh_sidebar_get_selected_book (DhSidebar *self)
{
        return dh_book_tree_get_selected_book (DH_BOOK_TREE (self->priv->book_tree));
}

void
dh_sidebar_select_uri (DhSidebar   *self,
                       const gchar *uri)
{
        dh_book_tree_select_uri (DH_BOOK_TREE (self->priv->book_tree), uri);
}

/******************************************************************************/

GtkWidget *
dh_sidebar_new (DhBookManager *book_manager)
{
        DhSidebar        *self;
        GtkCellRenderer  *cell;
        GtkWidget        *hbox;
        GtkWidget        *button_box;

        self = g_object_new (DH_TYPE_SIDEBAR, NULL);
        gtk_container_set_border_width (GTK_CONTAINER (self), 2);
        gtk_box_set_spacing (GTK_BOX (self), 4);

        /* Setup keyword model */
        self->priv->model = dh_keyword_model_new ();

        /* Setup hitlist */
        self->priv->hitlist = gtk_tree_view_new ();
        gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->hitlist), GTK_TREE_MODEL (self->priv->model));
        gtk_tree_view_set_enable_search (GTK_TREE_VIEW (self->priv->hitlist), FALSE);

        /* Setup book manager */
        self->priv->book_manager = g_object_ref (book_manager);
        g_signal_connect (self->priv->book_manager,
                          "book-created",
                          G_CALLBACK (sidebar_book_created_or_enabled_cb),
                          self);
        g_signal_connect (self->priv->book_manager,
                          "book-deleted",
                          G_CALLBACK (sidebar_book_deleted_or_disabled_cb),
                          self);
        g_signal_connect (self->priv->book_manager,
                          "book-enabled",
                          G_CALLBACK (sidebar_book_created_or_enabled_cb),
                          self);
        g_signal_connect (self->priv->book_manager,
                          "book-disabled",
                          G_CALLBACK (sidebar_book_deleted_or_disabled_cb),
                          self);

        /* Setup the top-level box with entry search and Current|All buttons */
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_box_pack_start (GTK_BOX (self), hbox, FALSE, FALSE, 0);

        /* Setup the search entry */
        self->priv->entry = gtk_search_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), self->priv->entry, TRUE, TRUE, 0);
        g_signal_connect (self->priv->entry, "key-press-event",
                          G_CALLBACK (sidebar_entry_key_press_event_cb),
                          self);
        g_signal_connect (self->priv->hitlist, "button-press-event",
                          G_CALLBACK (sidebar_tree_button_press_cb),
                          self);
        g_signal_connect (self->priv->entry, "changed",
                          G_CALLBACK (sidebar_entry_changed_cb),
                          self);
        g_signal_connect (self->priv->entry, "insert-text",
                          G_CALLBACK (sidebar_entry_text_inserted_cb),
                          self);
        gtk_box_pack_start (GTK_BOX (self), self->priv->entry, FALSE, FALSE, 0);

	/* Setup the Current/All Files selector */
	self->priv->search_current_button = gtk_radio_button_new_with_label (NULL, _("Current"));
	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (self->priv->search_current_button), FALSE);
	self->priv->search_all_button = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (self->priv->search_current_button),
                                                                                     _("All Books"));
	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (self->priv->search_all_button), FALSE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->search_all_button), TRUE);
	g_signal_connect (self->priv->search_current_button,
                          "toggled",
			  G_CALLBACK (search_filter_button_toggled),
                          self);
	button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (hbox), button_box, FALSE, FALSE, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (button_box),
				     GTK_STYLE_CLASS_LINKED);
	gtk_style_context_add_class (gtk_widget_get_style_context (button_box),
				     GTK_STYLE_CLASS_RAISED);
	gtk_box_pack_start (GTK_BOX (button_box), self->priv->search_current_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_box), self->priv->search_all_button, FALSE, FALSE, 0);

        /* Setup the hitlist */
        self->priv->sw_hitlist = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_set_no_show_all (self->priv->sw_hitlist, TRUE);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (self->priv->sw_hitlist), GTK_SHADOW_IN);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->priv->sw_hitlist),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        cell = gtk_cell_renderer_text_new ();
        g_object_set (cell,
                      "ellipsize", PANGO_ELLIPSIZE_END,
                      NULL);
        gtk_tree_view_insert_column_with_data_func (
                GTK_TREE_VIEW (self->priv->hitlist),
                -1,
                NULL,
                cell,
                search_cell_data_func,
                self,
                NULL);
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self->priv->hitlist), FALSE);
        gtk_tree_view_set_search_column (GTK_TREE_VIEW (self->priv->hitlist), DH_KEYWORD_MODEL_COL_NAME);
        g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->hitlist)),
                          "changed",
                          G_CALLBACK (sidebar_selection_changed_cb),
                          self);
        gtk_widget_show (self->priv->hitlist);
        gtk_container_add (GTK_CONTAINER (self->priv->sw_hitlist), self->priv->hitlist);
        gtk_box_pack_start (GTK_BOX (self), self->priv->sw_hitlist, TRUE, TRUE, 0);

        /* Setup the book tree */
        self->priv->sw_book_tree = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (self->priv->sw_book_tree);
        gtk_widget_set_no_show_all (self->priv->sw_book_tree, TRUE);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->priv->sw_book_tree),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (self->priv->sw_book_tree),
                                             GTK_SHADOW_IN);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->sw_book_tree), 2);
        self->priv->book_tree = dh_book_tree_new (self->priv->book_manager);
        gtk_widget_show (self->priv->book_tree);
        g_signal_connect (self->priv->book_tree,
                          "link-selected",
                          G_CALLBACK (sidebar_book_tree_link_selected_cb),
                          self);
        gtk_container_add (GTK_CONTAINER (self->priv->sw_book_tree), self->priv->book_tree);
        gtk_box_pack_end (GTK_BOX (self), self->priv->sw_book_tree, TRUE, TRUE, 0);

        sidebar_completion_populate (self);

        dh_keyword_model_set_words (self->priv->model, self->priv->book_manager);

        gtk_widget_show_all (GTK_WIDGET (self));

        return GTK_WIDGET (self);
}

static void
sidebar_finalize (GObject *object)
{
        DhSidebar *self = DH_SIDEBAR (object);

        g_completion_free (self->priv->completion);
        g_object_unref (self->priv->book_manager);

        G_OBJECT_CLASS (dh_sidebar_parent_class)->finalize (object);
}

static void
dh_sidebar_init (DhSidebar *self)
{
        self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                                  DH_TYPE_SIDEBAR,
                                                  DhSidebarPrivate);
}

static void
dh_sidebar_class_init (DhSidebarClass *klass)
{
        GObjectClass   *object_class = (GObjectClass *) klass;

        object_class->finalize = sidebar_finalize;
        g_type_class_add_private (klass, sizeof (DhSidebarPrivate));

        signals[LINK_SELECTED] =
                g_signal_new ("link_selected",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DhSidebarClass, link_selected),
                              NULL, NULL,
                              _dh_marshal_VOID__POINTER,
                              G_TYPE_NONE,
                              1, G_TYPE_POINTER);
}
