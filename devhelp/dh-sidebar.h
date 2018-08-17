/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
 * Copyright (C) 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
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
#include <devhelp/dh-book-manager.h>
#include <devhelp/dh-link.h>
#include <devhelp/dh-profile.h>

G_BEGIN_DECLS

#define DH_TYPE_SIDEBAR            (dh_sidebar_get_type ())
G_DECLARE_DERIVABLE_TYPE (DhSidebar, dh_sidebar, DH, SIDEBAR, GtkGrid)

struct _DhSidebarClass {
        GtkGridClass parent_class;

        /* Signals */
        void (*link_selected) (DhSidebar *sidebar,
                               DhLink    *link);

        /* Padding for future expansion */
        gpointer padding[12];
};

G_DEPRECATED_FOR (dh_sidebar_new2)
GtkWidget *     dh_sidebar_new                  (DhBookManager *book_manager);

DhSidebar *     dh_sidebar_new2                 (DhProfile *profile);

DhProfile *     dh_sidebar_get_profile          (DhSidebar *sidebar);

DhLink *        dh_sidebar_get_selected_link    (DhSidebar *sidebar);

void            dh_sidebar_select_uri           (DhSidebar   *sidebar,
                                                 const gchar *uri);

void            dh_sidebar_set_search_string    (DhSidebar   *sidebar,
                                                 const gchar *str);

void            dh_sidebar_set_search_focus     (DhSidebar *sidebar);

G_END_DECLS

