/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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
 * Author: Johan Dahlin <zilch.am@home.se>
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glib.h>
#include <gtk/gtkwidget.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>

#include "install.h"
#include "book.h"

static void
gnome_message (const gchar *message)
{
	GtkWidget *widget;
	
	widget = GTK_WIDGET (gnome_ok_dialog (message));
	gtk_widget_show (widget);
}

static void
install_create_directory (const gchar *directory)
{
	GnomeVFSResult  result;
	
	result = gnome_vfs_make_directory (directory,
					   GNOME_VFS_PERM_USER_ALL |
					   GNOME_VFS_PERM_GROUP_READ |
					   GNOME_VFS_PERM_GROUP_EXEC |
					   GNOME_VFS_PERM_OTHER_READ |
					   GNOME_VFS_PERM_OTHER_EXEC);					   

	if (result != GNOME_VFS_OK) {
		g_error ("Could not create directory %s.", directory);
	}
}

void
install_create_directories (const gchar *root)
{
	gchar       *cmd;
	gchar       *real_root;
	
	if (root == NULL) {
		real_root = g_strdup_printf ("%s/.devhelp", getenv ("HOME"));
	} else {
		real_root = g_strdup (root);
	}
	
	/* Check so all directories are created ...
	   ~/.devhelp */
	if (g_file_exists (real_root) == FALSE) {
		install_create_directory (real_root);
	}		
	
	cmd = g_strdup_printf ("%s/books", real_root);
	if (g_file_exists (cmd) == FALSE) {
		install_create_directory (cmd);
	}
	g_free (cmd);
	
	cmd = g_strdup_printf ("%s/specs", real_root);
	if (g_file_exists (cmd) == FALSE) {
		install_create_directory (cmd);
	}
	g_free (cmd);
}

static gboolean
install_spec (const gchar *filename, const gchar *name, const gchar *root)
{
	GnomeVFSResult   result;
	gchar           *url;
	
	url = g_strdup_printf ("%s/specs/%s.devhelp", root, name);
	if (g_file_exists (url)) {
		gnome_message (_("The book is already installed."));
		g_free (url);
		return FALSE;
	} else {
		result = gnome_vfs_xfer_uri (gnome_vfs_uri_new (filename),
					     gnome_vfs_uri_new (url),
					     0,
					     GNOME_VFS_XFER_ERROR_MODE_ABORT,
					     GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
					     NULL,
					     NULL);

		if (result != GNOME_VFS_OK) {
			gnome_message (_("Failed to move book. This should never happen."));
			g_free (url);
			return FALSE;
		}
		g_free (url);
	}
       
	return TRUE;
}
	
