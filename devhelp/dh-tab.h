/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_TAB_H
#define DH_TAB_H

#include <gtk/gtk.h>
#include <devhelp/dh-web-view.h>

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

DhTab *         dh_tab_new              (DhWebView *web_view);

DhWebView *     dh_tab_get_web_view     (DhTab *tab);

G_END_DECLS

#endif /* DH_TAB_H */
