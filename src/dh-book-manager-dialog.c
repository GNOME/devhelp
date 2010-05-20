/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
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
#include "dh-util.h"
#include "dh-book-manager-dialog.h"
#include "ige-conf.h"
#include "dh-base.h"
#include "dh-link.h"
#include "dh-book-manager.h"
#include "dh-book.h"

typedef struct {
	GtkWidget     *dialog;
        GtkTreeView   *treeview;
        GtkListStore  *store;
        DhBase        *base;
} DhBookManagerDialog;

/* List store columns... */
#define LTCOLUMN_ENABLED  0
#define LTCOLUMN_TITLE    1
#define LTCOLUMN_BOOK     2

#define DH_CONF_PATH                  "/apps/devhelp"

static DhBookManagerDialog *prefs;

static void
book_manager_dialog_init (void)
{
	if (!prefs) {
                prefs = g_new0 (DhBookManagerDialog, 1);

                prefs->base = dh_base_get ();
        }
}

static void
book_manager_dialog_close_cb (GtkButton *button, gpointer user_data)
{
	DhBookManagerDialog *prefs = user_data;

	gtk_widget_destroy (GTK_WIDGET (prefs->dialog));

	prefs->dialog = NULL;
}

static void
book_manager_tree_selection_toggled_cb (GtkCellRendererToggle *cell_renderer,
                                        gchar *path,
                                        gpointer user_data)
{
        GtkTreeIter iter;

        if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (prefs->store),
                                                 &iter,
                                                 path))
        {
                gpointer book = NULL;
                gboolean enabled;

                gtk_tree_model_get (GTK_TREE_MODEL (prefs->store),
                                    &iter,
                                    LTCOLUMN_BOOK,       &book,
                                    LTCOLUMN_ENABLED,    &enabled,
                                    -1);

                if (book) {
                        /* Update book conf */
                        dh_book_set_enabled (book, !enabled);

                        gtk_list_store_set (prefs->store, &iter,
                                            LTCOLUMN_ENABLED, !enabled,
                                            -1);
                }
        }
}

static void
book_manager_dialog_populate_store (void)
{
        GList         *l;
        DhBookManager *book_manager;

        book_manager = dh_base_get_book_manager (prefs->base);

        for (l = dh_book_manager_get_books (book_manager);
             l;
             l = g_list_next (l)) {
                GtkTreeIter  iter;
                DhBook      *book;

                book = DH_BOOK (l->data);

                gtk_list_store_append (prefs->store, &iter);
                gtk_list_store_set (prefs->store, &iter,
                                    LTCOLUMN_ENABLED,  dh_book_get_enabled (book),
                                    LTCOLUMN_TITLE,    dh_book_get_title (book),
                                    LTCOLUMN_BOOK,     book,
                                    -1);
        }
}

void
dh_book_manager_dialog_show (GtkWindow *parent)
{
        gchar      *path;
	GtkBuilder *builder;

        book_manager_dialog_init ();

	if (prefs->dialog != NULL) {
		gtk_window_present (GTK_WINDOW (prefs->dialog));
		return;
	}

        path = dh_util_build_data_filename ("devhelp", "ui",
                                            "devhelp.builder",
                                            NULL);
	builder = dh_util_builder_get_file (
                path,
                "book_manager_dialog",
                NULL,
                "book_manager_dialog", &prefs->dialog,
                "book_manager_store", &prefs->store,
                "book_manager_treeview", &prefs->treeview,
                NULL);
        g_free (path);

	dh_util_builder_connect (
                builder,
                prefs,
                "book_manager_close_button", "clicked", book_manager_dialog_close_cb,
                "book_manager_toggle_enabled", "toggled", book_manager_tree_selection_toggled_cb,
                NULL);

	g_object_unref (builder);

        book_manager_dialog_populate_store ();

	gtk_window_set_transient_for (GTK_WINDOW (prefs->dialog), parent);
	gtk_widget_show_all (prefs->dialog);
}
