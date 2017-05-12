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
#include <glib/gi18n-lib.h>

/**
 * SECTION:dh-link
 * @Title: DhLink
 * @Short_description: A link inside a #DhBook
 *
 * A #DhLink represents a link to an HTML page or somewhere inside a page (with
 * an anchor) that is inside a #DhBook. The link can point to a specific symbol,
 * or a page, or the top-level page of the #DhBook.
 *
 * A #DhLink has a type that can be retrieved with dh_link_get_link_type().
 *
 * There is exactly one #DhLink of type %DH_LINK_TYPE_BOOK per #DhBook object.
 */

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

/**
 * dh_link_new:
 * @type: the #DhLinkType.
 * @base_path: (nullable): the base path for the book, or %NULL.
 * @book_id: (nullable): the book ID, or %NULL.
 * @name: the name of the link.
 * @book: (nullable): the book that the link is contained in, or %NULL.
 * @page: (nullable): the page that the link is contained in, or %NULL. This
 * parameter is actually broken.
 * @relative_url: the URL relative to the book @base_path. Can contain an
 * anchor.
 *
 * Creates a new #DhLink.
 *
 * @base_path and @book_id must be provided only for a link of type
 * %DH_LINK_TYPE_BOOK.
 *
 * If @type is not a #DH_LINK_TYPE_BOOK and not a #DH_LINK_TYPE_PAGE, then the
 * @book and @page links must be provided.
 *
 * @name and @relative_url must always be provided.
 *
 * Returns: a new #DhLink.
 */
DhLink *
dh_link_new (DhLinkType   type,
             const gchar *base_path,
             const gchar *book_id,
             const gchar *name,
             DhLink      *book,
             DhLink      *page,
             const gchar *relative_url)
{
        DhLink *link;

        g_return_val_if_fail (name != NULL, NULL);
        g_return_val_if_fail (relative_url != NULL, NULL);

        if (type == DH_LINK_TYPE_BOOK) {
                g_return_val_if_fail (base_path != NULL, NULL);
                g_return_val_if_fail (book_id != NULL, NULL);
        }
        if (type != DH_LINK_TYPE_BOOK &&
            type != DH_LINK_TYPE_PAGE) {
                g_return_val_if_fail (book != NULL, NULL);
                g_return_val_if_fail (page != NULL, NULL);
        }

        link = g_slice_new0 (DhLink);

        link->ref_count = 1;
        link->type = type;

        if (type == DH_LINK_TYPE_BOOK) {
                link->base = g_strdup (base_path);
                link->id = g_strdup (book_id);
        }

        link->name = g_strdup (name);
        link->filename = g_strdup (relative_url);

        if (book != NULL) {
                link->book = dh_link_ref (book);
        }
        if (page != NULL) {
                link->page = dh_link_ref (page);
        }

        return link;
}

/**
 * dh_link_compare:
 * @a: (type DhLink): a #DhLink.
 * @b: (type DhLink): a #DhLink.
 *
 * Compares the links @a and @b. This function is used to determine in which
 * order the links should be displayed.
 *
 * Returns: an integer less than zero if @a should appear before @b; zero if
 * there are no preferences; an integer greater than zero if @b should appear
 * before @a.
 */
gint
dh_link_compare (gconstpointer a,
                 gconstpointer b)
{
        DhLink *la = (DhLink *) a;
        DhLink *lb = (DhLink *) b;
        gint    flags_diff;
        gint    diff;

        /* Sort deprecated hits last. */
        flags_diff = (la->flags & DH_LINK_FLAGS_DEPRECATED) -
                (lb->flags & DH_LINK_FLAGS_DEPRECATED);
        if (flags_diff != 0) {
                return flags_diff;
        }

        /* Collation-based sorting */
        if (G_UNLIKELY (la->name_collation_key == NULL))
                la->name_collation_key = g_utf8_collate_key (la->name, -1);
        if (G_UNLIKELY (lb->name_collation_key == NULL))
                lb->name_collation_key = g_utf8_collate_key (lb->name, -1);

        diff = strcmp (la->name_collation_key,
                       lb->name_collation_key);

        /* For the same names, sort page links before other links. The page is
         * more important than a symbol (typically contained in that page).
         */
        if (diff == 0) {
                if (la->type == lb->type)
                        return 0;

                if (la->type == DH_LINK_TYPE_PAGE)
                        return -1;

                if (lb->type == DH_LINK_TYPE_PAGE)
                        return 1;

                return 0;
        }

        return diff;
}

