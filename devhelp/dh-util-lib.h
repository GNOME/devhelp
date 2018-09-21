/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004,2008 Imendio AB
 * Copyright (C) 2015, 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
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

#ifndef DH_UTIL_LIB_H
#define DH_UTIL_LIB_H

#include <gio/gio.h>
#include "dh-notebook.h"
#include "dh-sidebar.h"

G_BEGIN_DECLS

G_GNUC_INTERNAL
gchar *         _dh_util_build_data_filename            (const gchar *first_part,
                                                         ...);

G_GNUC_INTERNAL
void            _dh_util_ascii_strtitle                 (gchar *str);

G_GNUC_INTERNAL
gchar *         _dh_util_create_data_uri_for_filename   (const gchar *filename,
                                                         const gchar *mime_type);

G_GNUC_INTERNAL
void            _dh_util_queue_concat                   (GQueue *q1,
                                                         GQueue *q2);

G_GNUC_INTERNAL
void            _dh_util_free_book_tree                 (GNode *book_tree);

G_GNUC_INTERNAL
GSList *        _dh_util_get_possible_index_files       (GFile *book_directory);

G_GNUC_INTERNAL
void            _dh_util_bind_sidebar_and_notebook      (DhSidebar  *sidebar,
                                                         DhNotebook *notebook);

G_END_DECLS

#endif /* DH_UTIL_LIB_H */
