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

#pragma once

#include <glib-object.h>
#include <devhelp/dh-book-list.h>
#include <devhelp/dh-settings.h>

G_BEGIN_DECLS

#define DH_TYPE_PROFILE             (dh_profile_get_type ())
G_DECLARE_FINAL_TYPE (DhProfile, dh_profile, DH, PROFILE, GObject)

G_GNUC_INTERNAL
DhProfile  *_dh_profile_new           (DhSettings *settings,
                                       DhBookList *book_list);
DhProfile  *dh_profile_get_default    (void);
G_GNUC_INTERNAL
void        _dh_profile_unref_default (void);
DhSettings *dh_profile_get_settings   (DhProfile  *profile);
DhBookList *dh_profile_get_book_list  (DhProfile  *profile);

G_END_DECLS

