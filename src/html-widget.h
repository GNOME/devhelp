/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
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
 * Author: Johan Dahlin <zilch.am@home.se>
 */

#ifndef __HTML_WIDGET_H__
#define __HTML_WIDGET_H__

#include <gtk/gtkobject.h>
#include <gtk/gtktypeutils.h>
#include <gtk/gtkmarshal.h>
#include <gtkhtml/gtkhtml-types.h>

#define HTML_WIDGET_TYPE        (html_widget_get_type ())
#define HTML_WIDGET(o)          (GTK_CHECK_CAST ((o), HTML_WIDGET_TYPE, HtmlWidget))
#define HTML_WIDGET_CLASS(k)    (GTK_CHECK_FOR_CAST((k), HTML_WIDGET_TYPE, HtmlWidgetClass))
#define IS_HTML_WIDGET(o)       (GTK_CHECK_TYPE ((o), HTML_WIDGET_TYPE))
#define IS_HTML_WIDGET_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), HTML_WIDGET_TYPE))

typedef struct _HtmlWidget        HtmlWidget;
typedef struct _HtmlWidgetClass   HtmlWidgetClass;
typedef struct _HtmlWidgetPriv    HtmlWidgetPriv;

struct _HtmlWidget {
	GtkHTML           parent;
	
	HtmlWidgetPriv   *priv;
};

struct _HtmlWidgetClass {
        GtkHTMLClass   parent_class;
};

GtkType         html_widget_get_type  (void);
HtmlWidget     *html_widget_new       ();

void            html_widget_open_uri  (HtmlWidget          *html_widget,
				       const GnomeVFSURI   *uri);

#endif /* __HTML_WIDGET_H__ */

