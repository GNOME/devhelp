/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2005-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
 * Copyright (C) 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
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

#ifndef DH_BOOK_H
#define DH_BOOK_H

#include <gio/gio.h>
#include "dh-completion.h"

G_BEGIN_DECLS

typedef struct _DhBook      DhBook;
typedef struct _DhBookClass DhBookClass;

#define DH_TYPE_BOOK         (dh_book_get_type ())
#define DH_BOOK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DH_TYPE_BOOK, DhBook))
#define DH_BOOK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), DH_TYPE_BOOK, DhBookClass))
#define DH_IS_BOOK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DH_TYPE_BOOK))
#define DH_IS_BOOK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), DH_TYPE_BOOK))
#define DH_BOOK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DH_TYPE_BOOK, DhBookClass))

struct _DhBook {
        GObject parent_instance;
};

struct _DhBookClass {
        GObjectClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType        dh_book_get_type        (void) G_GNUC_CONST;

DhBook *     dh_book_new             (GFile *index_file);

GFile *      dh_book_get_index_file  (DhBook *book);

const gchar *dh_book_get_id          (DhBook *book);

const gchar *dh_book_get_title       (DhBook *book);

const gchar *dh_book_get_language    (DhBook *book);

GList *      dh_book_get_links       (DhBook *book);

GNode *      dh_book_get_tree        (DhBook *book);

DhCompletion *dh_book_get_completion (DhBook *book);

gboolean     dh_book_get_enabled     (DhBook *book);

void         dh_book_set_enabled     (DhBook   *book,
                                      gboolean  enabled);

gint         dh_book_cmp_by_id       (DhBook *a,
                                      DhBook *b);

gint         dh_book_cmp_by_title    (DhBook *a,
                                      DhBook *b);

G_END_DECLS

#endif /* DH_BOOK_H */
