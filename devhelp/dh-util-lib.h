/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2001-2002 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2004,2008 Imendio AB
 * SPDX-FileCopyrightText: 2015, 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_UTIL_LIB_H
#define DH_UTIL_LIB_H

#include <gio/gio.h>
#include "dh-notebook.h"
#include "dh-sidebar.h"
#include "dh-link.h"
#include <libdocset/docset-entry-type.h>

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

G_GNUC_INTERNAL
DhLinkType      _dh_util_link_type_for_docset_type      (DocsetEntryTypeId type);

G_END_DECLS

#endif /* DH_UTIL_LIB_H */
