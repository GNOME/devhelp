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
 */

#include <config.h>

#include <gsf/gsf-input.h>
#include <gsf/gsf-libxml.h>
#include <libgnome/gnome-i18n.h>
#include <libxml/parser.h>
#include <string.h>

#include "dh-error.h"
#include "dh-link.h"
#include "dh-book-old.h"

static gboolean   book_old_parse_chapter     (GNode         *root,
					      xmlNode       *node,
					      const gchar   *base_uri,
					      GError       **error);
static gboolean   book_old_parse_function    (GList        **keywords,
					      xmlNode       *node,
					      const gchar   *base_uri,
					      GError       **error);
static gchar *    book_old_get_base_uri      (const gchar   *spec_path,
					      const gchar   *name,
					      xmlChar       *read_base);
static gboolean   book_old_validate          (xmlDoc        *doc,
					      const gchar   *dtd_path);


static gboolean
book_old_parse_chapter (GNode        *parent,
			   xmlNode      *node, 
			   const gchar  *base_uri,
			   GError      **error)
{
	GNode   *new_parent;
	xmlNode *child;
	xmlChar *name;
	gchar   *uri;
	xmlChar *xml_str;
	DhLink  *link;
	
	if (xmlStrcmp (node->name, "sub") != 0 &&
	    xmlStrcmp (node->name, "chapter") != 0) {
		/* Set error */
		g_print ("NODE: %s != sub || chapter\n", node->name);
		
		return FALSE;
	}

	name = xmlGetProp (node, "name");
	if (!name) {
		g_warning ("Error in book file");
		return FALSE;
	}
	
	xml_str = xmlGetProp (node, "link");
	if (!xml_str) {
		g_warning ("Chapter '%s' doesn't have a URI, fix the book",
			   name);
		return FALSE;
	}
	uri = g_strconcat (base_uri, "/", xml_str, NULL);
	xmlFree (xml_str);

	link = dh_link_new (DH_LINK_TYPE_PAGE, name, uri);

	xmlFree (name);
	g_free (uri);

	new_parent = g_node_append_data (parent, link);

	for (child = node->xmlChildrenNode; child; child = child->next) {

		if (xmlStrcmp (child->name, "sub") == 0) {
			gboolean success;
			success = book_old_parse_chapter (new_parent, 
							     child, base_uri,
							     error);
		
			if (!success) {
				return FALSE;
			}
		}
	}
	
	return TRUE;
}

static gboolean
book_old_parse_function (GList       **keywords, 
			    xmlNode      *node, 
			    const gchar  *base_uri,
			    GError      **error)
{
	GNode   *new_root;
	xmlNode *child;
	xmlChar *name;
	gchar   *uri;
	xmlChar *xml_str;
	DhLink  *link;
	
	if (xmlStrcmp (node->name, "function") != 0) {
		g_print ("NODE: %s != function\n", node->name);
		/* Set error */
		return FALSE;
	}

	name = xmlGetProp (node, "name");
	if (!name) {
		g_warning ("Keyword without name, fix the book file");
		return FALSE;
	}
	
	xml_str = xmlGetProp (node, "link");
	if (!xml_str) {
		g_warning ("Keyword '%s' didn't have a URI", name);
		return FALSE;
	}
	uri = g_strconcat (base_uri, "/", xml_str, NULL);
	xmlFree (xml_str);

	link = dh_link_new (DH_LINK_TYPE_KEYWORD, name, uri);
	g_free (uri);
	xmlFree (name);
	
	*keywords = g_list_prepend (*keywords, link);

	return TRUE;
}

