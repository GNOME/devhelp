/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@codefactory.se>
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

#ifndef __DH_BASE_H__
#define __DH_BASE_H__

#include <glib-object.h>
#include <gtk/gtkwidget.h>

typedef struct _DhBase      DhBase;
typedef struct _DhBaseClass DhBaseClass;
typedef struct _DhBasePriv  DhBasePriv;

#define DH_TYPE_BASE         (dh_base_get_type ())
#define DH_BASE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DH_TYPE_BASE, DhBase))
#define DH_BASE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), DH_TYPE_BASE, DhBaseClass))
#define DH_IS_BASE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DH_TYPE_BASE))
#define DH_IS_BASE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), DH_TYPE_BASE))
#define DH_BASE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DH_TYPE_BASE, DhBaseClass))


struct _DhBase {
        GObject     parent;
        
        DhBasePriv *priv;
};

struct _DhBaseClass {
        GObjectClass parent_class;
};

GType            dh_base_get_type       (void);
DhBase *         dh_base_new            (void);

GtkWidget *      dh_base_new_window     (DhBase      *base);

GNode *          dh_base_get_book_tree  (DhBase      *base);
GList *          dh_base_get_keywords   (DhBase      *base);

GSList *         dh_base_get_windows    (DhBase      *base);

#endif /* __DH_BASE_H__ */
