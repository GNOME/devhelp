/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Richard Hult <rhult@codefactory.se>
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
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
 *
 * Author: Richard Hult <rhult@codefactory.se>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <glade/glade-xml.h>

#include "books-dialog.h"
#include "bookshelf.h"
#include "main.h"
#include "preferences.h"

struct _BooksDialog {
	Preferences    *prefs;
	GConfClient    *client;
	DevHelp        *devhelp;
	GtkWidget      *dialog;
	GtkCList       *clist_hidden;
	GtkCList       *clist_visible;
};

struct _BookInfo {
	Book           *book;
	GtkWidget      *dialog;
	GtkEntry       *path;
};

static void
gconf_sidebar_visible_changed_cb (GConfClient       *client,
				  guint              notify_id,
				  GConfEntry        *entry,
				  gpointer           data)
{
}

static void
books_button_show_clicked_cb (GtkWidget *button,
			      gpointer   user_data)
{
	BooksDialog *dialog;
	
	g_return_if_fail (user_data != NULL);
	
	dialog = BOOKS_DIALOG (user_data);
	puts (__FUNCTION__);
}

static void
books_button_hide_clicked_cb (GtkWidget *button,
			      gpointer   user_data)
{
	BooksDialog *dialog;
	gint         row;
	gchar      **text;
	
	g_return_if_fail (user_data != NULL);

	dialog = BOOKS_DIALOG (user_data);
	puts (__FUNCTION__);

	row = dialog->clist_visible->focus_row;
	gtk_clist_get_text (dialog->clist_visible, row, 0, text);
}

static gboolean
book_info_destroy_cb (GtkWidget *widget, BookInfo *dialog)
{
	g_free (dialog);
}

static void
info_button_ok_clicked_cb (GtkButton *button,
			   BookInfo  *book_info)
{
	gchar *new;
	
	g_return_if_fail (book_info != NULL);

	new = gtk_entry_get_text (book_info->path);
	if (strcmp (new, book_get_path (book_info->book)) == 0) {
		g_message ("New path; %s", new);
	}
	
	/* FIXME: g_free (new) ? */
	
	gtk_widget_destroy (book_info->dialog);
	book_info->dialog = NULL;
}
	
static void
books_button_edit_clicked_cb (GtkWidget *button,
			      gpointer   user_data)
{
	GladeXML       *gui;
	GtkLabel       *name;
	GtkLabel       *title;
	GtkLabel       *author;
	GtkLabel       *version;
	GtkButton      *button_ok;
	GtkButton      *button_cancel;
	BooksDialog    *dialog;
	BookInfo       *book_info;
	gint            row;
	
	g_return_if_fail (user_data != NULL);

	dialog = BOOKS_DIALOG (user_data);

	/* create the dialog */
	gui = glade_xml_new (DATA_DIR "/devhelp/glade/devhelp.glade", "info_dialog");

	book_info = g_new0 (BookInfo, 1);
	book_info->dialog = GTK_WIDGET (glade_xml_get_widget (gui, "info_dialog"));
	gtk_signal_connect (GTK_OBJECT (book_info->dialog),
			    "destroy",
			    GTK_SIGNAL_FUNC (book_info_destroy_cb),
			    book_info);
	
	button_ok = GTK_BUTTON (glade_xml_get_widget (gui, "button_info_ok"));
	gtk_signal_connect (GTK_OBJECT (button_ok),
			    "clicked",
			    info_button_ok_clicked_cb,
			    book_info);
	
	button_cancel = GTK_BUTTON (glade_xml_get_widget (gui, "button_info_cancel"));
	gtk_signal_connect_object (GTK_OBJECT (button_cancel),
				   "clicked",
				   gtk_widget_destroy,
				   (gpointer)book_info->dialog);

	name = GTK_LABEL (glade_xml_get_widget (gui, "label_info_name"));
	title = GTK_LABEL (glade_xml_get_widget (gui, "label_info_title"));
	author = GTK_LABEL (glade_xml_get_widget (gui, "label_info_author"));
	version = GTK_LABEL (glade_xml_get_widget (gui, "label_info_version"));
	book_info->path = GTK_ENTRY (glade_xml_get_widget (gui, "entry_info_path"));

	gtk_object_unref (GTK_OBJECT (gui));

	row = dialog->clist_visible->focus_row;
	book_info->book = BOOK (gtk_clist_get_row_data (dialog->clist_visible, row));

	/* Set info in lables */
	gtk_label_set_text (title, book_get_title (book_info->book));
	gtk_label_set_text (name, book_get_name (book_info->book));
	gtk_label_set_text (author, book_get_author (book_info->book));
	gtk_label_set_text (version, book_get_version (book_info->book));	
	gtk_entry_set_text (book_info->path, book_get_path (book_info->book));
	
	gtk_widget_show (book_info->dialog);
}

static void
update_clist (BooksDialog *dialog)
{
	Book         *book;
	Bookshelf    *bookshelf;
	GtkCList     *clist;
	GSList       *list;
	gchar        *tmp[1];
	gint          row;
	gboolean      visible;

	g_return_if_fail (dialog != NULL);
	
	bookshelf = dialog->devhelp->bookshelf;
	
	list = bookshelf_get_books (bookshelf);

	gtk_clist_clear (dialog->clist_visible);
	gtk_clist_clear (dialog->clist_hidden);
	
	for (; list; list = list->next) {
		book = BOOK (list->data);
		
		tmp[0] = (gchar*)book_get_name_full (book);

		/* TODO: Fix-li-fix */
		visible = TRUE;
		
		/* Put it in the correct clist */
		if (visible) {
			clist = dialog->clist_visible;
		} else {
			clist = dialog->clist_hidden;
		}
		
		row = gtk_clist_append (clist, tmp);
		gtk_clist_set_row_data (clist, row, book);
	}
	
	gtk_clist_sort (dialog->clist_visible);
	gtk_clist_sort (dialog->clist_hidden);	
}

