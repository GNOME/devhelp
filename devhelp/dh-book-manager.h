/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2010 Lanedo GmbH
 * Copyright (C) 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
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

G_BEGIN_DECLS

#define DH_TYPE_BOOK_MANAGER         (dh_book_manager_get_type ())
G_DECLARE_FINAL_TYPE (DhBookManager, dh_book_manager, DH, BOOK_MANAGER, GObject)

G_DEPRECATED
DhBookManager *dh_book_manager_new      (void);
G_DEPRECATED
void           dh_book_manager_populate (DhBookManager *book_manager);

G_END_DECLS

