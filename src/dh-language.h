/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2010 Lanedo GmbH
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

#ifndef __DH_LANGUAGE_H__
#define __DH_LANGUAGE_H__

#include <glib-object.h>

typedef struct _DhLanguage DhLanguage;

DhLanguage  *dh_language_new                 (const gchar      *name);
void         dh_language_free                (DhLanguage       *language);
const gchar *dh_language_get_name            (DhLanguage       *language);
gint         dh_language_compare             (const DhLanguage *language_a,
                                              const DhLanguage *language_b);
gint         dh_language_compare_by_name     (const DhLanguage *language_a,
                                              const gchar      *language_name_b);
gint         dh_language_get_n_books_enabled (DhLanguage       *language);
void         dh_language_inc_n_books_enabled (DhLanguage       *language);
gboolean     dh_language_dec_n_books_enabled (DhLanguage       *language);

#endif /* __DH_LANGUAGE_H__ */
