/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2002-2003 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2002-2003 CodeFactory AB
 * SPDX-FileCopyrightText: 2005, 2008 Imendio AB
 * SPDX-FileCopyrightText: 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"
#include "dh-parser.h"
#include <string.h>
#include "dh-error.h"
#include "dh-link.h"
#include "dh-util-lib.h"
#include <libdocset/docset.h>

/* Possible things to do for the version 3 of the Devhelp index file format (if
 * one day there is a strong desire to create a new version):
 * - Replace <functions> element by <keywords>.
 * - Maybe have an up-to-date URI for the NAMESPACE.
 * - Rename <book> attribute 'name' to 'id', because "book name" can be confused
 *   with the book title or other 'name' attributes (for <sub> and <keyword>,
 *   the 'name' attribute has a different meaning). With "book ID" there is no
 *   ambiguity.
 * - Maybe rename <book> attribute 'title' to 'name', to be consistent with
 *   the <sub> and <keyword> elements. dh_link_get_name() would also have a
 *   clearer meaning for book top-level links. But "book title" has the
 *   advantage that there is no ambiguity.
 */

/* It's the xmlns attribute. It is currently (well, in 2015 at least) used on
 * developer.gnome.org to look for <keyword> elements attached to that
 * namespace.
 *
 * devhelp.net was initially the Devhelp website, but it is now no longer the
 * case. But it is not a problem, a namespace qualifies a node name, it doesn't
 * have to be a real site. And it is now too late to change it, at least for the
 * format version 2.
 *
 * See:
 * https://bugzilla.gnome.org/show_bug.cgi?id=566447
 * https://bugzilla.gnome.org/show_bug.cgi?id=749591#c1
 */
#define NAMESPACE "http://www.devhelp.net/book"

#define BYTES_PER_READ 4096

typedef enum {
        FORMAT_VERSION_1,

        /* The main change is that version 2 uses <keyword> instead of
         * <function>.
         */
        FORMAT_VERSION_2,
        /* Docset support */
        FORMAT_DOCSET
} FormatVersion;

typedef struct {
        GMarkupParser *markup_parser;
        GMarkupParseContext *context;

        GFile *index_file;

        gchar *book_title;
        gchar *book_id;
        gchar *book_language;

        /* List of all DhLink* */
        GList *all_links;

        /* Tree of DhLink* (not including keywords).
         * The top node of the book.
         */
        GNode *book_node;

        /* Current sub section node */
        GNode *parent_node;

        FormatVersion version;

        guint parsing_chapters : 1;
        guint parsing_keywords : 1;
} DhParser;

static void
dh_parser_free (DhParser *parser)
{
        g_markup_parse_context_free (parser->context);
        g_free (parser->markup_parser);

        g_clear_object (&parser->index_file);

        g_free (parser->book_title);
        g_free (parser->book_id);
        g_free (parser->book_language);

        g_list_free_full (parser->all_links, (GDestroyNotify)dh_link_unref);
        _dh_util_free_book_tree (parser->book_node);

        g_free (parser);
}

static void
replace_newlines_by_spaces (gchar *str)
{
        gint i;

        if (str == NULL)
                return;

        for (i = 0; str[i] != '\0'; i++) {
                if (str[i] == '\n' || str[i] == '\r')
                        str[i] = ' ';
        }
}

