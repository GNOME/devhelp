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

#ifndef __DEVHELP_SEARCH_H__
#define __DEVHELP_SEARCH_H__

#include <glib.h>
#include <gtk/gtkobject.h>
#include <gtk/gtktypeutils.h>
#include <gtk/gtkwidget.h>
#include "bookshelf.h"

#define DEVHELP_SEARCH_ENTRY_OAFIID "OAFIID:GNOME_DevHelp_SearchEntry"
#define DEVHELP_SEARCH_RESULT_OAFIID "OAFIID:GNOME_DevHelp_SearchResult"

#define TYPE_DEVHELP_SEARCH		(devhelp_search_get_type ())
#define DEVHELP_SEARCH(obj)		(GTK_CHECK_CAST ((obj), TYPE_DEVHELP_SEARCH, DevHelpSearch))
#define DEVHELP_SEARCH_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), TYPE_DEVHELP_SEARCH, DevHelpSearchClass))
#define IS_DEVHELP_SEARCH(obj)		(GTK_CHECK_TYPE ((obj), TYPE_DEVHELP_SEARCH))
#define IS_DEVHELP_SEARCH_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), TYPE_DEVHELP_SEARCH))

typedef struct _DevHelpSearch       DevHelpSearch;
typedef struct _DevHelpSearchClass  DevHelpSearchClass;
typedef struct _DevHelpSearchPriv   DevHelpSearchPriv;

struct _DevHelpSearch
{
        GtkObject            parent;
        
        DevHelpSearchPriv   *priv;
};

struct _DevHelpSearchClass
{
        GtkObjectClass       parent_class;

        /* Signals */
        void (*search_match)  (DevHelpSearch       *search,
                               const gchar         *string);
        
        void (*uri_selected)  (DevHelpSearch       *search,
                               const GnomeVFSURI   *uri);
       
        
};

GtkType          devhelp_search_get_type           (void);
DevHelpSearch *  devhelp_search_new                (Bookshelf      *bookshelf);

GtkWidget *      devhelp_search_get_entry_widget   (DevHelpSearch  *search);
GtkWidget *      devhelp_search_get_result_widget  (DevHelpSearch  *search);

void             devhelp_search_set_search_string  (DevHelpSearch  *search,
						    const gchar    *str);

#endif /* __DEVHELP_SEARCH_H__ */
