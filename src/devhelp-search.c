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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkeditable.h>
#include <gtk/gtkentry.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkscrolledwindow.h>
#include "function-database.h"
#include "devhelp-search.h"

#define MAX_HITS 250

static void devhelp_search_class_init          (DevHelpSearchClass   *klass);
static void devhelp_search_init                (DevHelpSearch        *index);
 
static void devhelp_search_destroy             (GtkObject            *object);

static void devhelp_search_entry_activate_cb   (GtkEditable          *editable,
                                                DevHelpSearch        *search);

static void devhelp_search_entry_changed_cb    (GtkEditable          *editable,
						DevHelpSearch        *search);

static void devhelp_search_entry_insert_text_cb(GtkEditable          *editable,
                                                gchar                *new_text,
                                                gint                  length,
                                                gint                 *pos,
                                                DevHelpSearch        *search);

static gboolean
devhelp_search_entry_key_press_cb              (GtkEditable          *editable,
                                                GdkEventKey          *event,
						DevHelpSearch        *search);

static void devhelp_search_clist_select_row_cb (GtkCList             *clist,
                                                gint                  row,
                                                gint                  col,
                                                GdkEvent             *event,
                                                DevHelpSearch        *search);

static gint devhelp_search_complete_idle       (gpointer              data);

static void devhelp_search_do_search           (DevHelpSearch        *search, 
						const gchar          *string);
static gchar * 
devhelp_search_get_search_string_cb            (FunctionDatabase     *fd,
                                                DevHelpSearch        *search);

static void devhelp_search_exact_hit_found_cb  (FunctionDatabase     *fd,
                                                Function             *function,
                                                DevHelpSearch        *search);

static void devhelp_search_hits_found_cb       (FunctionDatabase     *fd,
                                                GSList               *hits,
                                                DevHelpSearch        *search);


static GtkObjectClass *parent_class = NULL;

enum {
        URI_SELECTED,
        LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

struct _DevHelpSearchPriv {
	GtkWidget           *entry; 
	GtkWidget           *clist; 

        Bookshelf           *bookshelf;
	FunctionDatabase    *fd;

