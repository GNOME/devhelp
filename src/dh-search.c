/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@codefactory.se>
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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkeditable.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkscrolledwindow.h>
#include "function-database.h"
#include "dh-search.h"

#define MAX_HITS 250

#define d(x)

static void dh_search_class_init          (DhSearchClass   *klass);
static void dh_search_init                (DhSearch        *index);
 
static void dh_search_destroy             (GObject              *object);

static void dh_search_entry_activate_cb   (GtkEditable          *editable,
					   DhSearch        *search);

static void dh_search_entry_changed_cb    (GtkEditable          *editable,
					   DhSearch        *search);

static void dh_search_entry_insert_text_cb(GtkEditable          *editable,
					   gchar                *new_text,
					   gint                  length,
					   gint                 *pos,
					   DhSearch        *search);

static gboolean
dh_search_entry_key_press_cb              (GtkEditable          *editable,
					   GdkEventKey          *event,
					   DhSearch        *search);

static void dh_search_clist_select_row_cb (GtkCList             *clist,
					   gint                  row,
					   gint                  col,
					   GdkEvent             *event,
					   DhSearch        *search);

static gint dh_search_complete_idle       (gpointer              data);

static void dh_search_do_search           (DhSearch        *search, 
					   const gchar          *string);
static gchar * 
dh_search_get_search_string_cb            (FunctionDatabase     *fd,
					   DhSearch        *search);

static void dh_search_exact_hit_found_cb  (FunctionDatabase     *fd,
					   Function             *function,
					   DhSearch        *search);

static void dh_search_hits_found_cb       (FunctionDatabase     *fd,
					   GSList               *hits,
					   DhSearch        *search);


enum {
        URI_SELECTED,
        LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

struct _DhSearchPriv {
	GtkWidget           *entry; 
	GtkWidget           *clist; 

        DhBookshelf         *bookshelf;
	FunctionDatabase    *fd;

