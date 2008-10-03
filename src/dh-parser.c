/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (c) 2002-2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (c) 2002-2003 CodeFactory AB
 * Copyright (C) 2005,2008 Imendio AB
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
#include <errno.h>
#include <zlib.h>
#include <glib/gi18n.h>

#include "dh-error.h"
#include "dh-link.h"
#include "dh-parser.h"

#define NAMESPACE      "http://www.devhelp.net/book"
#define BYTES_PER_READ 4096

typedef struct {
	GMarkupParser       *m_parser;
	GMarkupParseContext *context;

	const gchar         *path;
	gchar               *base;

	/* Top node of book */
	GNode               *book_node;

	/* Current sub section node */
	GNode               *parent;

	gboolean             parsing_chapters;
	gboolean             parsing_keywords;

 	GNode               *book_tree;
	GList              **keywords;

	/* Version 2 uses <keyword> instead of <function>. */
	gint                 version;
} DhParser;

static gchar *
extract_page_name (const gchar *uri)
{
	gchar  *page = NULL;
	gchar **split;

	if ((split = g_strsplit (uri, ".", 2)) != NULL) {
		page = split[0];
                split[0] = NULL;
		g_strfreev (split);
	}
	return page;
}

static void
parser_start_node_book (DhParser             *parser,
                        GMarkupParseContext  *context,
                        const gchar          *node_name,
                        const gchar         **attribute_names,
                        const gchar         **attribute_values,
                        GError              **error)
{
        gint         i;
        gint         line, col;
        const gchar *title = NULL;
        const gchar *base  = NULL;
        const gchar *name  = NULL;
        const gchar *uri  = NULL;
	gchar       *full_uri;
	DhLink      *link;

        if (g_ascii_strcasecmp (node_name, "book") != 0) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             _("Expected '%s' got '%s' at line %d, column %d"),
                             "book", node_name, line, col);
                return;
        }

        for (i = 0; attribute_names[i]; ++i) {
                const gchar *xmlns;

                if (g_ascii_strcasecmp (attribute_names[i], "xmlns") == 0) {
                        xmlns = attribute_values[i];
                        if (g_ascii_strcasecmp (xmlns, NAMESPACE) != 0) {
                                g_markup_parse_context_get_position (context,
                                                                     &line,
                                                                     &col);
                                g_set_error (error,
                                             DH_ERROR,
                                             DH_ERROR_MALFORMED_BOOK,
                                             _("Invalid namespace '%s' at"
                                               " line %d, column %d"),
                                             xmlns, line, col);
                                return;
                        }
                }
                else if (g_ascii_strcasecmp (attribute_names[i], "name") == 0) {
                        name = attribute_values[i];
                }
                else if (g_ascii_strcasecmp (attribute_names[i], "title") == 0) {
                        title = attribute_values[i];
                }
                else if (g_ascii_strcasecmp (attribute_names[i], "base") == 0) {
                        base = attribute_values[i];
			}
                else if (g_ascii_strcasecmp (attribute_names[i], "link") == 0) {
                        uri = attribute_values[i];
                }
        }

        if (!title || !name || !uri) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             _("title, name, and link elements are "
                               "required at line %d, column %d"),
                             line, col);
                return;
        }

        if (base) {
                parser->base = g_strdup (base);
        } else {
                parser->base = g_path_get_dirname (parser->path);
        }

        full_uri = g_strconcat (parser->base, "/", uri, NULL);
        link = dh_link_new (DH_LINK_TYPE_BOOK, title, NULL, NULL, full_uri);
        g_free (full_uri);

        *parser->keywords = g_list_prepend (*parser->keywords, link);

        parser->book_node = g_node_new (link);
        g_node_prepend (parser->book_tree, parser->book_node);
        parser->parent = parser->book_node;
}