static void
parser_start_node_book (DhParser             *parser,
                        GMarkupParseContext  *context,
                        const gchar          *node_name,
                        const gchar         **attribute_names,
                        const gchar         **attribute_values,
                        GError              **error)
{
        gint line;
        gint col;
        gint attr_num;
        gchar *base = NULL;
        const gchar *name = NULL;
        const gchar *title = NULL;
        const gchar *uri = NULL;
        const gchar *language = NULL;
        DhLink *link;

        if (g_ascii_strcasecmp (node_name, "book") != 0) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             "Expected <book> element, got <%s> at line %d, column %d.",
                             node_name, line, col);
                return;
        }

        for (attr_num = 0; attribute_names[attr_num] != NULL; attr_num++) {
                if (g_ascii_strcasecmp (attribute_names[attr_num], "xmlns") == 0) {
                        const gchar *xmlns;

                        xmlns = attribute_values[attr_num];
                        if (g_ascii_strcasecmp (xmlns, NAMESPACE) != 0) {
                                g_markup_parse_context_get_position (context, &line, &col);
                                g_set_error (error,
                                             DH_ERROR,
                                             DH_ERROR_MALFORMED_BOOK,
                                             "Expected xmlns value “" NAMESPACE "”, "
                                             "got “%s” at line %d, column %d.",
                                             xmlns, line, col);
                                return;
                        }
                } else if (g_ascii_strcasecmp (attribute_names[attr_num], "name") == 0) {
                        name = attribute_values[attr_num];
                } else if (g_ascii_strcasecmp (attribute_names[attr_num], "title") == 0) {
                        title = attribute_values[attr_num];
                } else if (g_ascii_strcasecmp (attribute_names[attr_num], "base") == 0) {
                        /* Dup this one */
                        base = g_strdup (attribute_values[attr_num]);
                } else if (g_ascii_strcasecmp (attribute_names[attr_num], "link") == 0) {
                        uri = attribute_values[attr_num];
                } else if (g_ascii_strcasecmp (attribute_names[attr_num], "language") == 0) {
                        language = attribute_values[attr_num];
                }
        }

        if (name == NULL || title == NULL || uri == NULL) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             "“title”, “name” and “link” attributes are required "
                             "inside the <book> element at line %d, column %d.",
                             line, col);
                return;
        }

        /* Store book metadata */
        g_free (parser->book_title);
        parser->book_title = g_strdup (title);
        replace_newlines_by_spaces (parser->book_title);

        g_free (parser->book_id);
        parser->book_id = g_strdup (name);

        g_free (parser->book_language);
        parser->book_language = g_strdup (language);

        if (base == NULL) {
                GFile *directory;

                directory = g_file_get_parent (parser->index_file);
                base = g_file_get_path (directory);
                g_object_unref (directory);
        }

        link = dh_link_new_book (base,
                                 parser->book_id,
                                 parser->book_title,
                                 uri);
        g_free (base);
        parser->all_links = g_list_prepend (parser->all_links, link);

        g_assert (parser->book_node == NULL);
        g_assert (parser->parent_node == NULL);

        parser->book_node = g_node_new (dh_link_ref (link));
        parser->parent_node = parser->book_node;
}

static void
parser_start_node_chapter (DhParser             *parser,
                           GMarkupParseContext  *context,
                           const gchar          *node_name,
                           const gchar         **attribute_names,
                           const gchar         **attribute_values,
                           GError              **error)
{
        gint line;
        gint col;
        gint attr_num;
        const gchar *name = NULL;
        const gchar *uri = NULL;
        DhLink *link;
        GNode *node;

        if (g_ascii_strcasecmp (node_name, "sub") != 0) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             "Expected <sub> element, got <%s> at line %d, column %d.",
                             node_name, line, col);
                return;
        }

        for (attr_num = 0; attribute_names[attr_num] != NULL; attr_num++) {
                if (g_ascii_strcasecmp (attribute_names[attr_num], "name") == 0)
                        name = attribute_values[attr_num];
                else if (g_ascii_strcasecmp (attribute_names[attr_num], "link") == 0)
                        uri = attribute_values[attr_num];
        }

        if (name == NULL || uri == NULL) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             "“name” and “link” elements are required inside "
                             "the <sub> element at line %d, column %d.",
                             line, col);
                return;
        }

        g_assert (parser->book_node != NULL);

        link = dh_link_new (DH_LINK_TYPE_PAGE,
                            parser->book_node->data,
                            name,
                            uri);

        parser->all_links = g_list_prepend (parser->all_links, link);

        g_assert (parser->parent_node != NULL);

        node = g_node_new (dh_link_ref (link));
        g_node_prepend (parser->parent_node, node);
        parser->parent_node = node;
}

