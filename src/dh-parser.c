/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * See the file LICENSE for redistribution information. 
 * If you have not received a copy of the license, please contact CodeFactory
 * by email at info@codefactory.se, or on the web at http://www.codefactory.se/
 * You may also write to: CodeFactory AB, SE-903 47, Umeå, Sweden.
 *
 * Copyright (c) 2002-2003 Mikael Hallendal <micke@codefactory.se>
 * Copyright (c) 2002-2003 CodeFactory AB.  All rights reserved.
 */

#include <config.h>
#include <string.h>

#ifdef HAVE_LIBZ
#include "zlib.h"
#endif

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
} DhParser;

/* Used while parsing */
static void    parser_start_node_cb (GMarkupParseContext  *context,
				     const gchar          *node_name,
				     const gchar         **attribute_names,
				     const gchar         **attribute_values,
				     gpointer              user_data,
				     GError              **error);
static void    parser_end_node_cb   (GMarkupParseContext  *context,
				     const gchar          *node_name,
				     gpointer              user_data,
				     GError              **error);
static void    parser_error_cb      (GMarkupParseContext  *context,
				     GError               *error,
				     gpointer              user_data); 

static void
parser_start_node_cb (GMarkupParseContext  *context,
		      const gchar          *node_name,
		      const gchar         **attribute_names,
		      const gchar         **attribute_values,
		      gpointer              user_data,
		      GError              **error)
{	
	DhParser *parser;
	gint      i;
	DhLink   *dh_link;
	gchar    *full_link;
	
	parser = DH_PARSER (user_data);;

	if (!parser->book_node) {
		const gchar *xmlns = NULL;
		const gchar *title = NULL;
		const gchar *base  = NULL;
		const gchar *name  = NULL;
		const gchar *link  = NULL;
		
		if (g_ascii_strcasecmp (node_name, "book") != 0) {
			/* Set error */
			return;
		}

		for (i = 0; attribute_names[i]; ++i) {
			if (g_ascii_strcasecmp (attribute_names[i],
						"xmlns") == 0) {
				xmlns = attribute_values[i];
				if (g_ascii_strcasecmp (xmlns, 
							"http://www.devhelp.net/book") != 0) {
					/* Set error */
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
			/* Set error */
			return;
		}

		if (base) {
			parser->base = g_strdup (base);
		} else {
			parser->base = g_path_get_dirname (parser->path);
		}
		
		full_link = g_strconcat (parser->base, "/", link, NULL);
		dh_link = dh_link_new (DH_LINK_TYPE_BOOK, title, full_link);
		g_free (full_link);

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
			/* Set error */
			g_print ("foo: %s\n", node_name);
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
			return;
		}

		full_link = g_strconcat (parser->base, "/", link, NULL);
		dh_link = dh_link_new (DH_LINK_TYPE_PAGE, name, full_link);
		g_free (full_link);

		node = g_node_new (dh_link);
		g_node_prepend (parser->parent, node);
		parser->parent = node;
		
/* 		g_print ("Parser sub: %s %s\n", name, link); */
		
		/* Insert new sub */
	}
	else if (parser->parsing_functions) {
		const gchar *name = NULL;
		const gchar *link = NULL;

		if (g_ascii_strcasecmp (node_name, "function") != 0) {
			/* Set error */ 
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
			return;
		}

		full_link = g_strconcat (parser->base, "/", link, NULL);
		dh_link = dh_link_new (DH_LINK_TYPE_KEYWORD, name, full_link);
		g_free (full_link);

 		*parser->keywords = g_list_prepend (*parser->keywords,
 						    dh_link);
		
/* 		g_print ("Parsed function: %s %s\n", name, link); */
		/* Insert new function */
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
		g_node_reverse_children(parser->parent);
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
	DhParser     *parser;
	
	g_return_if_fail (user_data != NULL);
	
	parser = DH_PARSER (user_data);

	g_markup_parse_context_free (parser->context);
	parser->context = NULL;
}

gboolean
dh_parse_file (const gchar  *path,
	       GNode        *book_tree,
	       GList       **keywords,
	       GError      **error)
{
	DhParser   *parser;
	GIOChannel *io;
	gchar       buf[BYTES_PER_READ];
	
	parser = g_new0 (DhParser, 1);
	if (!parser) {
		/* Set error */
		g_print ("1\n");
		return FALSE;
	}
	
	parser->m_parser = g_new0 (GMarkupParser, 1);
	if (!parser->m_parser) {
		g_free (parser);
		/* Set error */
		g_print ("2\n");
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
		g_markup_parse_context_free (parser->context);
		g_free (parser);
		g_print ("3\n");
		return FALSE;
	}
	
	while (TRUE) {
		GIOError io_error;
		gsize    bytes_read;
		
		io_error = g_io_channel_read (io, buf, BYTES_PER_READ,
					   &bytes_read);
		if (io_error != G_IO_ERROR_NONE) {
			g_markup_parse_context_free (parser->context);
			g_free (parser);
			/* Set error */
			g_print ("4\n");
			return FALSE;
		}
		
		g_markup_parse_context_parse (parser->context, buf,
					      bytes_read, error);
		if (bytes_read < BYTES_PER_READ) {
			break;
		}
	}

	g_markup_parse_context_free (parser->context);
	g_free (parser);
	
	return TRUE;
}

#ifdef HAVE_LIBZ
gboolean
dh_parse_gz_file (const gchar  *path,
	       GNode        *book_tree,
	       GList       **keywords,
	       GError      **error)
{
	DhParser   *parser;
	gchar       buf[BYTES_PER_READ];
	gzFile file;

	parser = g_new0 (DhParser, 1);
	if (!parser) {
		/* Set error */
		g_print ("1\n");
		return FALSE;
	}
	
	parser->m_parser = g_new0 (GMarkupParser, 1);
	if (!parser->m_parser) {
		g_free (parser);
		/* Set error */
		g_print ("2\n");
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
	file = gzopen (path, "r");
	
	if (!file) {
		g_markup_parse_context_free (parser->context);
		g_free (parser);
		g_print ("3\n");
		return FALSE;
	}
	
	while (TRUE) {
		gsize bytes_read;

		bytes_read = gzread(file, buf, BYTES_PER_READ);
		if (bytes_read == -1) {
			const char *message;
			int err;
			g_markup_parse_context_free (parser->context);
			g_free (parser);
			/* Set error */
			gzerror (file, &err);
			g_print ("zlib error %d: %s\n", err, message);
			return FALSE;
		}
		
		g_markup_parse_context_parse (parser->context, buf,
					      bytes_read, error);
		if (bytes_read < BYTES_PER_READ) {
			break;
		}
	}

	gzclose(file);

	g_markup_parse_context_free (parser->context);
	g_free (parser);
	
	return TRUE;
}
#endif
