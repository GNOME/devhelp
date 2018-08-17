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

#pragma once

#include <gtk/gtk.h>
#include <devhelp/dh-profile.h>
#include <devhelp/dh-tab.h>
#include <devhelp/dh-web-view.h>

G_BEGIN_DECLS

#define DH_TYPE_NOTEBOOK             (dh_notebook_get_type ())
G_DECLARE_DERIVABLE_TYPE (DhNotebook, dh_notebook, DH, NOTEBOOK, GtkNotebook)

struct _DhNotebookClass {
        GtkNotebookClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

DhNotebook *dh_notebook_new                 (DhProfile   *profile);
DhProfile  *dh_notebook_get_profile         (DhNotebook  *notebook);
void        dh_notebook_open_new_tab        (DhNotebook  *notebook,
                                             const gchar *uri,
                                             gboolean     switch_focus);
DhTab      *dh_notebook_get_active_tab      (DhNotebook  *notebook);
DhWebView  *dh_notebook_get_active_web_view (DhNotebook  *notebook);
GList      *dh_notebook_get_all_web_views   (DhNotebook  *notebook);

G_END_DECLS

