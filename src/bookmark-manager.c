/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <gtk/gtksignal.h>

#include "bookmark-manager.h"

#define d(x)

static void  bm_init                 (BookmarkManager   *bm);
static void  bm_class_init           (GtkObjectClass    *klass);
static void  bm_destroy              (GtkObject         *object);
static void  bm_add_bookmark_to_list (gpointer           key, 
				      gpointer           value, 
				      gpointer           user_data);
static void  bookmark_free           (Bookmark          *bookmark);
static gint  bookmark_list_compare   (gconstpointer      a,
				      gconstpointer      b);

enum {  
	BOOKMARK_ADDED,
	BOOKMARK_REMOVED,
	LAST_SIGNAL  
};  

static gint signals[LAST_SIGNAL] = { 0 }; 

struct _BookmarkManagerPriv {
	GHashTable    *bookmarks;

	gint           next_id;
};

GtkType
bookmark_manager_get_type (void)
{
	static GtkType bookmark_manager_type = 0;
	
	if (!bookmark_manager_type) {
		static const GtkTypeInfo bookmark_manager_info = {
			"BookmarkManager",
			sizeof (BookmarkManager),
			sizeof (BookmarkManagerClass),
			(GtkClassInitFunc)  bm_class_init,
			(GtkObjectInitFunc) bm_init,
			NULL, /* -- Reserved -- */
			NULL, /* -- Reserved -- */
			(GtkClassInitFunc) NULL,
		};
                
		bookmark_manager_type = gtk_type_unique (GTK_TYPE_OBJECT, 
							 &bookmark_manager_info);
	}

	return bookmark_manager_type;
}

static void
bm_init (BookmarkManager *bm)
{
	BookmarkManagerPriv   *priv;
	
	d(puts(__FUNCTION__));
        
	priv            = g_new0 (BookmarkManagerPriv, 1);
	priv->bookmarks = g_hash_table_new (g_str_hash, g_str_equal);
	priv->next_id   = 0;

	bm->priv        = priv;
}

static void
bm_class_init (GtkObjectClass *klass)
{
	d(puts(__FUNCTION__));

	klass->destroy = bm_destroy;

	signals[BOOKMARK_ADDED] = 
		gtk_signal_new ("bookmark_added",
				G_TYPE_FROM_CLASS (klass),
				GTK_RUN_LAST,
				GTK_SIGNAL_OFFSET (BookmarkManagerClass, 
						   bookmark_added),
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE,
				1, GTK_TYPE_POINTER);
	
	signals[BOOKMARK_REMOVED] =
		gtk_signal_new ("bookmark_removed",
				G_TYPE_FROM_CLASS (klass),				
				GTK_RUN_LAST,
				GTK_SIGNAL_OFFSET (BookmarkManagerClass,
						   bookmark_removed),
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE,
				1, GTK_TYPE_POINTER);
	
//	gtk_object_class_add_signals (klass, signals, LAST_SIGNAL);
}

static void
bm_destroy (GtkObject *object)
{
	BookmarkManager       *bm;
	BookmarkManagerPriv   *priv;
	
	bm   = BOOKMARK_MANAGER (object);
	priv = bm->priv;

	/* FIX: Free the keys and values */

	g_hash_table_destroy (priv->bookmarks);
	g_free (priv);

	bm->priv = NULL;
}

static void
bookmark_free (Bookmark *bookmark) 
{
	g_return_if_fail (bookmark != NULL);
	
	g_free (bookmark->name);
	g_free (bookmark->anchor);
	g_free (bookmark);
}

static gint
bookmark_list_compare (gconstpointer a, gconstpointer b)
{
	g_return_val_if_fail (a != NULL, 1);
	g_return_val_if_fail (b != NULL, -1);
	
	return g_strcasecmp (((Bookmark *) a)->name, ((Bookmark *) b)->name);
}

static void
bm_add_bookmark_to_list (gpointer key, gpointer value, gpointer user_data)
{
        GSList   **list;
        
        g_return_if_fail (value != NULL);
        g_return_if_fail (user_data != NULL);
        
        list = (GSList **) user_data;
        
        *list = g_slist_prepend (*list, value);
}

BookmarkManager *
bookmark_manager_new ()
{
	d(puts(__FUNCTION__));
        
	return gtk_type_new (TYPE_BOOKMARK_MANAGER);
}

const Bookmark *
bookmark_manager_add (BookmarkManager   *bm, 
		      const gchar       *name, 
		      const Document    *document,
		      const gchar       *anchor)
{
	BookmarkManagerPriv   *priv;
	Bookmark              *bookmark;
		
	g_return_val_if_fail (bm != NULL, NULL);
	g_return_val_if_fail (IS_BOOKMARK_MANAGER (bm), NULL);
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (document != NULL, NULL);
	
	priv = bm->priv;
	
	bookmark = g_new0 (Bookmark, 1);

	bookmark->name     = g_strstrip (g_strdup (name));
	bookmark->document = document;
	bookmark->anchor   = g_strdup (anchor);

	g_hash_table_insert (priv->bookmarks, bookmark->name, bookmark);

	gtk_signal_emit (GTK_OBJECT (bm), signals[BOOKMARK_ADDED], bookmark);

	return bookmark;
}

void
bookmark_manager_remove (BookmarkManager *bm, Bookmark *bookmark)
{
	bookmark_manager_remove_name (bm, bookmark->name);
}

void
bookmark_manager_remove_name (BookmarkManager   *bm,
			      const gchar       *name)
{
	BookmarkManagerPriv   *priv;
	Bookmark              *bookmark;
	gchar                 *key;

	g_return_if_fail (bm != NULL);
	g_return_if_fail (IS_BOOKMARK_MANAGER (bm));
	g_return_if_fail (name != NULL);

	priv = bm->priv;

	if (g_hash_table_lookup_extended (priv->bookmarks, name, 
					  (gpointer *)&key,
					  (gpointer *)&bookmark)) {
		g_hash_table_remove (priv->bookmarks, name);

		gtk_signal_emit (GTK_OBJECT (bm), 
				 signals[BOOKMARK_REMOVED],
				 bookmark);
						 
		g_free (key);
		bookmark_free (bookmark);
	}
}

Bookmark *
bookmark_manager_lookup (BookmarkManager   *bm, const gchar *name)
{
	BookmarkManagerPriv   *priv;
	
	g_return_val_if_fail (bm != NULL, NULL);
	g_return_val_if_fail (IS_BOOKMARK_MANAGER (bm), NULL);
	g_return_val_if_fail (name != NULL, NULL);
	
	priv = bm->priv;
	
	return g_hash_table_lookup (priv->bookmarks, name);
}

GSList *
bookmark_manager_get_bookmarks (BookmarkManager *bm)
{
	BookmarkManagerPriv   *priv;
	GSList                *list = NULL;
	
	g_return_val_if_fail (bm != NULL, NULL);
	g_return_val_if_fail (IS_BOOKMARK_MANAGER (bm), NULL);
	
	priv = bm->priv;
	
	g_hash_table_foreach (priv->bookmarks, bm_add_bookmark_to_list, &list);
	
	return list;
}

GSList *       
bookmark_list_sort (GSList *list)
{
	GSList *sorted_list;
	
	sorted_list = g_slist_sort (list, bookmark_list_compare);
	
	return sorted_list;
}

