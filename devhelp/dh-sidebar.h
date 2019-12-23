/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2001-2002 CodeFactory AB
 * SPDX-FileCopyrightText: 2001-2002 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2013 Aleksander Morgado <aleksander@gnu.org>
 * SPDX-FileCopyrightText: 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
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

DhLink *        dh_sidebar_get_selected_link    (DhSidebar *sidebar);

void            dh_sidebar_select_uri           (DhSidebar   *sidebar,
                                                 const gchar *uri);

void            dh_sidebar_set_search_string    (DhSidebar   *sidebar,
                                                 const gchar *str);

void            dh_sidebar_set_search_focus     (DhSidebar *sidebar);

G_END_DECLS

#endif /* DH_SIDEBAR_H */