static void
parser_start_node_keyword (DhParser             *parser,
                           GMarkupParseContext  *context,
                           const gchar          *node_name,
                           const gchar         **attribute_names,
                           const gchar         **attribute_values,
                           GError              **error)
{
        gint line;
        gint col;
        gint attr_num;
        const gchar *type = NULL;
        const gchar *name = NULL;
        const gchar *uri = NULL;
        const gchar *deprecated = NULL;
        DhLinkType link_type;
        DhLink *link;
        gchar *name_to_free = NULL;

        if (parser->version == FORMAT_VERSION_2 &&
            g_ascii_strcasecmp (node_name, "keyword") != 0) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             "Expected <keyword> element, got <%s> at line %d, column %d.",
                             node_name, line, col);
                return;
        } else if (parser->version == FORMAT_VERSION_1 &&
                   g_ascii_strcasecmp (node_name, "function") != 0) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             "Expected <function> element, got <%s> at line %d, column %d.",
                             node_name, line, col);
                return;
        }

        for (attr_num = 0; attribute_names[attr_num] != NULL; attr_num++) {
                if (g_ascii_strcasecmp (attribute_names[attr_num], "type") == 0)
                        type = attribute_values[attr_num];
                else if (g_ascii_strcasecmp (attribute_names[attr_num], "name") == 0)
                        name = attribute_values[attr_num];
                else if (g_ascii_strcasecmp (attribute_names[attr_num], "link") == 0)
                        uri = attribute_values[attr_num];
                else if (g_ascii_strcasecmp (attribute_names[attr_num], "deprecated") == 0)
                        deprecated = attribute_values[attr_num];
        }

        if (name == NULL || uri == NULL) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             "“name” and “link” attributes are required inside "
                             "the <%s> element at line %d, column %d.",
                             parser->version == FORMAT_VERSION_2 ? "keyword" : "function",
                             line, col);
                return;
        }

        if (parser->version == FORMAT_VERSION_2 && type == NULL) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             "“type” attribute is required inside the "
                             "<keyword> element at line %d, column %d.",
                             line, col);
                return;
        }

        if (parser->version == FORMAT_VERSION_2) {
                if (g_str_equal (type, "function"))
                        link_type = DH_LINK_TYPE_FUNCTION;
                else if (g_str_equal (type, "struct"))
                        link_type = DH_LINK_TYPE_STRUCT;
                else if (g_str_equal (type, "macro"))
                        link_type = DH_LINK_TYPE_MACRO;
                else if (g_str_equal (type, "enum"))
                        link_type = DH_LINK_TYPE_ENUM;
                else if (g_str_equal (type, "typedef"))
                        link_type = DH_LINK_TYPE_TYPEDEF;
                else if (g_str_equal (type, "property"))
                        link_type = DH_LINK_TYPE_PROPERTY;
                else if (g_str_equal (type, "signal"))
                        link_type = DH_LINK_TYPE_SIGNAL;
                else
                        link_type = DH_LINK_TYPE_KEYWORD;
        } else {
                link_type = DH_LINK_TYPE_KEYWORD;
        }

        /* Strip out trailing "() (handling variants with space or non-breaking
         * space before the parentheses).
         *
         * FIXME: gtk-doc still adds those parentheses. I thought that the code
         * was needed to support the format version 1. Maybe gtk-doc should no
         * longer add those trailing parentheses, since with the format version
         * 2 we already know the link type.
         */
        if (g_str_has_suffix (name, "\xc2\xa0()")) {
                name_to_free = g_strndup (name, strlen (name) - 4);

                if (link_type == DH_LINK_TYPE_KEYWORD)
                        link_type = DH_LINK_TYPE_FUNCTION;

                name = name_to_free;
        } else if (g_str_has_suffix (name, " ()")) {
                name_to_free = g_strndup (name, strlen (name) - 3);

                if (link_type == DH_LINK_TYPE_KEYWORD)
                        link_type = DH_LINK_TYPE_FUNCTION;

                name = name_to_free;
        } else if (g_str_has_suffix (name, "()")) {
                name_to_free = g_strndup (name, strlen (name) - 2);

                /* With old devhelp format, take a guess that this is a
                 * macro.
                 */
                if (link_type == DH_LINK_TYPE_KEYWORD)
                        link_type = DH_LINK_TYPE_MACRO;

                name = name_to_free;
        }

        /* Strip out prefixing "struct", "union", "enum", to make searching
         * easier. Also fix up the link type (only applies for old devhelp
         * format).
         */
        if (g_str_has_prefix (name, "struct ")) {
                name = name + 7;
                if (link_type == DH_LINK_TYPE_KEYWORD)
                        link_type = DH_LINK_TYPE_STRUCT;
        } else if (g_str_has_prefix (name, "union ")) {
                name = name + 6;
                if (link_type == DH_LINK_TYPE_KEYWORD)
                        link_type = DH_LINK_TYPE_STRUCT;
        } else if (g_str_has_prefix (name, "enum ")) {
                name = name + 5;
                if (link_type == DH_LINK_TYPE_KEYWORD)
                        link_type = DH_LINK_TYPE_ENUM;
        }

        g_assert (parser->book_node != NULL);

        link = dh_link_new (link_type,
                            parser->book_node->data,
                            name,
                            uri);

        g_free (name_to_free);

        if (deprecated != NULL)
                dh_link_set_flags (link, dh_link_get_flags (link) | DH_LINK_FLAGS_DEPRECATED);

        parser->all_links = g_list_prepend (parser->all_links, link);
}

