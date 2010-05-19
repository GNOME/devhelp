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

typedef struct {
	GtkWidget *dialog;

} DhBookManagerDialog;

#define DH_CONF_PATH                  "/apps/devhelp"

static DhBookManagerDialog *prefs;

static void
book_manager_dialog_init (void)
{
	if (!prefs) {
                prefs = g_new0 (DhBookManagerDialog, 1);
        }
}

static void
book_manager_dialog_close_cb (GtkButton *button, gpointer user_data)
{
	DhBookManagerDialog *prefs = user_data;

	gtk_widget_destroy (GTK_WIDGET (prefs->dialog));

	prefs->dialog = NULL;
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
                NULL);
        g_free (path);

	dh_util_builder_connect (
                builder,
                prefs,
                "book_manager_close_button", "clicked", book_manager_dialog_close_cb,
                NULL);

	g_object_unref (builder);

	gtk_window_set_transient_for (GTK_WINDOW (prefs->dialog), parent);
	gtk_widget_show_all (prefs->dialog);
}