	guint                complete;
};

GType
dh_search_get_type (void)
{
        static GType dh_search_type = 0;

        if (!dh_search_type) {
                static const GTypeInfo dh_search_info = {
                        sizeof (DhSearchClass),
			NULL,
			NULL,
			(GClassInitFunc)  dh_search_class_init,
			NULL,
			NULL,
			sizeof (DhSearch),
			0,
			(GInstanceInitFunc) dh_search_init
                };
		dh_search_type = g_type_register_static (G_TYPE_OBJECT,
							      "DhSearch",
							      &dh_search_info,
							      0);
        }

        return dh_search_type;
}

static void
dh_search_class_init (DhSearchClass *klass)
{
        GObjectClass *object_class;

        object_class = (GObjectClass *) klass;

	object_class->finalize = dh_search_destroy;

        signals[URI_SELECTED] =
                g_signal_new ("uri_selected",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (DhSearchClass,
					       uri_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);
}

static void
dh_search_init (DhSearch *search)
{
        DhSearchPriv   *priv;
        
        priv                  = g_new0 (DhSearchPriv, 1);
        priv->bookshelf       = NULL;
        search->priv          = priv;
}

static void
dh_search_destroy (GObject *object)
{
        /* FIX: Do something */
}

static void
dh_search_entry_activate_cb (GtkEditable *editable, DhSearch *search)
{
        DhSearchPriv   *priv;
        
        g_return_if_fail (editable != NULL);
        g_return_if_fail (GTK_IS_EDITABLE (editable));
        g_return_if_fail (search != NULL);
        g_return_if_fail (DH_IS_SEARCH (search));
        
        priv = search->priv;

        dh_search_do_search (search, 
				  gtk_entry_get_text (GTK_ENTRY (priv->entry)));
}

static void
dh_search_entry_changed_cb (GtkEditable *editable, DhSearch *search)
{
        DhSearchPriv   *priv;
        FunctionDatabase    *fd;

        g_return_if_fail (editable != NULL);
        g_return_if_fail (GTK_IS_EDITABLE (editable));
        g_return_if_fail (search != NULL);
        g_return_if_fail (DH_IS_SEARCH (search));
        
        priv = search->priv;
        fd   = dh_bookshelf_get_function_database (priv->bookshelf);

        function_database_idle_search (fd);
}

static void
dh_search_entry_insert_text_cb (GtkEditable     *editable,
                                     gchar           *new_text,
                                     gint             length,
                                     gint            *pos,
                                     DhSearch   *search)
{
        DhSearchPriv   *priv;
        
        g_return_if_fail (editable != NULL);
        g_return_if_fail (GTK_IS_EDITABLE (editable));
        g_return_if_fail (search != NULL);
        g_return_if_fail (DH_IS_SEARCH (search));
        
        priv = search->priv;

        if (!priv->complete) {
                priv->complete = gtk_idle_add (dh_search_complete_idle, 
                                               search);
        }
}

static gboolean
dh_search_entry_key_press_cb (GtkEditable     *editable, 
				   GdkEventKey     *event,
				   DhSearch   *search)
{
        DhSearchPriv   *priv;
	
        g_return_if_fail (editable != NULL);
        g_return_if_fail (GTK_IS_EDITABLE (editable));
        g_return_if_fail (search != NULL);
        g_return_if_fail (DH_IS_SEARCH (search));
        
        priv = search->priv;

	switch (event->keyval) {
 	case GDK_Tab:
                gtk_editable_select_region (editable, 0, 0);
                gtk_editable_set_position (editable, -1);
                return TRUE;
                break;

		/* Hack, needed to stop the entry from losing focus when
		 * arrow keys are pressed.
		 */
	case GDK_Left:
	case GDK_KP_Left:
	case GDK_Right:
	case GDK_KP_Right:
		gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "key_press_event");
                return TRUE;
		break;
		
	default:
                break;
        }

        return FALSE;
}

static void
dh_search_clist_select_row_cb (GtkCList        *clist,
                                    gint             row,
                                    gint             col,
                                    GdkEvent        *event,
                                    DhSearch   *search)
{
        DhSearchPriv   *priv;
        Function            *function;
        BookNode            *book_node;
        GnomeVFSURI         *uri;
	Book                *book;
	
        g_return_if_fail (clist != NULL);
        g_return_if_fail (GTK_IS_CLIST (clist));
        g_return_if_fail (search != NULL);
        g_return_if_fail (DH_IS_SEARCH (search));
        
        priv = search->priv;

        function = (Function *) gtk_clist_get_row_data (clist, row);
        
        if (!function) {
                return;
        }

	dh_bookshelf_open_document (priv->bookshelf, function->document);
	
	uri = document_get_uri (function->document, function->anchor);
        
	g_signal_emit (G_OBJECT (search),
		       signals[URI_SELECTED],
		       0,
		       uri);

	gnome_vfs_uri_unref (uri);
}

static gint
dh_search_complete_idle (gpointer user_data)
{
	DhSearch       *search;
	DhSearchPriv   *priv;
	const gchar         *text;
	gchar               *completed;
	gint                 text_length;
	
        g_return_if_fail (user_data != NULL);
        g_return_if_fail (DH_IS_SEARCH (user_data));

	search = DH_SEARCH (user_data);
	priv   = search->priv;
	
	text   = gtk_entry_get_text (GTK_ENTRY (priv->entry));
	
	completed = function_database_get_completion (priv->fd, text);
	
	if (completed) {
		text_length = strlen (text);

		gtk_entry_set_text (GTK_ENTRY (priv->entry), completed);

		gtk_editable_set_position (GTK_EDITABLE (priv->entry), 
					   text_length);

		gtk_editable_select_region (GTK_EDITABLE (priv->entry),
					    text_length, -1);
		
	}
	
	priv->complete = 0;
	
	return 0;
}

static void
dh_search_do_search (DhSearch *search, const gchar *string)
{
	GSList   *list;
	
	g_return_if_fail (search != NULL);
	g_return_if_fail (DH_IS_SEARCH (search));
	g_return_if_fail (string != NULL);
	
	function_database_search (search->priv->fd, string);
}

static gchar * 
dh_search_get_search_string_cb (FunctionDatabase   *fd, 
                                     DhSearch      *search)
{
	g_return_val_if_fail (fd != NULL, NULL);
	g_return_val_if_fail (IS_FUNCTION_DATABASE (fd), NULL);
	g_return_val_if_fail (search != NULL, NULL);
	g_return_val_if_fail (DH_IS_SEARCH (search), NULL);
	
	return g_strdup (gtk_entry_get_text (GTK_ENTRY (search->priv->entry)));
}

static void
dh_search_exact_hit_found_cb (FunctionDatabase   *fd,
                                   Function           *function,
                                   DhSearch      *search)
{
	DhSearchPriv   *priv;
	gint                 i = 0;
	gchar               *text;

	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));
	g_return_if_fail (search != NULL);
	g_return_if_fail (DH_IS_SEARCH (search));
	g_return_if_fail (function != NULL);
	
	priv = search->priv;

	while (gtk_clist_get_text (GTK_CLIST (priv->clist), i++, 0, &text)) {
		if (!strcmp (text, function->name)) {
			gtk_clist_moveto (GTK_CLIST (priv->clist), 
					  i - 1, 0, 0, 0);
			gtk_clist_select_row (GTK_CLIST (priv->clist),
					      i - 1, 0);
			break;
		}
	}
}