static void
parser_start_node_cb (GMarkupParseContext  *context,
                      const gchar          *node_name,
                      const gchar         **attribute_names,
                      const gchar         **attribute_values,
                      gpointer              user_data,
                      GError              **error)
{
        DhParser *parser = user_data;

        if (parser->book_node == NULL) {
                parser_start_node_book (parser,
                                        context,
                                        node_name,
                                        attribute_names,
                                        attribute_values,
                                        error);
                return;
        }

        if (parser->parsing_chapters) {
                parser_start_node_chapter (parser,
                                           context,
                                           node_name,
                                           attribute_names,
                                           attribute_values,
                                           error);
                return;
        } else if (parser->parsing_keywords) {
                parser_start_node_keyword (parser,
                                           context,
                                           node_name,
                                           attribute_names,
                                           attribute_values,
                                           error);
                return;
        } else if (g_ascii_strcasecmp (node_name, "chapters") == 0) {
                parser->parsing_chapters = TRUE;
        } else if (g_ascii_strcasecmp (node_name, "functions") == 0) {
                parser->parsing_keywords = TRUE;
        }
}

static void
parser_end_node_cb (GMarkupParseContext  *context,
                    const gchar          *node_name,
                    gpointer              user_data,
                    GError              **error)
{
        DhParser *parser = user_data;

        if (parser->parsing_keywords) {
                if (g_ascii_strcasecmp (node_name, "functions") == 0)
                        parser->parsing_keywords = FALSE;
        } else if (parser->parsing_chapters) {
                g_assert (parser->parent_node != NULL);
                g_node_reverse_children (parser->parent_node);

                if (g_ascii_strcasecmp (node_name, "sub") == 0) {
                        /* Move up in the tree */
                        parser->parent_node = parser->parent_node->parent;
                        g_assert (parser->parent_node != NULL);
                } else if (g_ascii_strcasecmp (node_name, "chapters") == 0) {
                        parser->parsing_chapters = FALSE;

                        /* All <sub> elements should be closed, we should have
                         * come back to the top node (corresponding to the
                         * <book> element).
                         *
                         * It could be a g_assert(), normally GMarkupParser
                         * already catches malformed XML files (if a <sub>
                         * element is not correctly closed). But just in case
                         * GMarkupParser is not smart enough, it's safer to have
                         * a g_return_if_fail() to avoid a crash.
                         */
                        g_return_if_fail (parser->parent_node == parser->book_node);
                }
        }
}

