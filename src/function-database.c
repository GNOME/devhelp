/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 CodeFactory AB
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
 * Copyright (C) 2001 Johan Dahlin <jdahlin@home.se>
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
 * Author: Mikael Hallendal
 *
 * GNOME2 port by Johan Dahlin 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "function-database.h"
#include "devhelp-marshal.h"

#define d(x)

static void     fd_init                 (FunctionDatabase   *history);
static void     fd_class_init           (GObjectClass       *klass);
static void     fd_destroy              (GObject            *object);

static gint     fd_idle_search          (gpointer            data);
static gchar *  fd_function_complete    (gpointer            data);

static void     fd_function_free        (gpointer            data,
					 gpointer            user_data);

static void     fd_function_list_free   (GSList             *function_list);

enum {  
	GET_SEARCH_STRING,  
	EXACT_HIT_FOUND,
	HITS_FOUND,
	FUNCTION_REMOVED,
	LAST_SIGNAL  
};  

static gint signals[LAST_SIGNAL] = { 0 };

struct _FunctionDatabasePriv {
	GSList        *functions;
	GCompletion   *function_completion;
        
	guint          idle_search;
	
	gboolean       frozen;
};

GType
function_database_get_type (void)
{
	static GType function_database_type = 0;
        
	if (!function_database_type) {
                static const GTypeInfo function_database_info = {
                	sizeof (FunctionDatabaseClass),
			NULL,
			NULL,
			(GClassInitFunc)  fd_class_init,
			NULL,
			NULL,
			sizeof (FunctionDatabase),
			0,
			(GInstanceInitFunc) fd_init,
                };
		
		function_database_type = g_type_register_static (G_TYPE_OBJECT,
								 "FunctionDatabase",
								 &function_database_info, 0);

	}

	return function_database_type;
}

static void
fd_init (FunctionDatabase *fd)
{
	FunctionDatabasePriv   *priv;

	d(puts(__FUNCTION__));
        
	priv = g_new0 (FunctionDatabasePriv, 1);

	priv->function_completion = g_completion_new (fd_function_complete);
	priv->functions           = NULL;
	priv->idle_search         = 0;

	fd->priv = priv;
}

static void
fd_class_init (GObjectClass *klass)
{
	FunctionDatabaseClass   *fdc;

	d(puts(__FUNCTION__));

	fdc = (FunctionDatabaseClass *) klass;
        
//	klass->finalize = fd_destroy;

	signals[GET_SEARCH_STRING] =
		g_signal_new ("get_search_string",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FunctionDatabaseClass,
					       get_search_string),
			      NULL, NULL,
			      devhelp_marshal_STRING__VOID,
			      G_TYPE_STRING,
			      0);
	
	signals[EXACT_HIT_FOUND] =
		g_signal_new ("exact_hit_found",
			      G_TYPE_FROM_CLASS (klass),				
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FunctionDatabaseClass,
					       exact_hit_found),
			      NULL, NULL,				
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);
	signals[HITS_FOUND] =
		g_signal_new ("hits_found",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FunctionDatabaseClass,
					       hits_found),
			      NULL, NULL,			      
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);
	signals[FUNCTION_REMOVED] =
		g_signal_new ("function_removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FunctionDatabaseClass,
					       function_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);	

//	gtk_object_class_add_signals (klass, signals, LAST_SIGNAL);
}
	
static void
fd_destroy (GObject *object)
{
	FunctionDatabase       *fd;
	FunctionDatabasePriv   *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (object));
	
	d(puts(__FUNCTION__));
        
	fd   = FUNCTION_DATABASE (object);
	priv = fd->priv;

	g_completion_free (priv->function_completion);
	
	fd_function_list_free (priv->functions);
	/* FIX: Clean up priv */

	g_free (priv);
        
	fd->priv = NULL;
}

static gint
fd_idle_search (gpointer data)
{
	FunctionDatabase       *fd;
	FunctionDatabasePriv   *priv;
	gchar                  *search_string;
	GSList                 *hits;
	
	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (IS_FUNCTION_DATABASE (data), FALSE);
	
	fd   = FUNCTION_DATABASE (data);
	priv = fd->priv;
	
	d(puts(__FUNCTION__));
        
	gtk_signal_emit (G_OBJECT (fd),
			 signals[GET_SEARCH_STRING],
			 &search_string);

	if (search_string) {
		function_database_search (fd, search_string);
	}

	priv->idle_search = 0;

	return FALSE;
}

static gchar *
fd_function_complete (gpointer data)
{
	Function   *function;
	
	g_return_val_if_fail (data != NULL, NULL);
	
	d(puts(__FUNCTION__));
        
	function = (Function *) data;
        
	return function->name;
}

