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

#ifndef __BOOKMARK_MANAGER_H__
#define __BOOKMARK_MANAGER_H__

#include <gtk/gtkobject.h>
#include <gtk/gtktypeutils.h>

#define TYPE_BOOKMARK_MANAGER        (bookmark_manager_get_type ())
#define BOOKMARK_MANAGER(o)          (GTK_CHECK_CAST ((o), TYPE_BOOKMARK_MANAGER, BookmarkManager))
#define BOOKMARK_MANAGER_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), TYPE_BOOKMARK_MANAGER, BookmarkManager))
#define IS_BOOKMARK_MANAGER(o)       (GTK_CHECK_TYPE ((o), TYPE_BOOKMARK_MANAGER))
#define IS_BOOKMARK_MANAGER_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), TYPE_BOOKMARK_MANAGER))

typedef struct _BookmarkManager      BookmarkManager;
typedef struct _BookmarkManagerClass BookmarkManagerClass;
typedef struct _BookmarkManagerPriv  BookmarkManagerPriv;

typedef struct _Bookmark             Bookmark;

struct _BookmarkManager {
	GtkObject              parent;
	
	BookmarkManagerPriv   *priv;
};

struct _BookmarkManagerClass 
{
	GtkObjectClass         parent_class;

	/* Signals */
	
	void  (*bookmark_added)    (BookmarkManager   *bm,
				    Bookmark          *bookmark);
	void  (*bookmark_removed)  (BookmarkManager   *bm,
				    Bookmark          *bookmark);
};

struct _Bookmark 
{
	gchar   *name;
	gchar   *url;
};


GtkType           bookmark_manager_get_type      (void);
BookmarkManager * bookmark_manager_new           (void);

const Bookmark *  bookmark_manager_add           (BookmarkManager   *bm,
						  const gchar       *name,
						  const gchar       *url);

void              bookmark_manager_remove        (BookmarkManager   *bm,
						  Bookmark          *bookmark);

void              bookmark_manager_remove_name   (BookmarkManager   *bm,
						  const gchar       *name);

Bookmark *        bookmark_manager_lookup        (BookmarkManager   *bm,
						  const gchar       *name);

GSList *          bookmark_manager_get_bookmarks (BookmarkManager   *bm);

GSList *          bookmark_list_sort             (GSList            *list);

#endif /* __BOOKMARK_MANAGER_H__ */
