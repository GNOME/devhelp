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
#include <devhelp/dh-profile.h>
#include <devhelp/dh-settings.h>

G_BEGIN_DECLS

#define DH_TYPE_PROFILE_BUILDER             (dh_profile_builder_get_type ())
G_DECLARE_FINAL_TYPE (DhProfileBuilder, dh_profile_builder, DH, PROFILE_BUILDER, GObject)

DhProfileBuilder *dh_profile_builder_new           (void);
void              dh_profile_builder_set_settings  (DhProfileBuilder *builder,
                                                    DhSettings       *settings);
void              dh_profile_builder_set_book_list (DhProfileBuilder *builder,
                                                    DhBookList       *book_list);
DhProfile        *dh_profile_builder_create_object (DhProfileBuilder *builder);

G_END_DECLS

