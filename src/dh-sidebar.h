/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
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

#ifndef DH_SIDEBAR_H
#define DH_SIDEBAR_H

#include <gtk/gtk.h>
#include "dh-link.h"
#include "dh-book-manager.h"

G_BEGIN_DECLS

#define DH_TYPE_SIDEBAR           (dh_sidebar_get_type ())
#define DH_SIDEBAR(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_SIDEBAR, DhSidebar))
#define DH_SIDEBAR_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_SIDEBAR, DhSidebarClass))
#define DH_IS_SIDEBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_SIDEBAR))
#define DH_IS_SIDEBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_SIDEBAR))

typedef struct _DhSidebar        DhSidebar;
typedef struct _DhSidebarClass   DhSidebarClass;

struct _DhSidebar {
        GtkBox parent_instance;
};

struct _DhSidebarClass {
        GtkBoxClass parent_class;

        /* Signals */
        void (*link_selected) (DhSidebar *search,
                               DhLink    *link);
};

GType      dh_sidebar_get_type (void);
GtkWidget *dh_sidebar_new      (DhBookManager *book_manager);

DhLink    *dh_sidebar_get_selected_book (DhSidebar *sidebar);
void       dh_sidebar_select_uri        (DhSidebar   *sidebar,
                                         const gchar *uri);
void       dh_sidebar_set_search_string (DhSidebar   *sidebar,
                                         const gchar *str);
void       dh_sidebar_set_search_focus  (DhSidebar   *sidebar);

G_END_DECLS

#endif /* DH_SIDEBAR_H */
