/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@codefactory.se>
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
 *
 * Author: Mikael Hallendal <micke@codefactory.se>
 */

#include <config.h>

#include <libxml/parser.h>

#include "dh-book-parser.h"

static gboolean   book_parser_parse_book        (GNode         *book_tree,
						 GList        **keywords,
						 const gchar   *path,
						 GError       **error);
static gboolean   book_parser_parse_chapter     (GNode         *root,
						 xmlNode       *node,
						 GError       **error);
static gboolean   book_parser_parse_function    (GList        **keywords,
						 xmlNode       *node,
						 GError       **error);

					 

static gboolean
book_parser_parse_book (GNode        *book_tree,
			GList       **keywords, 
			const gchar  *path,
			GError      **error)
{
	xmlDoc  *doc;
	xmlNode *root_node, *node;
	xmlChar *xml_str;
	gchar   *name;
	gchar   *base;
	gchar   *link;
	gchar   *title;

	doc = xmlParseFile (path);
	
	if (!doc) {
		g_print ("Couldn't parse file\n");
		return FALSE;
	}
	 
	root_node = xmlDocGetRootElement (doc);
	
	if (!root_node || !root_node->name ||
	    xmlStrcmp (root_node->name, "book") != 0) {
		/* Set the error */
		g_print ("ROOT NODE: %s != book\n", root_node->name);
		return FALSE;
	}

	for (root_node = root_node->xmlChildrenNode;
	     root_node && xmlIsBlankNode (root_node);
	     root_node = root_node->next) { 
		;;
	}
	
	if (!root_node) {
		/* Set error */
		
		xmlFreeDoc (doc);
		return FALSE;
	}

	if (xmlStrcmp (root_node->name, "chapters") != 0) {
		/* Set error */
		xmlFreeDoc (doc);
		return FALSE;
	}

	for (node = root_node->xmlChildrenNode; node; node = node->next) {
		if (xmlStrcmp (node->name, "sub") == 0) {
			if (!book_parser_parse_chapter (book_tree, 
							node, error)) {
				return FALSE;
			}
		}
	}
	
	for (root_node = root_node->next;
	     root_node && xmlIsBlankNode (root_node);
	     root_node = root_node->next) { 
		;;
	}
	
	if (!root_node) {
		/* Set error */
		xmlFreeDoc (doc);
		return TRUE;
	}

	if (xmlStrcmp (root_node->name, "functions") != 0) {
		/* Set error */
		
		xmlFreeDoc (doc);
		return TRUE;
	}
	
	for (node = root_node->xmlChildrenNode; node; node = node->next) {
		if (xmlStrcmp (node->name, "function") == 0) {
			if (!book_parser_parse_function (keywords, 
							 node, error)) {
				return FALSE;
			}
		}
	}
}

static gboolean
book_parser_parse_chapter (GNode *root, xmlNode *node, GError **error)
{
	GNode   *new_root;
	xmlNode *child;
	gchar   *name;
	gchar   *link;
	xmlChar *xml_str;
	
	if (xmlStrcmp (node->name, "sub") != 0) {
		/* Set error */
		g_print ("NODE: %s != sub\n", node->name);
		
		return FALSE;
	}

	xml_str = xmlGetProp (node, "name");
	if (xml_str) {
		name = g_strdup (xml_str);
		xmlFree (xml_str);
	} else {
		name = g_strdup ("");
	}
	
	xml_str = xmlGetProp (node, "link");
	if (xml_str) {
		link = g_strdup (xml_str);
		xmlFree (xml_str);
	}

	g_print ("FIXME: Insert chapter '%s' into tree\n", name);
	/* Insert */

	for (child = node->xmlChildrenNode; child; child = child->next) {

		if (xmlStrcmp (child->name, "sub") == 0) {
			gboolean success;
			success = book_parser_parse_chapter (new_root, 
							     child, error);
		
			if (!success) {
				return FALSE;
			}
		}
	}
	
	return TRUE;
}

static gboolean
book_parser_parse_function (GList **keywords, xmlNode *node, GError **error)
{
	GNode   *new_root;
	xmlNode *child;
	gchar   *name;
	gchar   *link;
	xmlChar *xml_str;
	
	if (xmlStrcmp (node->name, "function") != 0) {
		g_print ("NODE: %s != function\n", node->name);
		/* Set error */
		return FALSE;
	}

	xml_str = xmlGetProp (node, "name");
	if (xml_str) {
		name = g_strdup (xml_str);
		xmlFree (xml_str);
	} else {
		name = g_strdup ("");
	}
	
	
	xml_str = xmlGetProp (node, "link");
	if (xml_str) {
		link = g_strdup (xml_str);
		xmlFree (xml_str);
	}

	g_print ("FIXME: Insert function '%s' into list\n", name);

	/* Insert */

	return TRUE;
}

gboolean
dh_book_parser_read_books (GSList  *books,
			   GNode   *book_tree, 
			   GList  **keywords, 
			   GError **error)
{
	GSList *l;
	
	for (l = books; l; l = l->next) {
		book_parser_parse_book (book_tree, 
					keywords, 
					(gchar *) l->data, error);
	}
}
