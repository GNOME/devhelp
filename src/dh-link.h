/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2008 Imendio AB
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
 * @DH_LINK_TYPE_BOOK: a book
 * @DH_LINK_TYPE_PAGE: a page
 * @DH_LINK_TYPE_KEYWORD: a keyword
 * @DH_LINK_TYPE_FUNCTION: a function
 * @DH_LINK_TYPE_STRUCT: a struct
 * @DH_LINK_TYPE_MACRO: a macro
 * @DH_LINK_TYPE_ENUM: an enum
 * @DH_LINK_TYPE_TYPEDEF: a typedef
 * @DH_LINK_TYPE_PROPERTY: a property
 * @DH_LINK_TYPE_SIGNAL: a signal
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
DhLink *     dh_link_new                (DhLinkType     type,
                                         const gchar   *base,
                                         const gchar   *id,
                                         const gchar   *name,
                                         DhLink        *book,
                                         DhLink        *page,
                                         const gchar   *filename);
DhLink *     dh_link_ref                (DhLink        *link);
void         dh_link_unref              (DhLink        *link);
gint         dh_link_compare            (gconstpointer  a,
                                         gconstpointer  b);
const gchar *dh_link_get_name           (DhLink        *link);
const gchar *dh_link_get_book_name      (DhLink        *link);
const gchar *dh_link_get_page_name      (DhLink        *link);
const gchar *dh_link_get_file_name      (DhLink        *link);
const gchar *dh_link_get_book_id        (DhLink        *link);
gchar       *dh_link_get_uri            (DhLink        *link);
DhLinkFlags  dh_link_get_flags          (DhLink        *link);
void         dh_link_set_flags          (DhLink        *link,
                                         DhLinkFlags    flags);
DhLinkType   dh_link_get_link_type      (DhLink        *link);
const gchar *dh_link_get_type_as_string (DhLink        *link);

G_END_DECLS

#endif /* DH_LINK_H */