/**
 * dh_link_ref:
 * @link: a #DhLink.
 *
 * Increases the reference count of @link.
 *
 * Returns: (transfer full): the @link.
 */
DhLink *
dh_link_ref (DhLink *link)
{
        g_return_val_if_fail (link != NULL, NULL);

        g_atomic_int_inc (&link->ref_count);

        return link;
}

/**
 * dh_link_unref:
 * @link: a #DhLink.
 *
 * Decreases the reference count of @link.
 */
void
dh_link_unref (DhLink *link)
{
        g_return_if_fail (link != NULL);

        if (g_atomic_int_dec_and_test (&link->ref_count))
        {
                link_free (link);
        }
}

/**
 * dh_link_get_name:
 * @link: a #DhLink.
 *
 * Returns: the name of the @link.
 */
const gchar *
dh_link_get_name (DhLink *link)
{
        return link->name;
}

/**
 * dh_link_get_book_name:
 * @link: a #DhLink.
 *
 * Returns: the name of the book that the @link is contained in.
 */
const gchar *
dh_link_get_book_name (DhLink *link)
{
        if (link->book != NULL) {
                return link->book->name;
        }

        return "";
}

/**
 * dh_link_get_page_name:
 * @link: a #DhLink.
 *
 * Returns: the name of the page that the @link is contained in.
 * Deprecated: 3.26: This function is used nowhere and is actually broken, it
 * returns the book name.
 */
const gchar *
dh_link_get_page_name (DhLink *link)
{
        if (link->page != NULL) {
                return link->page->name;
        }

        return "";
}

/**
 * dh_link_get_file_name:
 * @link: a #DhLink.
 *
 * Returns: the name of the file that the @link is contained in.
 */
const gchar *
dh_link_get_file_name (DhLink *link)
{
        /* Return filename if the link is itself a page or if the link is within
         * a page.
         */
        if (link->page != NULL ||
            link->type == DH_LINK_TYPE_PAGE) {
                return link->filename;
        }

        return "";
}

/**
 * dh_link_get_book_id:
 * @link: a #DhLink.
 *
 * Returns: the book ID.
 */
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

/**
 * dh_link_get_uri:
 * @link: a #DhLink.
 *
 * Returns: the @link URI. Free with g_free() when no longer needed.
 */
gchar *
dh_link_get_uri (DhLink *link)
{
        const gchar *base;
        gchar *filename;
        gchar *uri;
        gchar *anchor;

        if (link->type == DH_LINK_TYPE_BOOK)
                base = link->base;
        else
                base = link->book->base;

        filename = g_build_filename (base, link->filename, NULL);

        anchor = strrchr (filename, '#');
        if (anchor != NULL) {
                *anchor = '\0';
                anchor++;
        }

        uri = g_filename_to_uri (filename, NULL, NULL);

        if (anchor != NULL) {
                gchar *uri_with_anchor;

                uri_with_anchor = g_strconcat (uri, "#", anchor, NULL);
                g_free (uri);
                uri = uri_with_anchor;
        }
        g_free (filename);

        return uri;
}

/**
 * dh_link_get_link_type:
 * @link: a #DhLink.
 *
 * Returns: the #DhLinkType of @link.
 */
DhLinkType
dh_link_get_link_type (DhLink *link)
{
        return link->type;
}

/**
 * dh_link_get_flags:
 * @link: a #DhLink.
 *
 * Returns: the #DhLinkFlags of @link.
 */
DhLinkFlags
dh_link_get_flags (DhLink *link)
{
        return link->flags;
}

/**
 * dh_link_set_flags:
 * @link: a #DhLink.
 * @flags: the new flags of the link.
 *
 * Sets the flags of the link.
 */
void
dh_link_set_flags (DhLink      *link,
                   DhLinkFlags  flags)
{
        link->flags = flags;
}

/**
 * dh_link_get_type_as_string:
 * @link: a #DhLink.
 *
 * Returns: a string representation of the #DhLinkType of @link, translated in
 * the current language.
 */
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
        case DH_LINK_TYPE_PROPERTY:
                /* i18n: in the programming language context, if you don't
                 * have an ESTABLISHED term for it, leave it
                 * untranslated. */
                return _("Property");
        case DH_LINK_TYPE_SIGNAL:
                /* i18n: in the programming language context, if you don't
                 * have an ESTABLISHED term for it, leave it
                 * untranslated. */
                return _("Signal");
        default:
                break;
        }

        return "";
}
