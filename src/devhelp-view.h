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

#ifndef __DEVHELP_VIEW_H__
#define __DEVHELP_VIEW_H__

#include <gtk/gtkobject.h>
#include <gtk/gtktypeutils.h>
#include <gtk/gtkmarshal.h>
#include <libgtkhtml/gtkhtml.h>

#define DEVHELP_TYPE_VIEW        (devhelp_view_get_type ())
#define DEVHELP_VIEW(o)          (GTK_CHECK_CAST ((o), DEVHELP_TYPE_VIEW, DevHelpView))
#define DEVHELP_VIEW_CLASS(k)    (GTK_CHECK_FOR_CAST((k), DEVHELP_TYPE_VIEW, DevHelpViewClass))
#define DEVHELP_IS_VIEW(o)       (GTK_CHECK_TYPE ((o), DEVHELP_TYPE_VIEW))
#define DEVHELP_IS_VIEW_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), DEVHELP_TYPE_VIEW))

typedef struct _DevHelpView        DevHelpView;
typedef struct _DevHelpViewClass   DevHelpViewClass;
typedef struct _DevHelpViewPriv    DevHelpViewPriv;

struct _DevHelpView {
	HtmlView         parent;
	
	DevHelpViewPriv    *priv;
};

struct _DevHelpViewClass {
        HtmlViewClass    parent_class;

	/* Signals */
	void (*uri_selected) (DevHelpView *view,
			      const gchar *uri,
			      const gchar *anchor);
};

GType           devhelp_view_get_type       (void);
GtkWidget      *devhelp_view_new            (void);
 
#if 0
void            devhelp_view_open_uri       (DevHelpView   *view, 
					     GnomeVFSURI   *uri); 
#endif

void            devhelp_view_open_uri       (DevHelpView   *view,
					     const gchar   *uri,
					     const gchar   *anchor);

#endif /* __DEVHELP_VIEW_H__ */

