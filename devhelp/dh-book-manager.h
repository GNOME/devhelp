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

#ifndef DH_BOOK_MANAGER_H
#define DH_BOOK_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _DhBookManager         DhBookManager;
typedef struct _DhBookManagerClass    DhBookManagerClass;

#define DH_TYPE_BOOK_MANAGER         (dh_book_manager_get_type ())
#define DH_BOOK_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DH_TYPE_BOOK_MANAGER, DhBookManager))
#define DH_BOOK_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), DH_TYPE_BOOK_MANAGER, DhBookManagerClass))
#define DH_IS_BOOK_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DH_TYPE_BOOK_MANAGER))
#define DH_IS_BOOK_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), DH_TYPE_BOOK_MANAGER))
#define DH_BOOK_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DH_TYPE_BOOK_MANAGER, DhBookManagerClass))

struct _DhBookManager {
        GObject parent_instance;
};

struct _DhBookManagerClass {
        GObjectClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_book_manager_get_type                (void) G_GNUC_CONST;

G_DEPRECATED
DhBookManager * dh_book_manager_new                     (void);

G_DEPRECATED
void            dh_book_manager_populate                (DhBookManager *book_manager);

G_END_DECLS

#endif /* DH_BOOK_MANAGER_H */
