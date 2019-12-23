/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2001 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
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

G_END_DECLS

#endif /* DH_BOOK_TREE_H */
