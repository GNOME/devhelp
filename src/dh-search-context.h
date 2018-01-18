/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DH_SEARCH_CONTEXT_H
#define DH_SEARCH_CONTEXT_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _DhSearchContext DhSearchContext;

G_GNUC_INTERNAL
DhSearchContext *       _dh_search_context_new                  (const gchar *search_string);

G_GNUC_INTERNAL
void                    _dh_search_context_free                 (DhSearchContext *search);

G_GNUC_INTERNAL
const gchar *           _dh_search_context_get_book_id          (DhSearchContext *search);

G_GNUC_INTERNAL
const gchar *           _dh_search_context_get_page_id          (DhSearchContext *search);

G_GNUC_INTERNAL
GStrv                   _dh_search_context_get_keywords         (DhSearchContext *search);

G_END_DECLS

#endif /* DH_SEARCH_CONTEXT_H */
