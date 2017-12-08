/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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

/* Fields used only by DhLink's of type DH_LINK_TYPE_BOOK. */
typedef struct {
        gchar *base_path;
        gchar *book_id;
} BookData;

struct _DhLink {
        /* To avoid some memory padding inside the struct, to use less memory,
         * the fields are placed in this order:
         * 1. All the pointers.
         * 2. Other types.
         * 3. Bit fields.
         *
         * Also, a union is used to use less memory. This struct is allocated a
         * lot, so it is worth optimizing it.
         */

        union {
                /* @book.data is set only for links of @type DH_LINK_TYPE_BOOK. */
                BookData *data;

                /* @book.link is set only for links of @type != DH_LINK_TYPE_BOOK. */
                DhLink *link;
        } book;

        gchar *name;
        gchar *name_collation_key;

        gchar *relative_url;

        guint ref_count;

        DhLinkType type : 8;
        DhLinkFlags flags : 8;
};

/* If the relative_url is empty. */
#define DEFAULT_PAGE "index.html"

G_DEFINE_BOXED_TYPE (DhLink, dh_link,
                     dh_link_ref, dh_link_unref)

static BookData *
book_data_new (const gchar *base_path,
               const gchar *book_id)
{
        BookData *data;

        data = g_slice_new (BookData);
        data->base_path = g_strdup (base_path);
        data->book_id = g_strdup (book_id);

        return data;
}

static void
book_data_free (BookData *data)
{
        if (data == NULL)
                return;

        g_free (data->base_path);
        g_free (data->book_id);
        g_slice_free (BookData, data);
}

