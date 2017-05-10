/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 CodeFactory AB
 * Copyright (C) 2003 Mikael Hallendal <micke@imendio.com>
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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DH_PARSER_H
#define DH_PARSER_H

#include <glib.h>

G_BEGIN_DECLS

gboolean dh_parser_read_file (const gchar  *index_file_path,
                              gchar       **book_title,
                              gchar       **book_name,
                              gchar       **book_language,
                              GNode       **book_tree,
                              GList       **keywords,
                              GError      **error);

G_END_DECLS

#endif /* DH_PARSER_H */