	guint                complete;
};

GtkType
devhelp_search_get_type (void)
{
        static GtkType devhelp_search_type = 0;

        if (!devhelp_search_type) {
                static const GtkTypeInfo devhelp_search_info = {
                        "DevHelpSearch",
                        sizeof (DevHelpSearch),
                        sizeof (DevHelpSearchClass),
                        (GtkClassInitFunc)  devhelp_search_class_init,
                        (GtkObjectInitFunc) devhelp_search_init,
                        /* reserved_1 */ NULL,
                        /* reserved_2 */ NULL,
                        (GtkClassInitFunc) NULL,
                };

                devhelp_search_type = gtk_type_unique (gtk_object_get_type (), 
                                                       &devhelp_search_info);
        }

        return devhelp_search_type;
}

static void
devhelp_search_class_init (DevHelpSearchClass *klass)
{
        GtkObjectClass *object_class;

        object_class = (GtkObjectClass *) klass;
        parent_class = gtk_type_class (gtk_object_get_type ());

	object_class->destroy = devhelp_search_destroy;

        signals[URI_SELECTED] =
                gtk_signal_new ("uri_selected",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (DevHelpSearchClass,
                                                   uri_selected),
                                gtk_marshal_NONE__POINTER,
                                GTK_TYPE_NONE,
                                1, GTK_TYPE_POINTER);
        
        gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
}

static void
devhelp_search_init (DevHelpSearch *search)
{
        DevHelpSearchPriv   *priv;
        
        priv                  = g_new0 (DevHelpSearchPriv, 1);
        priv->bookshelf       = NULL;
        search->priv          = priv;
}

static void
devhelp_search_destroy (GtkObject *object)
{
        /* FIX: Do something */
}

static void
devhelp_search_entry_activate_cb (GtkEditable *editable, DevHelpSearch *search)
{
        DevHelpSearchPriv   *priv;
        
        g_return_if_fail (editable != NULL);
        g_return_if_fail (GTK_IS_EDITABLE (editable));
        g_return_if_fail (search != NULL);
        g_return_if_fail (IS_DEVHELP_SEARCH (search));
        
        priv = search->priv;

        devhelp_search_do_search (search, 
				  gtk_entry_get_text (GTK_ENTRY (priv->entry)));
}

static void
devhelp_search_entry_changed_cb (GtkEditable *editable, DevHelpSearch *search)
{
        DevHelpSearchPriv   *priv;
        FunctionDatabase    *fd;
        
        g_return_if_fail (editable != NULL);
        g_return_if_fail (GTK_IS_EDITABLE (editable));
        g_return_if_fail (search != NULL);
        g_return_if_fail (IS_DEVHELP_SEARCH (search));
        
        priv = search->priv;
        fd   = bookshelf_get_function_database (priv->bookshelf);

        function_database_idle_search (fd);
}

static void
devhelp_search_entry_insert_text_cb (GtkEditable     *editable,
                                     gchar           *new_text,
                                     gint             length,
                                     gint            *pos,
                                     DevHelpSearch   *search)
{
        DevHelpSearchPriv   *priv;
        
        g_return_if_fail (editable != NULL);
        g_return_if_fail (GTK_IS_EDITABLE (editable));
        g_return_if_fail (search != NULL);
        g_return_if_fail (IS_DEVHELP_SEARCH (search));
        
        priv = search->priv;

        if (!priv->complete) {
                priv->complete = gtk_idle_add (devhelp_search_complete_idle, 
                                               search);
        }
}

static gboolean
devhelp_search_entry_key_press_cb (GtkEditable     *editable, 
				   GdkEventKey     *event,
				   DevHelpSearch   *search)
{
        DevHelpSearchPriv   *priv;
        
        g_return_if_fail (editable != NULL);
        g_return_if_fail (GTK_IS_EDITABLE (editable));
        g_return_if_fail (search != NULL);
        g_return_if_fail (IS_DEVHELP_SEARCH (search));
        
        priv = search->priv;

        switch (event->keyval) {
 	case GDK_Tab:
                gtk_editable_select_region (editable, 0, 0);
                gtk_editable_set_position (editable, -1);
                return TRUE;
                break;
        default:
                break;
        }

        return FALSE;
}

static void
devhelp_search_clist_select_row_cb (GtkCList        *clist,
                                    gint             row,
                                    gint             col,
                                    GdkEvent        *event,
                                    DevHelpSearch   *search)
{
        DevHelpSearchPriv   *priv;
        Function            *function;
        BookNode            *book_node;
        GnomeVFSURI         *uri;
	Book                *book;
	
        g_return_if_fail (clist != NULL);
        g_return_if_fail (GTK_IS_CLIST (clist));
        g_return_if_fail (search != NULL);
        g_return_if_fail (IS_DEVHELP_SEARCH (search));
        
        priv = search->priv;

        function = (Function *) gtk_clist_get_row_data (clist, row);
        
        if (!function) {
                return;
        }

	bookshelf_open_document (priv->bookshelf, function->document);
	
	uri = document_get_uri (function->document, function->anchor);
        
	gtk_signal_emit (GTK_OBJECT (search),
			 signals[URI_SELECTED],
			 uri);

	gnome_vfs_uri_unref (uri);
}

static gint
devhelp_search_complete_idle (gpointer user_data)
{
	DevHelpSearch       *search;
	DevHelpSearchPriv   *priv;
	gchar               *text, *completed;
	gint                 text_length;
	
        g_return_if_fail (user_data != NULL);
        g_return_if_fail (IS_DEVHELP_SEARCH (user_data));

	search = DEVHELP_SEARCH (user_data);
	priv   = search->priv;
	
	text   = gtk_entry_get_text (GTK_ENTRY (priv->entry));
	
	completed = function_database_get_completion (priv->fd, text);
	
	if (completed) {
		text_length = strlen (text);
		
		gtk_entry_set_text (GTK_ENTRY (priv->entry), completed);
		
		gtk_editable_select_region (GTK_EDITABLE (priv->entry),
					    text_length, -1);
		
		gtk_editable_set_position (GTK_EDITABLE (priv->entry), 
					   text_length);
	}
	
	priv->complete = 0;
	
	return 0;
}

static void
devhelp_search_do_search (DevHelpSearch *search, const gchar *string)
{
	GSList   *list;
	
	g_return_if_fail (search != NULL);
	g_return_if_fail (IS_DEVHELP_SEARCH (search));
	g_return_if_fail (string != NULL);
	
	function_database_search (search->priv->fd, string);
}

static gchar * 
devhelp_search_get_search_string_cb (FunctionDatabase   *fd, 
                                     DevHelpSearch      *search)
{
	g_return_val_if_fail (fd != NULL, NULL);
	g_return_val_if_fail (IS_FUNCTION_DATABASE (fd), NULL);
	g_return_val_if_fail (search != NULL, NULL);
	g_return_val_if_fail (IS_DEVHELP_SEARCH (search), NULL);
	
	return gtk_entry_get_text (GTK_ENTRY (search->priv->entry));
}

static void
devhelp_search_exact_hit_found_cb (FunctionDatabase   *fd,
                                   Function           *function,
                                   DevHelpSearch      *search)
{
	DevHelpSearchPriv   *priv;
	gint                 i = 0;
	gchar               *text;

	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));
	g_return_if_fail (search != NULL);
	g_return_if_fail (IS_DEVHELP_SEARCH (search));
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
devhelp_search_hits_found_cb (FunctionDatabase   *fd,
                              GSList             *hits,
                              DevHelpSearch      *search)
{
	DevHelpSearchPriv   *priv;
	GSList              *node;
	gint                 row;
	gint                 nr_hits = 0;
	Function            *function;

	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));
	g_return_if_fail (search != NULL);
	g_return_if_fail (IS_DEVHELP_SEARCH (search));
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

