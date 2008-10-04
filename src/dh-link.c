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
        gchar       *id;

        gchar       *name;
        gchar       *uri;

        DhLink      *book;
        DhLink      *page;

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
	g_free (link->id);
	g_free (link->name);
	g_free (link->uri);

        if (link->book) {
                dh_link_unref (link->book);
        }
	if (link->page) {
                dh_link_unref (link->page);
        }

	g_slice_free (DhLink, link);
}

DhLink *
dh_link_new (DhLinkType   type,
	     const gchar *id,
	     const gchar *name,
	     DhLink      *book,
	     DhLink      *page,
	     const gchar *uri)
{
	DhLink *link;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (uri != NULL, NULL);

	link = g_slice_new0 (DhLink);

	link->ref_count = 1;
	link->type = type;

	link->id = g_strdup (id);
	link->name = g_strdup (name);
	link->uri  = g_strdup (uri);

	if (book) {
                link->book = dh_link_ref (book);
        }
	if (page) {
                link->page = dh_link_ref (page);
        }

	return link;
}

gint
dh_link_compare  (gconstpointer a,
                  gconstpointer b)
{
        DhLink *la = (DhLink *) a;
        DhLink *lb = (DhLink *) b;
	gint    flags_diff;
	gint    book_diff;
	gint    page_diff;

        /* Sort deprecated hits last. */
        flags_diff = (la->flags & DH_LINK_FLAGS_DEPRECATED) - 
                (lb->flags & DH_LINK_FLAGS_DEPRECATED);
        if (flags_diff != 0) {
                return flags_diff;
        }

	book_diff = strcmp (dh_link_get_book_name (la), dh_link_get_book_name (lb));
	if (book_diff == 0) {
                page_diff = strcmp (dh_link_get_page_name (la), dh_link_get_page_name (lb));
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
dh_link_get_book_name (DhLink *link)
{
        if (link->book) {
                return link->book->name;
        }

        return "";
}

const gchar *
dh_link_get_page_name (DhLink *link)
{
        if (link->page) {
                return link->page->name;
        }

        return "";
}

const gchar *
dh_link_get_book_id (DhLink *link)
{
        if (link->type == DH_LINK_TYPE_BOOK) {
                return link->id;
        }

        if (link->book) {
                return link->book->id;
        }

        return "";
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