static void
dh_search_hits_found_cb (FunctionDatabase   *fd,
                              GSList             *hits,
                              DhSearch      *search)
{
	DhSearchPriv   *priv;
	GSList              *node;
	gint                 row;
	gint                 nr_hits = 0;
	Function            *function;

	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));
	g_return_if_fail (search != NULL);
	g_return_if_fail (DH_IS_SEARCH (search));
	g_return_if_fail (hits != NULL);

	priv = search->priv;

	gtk_clist_clear (GTK_CLIST (priv->clist));
	gtk_clist_freeze (GTK_CLIST (priv->clist));

	for (node = hits; node && nr_hits < MAX_HITS; (node = node->next) && nr_hits++) {
		function = (Function *) node->data;
		
		row = gtk_clist_append (GTK_CLIST (priv->clist),
					&function->name);

		gtk_clist_set_row_data (GTK_CLIST (priv->clist), 
					row, function);
	}
	
	gtk_clist_thaw (GTK_CLIST (priv->clist));
}

void
dh_search_function_removed_cb (FunctionDatabase  *fd,
				    Function          *function,
				    DhSearch     *search)
{
	DhSearchPriv   *priv;
	gint                 row;

	d(puts(__FUNCTION__));
	
	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));
	g_return_if_fail (search != NULL);
	g_return_if_fail (DH_IS_SEARCH (search));
	g_return_if_fail (function != NULL);

	priv = search->priv;
	
	row = gtk_clist_find_row_from_data (GTK_CLIST (priv->clist),
					    function);
	if (row != -1) {
		d(g_print ("%s: remove from clist",
			   __FUNCTION__));
		gtk_clist_remove (GTK_CLIST (priv->clist), row);
	}
}

DhSearch *
dh_search_new (DhBookshelf *bookshelf)
{
        DhSearch       *search;
        DhSearchPriv   *priv;

        search = g_object_new (DH_TYPE_SEARCH, NULL);
        priv   = search->priv;
        
        priv->bookshelf = bookshelf;
        priv->clist     = gtk_clist_new (1);
        priv->entry     = gtk_entry_new ();
	priv->fd        = dh_bookshelf_get_function_database (bookshelf);

        g_signal_connect (priv->clist, 
			  "select_row",
			  G_CALLBACK (dh_search_clist_select_row_cb),
			  search);
        
        g_signal_connect (priv->entry,
			  "activate",
			  G_CALLBACK (dh_search_entry_activate_cb),
			  search);

        g_signal_connect_after (priv->entry,
				"changed",
				G_CALLBACK (dh_search_entry_changed_cb),
				search);
       
        g_signal_connect (priv->entry,
			  "insert-text",
			  G_CALLBACK (dh_search_entry_insert_text_cb),
			  search);
    
        g_signal_connect_after (priv->entry,
				"key-press-event",
				G_CALLBACK (dh_search_entry_key_press_cb),
				search);

        g_signal_connect (priv->fd, 
			  "get_search_string",
                          G_CALLBACK (dh_search_get_search_string_cb),
                          search);
        
        g_signal_connect (priv->fd,
			  "exact_hit_found",
                          G_CALLBACK (dh_search_exact_hit_found_cb),
                          search);
        
        g_signal_connect (priv->fd,
			  "hits_found",
                          G_CALLBACK (dh_search_hits_found_cb),
                          search);

	g_signal_connect (priv->fd,
			  "function_removed",
			  G_CALLBACK (dh_search_function_removed_cb),
			  search);
        return search;
}

GtkWidget *
dh_search_get_result_widget (DhSearch *search)
{
	DhSearchPriv   *priv;
	GtkWidget           *sw;
	
        g_return_val_if_fail (search != NULL, NULL);
        g_return_val_if_fail (DH_IS_SEARCH (search), NULL);

	priv     = search->priv;

	sw = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	
	gtk_clist_set_column_width (GTK_CLIST (priv->clist), 0, 80);
	gtk_container_add (GTK_CONTAINER (sw), priv->clist);

	gtk_widget_show_all (sw);

	return sw;
}

GtkWidget *
dh_search_get_entry_widget (DhSearch *search) 
{
        g_return_val_if_fail (search != NULL, NULL);
        g_return_val_if_fail (DH_IS_SEARCH (search), NULL);

	return search->priv->entry;
}

void
dh_search_set_search_string (DhSearch *search, const gchar *str)
{
        g_return_if_fail (search != NULL);
        g_return_if_fail (DH_IS_SEARCH (search));

	gtk_entry_set_text (GTK_ENTRY (search->priv->entry), str);
}
