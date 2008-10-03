/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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

#include "config.h"
#include <string.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include "dh-link.h"

struct _DhLink {
        gchar       *name;
        gchar       *uri;

        gchar       *book;
        gchar       *page;

        guint        ref_count;

        DhLinkType   type : 8;
        DhLinkFlags  flags : 8;
};

GType
dh_link_get_type (void)
{
        static GType type = 0;

        if (G_UNLIKELY (type == 0)) {
                type = g_boxed_type_register_static (
                        "DhLink",
                        (GBoxedCopyFunc) dh_link_ref,
                        (GBoxedFreeFunc) dh_link_unref);
        }
        return type;
}

static void
link_free (DhLink *link)
{
	g_free (link->name);
	g_free (link->book);
	g_free (link->page);
	g_free (link->uri);

	g_slice_free (DhLink, link);
}

DhLink *
dh_link_new (DhLinkType   type,
	     const gchar *name,
	     const gchar *book,
	     const gchar *page,
	     const gchar *uri)
{
	DhLink *link;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (uri != NULL, NULL);

	link = g_slice_new0 (DhLink);

	link->type = type;

	link->name = g_strdup (name);
	link->book = g_strdup (book);
	link->page = g_strdup (page);
	link->uri  = g_strdup (uri);

	link->ref_count = 1;

	return link;
}

gint
dh_link_compare  (gconstpointer a,
                  gconstpointer b)
{
        const DhLink *la = a;
        const DhLink *lb = b;
	gint          flags_diff;
	gint          book_diff;
	gint          page_diff;

        /* Sort deprecated hits last. */
        flags_diff = (la->flags & DH_LINK_FLAGS_DEPRECATED) - 
                (lb->flags & DH_LINK_FLAGS_DEPRECATED);
        if (flags_diff != 0) {
                return flags_diff;
        }

	book_diff = strcmp (la->book, lb->book);
	if (book_diff == 0) {
		if (la->page == 0 && lb->page == 0) {
			page_diff = 0;
		} else {
			page_diff = (la->page && lb->page) ?
                                strcmp (la->page, lb->page) : -1;
		}

		if (page_diff == 0) {
			return strcmp (la->name, lb->name);
                }

		return page_diff;
	}

	return book_diff;
}

DhLink *
dh_link_ref (DhLink *link)
{
	g_return_val_if_fail (link != NULL, NULL);

	link->ref_count++;

	return link;
}

void
dh_link_unref (DhLink *link)
{
	g_return_if_fail (link != NULL);

	link->ref_count--;

	if (link->ref_count == 0) {
		link_free (link);
	}
}

const gchar *
dh_link_get_name (DhLink *link)
{
        return link->name;
}

const gchar *
dh_link_get_book (DhLink *link)
{
        return link->book;
}

const gchar *
dh_link_get_page (DhLink *link)
{
        return link->page;
}

const gchar *
dh_link_get_uri (DhLink *link)
{
        return link->uri;
}

DhLinkType
dh_link_get_link_type (DhLink *link)
{
        return link->type;
}

DhLinkFlags
dh_link_get_flags (DhLink *link)
{
	return link->flags;
}

void
dh_link_set_flags (DhLink      *link,
                   DhLinkFlags  flags)
{
        link->flags = flags;
}

const gchar *
dh_link_get_type_as_string (DhLink *link)
{
        switch (link->type) {
        case DH_LINK_TYPE_BOOK:
                /* i18n: a documentation book */
                return _("Book");
	case DH_LINK_TYPE_PAGE:
                /* i18n: a "page" in a documentation book */
                return _("Page");
	case DH_LINK_TYPE_KEYWORD:
                /* i18n: a search hit in the documentation, could be a
                 * function, macro, struct, etc */
                return _("Keyword");
        case DH_LINK_TYPE_FUNCTION:
                /* i18n: in the programming language context, if you don't
                 * have an ESTABLISHED term for it, leave it
                 * untranslated. */
                return _("Function");
	case DH_LINK_TYPE_STRUCT:
                /* i18n: in the programming language context, if you don't
                 * have an ESTABLISHED term for it, leave it
                 * untranslated. */
                return _("Struct");
	case DH_LINK_TYPE_MACRO:
                /* i18n: in the programming language context, if you don't
                 * have an ESTABLISHED term for it, leave it
                 * untranslated. */
                return _("Macro");
	case DH_LINK_TYPE_ENUM:
                /* i18n: in the programming language context, if you don't
                 * have an ESTABLISHED term for it, leave it
                 * untranslated. */
               return _("Enum");
	case DH_LINK_TYPE_TYPEDEF:
                /* i18n: in the programming language context, if you don't
                 * have an ESTABLISHED term for it, leave it
                 * untranslated. */
                return _("Type");
        }

        return "";
}
