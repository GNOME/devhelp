/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_BOOK_LIST_DIRECTORY_H
#define DH_BOOK_LIST_DIRECTORY_H

#include <gio/gio.h>
#include <devhelp/dh-book-list.h>

G_BEGIN_DECLS

#define DH_TYPE_BOOK_LIST_DIRECTORY             (dh_book_list_directory_get_type ())
#define DH_BOOK_LIST_DIRECTORY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_BOOK_LIST_DIRECTORY, DhBookListDirectory))
#define DH_BOOK_LIST_DIRECTORY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_BOOK_LIST_DIRECTORY, DhBookListDirectoryClass))
#define DH_IS_BOOK_LIST_DIRECTORY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_BOOK_LIST_DIRECTORY))
#define DH_IS_BOOK_LIST_DIRECTORY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_BOOK_LIST_DIRECTORY))
#define DH_BOOK_LIST_DIRECTORY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_BOOK_LIST_DIRECTORY, DhBookListDirectoryClass))

typedef struct _DhBookListDirectory         DhBookListDirectory;
typedef struct _DhBookListDirectoryClass    DhBookListDirectoryClass;
typedef struct _DhBookListDirectoryPrivate  DhBookListDirectoryPrivate;

struct _DhBookListDirectory {
        DhBookList parent;

        DhBookListDirectoryPrivate *priv;
};

struct _DhBookListDirectoryClass {
        DhBookListClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType                   dh_book_list_directory_get_type         (void);

DhBookListDirectory *   dh_book_list_directory_new              (GFile *directory);

GFile *                 dh_book_list_directory_get_directory    (DhBookListDirectory *list_directory);

G_END_DECLS

#endif /* DH_BOOK_LIST_DIRECTORY_H */