static void
parser_start_node_chapter (DhParser             *parser,
                           GMarkupParseContext  *context,
                           const gchar          *node_name,
                           const gchar         **attribute_names,
                           const gchar         **attribute_values,
                           GError              **error)
{
        gint         i;
        gint         line, col;
        const gchar *name = NULL;
        const gchar *uri = NULL;
	gchar       *full_uri;
	gchar       *page;
	DhLink      *link;
        GNode       *node;

        if (g_ascii_strcasecmp (node_name, "sub") != 0) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             _("Expected '%s' got '%s' at line %d, column %d"),
                             "sub", node_name, line, col);
                return;
        }

        for (i = 0; attribute_names[i]; ++i) {
                if (g_ascii_strcasecmp (attribute_names[i], "name") == 0) {
                        name = attribute_values[i];
                }
                else if (g_ascii_strcasecmp (attribute_names[i], "link") == 0) {
                        uri = attribute_values[i];
                }
        }

        if (!name || !uri) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             _("name and link elements are required "
                               "inside <sub> on line %d, column %d"),
                             line, col);
                return;
        }

        full_uri = g_strconcat (parser->base, "/", uri, NULL);
        page = extract_page_name (uri);
        link = dh_link_new (DH_LINK_TYPE_PAGE, name, 
                            parser->book_node->data,
                            NULL, full_uri);
        g_free (full_uri);
        g_free (page);

        *parser->keywords = g_list_prepend (*parser->keywords, link);

        node = g_node_new (link);
        g_node_prepend (parser->parent, node);
        parser->parent = node;
}

