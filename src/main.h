/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
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
 * Author: Johan Dahlin <zilch.am@home.se>
 */

#ifndef __DEVHELP_H__
#define __DEVHELP_H__

#include <gtk/gtk.h>
#include <gtkhtml/gtkhtml.h>
#include <glade/glade-xml.h>

#include "bookmark-manager.h"
#include "bookshelf.h"
#include "function-database.h"
#include "html-widget.h"
#include "history.h"
#include "book-index.h"

typedef struct _DevHelp DevHelp;

#include "preferences.h"

#define d(x)

struct _DevHelp {
	GtkWidget          *window;               /* Main window */
	GtkWidget          *bookmark_menu;
	GtkWidget          *zoom_tiny, *zoom_small, *zoom_medium;
	GtkWidget          *zoom_large, *zoom_huge;
	GtkNotebook        *notebook;
	GtkWidget          *hbox;
	GtkWidget          *hpaned;
	GtkWidget          *right_frame;
	GtkWidget          *book_index;
	GtkCList           *clist;                /* Search clist */
	GtkEntry           *entry;                /* Search entry */
	GtkWidget          *search_button;
	GtkWidget          *scrolled;
	GtkWidget          *autocomp_checkbutton;
	HtmlWidget         *html_widget;
	History            *history;
	FunctionDatabase   *function_database;
	BookmarkManager    *bookmark_manager;
	Preferences        *preferences;
	gchar              *baseurl;
	guint               complete;
	Bookshelf          *bookshelf;
};

void              parse_books            (DevHelp *devhelp);

#endif /* __DEVHELP_H__ */
