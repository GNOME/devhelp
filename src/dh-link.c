/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2008      Imendio AB
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

#include "config.h"
#include "dh-link.h"
#include <string.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

struct _DhLink {
        /* FIXME: Those two could exist only for book to save some
         * memory.
         */
        gchar       *id;
        gchar       *base;

        gchar       *name;
        gchar       *name_collation_key;
        gchar       *filename;

        DhLink      *book;
        DhLink      *page;

        guint        ref_count;

        DhLinkType   type : 8;
        DhLinkFlags  flags : 8;
};

G_DEFINE_BOXED_TYPE (DhLink, dh_link,
                     dh_link_ref, dh_link_unref)

static void
link_free (DhLink *link)
{
        g_free (link->base);
        g_free (link->id);
        g_free (link->name);
        g_free (link->filename);
        g_free (link->name_collation_key);

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
             const gchar *base,
             const gchar *id,
             const gchar *name,
             DhLink      *book,
             DhLink      *page,
             const gchar *filename)
{
        DhLink *link;

        g_return_val_if_fail (name != NULL, NULL);
        g_return_val_if_fail (filename != NULL, NULL);

        if (type == DH_LINK_TYPE_BOOK) {
                g_return_val_if_fail (base != NULL, NULL);
                g_return_val_if_fail (id != NULL, NULL);
        }
        if (type != DH_LINK_TYPE_BOOK && type != DH_LINK_TYPE_PAGE) {
                g_return_val_if_fail (book != NULL, NULL);
                g_return_val_if_fail (page != NULL, NULL);
        }

        link = g_slice_new0 (DhLink);

        link->ref_count = 1;
        link->type = type;

        if (type == DH_LINK_TYPE_BOOK) {
                link->base = g_strdup (base);
                link->id = g_strdup (id);
        }

        link->name = g_strdup (name);
        link->filename = g_strdup (filename);

        if (book) {
                link->book = dh_link_ref (book);
        }
        if (page) {
                link->page = dh_link_ref (page);
        }

        return link;
}

gint
dh_link_compare (gconstpointer a,
                 gconstpointer b)
{
        DhLink *la = (DhLink *) a;
        DhLink *lb = (DhLink *) b;
        gint    flags_diff;

        /* Sort deprecated hits last. */
        flags_diff = (la->flags & DH_LINK_FLAGS_DEPRECATED) -
                (lb->flags & DH_LINK_FLAGS_DEPRECATED);
        if (flags_diff != 0) {
                return flags_diff;
        }

        /* Collation-based sorting */
        if (G_UNLIKELY (!la->name_collation_key))
                la->name_collation_key = g_utf8_collate_key (la->name, -1);
        if (G_UNLIKELY (!lb->name_collation_key))
                lb->name_collation_key = g_utf8_collate_key (lb->name, -1);

        return strcmp (la->name_collation_key,
                       lb->name_collation_key);
}

DhLink *
dh_link_ref (DhLink *link)
{
        g_return_val_if_fail (link != NULL, NULL);

        g_atomic_int_inc (&link->ref_count);

        return link;
}

void
dh_link_unref (DhLink *link)
{
        g_return_if_fail (link != NULL);

        if (g_atomic_int_dec_and_test (&link->ref_count))
        {
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
dh_link_get_file_name (DhLink *link)
{
        /* Return filename if the link is itself a page
         * or if the link is within a page */
        if (link->page ||
            link->type == DH_LINK_TYPE_PAGE) {
                return link->filename;
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

gchar *
dh_link_get_uri (DhLink *link)
{
        gchar *base, *filename, *uri, *anchor;

        if (link->type == DH_LINK_TYPE_BOOK)
                base = link->base;
        else
                base = link->book->base;

        filename = g_build_filename (base, link->filename, NULL);

        anchor = strrchr (filename, '#');
        if (anchor) {
                *anchor = '\0';
                anchor++;
        }

        uri = g_filename_to_uri (filename, NULL, NULL);

        if (anchor) {
                gchar *uri_with_anchor;

                uri_with_anchor = g_strconcat (uri, "#", anchor, NULL);
                g_free (uri);
                uri = uri_with_anchor;
        }
        g_free (filename);

        return uri;
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
