/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2002 CodeFactory AB
 * SPDX-FileCopyrightText: 2002 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2004-2008 Imendio AB
 * SPDX-FileCopyrightText: 2010 Lanedo GmbH
 * SPDX-FileCopyrightText: 2012 Thomas Bechtold <toabctl@gnome.org>
 * SPDX-FileCopyrightText: 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
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
