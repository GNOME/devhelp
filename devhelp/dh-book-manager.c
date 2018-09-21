/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
 * Copyright (C) 2012 Thomas Bechtold <toabctl@gnome.org>
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

#include "dh-book-manager.h"

/**
 * SECTION:dh-book-manager
 * @Title: DhBookManager
 * @Short_description: Aggregation of all #DhBook's
 *
 * #DhBookManager was a singleton class containing all the #DhBook's. It is now
 * empty.
 *
 * <warning>
 * This class is entirely deprecated, you need to use #DhProfile, #DhSettings
 * and #DhBookList instead.
 * </warning>
 */

G_DEFINE_TYPE (DhBookManager, dh_book_manager, G_TYPE_OBJECT);

static void
dh_book_manager_class_init (DhBookManagerClass *klass)
{
}

static void
dh_book_manager_init (DhBookManager *book_manager)
{
}

/**
 * dh_book_manager_new:
 *
 * Returns: (transfer full): a new #DhBookManager object.
 * Deprecated: 3.26: the #DhBookManager class is deprecated.
 */
DhBookManager *
dh_book_manager_new (void)
{
        return g_object_new (DH_TYPE_BOOK_MANAGER, NULL);
}

/**
 * dh_book_manager_populate:
 * @book_manager: a #DhBookManager.
 *
 * Deprecated: 3.26: the #DhBookManager class is deprecated.
 */
void
dh_book_manager_populate (DhBookManager *book_manager)
{
}
