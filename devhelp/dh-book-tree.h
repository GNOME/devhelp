/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2001 Mikael Hallendal <micke@imendio.com>
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

#ifndef DH_BOOK_TREE_H
#define DH_BOOK_TREE_H

#include <gtk/gtk.h>
#include <devhelp/dh-link.h>
#include <devhelp/dh-profile.h>

G_BEGIN_DECLS

#define DH_TYPE_BOOK_TREE            (dh_book_tree_get_type ())
#define DH_BOOK_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_BOOK_TREE, DhBookTree))
#define DH_BOOK_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_BOOK_TREE, DhBookTreeClass))
#define DH_IS_BOOK_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_BOOK_TREE))
#define DH_IS_BOOK_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), DH_TYPE_BOOK_TREE))

typedef struct _DhBookTree      DhBookTree;
typedef struct _DhBookTreeClass DhBookTreeClass;

struct _DhBookTree {
        GtkTreeView parent_instance;
};

struct _DhBookTreeClass {
        GtkTreeViewClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_book_tree_get_type           (void) G_GNUC_CONST;

DhBookTree *    dh_book_tree_new                (DhProfile *profile);

DhProfile *     dh_book_tree_get_profile        (DhBookTree *tree);

DhLink *        dh_book_tree_get_selected_link  (DhBookTree *tree);

void            dh_book_tree_select_uri         (DhBookTree  *tree,
                                                 const gchar *uri);

DhLink *        dh_book_tree_get_selected_book  (DhBookTree *tree);

G_END_DECLS

#endif /* DH_BOOK_TREE_H */
