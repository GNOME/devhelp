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

#ifndef __DH_VIEW_H__
#define __DH_VIEW_H__

#include <gtk/gtkobject.h>
#include <gtk/gtktypeutils.h>
#include <gtk/gtkmarshal.h>
#include <libgtkhtml/gtkhtml.h>

#define DH_TYPE_VIEW        (dh_view_get_type ())
#define DH_VIEW(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), DH_TYPE_VIEW, DhView))
#define DH_VIEW_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), DH_TYPE_VIEW, DhViewClass))
#define DH_IS_VIEW(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), DH_TYPE_VIEW))
#define DH_IS_VIEW_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), DH_TYPE_VIEW))

typedef struct _DhView        DhView;
typedef struct _DhViewClass   DhViewClass;
typedef struct _DhViewPriv    DhViewPriv;

struct _DhView {
	HtmlView         parent;
	
	DhViewPriv    *priv;
};

struct _DhViewClass {
        HtmlViewClass    parent_class;

	/* Signals */
	void (*uri_selected) (DhView *view,
			      const gchar *uri,
			      const gchar *anchor);
};

GType           dh_view_get_type       (void);
GtkWidget      *dh_view_new            (void);
 
void            dh_view_open_uri       (DhView        *view,
					const gchar   *uri);

#endif /* __DH_VIEW_H__ */

