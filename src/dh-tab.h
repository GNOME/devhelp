/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DH_TAB_H
#define DH_TAB_H

#include <gtk/gtk.h>
#include "dh-web-view.h"

G_BEGIN_DECLS

#define DH_TYPE_TAB             (dh_tab_get_type ())
#define DH_TAB(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_TAB, DhTab))
#define DH_TAB_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_TAB, DhTabClass))
#define DH_IS_TAB(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_TAB))
#define DH_IS_TAB_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_TAB))
#define DH_TAB_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_TAB, DhTabClass))

typedef struct _DhTab         DhTab;
typedef struct _DhTabClass    DhTabClass;
typedef struct _DhTabPrivate  DhTabPrivate;

struct _DhTab {
        GtkGrid parent;

        DhTabPrivate *priv;
};

struct _DhTabClass {
        GtkGridClass parent_class;

	/* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_tab_get_type         (void);

DhTab *         dh_tab_new              (void);

DhWebView *     dh_tab_get_web_view     (DhTab *tab);

G_END_DECLS

#endif /* DH_TAB_H */
