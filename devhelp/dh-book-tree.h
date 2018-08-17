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

#pragma once

#include <gtk/gtk.h>
#include <devhelp/dh-link.h>
#include <devhelp/dh-profile.h>

G_BEGIN_DECLS

#define DH_TYPE_BOOK_TREE            (dh_book_tree_get_type ())
G_DECLARE_FINAL_TYPE (DhBookTree, dh_book_tree, DH, BOOK_TREE, GtkTreeView)

DhBookTree *dh_book_tree_new               (DhProfile   *profile);
DhProfile  *dh_book_tree_get_profile       (DhBookTree  *tree);
DhLink     *dh_book_tree_get_selected_link (DhBookTree  *tree);
void        dh_book_tree_select_uri        (DhBookTree  *tree,
                                            const gchar *uri);

G_END_DECLS

