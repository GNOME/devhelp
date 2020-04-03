/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2003 CodeFactory AB
 * SPDX-FileCopyrightText: 2003 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_PARSER_H
#define DH_PARSER_H

#include <gio/gio.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL
gboolean _dh_parser_read_file (GFile   *index_file,
                               gchar  **book_title,
                               gchar  **book_id,
                               gchar  **book_language,
                               GNode  **book_tree,
                               GList  **all_links,
                               GError **error);

G_END_DECLS

#endif /* DH_PARSER_H */
