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

#ifndef DH_NOTEBOOK_H
#define DH_NOTEBOOK_H

#include <gtk/gtk.h>
#include "dh-tab.h"
#include "dh-web-view.h"

G_BEGIN_DECLS

#define DH_TYPE_NOTEBOOK             (dh_notebook_get_type ())
#define DH_NOTEBOOK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_NOTEBOOK, DhNotebook))
#define DH_NOTEBOOK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_NOTEBOOK, DhNotebookClass))
#define DH_IS_NOTEBOOK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_NOTEBOOK))
#define DH_IS_NOTEBOOK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_NOTEBOOK))
#define DH_NOTEBOOK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_NOTEBOOK, DhNotebookClass))

typedef struct _DhNotebook         DhNotebook;
typedef struct _DhNotebookClass    DhNotebookClass;

struct _DhNotebook {
        GtkNotebook parent;
};

struct _DhNotebookClass {
        GtkNotebookClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_notebook_get_type                    (void);

DhNotebook *    dh_notebook_new                         (void);

DhTab *         dh_notebook_get_active_tab              (DhNotebook *notebook);

DhWebView *     dh_notebook_get_active_web_view         (DhNotebook *notebook);

GList *         dh_notebook_get_all_web_views           (DhNotebook *notebook);

G_END_DECLS

#endif /* DH_NOTEBOOK_H */