static gchar *
book_old_get_base_uri (const gchar *spec_path, 
			  const gchar *name,
			  xmlChar     *read_base)
{
	gchar *ret_val;
	gchar *tmp_url;
	gchar *ch;

	tmp_url = g_strdup (spec_path);
	
	ch = strrchr (tmp_url, '/');
	if (!ch) {
		return g_strdup (read_base);
	}
	
	*ch = '\0';

	ch = strrchr (tmp_url, '/');
	if (!ch) {
		return g_strdup (read_base);
	}
	*ch = '\0';

	ret_val = g_build_filename (tmp_url, "books", name, NULL);
	g_free (tmp_url);
	
	if (!ret_val || !g_file_test (ret_val, 
				      G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
		return g_strdup (read_base);
	}
	
	return ret_val;
}

static gboolean
book_old_validate (xmlDoc *doc, const gchar *dtd_path)
{
	xmlValidCtxt  cvp;
	xmlDtd       *dtd;
	gboolean      ret_val;
	
	g_return_val_if_fail (doc != NULL, FALSE);
	g_return_val_if_fail (dtd_path != NULL, FALSE);
	
	cvp.userData = NULL;
	cvp.error    = NULL;
	cvp.warning  = NULL;

	dtd = xmlParseDTD (NULL, dtd_path);
	
	ret_val = xmlValidateDtd (&cvp, doc, dtd);
	
	xmlFreeDtd (dtd);
	
        return ret_val;
}

gboolean
dh_book_old_read (GsfInput  *input,
		  DhBook    *book,
		  GNode     *contents,
		  GList    **keywords, 
		  GError   **error)
{
	xmlParserCtxt *ctxt;
	xmlDoc        *doc;
	xmlNode       *root_node;
	xmlNode       *node;
	xmlChar       *xml_str;
	gchar         *base;
	gchar         *title;
	gchar         *uri;
	DhLink        *link;
	GNode         *parent;

	ctxt = gsf_xml_parser_context (input);
	
	if (!ctxt) {
		g_set_error (error,
			     DH_ERROR,
			     DH_ERROR_MALFORMED_BOOK,
			     _("Book '%s' is malformed."), 
			     gsf_input_name (input));
		return FALSE;
	}
	
	xmlParseDocument (ctxt);

	if (!ctxt->wellFormed) {
		g_set_error (error,
			     DH_ERROR,
			     DH_ERROR_MALFORMED_BOOK,
			     _("Book '%s' is malformed."),
			     gsf_input_name (input));
		return FALSE;
	}
	
	doc = ctxt->myDoc;
	xmlFreeParserCtxt (ctxt);
	
	if (!book_old_validate (doc, DTD_DIR"/devhelp-1.dtd")) {
		g_set_error (error,
			     DH_ERROR,
			     DH_ERROR_MALFORMED_BOOK,
			     _("Book '%s' is malformed."),
			     gsf_input_name (input));
		return FALSE;
	}
	
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

	xml_str = xmlGetProp (root_node, "title");
	if (!xml_str) {
		g_warning ("Book doesn't have a title, fix the book file");
		return FALSE;
	}
	title = g_strdup (xml_str);
	xmlFree (xml_str);
	
	xml_str = xmlGetProp (root_node, "base");
	if (!xml_str) {
		g_warning ("Book '%s' misses the base URL, fix the book file",
			   title);
		g_free (title);
		return FALSE;
	}
	base = book_old_get_base_uri (uri, title, xml_str);
	xmlFree (xml_str);
	
	xml_str = xmlGetProp (root_node, "link");
	if (!xml_str) {
		g_warning ("Book '%s' misses link, fix the book file", title);
		g_free (base);
		g_free (uri);
		return FALSE;
	}
	uri = g_strconcat (base, "/", xml_str, NULL);
	xmlFree (xml_str);

	link = dh_link_new (DH_LINK_TYPE_BOOK, title, uri);
	g_free (uri);

	if (book) {
		book->title = g_strdup (title);
		book->author = NULL;
	}

	g_free (title);

	parent = g_node_append_data (contents, link);

	for (root_node = root_node->xmlChildrenNode;
	     root_node && xmlIsBlankNode (root_node);
	     root_node = root_node->next) { 
		;;
	}
	
	if (!root_node) {
		/* Set error */
		
		xmlFreeDoc (doc);
		g_free (base);
		return FALSE;
	}

	if (xmlStrcmp (root_node->name, "chapters") != 0) {
		/* Set error */
		xmlFreeDoc (doc);
		g_free (base);
		return FALSE;
	}

	for (node = root_node->xmlChildrenNode; node; node = node->next) {
		if (xmlStrcmp (node->name, "sub") == 0 ||
		    xmlStrcmp (node->name, "chapter") == 0) {
			if (!book_old_parse_chapter (parent, node,
						     base, error)) {
				return FALSE;
				g_free (base);
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
		g_free (base);
		return TRUE;
	}

	if (xmlStrcmp (root_node->name, "functions") != 0) {
		/* Set error */
		
		xmlFreeDoc (doc);
		g_free (base);
		return TRUE;
	}
	
	for (node = root_node->xmlChildrenNode; node; node = node->next) {
		if (xmlStrcmp (node->name, "function") == 0) {
			if (!book_old_parse_function (keywords, 
						      node, base, 
						      error)) {
				g_free (base);
				return FALSE;
			}
		}
	}

	g_free (base);
	return TRUE;
}