static Book *
install_unpack_book (Bookshelf     *bookshelf, 
		     const gchar   *filename, 
		     const gchar   *root)
{
	Book             *book;
	GnomeVFSURI      *uri;
	GnomeVFSResult    result;
	const gchar      *name;
	gchar            *cmd;
	gchar            *old_uri;
	gchar            *dir;
	guint             retval;
	gboolean          status;
	FunctionDatabase *fd;

	g_return_val_if_fail (bookshelf != NULL, NULL);
	g_return_val_if_fail (IS_BOOKSHELF (bookshelf), NULL);

	fd = bookshelf_get_function_database (bookshelf);

	/* Create temporary directory */
	dir = g_strdup_printf ("%s/tmp", root);
	if (g_file_exists (dir) == FALSE) {
		install_create_directory (dir);
	}
	
	cmd = g_strdup_printf ("cd %s && gzip -dc -f \"%s\" | tar -xf - book.devhelp >& /dev/null", dir, filename);
	retval = system (cmd);
	if (retval != 0 && g_file_exists ("book.devhelp") != FALSE) {
		gnome_message (_("Failed to extract spec file from book."));
		g_free (cmd);
		g_free (dir);
		return NULL;
	}
	g_free (cmd);
	
	/* Create the book */
	cmd = g_strdup_printf ("%s/tmp/book.devhelp", root);
	uri = gnome_vfs_uri_new (cmd);
	book = book_new (uri, fd);
	name = book_get_name (book);
	g_free (cmd);

	if (name == NULL) {
		gnome_message (_("Error, the book is corrupted or very old."));
		g_free (dir);
		return NULL;
	}

	/* TODO: Is book installed */
	
	/* Extract the book */
	cmd = g_strdup_printf ("cd %s && gzip -dc -f \"%s\" | tar -xf - book >& /dev/null", dir, filename);
	retval = system (cmd);
	if (retval != 0 && g_file_exists ("book") != FALSE) {
		gnome_message (_("Failed to extract the book."));
		g_free (dir);
		return NULL;
	}	
	g_free (cmd);
	g_free (dir);

	/* Install the spec */
	cmd = g_strdup_printf ("%s/specs/%s.devhelp", root, book_get_name_full (book));
	if (g_file_exists (cmd)) {
		gnome_message (_("The book is already installed."));
		g_free (cmd);
		return NULL;
	} else {
		old_uri = g_strdup_printf ("%s/tmp/book.devhelp", root);
		result = gnome_vfs_move (old_uri, cmd, FALSE);

		if (result != GNOME_VFS_OK) {
			gnome_message (_("Failed to move book. This should never happen."));
			return NULL;
		}
		g_free (cmd);
		g_free (old_uri);
	}
	
	/* Set the new path to the book */
	cmd = g_strdup_printf ("%s/specs/%s.devhelp",
			       root, book_get_name_full (book));
	book_set_spec_file (book, cmd);
	g_free (cmd);
	
	/* Install the book */
	cmd = g_strdup_printf ("%s/books/%s", root, book_get_name_full (book));
	if (g_file_exists (cmd)) {
		gnome_message (_("The book is already installed."));
		g_free (cmd);
		
		return NULL;
	} else {
		old_uri = g_strdup_printf ("%s/tmp/book", root);
		result = gnome_vfs_move (old_uri, cmd, FALSE);

		if (result != GNOME_VFS_OK) {
			gnome_message (_("Failed to move book. This should never happen."));
			return NULL;
		}
	}
	
	g_free (cmd);
	g_free (old_uri);
	
	return book;
}

static void
install_insert_book (Bookshelf *bookshelf, Book *book, const gchar *root)
{
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));

	bookshelf_add_book (bookshelf, book);

        bookshelf_write_xml (bookshelf);
}

static void
install_cleanup (const gchar *root)
{
	gchar *cmd;

	cmd = g_strdup_printf ("rm -fr \"%s/tmp/book\"", root);
	system (cmd);
	g_free (cmd);

	cmd = g_strdup_printf ("rm -f \"%s/tmp/book.devhelp\"", root);
	system (cmd);
	g_free (cmd);	
}

gboolean
install_book (Bookshelf *bookshelf, const gchar *filename, const gchar* root)
{
	Book               *book;
	const gchar        *mime_type;
	const gchar        *name;
	gchar              *message;
	gchar              *xml_path;
	FunctionDatabase   *fd;
	
	install_create_directories (root);
	
	mime_type = gnome_vfs_mime_type_from_name (filename);
	fd = bookshelf_get_function_database (bookshelf);
	
	/* Is it a .tar.gz? */
	if (strcmp (mime_type, "application/x-compressed-tar") == 0) {
		book = install_unpack_book (bookshelf, filename, root);
	
		if (book == NULL) {
			install_cleanup (root);
			return FALSE;
		}
	} else if (strcmp (mime_type, "application/octet-stream") == 0) {
		book = book_new (gnome_vfs_uri_new (filename), fd);
		if (book == NULL) {
			message = g_strdup_printf (_("Wrong type (mime_type=%s)."), mime_type);
			gnome_message (message);
			g_free (message);
			return FALSE;
		}
		name = book_get_name (book);
		if (install_spec (filename, name, root) == FALSE) {
			return FALSE;
		}
	} else {
		message = g_strdup_printf (_("Wrong type (mime_type=%s)."), mime_type);
		gnome_message (message);
		g_free (message);
		return FALSE;
	}

	if (bookshelf_have_book (bookshelf, book)) {
		gchar *tmp;
		
		gnome_message (_("The book is already installed."));
		tmp = g_strdup_printf ("rm -fr %s/books/%s",
				       root, book_get_name_full (book));
		system (tmp);
		g_free (tmp);

		tmp = g_strdup_printf ("rm -fr %s/specs/%s.devhelp",
				       root, book_get_name_full (book));
		system (tmp);
		g_free (tmp);		
		return FALSE;
	}

	install_insert_book (bookshelf, book, root);
	
	return TRUE;
}
