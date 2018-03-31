/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
 *
 * Devhelp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Devhelp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Devhelp.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DH_TAB_LABEL_H
#define DH_TAB_LABEL_H

#include <gtk/gtk.h>
#include "dh-tab.h"

G_BEGIN_DECLS

#define DH_TYPE_TAB_LABEL             (dh_tab_label_get_type ())
#define DH_TAB_LABEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_TAB_LABEL, DhTabLabel))
#define DH_TAB_LABEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_TAB_LABEL, DhTabLabelClass))
#define DH_IS_TAB_LABEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_TAB_LABEL))
#define DH_IS_TAB_LABEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_TAB_LABEL))
#define DH_TAB_LABEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_TAB_LABEL, DhTabLabelClass))

typedef struct _DhTabLabel         DhTabLabel;
typedef struct _DhTabLabelClass    DhTabLabelClass;
typedef struct _DhTabLabelPrivate  DhTabLabelPrivate;

struct _DhTabLabel {
        GtkGrid parent;

        DhTabLabelPrivate *priv;
};

struct _DhTabLabelClass {
        GtkGridClass parent_class;

	/* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_tab_label_get_type           (void);

GtkWidget *     dh_tab_label_new                (DhTab *tab);

DhTab *         dh_tab_label_get_tab            (DhTabLabel *tab_label);

G_END_DECLS

#endif /* DH_TAB_LABEL_H */
