/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 CodeFactory AB
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

#ifndef __FUNCTION_DATABASE_H__
#define __FUNCTION_DATABASE_H__

#include <gtk/gtkobject.h>
#include <gtk/gtktypeutils.h>
#include <libgnomevfs/gnome-vfs.h>
#include "book-node.h"

#define TYPE_FUNCTION_DATABASE        (function_database_get_type ())
#define FUNCTION_DATABASE(o)          (GTK_CHECK_CAST ((o), TYPE_FUNCTION_DATABASE, FunctionDatabase))
#define FUNCTION_DATABASE_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), TYPE_FUNCTION_DATABASE, FunctionDatabaseClass))
#define IS_FUNCTION_DATABASE(o)       (GTK_CHECK_TYPE ((o), TYPE_FUNCTION_DATABASE))
#define IS_FUNCTION_DATABASE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), TYPE_FUNCTION_DATABASE))

typedef struct _FunctionDatabase      FunctionDatabase;
typedef struct _FunctionDatabaseClass FunctionDatabaseClass;
typedef struct _FunctionDatabasePriv  FunctionDatabasePriv;

typedef struct _Function              Function;

struct _FunctionDatabase {
	GtkObject               parent;
	
	FunctionDatabasePriv   *priv;
};

struct _FunctionDatabaseClass {
	GtkObjectClass          parent_class;

	/* Signals */
	gchar *       (*get_search_string)    (FunctionDatabase   *fd);
	
	/* Query done signals */
	void          (*exact_hit_found)      (FunctionDatabase   *fd,
					       Function           *function);
	void          (*hits_found)           (FunctionDatabase   *fd,
					       GSList             *hits);

	void          (*function_removed)     (FunctionDatabase   *fd,
					       Function           *function);
};

struct _Function {
	gchar            *name;
	const Document   *document;
	gchar            *anchor;
};

GtkType             function_database_get_type      (void);
FunctionDatabase *  function_database_new           (void);

void       function_database_idle_search       (FunctionDatabase    *fd);
 
gboolean   function_database_search            (FunctionDatabase    *fd,
						const gchar         *func_name);
gchar *    function_database_get_completion    (FunctionDatabase    *fd,
						const gchar         *string);

Function * function_database_add_function      (FunctionDatabase    *fd,
						const gchar         *name,
						const Document      *document,
						const gchar         *anchor);

void       function_database_remove_function   (FunctionDatabase    *fd,
						Function            *function);

void       function_database_freeze            (FunctionDatabase    *fd);
void       function_database_thaw              (FunctionDatabase    *fd);

void       function_free                       (Function            *function);

#endif /* __FUNCTION_DATABASE_H__ */