static void
fd_function_free (gpointer data, gpointer user_data)
{
	Function   *function;
	
	g_return_if_fail (data != NULL);

	function = (Function *) data;

	g_free (function->name);
	if (function->anchor) {
		g_free (function->anchor);
	}
	
	g_free (function);
}

static void
fd_function_list_free (GSList *function_list)
{
	g_slist_foreach (function_list, fd_function_free, NULL);
	g_slist_free (function_list);
}

FunctionDatabase *
function_database_new (void)
{
	d(puts(__FUNCTION__));
        
	return g_object_new (TYPE_FUNCTION_DATABASE, NULL);
}

void
function_database_idle_search (FunctionDatabase *fd)
{
	FunctionDatabasePriv   *priv;
        
	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));

	d(puts(__FUNCTION__));
        
	priv = fd->priv;
        
	if (!priv->idle_search) {
		priv->idle_search = gtk_idle_add (fd_idle_search, fd);
	}
}

static gint
function_compare (gconstpointer a,
		  gconstpointer b)
{
	const GSList *la, *lb;

	la = a;
	lb = b;

	return strcmp (la->data, lb->data);
}

gboolean
function_database_search (FunctionDatabase *fd, const gchar *string)
{
	FunctionDatabasePriv   *priv;
	GSList                 *list;
	GSList                 *node;
	Function               *function;
	Function               *exact_hit = NULL;
	
	g_return_val_if_fail (fd != NULL, FALSE);
	g_return_val_if_fail (IS_FUNCTION_DATABASE (fd), FALSE);
	g_return_val_if_fail (string != NULL, FALSE);
	        
	d(puts(__FUNCTION__));
    
	priv = fd->priv;
	
	list = NULL;

	for (node = priv->functions; node; node = node->next) {
		function = (Function *) node->data;
                
		if (strstr (function->name, string)) {
			if (!strcmp (function->name, string)) {
				exact_hit = function;
			}
			
			list = g_slist_prepend (list, function);
		}
	}

	list = g_slist_sort (list, function_compare);

	if (list) {
		gtk_signal_emit (G_OBJECT (fd),
				 signals[HITS_FOUND],
				 list);
	}

	if (exact_hit) {
		gtk_signal_emit (G_OBJECT (fd),
				 signals[EXACT_HIT_FOUND],
				 exact_hit);
	}

        if (list) {
		g_slist_free (list);
		return TRUE;
	}
	
	return FALSE;
}

gchar *
function_database_get_completion (FunctionDatabase *fd, const gchar *string)
{
	FunctionDatabasePriv   *priv;
	gchar                  *text;
	
	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));

	d(puts(__FUNCTION__));
        
	priv = fd->priv;
        
	g_completion_complete (priv->function_completion, (gchar *)string, &text);
        
	return text;
}

Function *
function_database_add_function (FunctionDatabase    *fd,
				const gchar         *name,
				const Document      *document,
                                const gchar         *anchor)
{
	FunctionDatabasePriv   *priv;
	Function               *function;
	GList                  *list;
        
	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));
	g_return_if_fail (name != NULL);
	g_return_if_fail (document != NULL);

	d(puts(__FUNCTION__));
        
	priv     = fd->priv;
	function = g_new0 (Function, 1);

	function->name      = g_strdup (name);
	function->document  = document;
	
	if (anchor) {
		function->anchor = g_strdup (anchor);
	} else {
		function->anchor = NULL;
	}
	
	priv->functions = g_slist_prepend (priv->functions, function);
	
	if (!priv->frozen) {
		list = g_list_append (NULL, function);
		g_completion_add_items (priv->function_completion, list);
		g_list_free (list);
	}
	
	return function;
}

void
function_database_remove_function (FunctionDatabase    *fd,
				   Function            *function)
{
	FunctionDatabasePriv   *priv;
	GList                  *list;
        
	d(puts(__FUNCTION__));
        
	priv = fd->priv;
	
	priv->functions = g_slist_remove (priv->functions, function);
	if (!priv->frozen) {
		list = g_list_append (NULL, function);
		g_completion_remove_items (priv->function_completion, list);
		g_list_free (list);
	}
	
	gtk_signal_emit (G_OBJECT (fd),
			 signals[FUNCTION_REMOVED],
			 function);
}

void
function_database_freeze (FunctionDatabase *fd)
{
	FunctionDatabasePriv   *priv;
	
	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));
	
	priv = fd->priv;
	
	priv->frozen = TRUE;
}


void
function_database_thaw (FunctionDatabase *fd)
{
	FunctionDatabasePriv   *priv;
	
	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));
	
	priv = fd->priv;

	if (!priv->frozen) {
		return;
	}

	/* This is necessary to get rid of a warning if no books are found */
	if (priv->functions == NULL)
		return;

	g_completion_clear_items (priv->function_completion);
	g_completion_add_items (priv->function_completion,
				(GList *)priv->functions);
	
	priv->frozen = FALSE;
}