DevHelpSearch *
devhelp_search_new (Bookshelf *bookshelf)
{
        DevHelpSearch       *search;
        DevHelpSearchPriv   *priv;

        search = gtk_type_new (TYPE_DEVHELP_SEARCH);
        priv   = search->priv;
        
        priv->bookshelf = bookshelf;
        priv->clist     = gtk_clist_new (1);
        priv->entry     = gtk_entry_new ();
	priv->fd        = bookshelf_get_function_database (bookshelf);
        
        gtk_signal_connect (GTK_OBJECT (priv->clist), 
                            "select_row",
                            GTK_SIGNAL_FUNC (devhelp_search_clist_select_row_cb),
                            search);
        
        gtk_signal_connect (GTK_OBJECT (priv->entry),
                            "activate",
                            GTK_SIGNAL_FUNC (devhelp_search_entry_activate_cb),
                            search);
        
        gtk_signal_connect (GTK_OBJECT (priv->entry),
                            "changed",
                            GTK_SIGNAL_FUNC (devhelp_search_entry_changed_cb),
                            search);
        
        gtk_signal_connect (GTK_OBJECT (priv->entry),
                            "insert-text",
                            GTK_SIGNAL_FUNC (devhelp_search_entry_insert_text_cb),
                            search);
        
        gtk_signal_connect_after (GTK_OBJECT (priv->entry),
                                  "key-press-event",
                                  GTK_SIGNAL_FUNC (devhelp_search_entry_key_press_cb),
                                  search);

        gtk_signal_connect (GTK_OBJECT (priv->fd), 
			    "get_search_string",
                            GTK_SIGNAL_FUNC (devhelp_search_get_search_string_cb),
                            search);
        
        gtk_signal_connect (GTK_OBJECT (priv->fd),
			    "exact_hit_found",
                            GTK_SIGNAL_FUNC (devhelp_search_exact_hit_found_cb),
                            search);
        
        gtk_signal_connect (GTK_OBJECT (priv->fd),
			    "hits_found",
                            GTK_SIGNAL_FUNC (devhelp_search_hits_found_cb),
                            search);

        return search;
}

GtkWidget *
devhelp_search_get_result_widget (DevHelpSearch *search)
{
	DevHelpSearchPriv   *priv;
	GtkWidget           *sw;
	
        g_return_val_if_fail (search != NULL, NULL);
        g_return_val_if_fail (IS_DEVHELP_SEARCH (search), NULL);

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
devhelp_search_get_entry_widget (DevHelpSearch *search) 
{
        g_return_val_if_fail (search != NULL, NULL);
        g_return_val_if_fail (IS_DEVHELP_SEARCH (search), NULL);

	return search->priv->entry;
}

void
devhelp_search_set_search_string (DevHelpSearch *search, const gchar *str)
{
        g_return_if_fail (search != NULL);
        g_return_if_fail (IS_DEVHELP_SEARCH (search));

	gtk_entry_set_text (GTK_ENTRY (search->priv->entry), str);
}
