/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
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

#include <config.h>
#include <libgnomevfs/gnome-vfs.h>

#include "dh-book-old.h"
#include "dh-profile.h"
#include "dh-error.h"

#define d(x)

struct _DhProfilePriv {
        gchar    *name;
	GSList   *books;
        GNode    *contents;
	GList    *keywords;

	gboolean  open;
};

static gboolean   profile_find_books    (DhProfile      *profile,
					 const gchar    *directory);

static void       profile_init          (DhProfile      *profile);
static void       profile_class_init    (GObjectClass   *klass);
static void       profile_finalize      (GObject        *object);

static GObjectClass *parent_class = NULL;

GType
dh_profile_get_type (void)
{
	static GType type = 0;
        
	if (!type) {
		static const GTypeInfo info = {
			sizeof (DhProfileClass),
			NULL,
			NULL,
			(GClassInitFunc) profile_class_init,
			NULL,
			NULL,
			sizeof (DhProfile),
			0,
			(GInstanceInitFunc) profile_init,
		};
                
		type = g_type_register_static (G_TYPE_OBJECT,
					       "DhProfile",
					       &info, 0);
	}

	return type;
}

static void
profile_init (DhProfile *profile)
{
	DhProfilePriv *priv;

	d(puts(__FUNCTION__));
        
	priv = g_new0 (DhProfilePriv, 1);
        
	priv->name      = NULL;
	priv->contents  = NULL;
	priv->keywords  = NULL;
	priv->open      = FALSE;
	
	profile->priv   = priv;
}

static void
profile_class_init (GObjectClass *klass)
{
        parent_class = g_type_class_peek_parent (klass);

	klass->finalize = profile_finalize;
}

static void
profile_finalize (GObject *object)
{
	g_print ("FIXME: Free profile\n");
	
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		G_OBJECT_CLASS (parent_class)->finalize (object);
	}
}

static gboolean
profile_find_books (DhProfile *profile, const gchar *directory)
{
	DhProfilePriv    *priv;
	GList            *dir_list;
	GList            *l;
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	gchar            *book_file;
	
	g_return_val_if_fail (DH_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (directory != NULL, FALSE);

	priv = profile->priv;

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
		
		priv->books = g_slist_prepend (priv->books, book_file);

		gnome_vfs_file_info_unref (info);
	}

	g_list_free (dir_list);

	return TRUE;
}

/* For now, return a profile containing the old hardcoded list */
DhProfile *
dh_profile_new (void)
{
	DhProfile     *profile;
	DhProfilePriv *priv;
	gchar         *dir;	
	

	profile = g_object_new (DH_TYPE_PROFILE, NULL);
	priv    = profile->priv;
	
	priv->name = g_strdup ("Default");

	/* Fill profile->books with $(home)/.devhelp/specs and   *
	 * $(prefix)/share/devhelp/specs                         */

	dir = g_strconcat (g_getenv ("HOME"), "/.devhelp2/books", NULL);
	profile_find_books (profile, dir);
	g_free (dir);

	dir = g_strconcat (g_getenv ("HOME"), "/.devhelp/specs", NULL);
	profile_find_books (profile, dir);
	g_free (dir);
	
	profile_find_books (profile, DATADIR"/devhelp/specs");
	profile_find_books (profile, "/usr/share/devhelp/specs");

	return profile;
}

GNode *   
dh_profile_open (DhProfile *profile, GList **keywords, GError **error)
{
	DhProfilePriv *priv;
	GSList        *l;
	GString       *failures;
	
	g_return_val_if_fail (DH_IS_PROFILE (profile), NULL);

	priv = profile->priv;

	if (priv->open) {
		*keywords = priv->keywords;
		return priv->contents;
	}

	failures = g_string_new ("");
	
	priv->contents = g_node_new (NULL);

	for (l = priv->books; l; l = l->next) {
		GError *load_error = NULL;
		if (!dh_book_read ((const gchar *) l->data,
				   NULL,
				   priv->contents,
				   &priv->keywords,
				   &load_error)) {
			g_string_append (failures, load_error->message);
			g_string_append_c (failures, '\n');
			g_clear_error (&load_error);
		}

		/*g_message ("Pulse");*/
	}

	priv->open = TRUE;
	*keywords = priv->keywords;

	if (failures->len > 0) {
		g_set_error (error,
			     DH_ERROR, DH_ERROR_MALFORMED_BOOK,
			     g_string_free (failures, FALSE));
	}

	return priv->contents;
}

GSList *
dh_profiles_init (void)
{
	GSList    *profiles = NULL;
	DhProfile *profile;
	
	profile = dh_profile_new ();
	profile->priv->name = g_strdup ("Default");

	profiles = g_slist_prepend (profiles, profile);
	
	return profiles;
}

