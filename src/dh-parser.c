/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (c) 2002-2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (c) 2002-2003 CodeFactory AB
 * Copyright (C) 2005 Imendio AB
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

#include <config.h>
#include <string.h>
#include <errno.h>
#include <glib/gi18n.h>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include "dh-error.h"
#include "dh-link.h"
#include "dh-parser.h"

#define d(x)
#define DH_PARSER(o) ((DhParser *) o)
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
	gboolean             parsing_functions;

 	GNode               *book_tree;
	GList              **keywords;

	/* Version 2 uses <keyword> instead of <function>. */
	gint                 version;
} DhParser;

static void     parser_start_node_cb (GMarkupParseContext  *context,
				      const gchar          *node_name,
				      const gchar         **attribute_names,
				      const gchar         **attribute_values,
				      gpointer              user_data,
				      GError              **error);
static void     parser_end_node_cb   (GMarkupParseContext  *context,
				      const gchar          *node_name,
				      gpointer              user_data,
				      GError              **error);
static void     parser_error_cb      (GMarkupParseContext  *context,
				      GError               *error,
				      gpointer              user_data);
static gboolean parser_read_gz_file  (const gchar          *path,
				      GNode                *book_tree,
				      GList               **keywords,
				      GError              **error);
static gchar   *extract_page_name    (const gchar          *uri);


