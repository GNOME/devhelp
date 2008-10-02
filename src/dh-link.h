/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
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
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __DH_LINK_H__
#define __DH_LINK_H__

#include <glib-object.h>

typedef enum {
        DH_LINK_TYPE_BOOK,
        DH_LINK_TYPE_PAGE,
        DH_LINK_TYPE_KEYWORD,
        DH_LINK_TYPE_FUNCTION,
        DH_LINK_TYPE_STRUCT,
        DH_LINK_TYPE_MACRO,
        DH_LINK_TYPE_ENUM,
        DH_LINK_TYPE_TYPEDEF
} DhLinkType;

typedef struct {
        gchar       *name;
        gchar       *book;
        gchar       *page;
        gchar       *uri;
        DhLinkType   type;

        /* FIXME: Use an enum or flags for this when we know more about what we
         * can do (or keep the actual deprecation string).
         */
        gboolean     is_deprecated;

        guint        ref_count;
} DhLink;

#define DH_TYPE_LINK dh_link_get_type ()

GType        dh_link_get_type           (void);
DhLink *     dh_link_new                (DhLinkType     type,
					 const gchar   *name,
					 const gchar   *book,
					 const gchar   *page,
					 const gchar   *uri);
DhLink *     dh_link_copy               (const DhLink  *link);
void         dh_link_free               (DhLink        *link);
gint         dh_link_compare            (gconstpointer  a,
					 gconstpointer  b);
DhLink *     dh_link_ref                (DhLink        *link);
void         dh_link_unref              (DhLink        *link);
gboolean     dh_link_get_is_deprecated  (DhLink        *link);
void         dh_link_set_is_deprecated  (DhLink        *link,
					 gboolean       is_deprecated);
const gchar *dh_link_get_type_as_string (DhLink        *link);

#endif /* __DH_LINK_H__ */
