/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
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
#include <devhelp/dh-link.h>
#include <devhelp/dh-profile.h>

G_BEGIN_DECLS

#define DH_TYPE_KEYWORD_MODEL            (dh_keyword_model_get_type ())
G_DECLARE_FINAL_TYPE (DhKeywordModel, dh_keyword_model, DH, KEYWORD_MODEL, GObject)

enum {
        DH_KEYWORD_MODEL_COL_NAME,
        DH_KEYWORD_MODEL_COL_LINK,
        DH_KEYWORD_MODEL_COL_CURRENT_BOOK_FLAG,
        DH_KEYWORD_MODEL_NUM_COLS
};

DhKeywordModel *dh_keyword_model_new    (void);
DhLink         *dh_keyword_model_filter (DhKeywordModel *model,
                                         const gchar    *search_string,
                                         const gchar    *current_book_id,
                                         DhProfile      *profile);

G_END_DECLS

