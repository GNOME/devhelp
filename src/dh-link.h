/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2008 Imendio AB
 * Copyright (C) 2017 Sébastien Wilmet <swilmet@gnome.org>
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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DH_LINK_H
#define DH_LINK_H

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * DhLinkType:
 * @DH_LINK_TYPE_BOOK: The top-level page of a #DhBook.
 * @DH_LINK_TYPE_PAGE: A page.
 * @DH_LINK_TYPE_KEYWORD: Another kind of keyword.
 * @DH_LINK_TYPE_FUNCTION: A function keyword.
 * @DH_LINK_TYPE_STRUCT: A struct keyword.
 * @DH_LINK_TYPE_MACRO: A macro keyword.
 * @DH_LINK_TYPE_ENUM: An enum keyword.
 * @DH_LINK_TYPE_TYPEDEF: A typedef keyword.
 * @DH_LINK_TYPE_PROPERTY: A property keyword.
 * @DH_LINK_TYPE_SIGNAL: A signal keyword.
 *
 * The type of the content the link points to.
 */
typedef enum {
        DH_LINK_TYPE_BOOK,
        DH_LINK_TYPE_PAGE,
        DH_LINK_TYPE_KEYWORD,
        DH_LINK_TYPE_FUNCTION,
        DH_LINK_TYPE_STRUCT,
        DH_LINK_TYPE_MACRO,
        DH_LINK_TYPE_ENUM,
        DH_LINK_TYPE_TYPEDEF,
        DH_LINK_TYPE_PROPERTY,
        DH_LINK_TYPE_SIGNAL
} DhLinkType;

/**
 * DhLinkFlags:
 * @DH_LINK_FLAGS_NONE: No flags set.
 * @DH_LINK_FLAGS_DEPRECATED: The symbol that the link points to is deprecated.
 */
typedef enum {
        DH_LINK_FLAGS_NONE       = 0,
        DH_LINK_FLAGS_DEPRECATED = 1 << 0
} DhLinkFlags;

typedef struct _DhLink DhLink;

#define DH_TYPE_LINK (dh_link_get_type ())

GType        dh_link_get_type           (void);

DhLink *     dh_link_new_book           (const gchar   *base_path,
                                         const gchar   *book_id,
                                         const gchar   *book_title,
                                         const gchar   *relative_url);

DhLink *     dh_link_new                (DhLinkType     type,
                                         DhLink        *book_link,
                                         const gchar   *name,
                                         const gchar   *relative_url);

DhLink *     dh_link_ref                (DhLink        *link);

void         dh_link_unref              (DhLink        *link);

DhLinkType   dh_link_get_link_type      (DhLink        *link);

DhLinkFlags  dh_link_get_flags          (DhLink        *link);

void         dh_link_set_flags          (DhLink        *link,
                                         DhLinkFlags    flags);

const gchar *dh_link_get_name           (DhLink        *link);

gboolean     dh_link_match_relative_url (DhLink        *link,
                                         const gchar   *relative_url);

gboolean     dh_link_belongs_to_page    (DhLink        *link,
                                         const gchar   *page_id,
                                         gboolean       case_sensitive);

gchar *      dh_link_get_uri            (DhLink        *link);

const gchar *dh_link_get_book_title     (DhLink        *link);

const gchar *dh_link_get_book_id        (DhLink        *link);

gint         dh_link_compare            (gconstpointer  a,
                                         gconstpointer  b);

const gchar *dh_link_type_to_string     (DhLinkType     link_type);

G_END_DECLS

#endif /* DH_LINK_H */
