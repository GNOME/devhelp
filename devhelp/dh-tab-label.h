/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_TAB_LABEL_H
#define DH_TAB_LABEL_H

#include <gtk/gtk.h>
#include <devhelp/dh-tab.h>

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