gboolean
parser_read_docset   (GFile   *index_file,
                      gchar  **book_title,
                      gchar  **book_id,
                      gchar  **book_language,
                      GNode  **book_tree,
                      GList  **all_links,
                      GError **error)
{
        DocsetFile *docset;
        DocsetCursor *cursor;
        GList *entries = NULL;
        DhLink *link = NULL;
        DhLinkType link_type;
        DhLink *book_link = NULL;
        GNode *book_node = NULL;
        gchar *book_base = NULL;
        DocsetEntry *entry = NULL;
        gboolean ok = TRUE;

        docset = docset_file_new (g_object_ref (index_file));
        if (!DOCSET_IS_FILE (docset)) {
                ok = FALSE;
                goto exit;
        }

        cursor = docset_file_get_cursor (docset);

        *book_title = g_strdup (docset_file_get_props_bundle_name (docset));
        *book_id = g_strdup (docset_file_get_props_bundle_id (docset));
        *book_language = g_strdup (docset_file_get_props_platform_family (docset));

        book_base = docset_file_get_document_path(docset);
        book_link = dh_link_new_book (book_base,
                                      *book_id,
                                      *book_title,
                                      docset_file_get_props_index_filepath(docset));
        g_free (book_base);

        *book_tree = g_node_new (dh_link_ref (book_link));
        *all_links = g_list_prepend(*all_links, dh_link_ref (book_link));

        for (entries = docset_cursor_get_entries(cursor);
                        entries != NULL;
                        entries = entries->next) {
                entry = entries->data;
                link_type = _dh_util_link_type_for_docset_type(entry->type->id);
                link = dh_link_new (link_type,
                                    book_link,
                                    entry->name,
                                    entry->path);

                *all_links = g_list_prepend (*all_links, dh_link_ref (link));
                if (link_type == DH_LINK_TYPE_PAGE) {
                        book_node = g_node_new (dh_link_ref (link));
                        g_node_prepend(*book_tree, book_node);
                }

                dh_link_unref (link);
                docset_entry_free (entry);
        }

        g_list_free(entries);
        g_node_reverse_children (*book_tree);
        dh_link_unref (book_link);

exit:
        g_clear_object (&docset);
        return ok;
}

