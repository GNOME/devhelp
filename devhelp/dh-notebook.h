/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_NOTEBOOK_H
#define DH_NOTEBOOK_H

#include <gtk/gtk.h>
#include <devhelp/dh-profile.h>
#include <devhelp/dh-tab.h>
#include <devhelp/dh-web-view.h>

G_BEGIN_DECLS

#define DH_TYPE_NOTEBOOK             (dh_notebook_get_type ())
#define DH_NOTEBOOK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_NOTEBOOK, DhNotebook))
#define DH_NOTEBOOK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_NOTEBOOK, DhNotebookClass))
#define DH_IS_NOTEBOOK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_NOTEBOOK))
#define DH_IS_NOTEBOOK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_NOTEBOOK))
#define DH_NOTEBOOK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_NOTEBOOK, DhNotebookClass))

typedef struct _DhNotebook         DhNotebook;
typedef struct _DhNotebookClass    DhNotebookClass;
typedef struct _DhNotebookPrivate  DhNotebookPrivate;

struct _DhNotebook {
        GtkNotebook parent;

        DhNotebookPrivate *priv;
};

struct _DhNotebookClass {
        GtkNotebookClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_notebook_get_type                    (void);

DhNotebook *    dh_notebook_new                         (DhProfile *profile);

DhProfile *     dh_notebook_get_profile                 (DhNotebook *notebook);

void            dh_notebook_open_new_tab                (DhNotebook  *notebook,
                                                         const gchar *uri,
                                                         gboolean     switch_focus);

DhTab *         dh_notebook_get_active_tab              (DhNotebook *notebook);

DhWebView *     dh_notebook_get_active_web_view         (DhNotebook *notebook);

GList *         dh_notebook_get_all_web_views           (DhNotebook *notebook);

G_END_DECLS

#endif /* DH_NOTEBOOK_H */
