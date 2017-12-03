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
        gchar       *book_id;
        gchar       *base_path;

        DhLink      *book;

        gchar       *name;
        gchar       *name_collation_key;
        gchar       *relative_url;

        guint        ref_count;

        DhLinkType   type : 8;
        DhLinkFlags  flags : 8;
};

G_DEFINE_BOXED_TYPE (DhLink, dh_link,
                     dh_link_ref, dh_link_unref)

static void
link_free (DhLink *link)
{
        g_free (link->book_id);
        g_free (link->base_path);
        g_free (link->name);
        g_free (link->name_collation_key);
        g_free (link->relative_url);

        if (link->book != NULL)
                dh_link_unref (link->book);

        g_slice_free (DhLink, link);
}

static DhLink *
dh_link_new_common (DhLinkType   type,
                    const gchar *name,
                    const gchar *relative_url)
{
        DhLink *link;

        link = g_slice_new0 (DhLink);
        link->ref_count = 1;
        link->type = type;
        link->name = g_strdup (name);
        link->relative_url = g_strdup (relative_url);

        return link;
}

/**
 * dh_link_new_book:
 * @base_path: the base path for the book.
 * @book_id: the book ID.
 * @name: the name of the link.
 * @relative_url: the URL relative to the book @base_path. Can contain an
 * anchor.
 *
 * Returns: a new #DhLink of type %DH_LINK_TYPE_BOOK.
 * Since: 3.28
 */
DhLink *
dh_link_new_book (const gchar *base_path,
                  const gchar *book_id,
                  const gchar *name,
                  const gchar *relative_url)
{
        DhLink *link;

        g_return_val_if_fail (base_path != NULL, NULL);
        g_return_val_if_fail (book_id != NULL, NULL);
        g_return_val_if_fail (name != NULL, NULL);
        g_return_val_if_fail (relative_url != NULL, NULL);

        link = dh_link_new_common (DH_LINK_TYPE_BOOK, name, relative_url);

        link->base_path = g_strdup (base_path);
        link->book_id = g_strdup (book_id);

        return link;
}

/**
 * dh_link_new:
 * @type: the #DhLinkType. Must be different than %DH_LINK_TYPE_BOOK.
 * @book: the book that the link is contained in.
 * @name: the name of the link.
 * @relative_url: the URL relative to the book @base_path. Can contain an
 * anchor.
 *
 * Returns: a new #DhLink.
 */
DhLink *
dh_link_new (DhLinkType   type,
             DhLink      *book,
             const gchar *name,
             const gchar *relative_url)
{
        DhLink *link;

        g_return_val_if_fail (type != DH_LINK_TYPE_BOOK, NULL);
        g_return_val_if_fail (book != NULL, NULL);
        g_return_val_if_fail (name != NULL, NULL);
        g_return_val_if_fail (relative_url != NULL, NULL);

        link = dh_link_new_common (type, name, relative_url);

        link->book = dh_link_ref (book);

        return link;
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
 * dh_link_get_link_type:
 * @link: a #DhLink.
 *
 * Returns: the #DhLinkType of @link.
 */
DhLinkType
dh_link_get_link_type (DhLink *link)
{
        g_return_val_if_fail (link != NULL, 0);

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
        g_return_val_if_fail (link != NULL, DH_LINK_FLAGS_NONE);

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
        g_return_if_fail (link != NULL);

        link->flags = flags;
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
        g_return_val_if_fail (link != NULL, NULL);

        return link->name;
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
        g_return_val_if_fail (link != NULL, NULL);

        /* Return filename if the link is itself a page or if the link is within
         * a page (i.e. every link type except a book).
         */
        if (link->type != DH_LINK_TYPE_BOOK)
                return link->relative_url;

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
        const gchar *base_path;
        gchar *filename;
        gchar *uri;
        gchar *anchor;

        g_return_val_if_fail (link != NULL, NULL);

        if (link->type == DH_LINK_TYPE_BOOK)
                base_path = link->base_path;
        else
                base_path = link->book->base_path;

        filename = g_build_filename (base_path, link->relative_url, NULL);

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
 * dh_link_get_book_name:
 * @link: a #DhLink.
 *
 * Returns: the name of the book that the @link is contained in.
 */
const gchar *
dh_link_get_book_name (DhLink *link)
{
        g_return_val_if_fail (link != NULL, NULL);

        if (link->book != NULL)
                return link->book->name;

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
        g_return_val_if_fail (link != NULL, NULL);

        if (link->type == DH_LINK_TYPE_BOOK)
                return link->book_id;

        if (link->book != NULL)
                return link->book->book_id;

        return "";
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
        gint flags_diff;
        gint diff;

        g_return_val_if_fail (a != NULL, 0);
        g_return_val_if_fail (b != NULL, 0);

        /* Sort deprecated hits last. */
        flags_diff = ((la->flags & DH_LINK_FLAGS_DEPRECATED) -
                      (lb->flags & DH_LINK_FLAGS_DEPRECATED));
        if (flags_diff != 0)
                return flags_diff;

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
 * dh_link_type_to_string:
 * @link_type: a #DhLinkType.
 *
 * Returns: a string representation of the #DhLinkType, translated in the
 * current language.
 */
const gchar *
dh_link_type_to_string (DhLinkType link_type)
{
        switch (link_type) {
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

        g_return_val_if_reached ("");
}