gboolean
parser_read_devhelp  (GFile   *index_file,
                      gchar  **book_title,
                      gchar  **book_id,
                      gchar  **book_language,
                      GNode  **book_tree,
                      GList  **all_links,
                      GError **error)
{
        DhParser *parser;
        gchar *index_file_uri;
        gboolean gz;
        GFileInputStream *file_input_stream = NULL;
        GInputStream *input_stream = NULL;
        gboolean ok = TRUE;

        parser = g_new0 (DhParser, 1);

        index_file_uri = g_file_get_uri (index_file);

        if (g_str_has_suffix (index_file_uri, ".devhelp2")) {
                parser->version = FORMAT_VERSION_2;
                gz = FALSE;
        } else if (g_str_has_suffix (index_file_uri, ".devhelp")) {
                parser->version = FORMAT_VERSION_1;
                gz = FALSE;
        } else if (g_str_has_suffix (index_file_uri, ".devhelp2.gz")) {
                parser->version = FORMAT_VERSION_2;
                gz = TRUE;
        } else {
                parser->version = FORMAT_VERSION_1;
                gz = TRUE;
        }

        parser->markup_parser = g_new0 (GMarkupParser, 1);
        parser->markup_parser->start_element = parser_start_node_cb;
        parser->markup_parser->end_element = parser_end_node_cb;

        parser->context = g_markup_parse_context_new (parser->markup_parser, 0, parser, NULL);

        parser->index_file = g_object_ref (index_file);

        file_input_stream = g_file_read (index_file, NULL, error);
        if (file_input_stream == NULL) {
                ok = FALSE;
                goto exit;
        }

        /* At this point we know that the file exists, the G_IO_ERROR_NOT_FOUND
         * has been catched earlier. So print warning.
         */
        if (parser->version == FORMAT_VERSION_1)
                g_warning ("The file '%s' uses the Devhelp index file format version 1, "
                           "which is deprecated. A future version of Devhelp may remove "
                           "the support for the format version 1. The index file should "
                           "be ported to the Devhelp index file format version 2.",
                           index_file_uri);

        if (gz) {
                GZlibDecompressor *zlib_decompressor;

                zlib_decompressor = g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP);
                input_stream = g_converter_input_stream_new (G_INPUT_STREAM (file_input_stream),
                                                             G_CONVERTER (zlib_decompressor));
                g_object_unref (zlib_decompressor);
        } else {
                input_stream = G_INPUT_STREAM (g_object_ref (file_input_stream));
        }

        while (TRUE) {
                gchar buffer[BYTES_PER_READ];
                gssize bytes_read;

                bytes_read = g_input_stream_read (input_stream,
                                                  buffer,
                                                  BYTES_PER_READ,
                                                  NULL,
                                                  error);

                if (bytes_read > 0) {
                        if (!g_markup_parse_context_parse (parser->context,
                                                           buffer,
                                                           bytes_read,
                                                           error)) {
                                ok = FALSE;
                                goto exit;
                        }

                } else if (bytes_read == 0) {
                        /* End of file */
                        break;
                } else {
                        ok = FALSE;
                        goto exit;
                }
        }

        if (!g_markup_parse_context_end_parse (parser->context, error)) {
                ok = FALSE;
                goto exit;
        }

        /* Index file successfully read. Set out parameters. */

        *book_title = parser->book_title;
        parser->book_title = NULL;

        *book_id = parser->book_id;
        parser->book_id = NULL;

        *book_language = parser->book_language;
        parser->book_language = NULL;

        *book_tree = parser->book_node;
        parser->book_node = NULL;
        parser->parent_node = NULL;

        *all_links = parser->all_links;
        parser->all_links = NULL;

exit:
        g_free (index_file_uri);
        g_clear_object (&file_input_stream);
        g_clear_object (&input_stream);
        dh_parser_free (parser);

        return ok;
}

gboolean
_dh_parser_read_file (GFile   *index_file,
                      gchar  **book_title,
                      gchar  **book_id,
                      gchar  **book_language,
                      GNode  **book_tree,
                      GList  **all_links,
                      GError **error)
{
        gchar *index_file_uri;
        gboolean is_docset;

        g_return_val_if_fail (G_IS_FILE (index_file), FALSE);
        g_return_val_if_fail (book_title != NULL && *book_title == NULL, FALSE);
        g_return_val_if_fail (book_id != NULL && *book_id == NULL, FALSE);
        g_return_val_if_fail (book_language != NULL && *book_language == NULL, FALSE);
        g_return_val_if_fail (book_tree != NULL && *book_tree == NULL, FALSE);
        g_return_val_if_fail (all_links != NULL && *all_links == NULL, FALSE);
        g_return_val_if_fail (error != NULL && *error == NULL, FALSE);

        index_file_uri = g_file_get_uri (index_file);
        is_docset = g_str_has_suffix (index_file_uri, ".docset");
        g_free (index_file_uri);

        if (is_docset) {
                return parser_read_docset (index_file, book_title, book_id, book_language, book_tree, all_links, error);
        } else {
                return parser_read_devhelp (index_file, book_title, book_id, book_language, book_tree, all_links, error);
        }
}