static void
parser_start_node_keyword (DhParser             *parser,
                           GMarkupParseContext  *context,
                           const gchar          *node_name,
                           const gchar         **attribute_names,
                           const gchar         **attribute_values,
                           GError              **error)
{
        gint         i;
        gint         line, col;
        const gchar *name = NULL;
        const gchar *uri = NULL;
        const gchar *type = NULL;
        const gchar *deprecated = NULL;
	gchar       *full_uri;
	gchar       *page;
        DhLinkType   link_type;
	DhLink      *link;
        gchar       *tmp;

        if (parser->version == 2 &&
            g_ascii_strcasecmp (node_name, "keyword") != 0) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             _("Expected '%s' got '%s' at line %d, column %d"),
                             "keyword", node_name, line, col);
                return;
        }
        else if (parser->version == 1 &&
            g_ascii_strcasecmp (node_name, "function") != 0) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             _("Expected '%s' got '%s' at line %d, column %d"),
                             "function", node_name, line, col);
                return;
        }
		
        for (i = 0; attribute_names[i]; ++i) {
                if (g_ascii_strcasecmp (attribute_names[i], "type") == 0) {
                        type = attribute_values[i];
                }
                else if (g_ascii_strcasecmp (attribute_names[i], "name") == 0) {
                        name = attribute_values[i];
                }
                else if (g_ascii_strcasecmp (attribute_names[i], "link") == 0) {
                        uri = attribute_values[i];
                }
                else if (g_ascii_strcasecmp (attribute_names[i], "deprecated") == 0) {
                        deprecated = attribute_values[i];
                }
        }

        if (!name || !uri) {
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             _("name and link elements are required "
                               "inside '%s' on line %d, column %d"),
                             parser->version == 2 ? "keyword" : "function",
                             line, col);
                return;
        }

        if (parser->version == 2 && !type) {
                /* Required */
                g_markup_parse_context_get_position (context, &line, &col);
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             _("type element is required "
                               "inside <keyword> on line %d, column %d"),
                             line, col);
                return;
        }

        full_uri = g_strconcat (parser->base, "/", uri, NULL);
        page = extract_page_name (uri);

        if (parser->version == 2) {
                if (strcmp (type, "function") == 0) {
                        link_type = DH_LINK_TYPE_FUNCTION;
                }
                else if (strcmp (type, "struct") == 0) {
                        link_type = DH_LINK_TYPE_STRUCT;
                }
                else if (strcmp (type, "macro") == 0) {
                        link_type = DH_LINK_TYPE_MACRO;
                }
                else if (strcmp (type, "enum") == 0) {
                        link_type = DH_LINK_TYPE_ENUM;
                }
                else if (strcmp (type, "typedef") == 0) {
                        link_type = DH_LINK_TYPE_TYPEDEF;
                } else {
                        link_type = DH_LINK_TYPE_KEYWORD;
                }
        } else {
                link_type = DH_LINK_TYPE_KEYWORD;
        }

        if (g_str_has_suffix (name, " ()")) {
                tmp = g_strndup (name, strlen (name) - 3);

                if (link_type == DH_LINK_TYPE_KEYWORD) {
                        link_type = DH_LINK_TYPE_FUNCTION;
                }
                name = tmp;
        } else {
                tmp = NULL;
        }

        /* We only get "keyword" from old gtk-doc files, try to fix up the
         * metadata as well as we can.
         */
        if (link_type == DH_LINK_TYPE_KEYWORD) {
                if (g_str_has_prefix (name, "struct ")) {
                        name = name + 7;
                        link_type = DH_LINK_TYPE_STRUCT;
                }
                else if (g_str_has_prefix (name, "union ")) {
                        name = name + 6;
                }
                else if (g_str_has_prefix (name, "enum ")) {
                        name = name + 5;
                        link_type = DH_LINK_TYPE_ENUM;
                }
        }

        link = dh_link_new (link_type, name, 
                            parser->book_node->data,
                            parser->parent->data,
                            full_uri);

        g_free (tmp);
        g_free (full_uri);
        g_free (page);

        if (deprecated) {
                dh_link_set_flags (
                        link,
                        dh_link_get_flags (link) | DH_LINK_FLAGS_DEPRECATED);
        }

        *parser->keywords = g_list_prepend (*parser->keywords, link);
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

        if (parser->parsing_keywords) {
                parser_start_node_keyword (parser,
                                           context,
                                           node_name,
                                           attribute_names,
                                           attribute_values,
                                           error);
                return;
        }
        else if (parser->parsing_chapters) {
                parser_start_node_chapter (parser,
                                           context,
                                           node_name,
                                           attribute_names,
                                           attribute_values,
                                           error);
                return;
        }
	else if (g_ascii_strcasecmp (node_name, "functions") == 0) {
		parser->parsing_keywords = TRUE;
	}
	else if (g_ascii_strcasecmp (node_name, "chapters") == 0) {
		parser->parsing_chapters = TRUE;
	}
	if (!parser->book_node) {
                parser_start_node_book (parser,
                                        context,
                                        node_name,
                                        attribute_names,
                                        attribute_values,
                                        error);
		return;
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
                if (g_ascii_strcasecmp (node_name, "functions") == 0) {
			parser->parsing_keywords = FALSE;
		}
	}
	else if (parser->parsing_chapters) {
		g_node_reverse_children (parser->parent);
		if (g_ascii_strcasecmp (node_name, "sub") == 0) {
			parser->parent = parser->parent->parent;
			/* Move up in the tree */
		}
		else if (g_ascii_strcasecmp (node_name, "chapters") == 0) {
			parser->parsing_chapters = FALSE;
		}
	}
}

static void
parser_error_cb (GMarkupParseContext *context,
		 GError              *error,
		 gpointer             user_data)
{
	DhParser *parser = user_data;

	g_markup_parse_context_free (parser->context);
 	parser->context = NULL;
}

