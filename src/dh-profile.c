/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
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
 *
 * Author: Mikael Hallendal <micke@codefactory.se>
 */

#include <config.h>

#include <libgnomevfs/gnome-vfs.h>

#include "dh-profile.h"

#define d(x) x

struct _DhProfile {
        gchar  *name;
        
        GSList *books;
};

static gboolean   profile_find_books (GSList        **books, 
				      const gchar    *directory);

static gboolean
profile_find_books (GSList **books, const gchar *directory)
{
	GList            *dir_list;
	GList            *l;
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	gchar            *book_file;
	
	g_return_val_if_fail (directory != NULL, FALSE);

	result  = gnome_vfs_directory_list_load (&dir_list, directory,
						 GNOME_VFS_FILE_INFO_DEFAULT);

	d(g_print ("Calling profile_find_books with: '%s'\n", directory));

	if (result != GNOME_VFS_OK) {
		return FALSE;
	}

	for (l = dir_list; l; l = l->next) {
		int len;

		info = (GnomeVFSFileInfo *) l->data;

		if (strlen (info->name) <= 8 ||
		    (strncmp (info->name + (strlen (info->name) - 9), ".devhelp2", 9) != 0 && 
		     strncmp (info->name + (strlen (info->name) - 8), ".devhelp", 8) != 0)) {
			gnome_vfs_file_info_unref (info);
			continue;
		}

		book_file = g_strconcat (directory, "/", info->name, NULL);
		d(g_print ("Found book: '%s'\n", book_file));
		
		*books = g_slist_prepend (*books, book_file);

		gnome_vfs_file_info_unref (info);
	}

	g_list_free (dir_list);

	return TRUE;
}

/* For now, return a profile containing the old hardcoded list */
DhProfile *
dh_profile_new (void)
{
	DhProfile *profile;
	gchar     *dir;
	
	profile = g_new0 (DhProfile, 1);
	profile->books = NULL;

	/* Fill profile->books with $(home)/.devhelp/specs and   *
	 * $(prefix)/share/devhelp/specs                         */

	dir = g_strconcat (g_getenv ("HOME"), "/.devhelp2/books", NULL);
	profile_find_books (&profile->books, dir);
	g_free (dir);

	dir = g_strconcat (g_getenv ("HOME"), "/.devhelp/specs", NULL);
	profile_find_books (&profile->books, dir);
	g_free (dir);
	
	profile_find_books (&profile->books, DATADIR"/devhelp/specs");
	profile_find_books (&profile->books, "/usr/share/devhelp/specs");

	return profile;
}

GNode *   
dh_profile_open (DhProfile *profile, GList *keyword, GError **error)
{
	GNode *root;
	GList *keywords;

	root = g_node_new (NULL);

	if (!dh_book_parser_read_books (profile->books,
					root,
					&keywords,
					error)) {
		return NULL;
	}

	return root;
}

