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

#ifndef DH_SIDEBAR_H
#define DH_SIDEBAR_H

#include <gtk/gtk.h>
#include <devhelp/dh-book-manager.h>
#include <devhelp/dh-link.h>
#include <devhelp/dh-profile.h>

G_BEGIN_DECLS

#define DH_TYPE_SIDEBAR            (dh_sidebar_get_type ())
#define DH_SIDEBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_SIDEBAR, DhSidebar))
#define DH_SIDEBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_SIDEBAR, DhSidebarClass))
#define DH_IS_SIDEBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_SIDEBAR))
#define DH_IS_SIDEBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_SIDEBAR))

typedef struct _DhSidebar        DhSidebar;
typedef struct _DhSidebarClass   DhSidebarClass;

struct _DhSidebar {
        GtkGrid parent_instance;
};

struct _DhSidebarClass {
        GtkGridClass parent_class;

        /* Signals */
        void (*link_selected) (DhSidebar *sidebar,
                               DhLink    *link);

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_sidebar_get_type             (void);

G_DEPRECATED_FOR (dh_sidebar_new2)
GtkWidget *     dh_sidebar_new                  (DhBookManager *book_manager);

DhSidebar *     dh_sidebar_new2                 (DhProfile *profile);

DhProfile *     dh_sidebar_get_profile          (DhSidebar *sidebar);

void            dh_sidebar_select_uri           (DhSidebar   *sidebar,
                                                 const gchar *uri);

void            dh_sidebar_set_search_string    (DhSidebar   *sidebar,
                                                 const gchar *str);

void            dh_sidebar_set_search_focus     (DhSidebar *sidebar);

G_END_DECLS

#endif /* DH_SIDEBAR_H */
