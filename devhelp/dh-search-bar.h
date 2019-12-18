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
        GtkSearchEntry *search_entry;

        DhSearchBarPrivate *priv;
};

struct _DhSearchBarClass {
        GtkSearchBarClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_search_bar_get_type          (void);

DhSearchBar *   dh_search_bar_new               (DhNotebook *notebook);

DhNotebook *    dh_search_bar_get_notebook      (DhSearchBar *search_bar);

G_END_DECLS

#endif /* DH_SEARCH_BAR_H */
