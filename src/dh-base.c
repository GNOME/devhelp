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
 */

#include <config.h>

#include <string.h>
#include <libgnomevfs/gnome-vfs.h>
#include <gtk/gtkmain.h>

#include "dh-window.h"
#include "dh-parser.h"
#include "dh-base.h"

#define d(x)

struct _DhBasePriv {
	GSList *windows;

	GNode  *book_tree;
	GList  *keywords;
	
	GHashTable *books;
};

static void        base_init                  (DhBase         *base);
static void        base_class_init            (DhBaseClass    *klass);
#if 0
static void        base_new_window_cb         (DhWindow       *window,
					       DhBase         *base);
#endif
static void        base_window_finalized_cb   (DhBase         *base,
					       DhWindow       *window);
static void        base_init_books            (DhBase         *base);
static void        base_add_books             (DhBase         *base,
					       const gchar    *directory);

static GObjectClass *parent_class;

GType
dh_base_get_type (void)
{
	static GType type = 0;
        
	if (!type) {
		static const GTypeInfo info = {
			sizeof (DhBaseClass),
			NULL,
			NULL,
			(GClassInitFunc) base_class_init,
			NULL,
			NULL,
			sizeof (DhBase),
			0,
			(GInstanceInitFunc) base_init,
		};
                
		type = g_type_register_static (G_TYPE_OBJECT, "DhBase",
					       &info, 0);
	}

	return type;
}

static void
base_init (DhBase *base)
{ 
        DhBasePriv *priv;

        priv = g_new0 (DhBasePriv, 1);
        
	priv->windows   = NULL;
	priv->book_tree = g_node_new (NULL);
	priv->keywords  = NULL;
	priv->books     = g_hash_table_new_full (g_str_hash, g_str_equal, 
						 g_free, g_free);
        base->priv      = priv;
}

static void
base_class_init (DhBaseClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);
}

#if 0
static void
base_new_window_cb (DhWindow *window, DhBase *base)
{
	GtkWidget *new_window;
	
	g_return_if_fail (DH_IS_WINDOW (window));
	g_return_if_fail (DH_IS_BASE (base));
	
	new_window = dh_base_new_window (base);
	
	gtk_widget_show_all (new_window);
}
#endif

static void
base_window_finalized_cb (DhBase *base, DhWindow *window)
{
	DhBasePriv *priv;
	
	g_return_if_fail (DH_IS_BASE (base));
	
	priv = base->priv;
	
	priv->windows = g_slist_remove (priv->windows, window);

	if (g_slist_length (priv->windows) == 0) {
		gtk_main_quit ();
	}
}

static void
base_init_books (DhBase *base)
{
	const gchar *env;
	gchar *dir;
	
	env = g_getenv ("DEVHELP_SEARCH_PATH");
	if (env) {
		gchar **paths, **p;
		/* Insert all books from this path first */
		paths = g_strsplit (env, ":", -1);
		for (p = paths; *p != NULL; p++) {
			base_add_books (base, *p);
		}
		g_strfreev (paths);
	}
	
	env = g_getenv ("GNOME2_PATH");
	if (env) {
		base_add_books (base, env);
	}
	
	/* Insert the books from default gtk-doc install path */
	base_add_books (base, DATADIR"/gtk-doc/html");
	base_add_books (base, "/usr/share/gtk-doc/html");
	base_add_books (base, DATADIR"/devhelp/books");
	dir = g_strconcat (g_getenv ("HOME"), "/.devhelp/books", NULL);
	base_add_books (base, dir);
	g_free (dir);
}


static void
base_add_books (DhBase *base, const gchar *directory)
{
	DhBasePriv       *priv;
	GList            *dir_list;
	GList            *l;
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	
	priv = base->priv;

	d(g_print ("Adding books from %s\n", directory));
 
	result  = gnome_vfs_directory_list_load (&dir_list, directory,
						 GNOME_VFS_FILE_INFO_DEFAULT);

	if (result != GNOME_VFS_OK) {
		if (result == GNOME_VFS_ERROR_NOT_FOUND) {
			d(g_print ("Cannot find %s\n", directory));
			return;
		}
		g_print ("Cannot read directory %s: %s",
			directory, gnome_vfs_result_to_string (result));
		return;
	}

	for (l = dir_list; l; l = l->next) {
		gchar *book_path;
		GError *error = NULL;
		
		info = (GnomeVFSFileInfo *) l->data;
		
		if (g_hash_table_lookup (priv->books, info->name)) {
			gnome_vfs_file_info_unref (info);
			continue;
		}
		
		book_path = g_strdup_printf ("%s/%s/%s.devhelp",
					     directory, 
					     info->name, info->name);

		if (g_file_test (book_path, G_FILE_TEST_EXISTS)) {
			g_hash_table_insert (priv->books,
					     g_strdup (info->name),
					     book_path);
			d(g_print ("Found book: '%s'\n", book_path));
			
			if (!dh_parse_file  (book_path,
					     priv->book_tree,
					     &priv->keywords,
					     &error)) {
				g_warning ("Failed to read '%s': %s", book_path, error->message);
				g_error_free (error);
				error = NULL;
			}
			goto next;
		}

#ifdef HAVE_LIBZ
		g_free (book_path);
		book_path = g_strdup_printf ("%s/%s/%s.devhelp.gz",
					     directory, 
					     info->name, info->name);

		if (g_file_test (book_path, G_FILE_TEST_EXISTS)) {
			g_hash_table_insert (priv->books,
					     g_strdup (info->name),
					     book_path);
			d(g_print ("Found book: '%s'\n", book_path));
			
			if (!dh_parse_gz_file  (book_path,
					     priv->book_tree,
					     &priv->keywords,
					     &error)) {
				g_warning ("Failed to read '%s': %s", book_path, error->message);
				g_error_free (error);
				error = NULL;
			}
			goto next;
		}
#endif
	next:
		gnome_vfs_file_info_unref (info);
		g_free (book_path);
	}

	g_list_free (dir_list);
}

DhBase *
dh_base_new (void)
{
        DhBase     *base;
	DhBasePriv *priv;
	
        base = g_object_new (DH_TYPE_BASE, NULL);
	priv = base->priv;
	
	base_init_books (base);

	return base;
}

GtkWidget *
dh_base_new_window (DhBase *base)
{
	DhBasePriv *priv;
	GtkWidget  *window;
        
        g_return_val_if_fail (DH_IS_BASE (base), NULL);

	priv = base->priv;
        
        window = dh_window_new (base);
        
	priv->windows = g_slist_prepend (priv->windows, window);

	g_object_weak_ref (G_OBJECT (window),
			   (GWeakNotify) base_window_finalized_cb,
			   base);
	
	/*g_signal_connect (window, "new_window_requested",
			  G_CALLBACK (base_new_window_cb),
			  base);*/

	gtk_widget_show_all (window);

	return window;
}

GNode *
dh_base_get_book_tree (DhBase *base)
{
	g_return_val_if_fail (DH_IS_BASE (base), NULL);
	
	return base->priv->book_tree;
}

GList *
dh_base_get_keywords (DhBase *base)
{
	g_return_val_if_fail (DH_IS_BASE (base), NULL);
	
	return base->priv->keywords;
}

GSList *
dh_base_get_windows (DhBase *base)
{
	DhBasePriv          *priv;

	g_return_val_if_fail (DH_IS_BASE (base), NULL);
	
	priv = base->priv;
	
	return priv->windows;
}


