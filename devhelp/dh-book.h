/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2002 CodeFactory AB
 * SPDX-FileCopyrightText: 2002 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2005-2008 Imendio AB
 * SPDX-FileCopyrightText: 2010 Lanedo GmbH
 * SPDX-FileCopyrightText: 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_BOOK_H
#define DH_BOOK_H

#include <gio/gio.h>
#include <devhelp/dh-completion.h>

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

gint         dh_book_cmp_by_id       (DhBook *a,
                                      DhBook *b);

gint         dh_book_cmp_by_title    (DhBook *a,
                                      DhBook *b);

G_END_DECLS

#endif /* DH_BOOK_H */