static void
parser_start_node_cb (GMarkupParseContext  *context,
		      const gchar          *node_name,
		      const gchar         **attribute_names,
		      const gchar         **attribute_values,
		      gpointer              user_data,
		      GError              **error)
{
	DhParser *parser;
	gint      i, line, col;
	DhLink   *dh_link;
	gchar    *full_link, *page;

	parser = DH_PARSER (user_data);

	if (!parser->book_node) {
		const gchar *xmlns = NULL;
		const gchar *title = NULL;
		const gchar *base  = NULL;
		const gchar *name  = NULL;
		const gchar *link  = NULL;

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
			if (g_ascii_strcasecmp (attribute_names[i],
						"xmlns") == 0) {
				xmlns = attribute_values[i];
				if (g_ascii_strcasecmp (xmlns,
							"http://www.devhelp.net/book") != 0) {
					/* Set error */
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
			else if (g_ascii_strcasecmp (attribute_names[i],
						     "name") == 0) {
				name = attribute_values[i];
			}
			else if (g_ascii_strcasecmp (attribute_names[i],
						     "title") == 0) {
				title = attribute_values[i];
			}
			else if (g_ascii_strcasecmp (attribute_names[i],
						     "base") == 0) {
				base = attribute_values[i];
			}
			else if (g_ascii_strcasecmp (attribute_names[i],
						     "link") == 0) {
				link = attribute_values[i];
			}
		}

		if (!title || !name || !link) {
			/* Required attributes */
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

		full_link = g_strconcat (parser->base, "/", link, NULL);
		dh_link = dh_link_new (DH_LINK_TYPE_BOOK, title, name, NULL, full_link);
		g_free (full_link);

 		*parser->keywords = g_list_prepend (*parser->keywords,
 						    dh_link);

		parser->book_node = g_node_new (dh_link);
		g_node_prepend (parser->book_tree, parser->book_node);
		parser->parent = parser->book_node;

		return;
	}

	if (parser->parsing_chapters) {
		const gchar *name = NULL;
		const gchar *link = NULL;
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
			if (g_ascii_strcasecmp (attribute_names[i],
						"name") == 0) {
				name = attribute_values[i];
			}
			else if (g_ascii_strcasecmp (attribute_names[i],
						     "link") == 0) {
				link = attribute_values[i];
			}
		}

		if (!name || !link) {
			g_markup_parse_context_get_position (context, &line, &col);
			g_set_error (error,
				     DH_ERROR,
				     DH_ERROR_MALFORMED_BOOK,
				     _("name and link elements are required "
				       "inside <sub> on line %d, column %d"),
				     line, col);
			return;
		}

		

		full_link = g_strconcat (parser->base, "/", link, NULL);
		page = extract_page_name (link);
		dh_link = dh_link_new (DH_LINK_TYPE_PAGE, name, 
				       DH_LINK(parser->book_node->data)->book,
				       page, full_link);
		g_free (full_link);
		g_free (page);

 		*parser->keywords = g_list_prepend (*parser->keywords,
 						    dh_link);

		node = g_node_new (dh_link);
		g_node_prepend (parser->parent, node);
		parser->parent = node;
	}
	else if (parser->parsing_functions) {
		gboolean     ok = FALSE;
		const gchar *name = NULL;
		const gchar *link = NULL;
		
		if (g_ascii_strcasecmp (node_name, "function") == 0) {
			ok = TRUE;

			if (parser->version == 2) {
				/* Skip this keyword. */
				return;
			}
		}
		else if (g_ascii_strcasecmp (node_name, "keyword") == 0) {
			ok = TRUE;

			/* Note: We have this hack since there are released
			 * tarballs out there of GTK+ etc that have been built
			 * with a non-released version of gtk-doc that didn't
			 * have the proper versioning scheme. So when we find
			 * this new tag, we force the version to the newer one.
			 */
			if (parser->version < 2) {
				parser->version = 2;
			}
		}
		
		if (!ok) {
			g_markup_parse_context_get_position (context, &line, &col);
			g_set_error (error,
				     DH_ERROR,
				     DH_ERROR_MALFORMED_BOOK,
				     _("Expected '%s' got '%s' at line %d, column %d"),
				     "function or keyword", node_name, line, col);
			return;
		}

		for (i = 0; attribute_names[i]; ++i) {
			if (g_ascii_strcasecmp (attribute_names[i],
						"name") == 0) {
				name = attribute_values[i];
			}
			else if (g_ascii_strcasecmp (attribute_names[i],
						     "link") == 0) {
				link = attribute_values[i];
			}
		}

		if (!name || !link) {
			/* Required */
			g_markup_parse_context_get_position (context, &line, &col);
			g_set_error (error,
				     DH_ERROR,
				     DH_ERROR_MALFORMED_BOOK,
				     _("name and link elements are required "
				       "inside <function> on line %d, column %d"),
				     line, col);
			return;
		}

		/* Strip out these, they are only present for code that gtk-doc
		 * couldn't parse properly. We'll get this information in a
		 * better way soon from gtk-doc.
		 */
		if (g_str_has_prefix (name, "struct ")) {
			name = name + 7;
		}
		else if (g_str_has_prefix (name, "union ")) {
			name = name + 6;
		}
		else if (g_str_has_prefix (name, "enum ")) {
			name = name + 5;
		}
	
		full_link = g_strconcat (parser->base, "/", link, NULL);
		page = extract_page_name (link);
		if (g_str_has_suffix (name, " ()")) {
			gchar *tmp;

			tmp = g_strndup (name, strlen (name) - 3);
			dh_link = dh_link_new (DH_LINK_TYPE_KEYWORD, tmp, 
					       DH_LINK(parser->book_node->data)->book,
					       page, full_link);
			g_free (tmp);
		} else {
			dh_link = dh_link_new (DH_LINK_TYPE_KEYWORD, name, 
					       DH_LINK(parser->book_node->data)->book,
					       page, full_link);
		}
		g_free (full_link);
		g_free (page);

 		*parser->keywords = g_list_prepend (*parser->keywords,
 						    dh_link);
	}
	else if (g_ascii_strcasecmp (node_name, "chapters") == 0) {
		parser->parsing_chapters = TRUE;
	}
	else if (g_ascii_strcasecmp (node_name, "functions") == 0) {
		parser->parsing_functions = TRUE;
	}
}

static void
parser_end_node_cb (GMarkupParseContext  *context,
		    const gchar          *node_name,
		    gpointer              user_data,
		    GError              **error)
{
	DhParser *parser;

	parser = DH_PARSER (user_data);

	if (parser->parsing_chapters) {
		g_node_reverse_children (parser->parent);
		if (g_ascii_strcasecmp (node_name, "sub") == 0) {
			parser->parent = parser->parent->parent;
			/* Move up in the tree */
		}
		else if (g_ascii_strcasecmp (node_name, "chapters") == 0) {
			parser->parsing_chapters = FALSE;
		}
	}
	else if (parser->parsing_functions) {
		if (g_ascii_strcasecmp (node_name, "function") == 0) {
			/* Do nothing */
			return;
		}
		else if (g_ascii_strcasecmp (node_name, "functions") == 0) {
			parser->parsing_functions = FALSE;
		}
	}
/* 	else if (g_ascii_strcasecmp (node_name, "book") == 0) { */
/* 	} */
}

static void
parser_error_cb (GMarkupParseContext *context,
		 GError              *error,
		 gpointer             user_data)
{
	DhParser *parser;

	parser = DH_PARSER (user_data);

	g_markup_parse_context_free (parser->context);
 	parser->context = NULL;
}

gboolean
dh_parser_read_file (const gchar  *path,
		     GNode        *book_tree,
		     GList       **keywords,
		     GError      **error)
{
	DhParser   *parser;
	GIOChannel *io;
	gchar       buf[BYTES_PER_READ];
	gboolean    result = TRUE;

	if (g_str_has_suffix (path, ".gz")) {
		return parser_read_gz_file (path,
					    book_tree,
					    keywords,
					    error);
	}

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
	} else {
		parser->version = 1;
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
	parser->m_parser->end_element   = parser_end_node_cb;
	parser->m_parser->error         = parser_error_cb;

	parser->context = g_markup_parse_context_new (parser->m_parser, 0,
						      parser, NULL);

	parser->parent = NULL;

	parser->parsing_functions = FALSE;
	parser->parsing_chapters  = FALSE;

	parser->path      = path;
	parser->book_tree = book_tree;
	parser->keywords  = keywords;

	/* Parse the string */
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

 exit:
	g_markup_parse_context_free (parser->context);
	g_free (parser->m_parser);
	g_free (parser);

	return result;
}

static gboolean
parser_read_gz_file (const gchar  *path,
		     GNode        *book_tree,
		     GList       **keywords,
		     GError      **error)
{
#ifdef HAVE_LIBZ
	DhParser *parser;
	gchar     buf[BYTES_PER_READ];
	gzFile    file;

	parser = g_new0 (DhParser, 1);
	if (!parser) {
		g_set_error (error,
			     DH_ERROR,
			     DH_ERROR_INTERNAL_ERROR,
			     _("Could not create book parser"));
		return FALSE;
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

	if (g_str_has_suffix (path, ".devhelp2")) {
		parser->version = 2;
	} else {
		parser->version = 1;
	}
	
	parser->m_parser->start_element = parser_start_node_cb;
	parser->m_parser->end_element   = parser_end_node_cb;
	parser->m_parser->error         = parser_error_cb;

	parser->context = g_markup_parse_context_new (parser->m_parser, 0,
						      parser, NULL);

	parser->parent = NULL;

	parser->parsing_functions = FALSE;
	parser->parsing_chapters  = FALSE;

	parser->path      = path;
	parser->book_tree = book_tree;
	parser->keywords  = keywords;

	/* Parse the string */
	file = gzopen (path, "r");

	if (!file) {
		g_markup_parse_context_free (parser->context);
		g_free (parser);
		g_set_error (error,
			     DH_ERROR,
			     DH_ERROR_FILE_NOT_FOUND,
			     g_strerror (errno));
		return FALSE;
	}

	while (TRUE) {
		gsize bytes_read;

		bytes_read = gzread (file, buf, BYTES_PER_READ);
		if (bytes_read == -1) {
			const gchar *message;
			gint         err;

			g_markup_parse_context_free (parser->context);
			g_free (parser);
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

	g_markup_parse_context_free (parser->context);
	g_free (parser);

	return TRUE;
#else
	g_set_error (error,
		     DH_ERROR,
		     DH_ERROR_INTERNAL_ERROR,
		     _("Devhelp is not built with zlib support"));
	return FALSE;
#endif
}


static gchar *
extract_page_name (const gchar *uri)
{
	gchar  *page = NULL;
	gchar **split;

	if ((split = g_strsplit (uri, ".", 2)) != NULL) {
		page = g_strdup (split[0]);
		g_strfreev (split);
	}
	return page;
}
