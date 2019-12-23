/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_SEARCH_BAR_H
#define DH_SEARCH_BAR_H

#include <gtk/gtk.h>
#include <devhelp/dh-notebook.h>

G_BEGIN_DECLS

#define DH_TYPE_SEARCH_BAR             (dh_search_bar_get_type ())
#define DH_SEARCH_BAR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_SEARCH_BAR, DhSearchBar))
#define DH_SEARCH_BAR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_SEARCH_BAR, DhSearchBarClass))
#define DH_IS_SEARCH_BAR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_SEARCH_BAR))
#define DH_IS_SEARCH_BAR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_SEARCH_BAR))
#define DH_SEARCH_BAR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_SEARCH_BAR, DhSearchBarClass))

typedef struct _DhSearchBar         DhSearchBar;
typedef struct _DhSearchBarClass    DhSearchBarClass;
typedef struct _DhSearchBarPrivate  DhSearchBarPrivate;

struct _DhSearchBar {
        GtkSearchBar parent;

        DhSearchBarPrivate *priv;
};

struct _DhSearchBarClass {
        GtkSearchBarClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_search_bar_get_type                   (void);

DhSearchBar *   dh_search_bar_new                        (DhNotebook *notebook);

DhNotebook *    dh_search_bar_get_notebook               (DhSearchBar *search_bar);

void            dh_search_bar_grab_focus_to_search_entry (DhSearchBar *search_bar);

G_END_DECLS

#endif /* DH_SEARCH_BAR_H */