static void
addbook_dialog_clicked_cb (GtkButton *button, GtkFileSelection *selector)
{
	BooksDialog *dialog;
	gchar       *dirname;
	gchar       *filename;
	gchar       *root;
	struct stat  sb;
	
	dialog = gtk_object_get_data (GTK_OBJECT (selector), "devhelp");
		
	filename = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (selector)));

	/* Change into directory if that's what user selected. */
	if ((stat (filename, &sb) == 0) && S_ISDIR (sb.st_mode)) {
		gchar *last_slash = strrchr (filename, '/');
		
		/* The file selector needs a '/' at the end of a
		 * directory name.
		 */
		if (!last_slash || *(last_slash + 1) != '\0') {
			dirname = g_strconcat (filename, "/", NULL);
		} else {
			dirname = g_strdup (filename);
		}
		gtk_file_selection_set_filename (selector, dirname);
		g_free (dirname);
	}
	else {
		gint pos = strlen (filename)-1;
		
		gtk_widget_destroy (GTK_WIDGET (selector));
		
		while (filename[pos--] != '/');
		dirname = g_strndup (filename, pos+1);

		chdir (dirname);
		g_free (dirname);
		
		/* Install the book in the users home directory */
		root = g_strdup_printf ("%s/.devhelp", getenv ("HOME"));
		install_book (dialog->devhelp, filename, root);
		
		update_clist (dialog);
		g_free (root);
	}
}

static gboolean 
addbook_key_event (GtkFileSelection *file_sel, GdkEventKey *event)
{
	if (event->keyval == GDK_Escape) {
		gtk_button_clicked (GTK_BUTTON (file_sel->cancel_button));
		return TRUE;
	} else
		return FALSE;
}

static void
books_button_install_clicked_cb (GtkWidget *widget,
				 gpointer   user_data)
{
	BooksDialog *dialog;
	GtkWidget   *selector;
	
	g_return_if_fail (user_data != NULL);
	
	dialog = BOOKS_DIALOG (user_data);
	puts (__FUNCTION__);
	
	selector = gtk_file_selection_new (_("Choose a book..."));
	gtk_window_set_wmclass (GTK_WINDOW (selector), "FileSel", "DevHelp"); 
	
	gtk_object_set_data (GTK_OBJECT (selector), "devhelp", dialog);
	
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (selector)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (addbook_dialog_clicked_cb), selector);
	
	gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (selector)->cancel_button),
				   "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
				   (gpointer) selector);
	
	gtk_signal_connect (GTK_OBJECT (selector), "key_press_event",
			    GTK_SIGNAL_FUNC (addbook_key_event), NULL);
	
	gtk_widget_show (selector);
}

static gboolean
books_destroy_cb (GtkWidget *widget, BooksDialog *dialog)
{
	gconf_client_remove_dir (dialog->client, "/apps/devhelp", NULL);
	g_free (dialog);
}

static void
books_button_close_clicked_cb (GtkWidget *button,
			       gpointer   user_data)
{
	BooksDialog *dialog;

	g_return_if_fail (user_data != NULL);

	dialog = BOOKS_DIALOG (user_data);

	gtk_widget_destroy (dialog->dialog);
	dialog->dialog = NULL;
}

static void
books_button_sidebar_toggled_cb (GtkToggleButton *tb, BooksDialog *dialog)
{
//	preferences_set_sidebar_visible (dialog->books,
//					 gtk_toggle_button_get_active (tb));
}

void
menu_options_books_activate_cb (GtkMenuItem *menu_item,
				DevHelp     *devhelp)
{
	BooksDialog *dialog;
	GladeXML    *gui;
	GtkWidget   *w;

	g_return_if_fail (devhelp != NULL);

	dialog = g_new (BooksDialog, 1);

	dialog->client = gconf_client_get_default ();
	gconf_client_add_dir (dialog->client, "/apps/devhelp",
			      GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);

	gui = glade_xml_new (DATA_DIR "/devhelp/glade/devhelp.glade", "books_dialog");
	
	dialog->dialog = glade_xml_get_widget (gui, "books_dialog");
	gtk_signal_connect (GTK_OBJECT (dialog->dialog), "destroy",
			    GTK_SIGNAL_FUNC (books_destroy_cb), dialog);
	
	dialog->prefs = devhelp->preferences;
	dialog->devhelp = devhelp;

	dialog->clist_hidden = GTK_CLIST (glade_xml_get_widget (gui, "books_clist_hidden"));
	dialog->clist_visible = GTK_CLIST (glade_xml_get_widget (gui, "books_clist_visible"));

	w = glade_xml_get_widget (gui, "books_button_add");
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (books_button_show_clicked_cb), dialog);

	w = glade_xml_get_widget (gui, "books_button_remove");
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (books_button_hide_clicked_cb), dialog);

	w = glade_xml_get_widget (gui, "books_button_edit");
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (books_button_edit_clicked_cb), dialog);

	w = glade_xml_get_widget (gui, "books_button_install");
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (books_button_install_clicked_cb), dialog);

	w = glade_xml_get_widget (gui, "books_button_close");
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (books_button_close_clicked_cb), dialog);

	gtk_object_unref (GTK_OBJECT (gui));

/*	gconf_client_notify_add (
		dialog->client, "/apps/devhelp/sidebar_visible",
		gconf_sidebar_visible_changed_cb, dialog,
		NULL, NULL);
*/
	update_clist (dialog);
	gtk_widget_show_all (dialog->dialog);
}

