/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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

#ifndef __DH_HTML_H__
#define __DH_HTML_H__

#include <glib-object.h>
#include <gtk/gtkwidget.h>

#define DH_TYPE_HTML        (dh_html_get_type ())
#define DH_HTML(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), DH_TYPE_HTML, DhHtml))
#define DH_HTML_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), DH_TYPE_HTML, DhHtmlClass))
#define DH_IS_HTML(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), DH_TYPE_HTML))
#define DH_IS_HTML_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), DH_TYPE_HTML))

typedef struct _DhHtml        DhHtml;
typedef struct _DhHtmlClass   DhHtmlClass;
typedef struct _DhHtmlPriv    DhHtmlPriv;

struct _DhHtml {
	GObject        parent;

	DhHtmlPriv    *priv;
};

struct _DhHtmlClass {
        GObjectClass   parent_class;
};

GType           dh_html_get_type       (void);
DhHtml         *dh_html_new            (void);
 
void            dh_html_open_uri       (DhHtml        *html,
					const gchar   *uri);
GtkWidget *     dh_html_get_widget     (DhHtml        *html);
gboolean        dh_html_can_go_forward (DhHtml        *html);
gboolean        dh_html_can_go_back    (DhHtml        *html);
void            dh_html_go_forward     (DhHtml        *html);
void            dh_html_go_back        (DhHtml        *html);

#endif /* __DH_HTML_H__ */