static gboolean
parser_read_gz_file (DhParser     *parser,
                     const gchar  *path,
		     GNode        *book_tree,
		     GList       **keywords,
		     GError      **error)
{
	gchar  buf[BYTES_PER_READ];
	gzFile file;

	file = gzopen (path, "r");
	if (!file) {
		g_set_error (error,
			     DH_ERROR,
			     DH_ERROR_FILE_NOT_FOUND,
			     "%s", g_strerror (errno));
		return FALSE;
	}

	while (TRUE) {
		gsize bytes_read;

		bytes_read = gzread (file, buf, BYTES_PER_READ);
		if (bytes_read == -1) {
			gint         err;
			const gchar *message;

			message = gzerror (file, &err);
			g_set_error (error,
				     DH_ERROR,
				     DH_ERROR_INTERNAL_ERROR,
				     _("Cannot uncompress book '%s': %s"),
				     path, message);
			return FALSE;
		}

		g_markup_parse_context_parse (parser->context, buf,
					      bytes_read, error);
		if (error != NULL && *error != NULL) {
			return FALSE;
		}
		if (bytes_read < BYTES_PER_READ) {
			break;
		}
	}

	gzclose (file);

	return TRUE;
}

gboolean
dh_parser_read_file (const gchar  *path,
		     GNode        *book_tree,
		     GList       **keywords,
		     GError      **error)
{
	DhParser   *parser;
        gboolean    gz;
	GIOChannel *io = NULL;
	gchar       buf[BYTES_PER_READ];
	gboolean    result = TRUE;

	parser = g_new0 (DhParser, 1);
	if (!parser) {
		g_set_error (error,
			     DH_ERROR,
			     DH_ERROR_INTERNAL_ERROR,
			     _("Could not create book parser"));
		return FALSE;
	}

	if (g_str_has_suffix (path, ".devhelp2")) {
		parser->version = 2;
                gz = FALSE;
        }
        else if (g_str_has_suffix (path, ".devhelp")) {
		parser->version = 1;
                gz = FALSE;
        }
        else if (g_str_has_suffix (path, ".devhelp2.gz")) {
		parser->version = 2;
                gz = TRUE;
        } else {
		parser->version = 1;
                gz = TRUE;
        }

	parser->m_parser = g_new0 (GMarkupParser, 1);
	if (!parser->m_parser) {
		g_free (parser);
		g_set_error (error,
			     DH_ERROR,
			     DH_ERROR_INTERNAL_ERROR,
			     _("Could not create markup parser"));
		return FALSE;
	}

	parser->m_parser->start_element = parser_start_node_cb;
	parser->m_parser->end_element = parser_end_node_cb;
	parser->m_parser->error = parser_error_cb;

	parser->context = g_markup_parse_context_new (parser->m_parser, 0,
						      parser, NULL);

	parser->parent = NULL;

	parser->parsing_keywords = FALSE;
	parser->parsing_chapters = FALSE;

	parser->path = path;
	parser->book_tree = book_tree;
	parser->keywords = keywords;

        if (gz) {
                if (!parser_read_gz_file (parser,
                                          path,
                                          book_tree,
                                          keywords,
                                          error)) {
                        result = FALSE;
                }
                goto exit;
	} else {
                io = g_io_channel_new_file (path, "r", error);
                if (!io) {
                        result = FALSE;
                        goto exit;
                }

                while (TRUE) {
                        GIOStatus io_status;
                        gsize     bytes_read;

                        io_status = g_io_channel_read_chars (io, buf, BYTES_PER_READ,
                                                             &bytes_read, error);
                        if (io_status == G_IO_STATUS_ERROR) {
                                result = FALSE;
                                goto exit;
                        }
                        if (io_status != G_IO_STATUS_NORMAL) {
                                break;
                        }

                        g_markup_parse_context_parse (parser->context, buf,
                                                      bytes_read, error);
                        if (error != NULL && *error != NULL) {
                                result = FALSE;
                                goto exit;
                        }

                        if (bytes_read < BYTES_PER_READ) {
                                break;
                        }
                }
        }

 exit:
	if (io) {
                g_io_channel_unref (io);
        }
	g_markup_parse_context_free (parser->context);
	g_free (parser->m_parser);
	g_free (parser);

	return result;
}
