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
#include "dh-book-list.h"
#include "dh-settings.h"

G_BEGIN_DECLS

#define DH_TYPE_BOOK_LIST_SIMPLE             (dh_book_list_simple_get_type ())
G_DECLARE_DERIVABLE_TYPE (DhBookListSimple, dh_book_list_simple, DH, BOOK_LIST_SIMPLE, DhBookList)

struct _DhBookListSimpleClass {
        DhBookListClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

G_GNUC_INTERNAL
DhBookList *_dh_book_list_simple_new (GList      *sub_book_lists,
                                      DhSettings *settings);
G_END_DECLS