static void
link_free (DhLink *link)
{
        if (link->type == DH_LINK_TYPE_BOOK)
                book_data_free (link->book.data);
        else
                dh_link_unref (link->book.link);

        g_free (link->name);
        g_free (link->name_collation_key);
        g_free (link->relative_url);

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
 * @book_title: the name of the link.
 * @relative_url: the URL relative to the book @base_path. Can contain an
 * anchor.
 *
 * Returns: a new #DhLink of type %DH_LINK_TYPE_BOOK.
 * Since: 3.28
 */
DhLink *
dh_link_new_book (const gchar *base_path,
                  const gchar *book_id,
                  const gchar *book_title,
                  const gchar *relative_url)
{
        DhLink *link;

        g_return_val_if_fail (base_path != NULL, NULL);
        g_return_val_if_fail (book_id != NULL, NULL);
        g_return_val_if_fail (book_title != NULL, NULL);
        g_return_val_if_fail (relative_url != NULL, NULL);

        link = dh_link_new_common (DH_LINK_TYPE_BOOK, book_title, relative_url);

        link->book.data = book_data_new (base_path, book_id);

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

        link->book.link = dh_link_ref (book);

        return link;
}

/**
 * dh_link_ref:
 * @link: a #DhLink.
 *
 * Increases the reference count of @link.
 *
 * Not thread-safe.
 *
 * Returns: (transfer full): the @link.
 */
DhLink *
dh_link_ref (DhLink *link)
{
        g_return_val_if_fail (link != NULL, NULL);

        link->ref_count++;

        return link;
}

/**
 * dh_link_unref:
 * @link: a #DhLink.
 *
 * Decreases the reference count of @link.
 *
 * Not thread-safe.
 */
void
dh_link_unref (DhLink *link)
{
        g_return_if_fail (link != NULL);

        if (link->ref_count == 1)
                link_free (link);
        else
                link->ref_count--;
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
 * Returns: the name of the @link. For a link of type %DH_LINK_TYPE_BOOK,
 * returns the book title.
 */
const gchar *
dh_link_get_name (DhLink *link)
{
        g_return_val_if_fail (link != NULL, NULL);

        return link->name;
}

/**
 * dh_link_match_relative_url:
 * @link: a #DhLink.
 * @relative_url: an URL relative to the book base path. Can contain an anchor.
 *
 * Returns: whether the relative URL of @link matches with @relative_url. There
 * is a special case for the index.html page, it can also match the empty
 * string.
 * Since: 3.28
 */
gboolean
dh_link_match_relative_url (DhLink      *link,
                            const gchar *relative_url)
{
        g_return_val_if_fail (link != NULL, FALSE);
        g_return_val_if_fail (link->relative_url != NULL, FALSE);
        g_return_val_if_fail (relative_url != NULL, FALSE);

        if (g_str_equal (link->relative_url, relative_url))
                return TRUE;

        /* Special case for index.html, can also match the empty string.
         * Example of full URLs:
         * file:///usr/share/gtk-doc/html/glib/
         * file:///usr/share/gtk-doc/html/glib/index.html
         *
         * This supports only the root index.html page of a DhBook, this doesn't
         * support index.html inside a sub-directory, if the relative_url
         * contains a sub-directory. But apparently GTK-Doc doesn't create
         * sub-directories, all the *.html pages are in the same directory.
         */
        if (relative_url[0] == '\0')
                return g_str_equal (link->relative_url, DEFAULT_PAGE);
        else if (link->relative_url[0] == '\0')
                return g_str_equal (relative_url, DEFAULT_PAGE);

        return FALSE;
}

/**
 * dh_link_belongs_to_page:
 * @link: a #DhLink.
 * @page_id: a page ID, i.e. the filename without its extension.
 * @case_sensitive: whether @page_id is case sensitive.
 *
 * This function permits to know if @link belongs to a certain page.
 *
 * @page_id is usually the HTML filename without the `.html` extension. More
 * generally, @page_id must be a relative URL (relative to the book base path),
 * without the anchor nor the file extension.
 *
 * For example if @link has the relative URL `"DhLink.html#dh-link-ref"`, then
 * this function will return %TRUE if the @page_id is `"DhLink"`.
 *
 * Returns: whether @link belongs to @page_id.
 * Since: 3.28
 */
gboolean
dh_link_belongs_to_page (DhLink      *link,
                         const gchar *page_id,
                         gboolean     case_sensitive)
{
        const gchar *relative_url;
        gsize page_id_len;
        gboolean has_prefix;

        g_return_val_if_fail (link != NULL, FALSE);
        g_return_val_if_fail (link->relative_url != NULL, FALSE);
        g_return_val_if_fail (page_id != NULL, FALSE);

        relative_url = link->relative_url;
        if (relative_url[0] == '\0')
                relative_url = DEFAULT_PAGE;

        page_id_len = strlen (page_id);

        if (case_sensitive)
                has_prefix = strncmp (relative_url, page_id, page_id_len) == 0;
        else
                has_prefix = g_ascii_strncasecmp (relative_url, page_id, page_id_len) == 0;

        /* Check that a file extension follows. */
        if (has_prefix)
                return relative_url[page_id_len] == '.';

        return FALSE;
}

/**
 * dh_link_get_uri:
 * @link: a #DhLink.
 *
 * Gets the @link URI, by concateneting the book base path with the @link
 * relative URL.
 *
 * Returns: (nullable): the @link URI, or %NULL if getting the URI failed. Free
 * with g_free() when no longer needed.
 */
gchar *
dh_link_get_uri (DhLink *link)
{
        const gchar *base_path;
        gchar *filename;
        gchar *uri;
        gchar *anchor;
        GError *error = NULL;

        g_return_val_if_fail (link != NULL, NULL);

        if (link->type == DH_LINK_TYPE_BOOK)
                base_path = link->book.data->base_path;
        else
                base_path = link->book.link->book.data->base_path;

        filename = g_build_filename (base_path, link->relative_url, NULL);

        anchor = strrchr (filename, '#');
        if (anchor != NULL) {
                *anchor = '\0';
                anchor++;
        }

        uri = g_filename_to_uri (filename, NULL, &error);
        if (error != NULL) {
                g_warning ("Failed to get DhLink URI: %s", error->message);
                g_clear_error (&error);
        }

        if (uri != NULL && anchor != NULL) {
                gchar *uri_with_anchor;

                uri_with_anchor = g_strconcat (uri, "#", anchor, NULL);
                g_free (uri);
                uri = uri_with_anchor;
        }

        g_free (filename);
        return uri;
}

/**
 * dh_link_get_book_title:
 * @link: a #DhLink.
 *
 * Returns: the title of the book that the @link is contained in.
 */
const gchar *
dh_link_get_book_title (DhLink *link)
{
        g_return_val_if_fail (link != NULL, NULL);

        if (link->type == DH_LINK_TYPE_BOOK)
                return link->name;

        if (link->book.link != NULL)
                return link->book.link->name;

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
                return link->book.data->book_id;

        if (link->book.link != NULL)
                return link->book.link->book.data->book_id;

        return "";
}

static gint
dh_link_type_compare (DhLinkType a,
                      DhLinkType b)
{
        if (a == b)
                return 0;

        /* Same order as in a tree: first the top-level book node, then pages,
         * then keywords (keywords are contained in a page).
         */

        if (a == DH_LINK_TYPE_BOOK)
                return -1;
        if (b == DH_LINK_TYPE_BOOK)
                return 1;

        if (a == DH_LINK_TYPE_PAGE)
                return -1;
        if (b == DH_LINK_TYPE_PAGE)
                return 1;

        return 0;
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

        if (diff != 0)
                return diff;

        return dh_link_type_compare (la->type, lb->type);
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
