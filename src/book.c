/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include "function-database.h"
#include "util.h"
#include "book.h"

#define d(x)

static void  book_init                    (Book             *book);
static void  book_class_init              (GtkObjectClass   *klass);
static void  book_destroy                 (GtkObject        *object);

static void  book_parse                   (Book                *book, 
					   GnomeVFSURI         *uri,
					   FunctionDatabase    *fd);
static void  book_parse_sub               (Book                *node,
					   BookNode            *parent,
				 	   xmlNode             *xml_node);
static void  book_parse_function          (Book                *booknode,
					   xmlNode             *node,
					   FunctionDatabase    *fd);
static Document *  document_new           (Book                *book,
					   const gchar         *link,
					   gchar              **anchor);

static BookNode *  book_node_new          (Book                *book,
					   BookNode            *parent);
static const gchar *
book_url_get_book_relative                (Book                *book,
					   const gchar         *url);

static BookNode *
book_node_find                            (BookNode            *node,
					   const Document      *document,
					   const gchar         *anchor);

struct _BookPriv {
	gchar         *name;
	gchar         *title;
	gchar         *author;
	gchar         *version;
	gchar         *spec_file;
	gboolean       visible;
	
	GnomeVFSURI   *base_uri;
	
	GHashTable    *documents;
	BookNode      *root;
	Document      *current_document;
	GSList        *functions;
};

struct _Document {
	Book          *book;
	gchar         *link;
};

struct _BookNode {
	Document      *document;
	BookNode      *parent;
	
	gchar         *title;
	gchar         *anchor;
	
	GSList        *contents;
};

GtkType
book_get_type (void)
{
	static GtkType book_type = 0;
        
	if (!book_type) {
		static const GtkTypeInfo book_info = {
			"Book",
			sizeof (Book),
			sizeof (BookClass),
			(GtkClassInitFunc) book_class_init,
			(GtkObjectInitFunc) book_init,
			NULL, /* -- Reserved -- */
			NULL, /* -- Reserved -- */
			(GtkClassInitFunc) NULL,
		};
                
		book_type = gtk_type_unique (GTK_TYPE_OBJECT, &book_info);
	}

	return book_type;
}

static void
book_init (Book *book) 
{
	BookPriv   *priv;
	
	priv            = g_new0 (BookPriv, 1);
	priv->name      = NULL;
	priv->title     = NULL;
	priv->author    = NULL;
	priv->version   = NULL;
	priv->visible   = TRUE;
	priv->base_uri  = NULL;
	priv->root      = NULL;
	priv->documents = g_hash_table_new (g_str_hash, g_str_equal);
	
	book->priv      = priv;
}

static void
book_class_init (GtkObjectClass *klass)
{
	klass->destroy = book_destroy;
}

static void
book_destroy (GtkObject *object)
{
	/* FIX: Free stuff */
}

static void
book_parse (Book *book, GnomeVFSURI *uri, FunctionDatabase *fd)
{
	BookPriv      *priv;
	Document      *document;
	BookNode      *book_node;
	xmlDocPtr      doc;
	xmlNode       *root_node, *cur;
	const gchar   *file_name;
	gchar         *xml_str;
	gchar         *anchor;
		
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	g_return_if_fail (uri != NULL);

	priv         = book->priv;
	book_node    = book_node_new (book, NULL);
	file_name    = gnome_vfs_uri_get_path (uri);
	doc          = xmlParseFile (file_name);
	root_node    = xmlDocGetRootElement (doc);
	
	if (!root_node) {
		g_warning ("Empty document: %s", file_name);
		xmlFreeDoc (doc);
		return;
	}
	
	if (xmlStrcmp (root_node->name, (const xmlChar *) "book")) {
		g_warning ("Document wrong type, got '%s', expected 'book': %s", 
			   root_node->name, file_name);
		xmlFreeDoc (doc);
		return;
	}

	xml_str = xmlGetProp (root_node, "name");
	
	if (xml_str) {
		priv->name = g_strdup (xml_str);
		xmlFree (xml_str);
	}

	/* Get book title */
	xml_str = xmlGetProp (root_node, "title");
	
	if (xml_str) {
		priv->title = g_strdup (xml_str);
		book_node->title = priv->title;
		xmlFree (xml_str);
	}

	xml_str = xmlGetProp (root_node, "author");
	
	if (xml_str) {
		priv->author = g_strdup (xml_str);
		xmlFree (xml_str);
	}

	xml_str = xmlGetProp (root_node, "version");
	
	if (xml_str) {
		priv->version = g_strdup (xml_str);
		xmlFree (xml_str);
	}
	
	/* Get default uri */
	xml_str = xmlGetProp (root_node, "base");
	
	if (xml_str) {
		book_set_base_url (book, xml_str);			
		xmlFree (xml_str);
	} else {
		priv->base_uri = gnome_vfs_uri_get_parent (uri);
	}

	xml_str = xmlGetProp (root_node, "link");
	
	if (xml_str) {
		document = document_new (book, xml_str, &anchor);
		book_node->document = document;
		book_node->anchor = anchor;
		xmlFree (xml_str);
	} else {
		document = document_new (book, "/", NULL);
		
		book_node->document = document;
		book_node->anchor = NULL;
	}

	priv->root = book_node;
	
	root_node = root_node->xmlChildrenNode;
	
	while (root_node && xmlIsBlankNode (root_node)) {
		root_node = root_node->next;
	}

	if (!root_node) {
		xmlFreeDoc (doc);
		return;
	}

	if (xmlStrcmp (root_node->name, (const xmlChar *) "chapters")) {
		g_warning ("Document wrong type, got '%s', expected 'chapters': %s", 
			   root_node->name, file_name);
		xmlFreeDoc (doc);
		return;
	}
	
	cur = root_node->xmlChildrenNode;
	
 	while (cur) {
		if (!xmlStrcmp (cur->name, (const xmlChar *) "sub") ||
		    !xmlStrcmp (cur->name, (const xmlChar *) "chapter")) {
			book_parse_sub (book, book_node, cur);
		}
		
		cur = cur->next;
	}
	
        while (1) 
        {
	        root_node = root_node->next;
	        
	        if (!root_node) {
		       xmlFreeDoc (doc);
		       return;
	        }

	        if (xmlStrcmp (root_node->name, (const xmlChar*) "text")) {
		       break;
		}
	}
   
	

	if (xmlStrcmp (root_node->name, (const xmlChar *) "functions")) {
		g_warning ("Document wrong type, got '%s', expected 'functions': %s", 
			   root_node->name, file_name);
		xmlFreeDoc (doc);
		return;
	}

	cur = root_node->xmlChildrenNode;
	
	while (cur) {
		if (!xmlStrcmp (cur->name, (const xmlChar *) "function")) {
			book_parse_function (book, cur, fd); 
		}
		
		cur = cur->next;
	}
	
	xmlFreeDoc (doc);

}

static void
book_parse_sub (Book *book, BookNode *parent, xmlNode *xml_node)
{
	BookPriv   *priv;
	gchar      *xml_str;
	Document   *document;
	BookNode   *book_node;
	gchar      *anchor;
	xmlNode    *cur;
	
	priv = book->priv;

	book_node = book_node_new (book, parent);
	
	xml_str = xmlGetProp (xml_node, "name");
	
	if (xml_str) {
		book_node->title = g_strdup (xml_str);
		xmlFree (xml_str);
	}
	
	xml_str = xmlGetProp (xml_node, "link");
	
	if (xml_str) {
		if (!(document = book_find_document (book, xml_str, &anchor))) {
			document = document_new (book, xml_str, NULL);
		}

		book_node->document = document;
		book_node->anchor = anchor;
		xmlFree (xml_str);
	}
	
	cur = xml_node->xmlChildrenNode;
	
	while (cur != NULL) {
		book_parse_sub (book, book_node, cur);
		cur = cur->next;
	}
}

static void  
book_parse_function (Book               *book, 
		     xmlNode            *node, 
		     FunctionDatabase   *fd)
{
	BookPriv      *priv;
	gchar         *xml_str;
	gchar         *name;
	gchar         *url;
	Document      *document;
	gchar         *anchor;
	Function      *function;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));

	priv = book->priv;
	
	xml_str = xmlGetProp (node, "name");
	
	if (xml_str) {
		name = g_strdup (xml_str);
		xmlFree (xml_str);
	}
	
	xml_str = xmlGetProp (node, "link");
	
	if (xml_str) {
		url = g_strdup (xml_str);
		xmlFree (xml_str);
	}
	
	if (!(document = book_find_document (book, url, &anchor))) {
		document = document_new (book, url, NULL);
	}
	
	function = function_database_add_function (fd, name, document, anchor);

	priv->functions = g_slist_append (priv->functions, function);
}

static Document *
document_new (Book *book, const gchar *link, gchar **anchor)
{
	BookPriv   *priv;
	Document   *document;
	
	g_return_val_if_fail (book != NULL, NULL);
	g_return_val_if_fail (IS_BOOK (book), NULL);
	g_return_val_if_fail (link != NULL, NULL);
	
	priv = book->priv;
	
	document = g_new0 (Document, 1);
	document->book = book;
	document->link = util_url_split (link, anchor);

	g_hash_table_insert (priv->documents, document->link, document);

	return document;
}

static BookNode *
book_node_new (Book *book, BookNode *parent)
{
	BookPriv   *priv;
	BookNode   *node;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	node           = g_new0 (BookNode, 1);
	node->document = NULL;
	node->parent   = parent;
	node->title    = NULL;
	node->anchor   = NULL;
	
	if (parent) {
		parent->contents = g_slist_append (parent->contents, node);
	}
 	
	return node;
}

static const gchar *
book_url_get_book_relative (Book *book, const gchar *url) 
{
	BookPriv      *priv;
	const gchar   *ch, *ch2;
	gchar         *base;
	gint           un_depth;
	 
	g_return_val_if_fail (book != NULL, NULL);
	g_return_val_if_fail (IS_BOOK (book), NULL);
	g_return_val_if_fail (url != NULL, NULL);
	
	priv     = book->priv;

	if (!util_uri_is_relative (url)) {
		base = gnome_vfs_uri_to_string (book->priv->base_uri,
						GNOME_VFS_URI_HIDE_NONE);
		
		ch = url;
		ch2 = base;
		
		while (*ch == *ch2) {
			++ch;
			++ch2;
		}

		if (*ch == '/') {
			++ch;
		}
		
		g_free (base);

	} else {
		un_depth = util_url_get_un_depth (url);
		
		ch = url;
		ch += un_depth * URL_DELIM_LENGTH;
		
		if (priv->name) {
			if (strstr (ch, priv->name) == ch && 
			    *(ch + strlen (priv->name)) == '/') {
				ch += strlen (priv->name) + 1;
			}
		}
		
		if (ch - url > strlen (url)) {
			return url;
		}
	}
	
	return ch;
}

static BookNode *
book_node_find (BookNode         *book_node, 
		const Document   *document, 
		const gchar      *anchor)
{
	GSList     *node;
	BookNode   *found_node = NULL;
	
	g_return_if_fail (book_node != NULL);
	g_return_if_fail (document != NULL);

	if (book_node->document == document) {
		if (!anchor) {
			return book_node;
		}
		if (book_node->anchor && !strcmp (book_node->anchor,
						  anchor)) {
			return book_node;
		}
	}

	for (node = book_node->contents; node && !found_node; node = node->next) {
		found_node = book_node_find (node->data, document, anchor);
	}
	
	return found_node;
}

Book *
book_new (GnomeVFSURI *book_uri, FunctionDatabase *fd)
{
	Book   *book;
	
	g_return_val_if_fail (book_uri != NULL, NULL);
	
	book = gtk_type_new (TYPE_BOOK);
	book->priv->spec_file = 
		gnome_vfs_uri_to_string (book_uri, GNOME_VFS_URI_HIDE_NONE);

	book_parse (book, book_uri, fd);

	return book;
}

const gchar *
book_get_name (Book *book)
{
	BookPriv   *priv;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	return priv->name;
}

const gchar *
book_get_name_full (Book *book)
{
	BookPriv   *priv;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	if (priv->version == NULL)
		return g_strdup (priv->name);
	else {
		return g_strdup_printf ("%s-%s", priv->name, priv->version);
	}
}

const gchar *
book_get_title (Book *book)
{
	BookPriv   *priv;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	return priv->title;
}

const gchar *
book_get_author (Book *book)
{
	BookPriv   *priv;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	return priv->author;
}

const gchar *
book_get_version (Book *book)
{
	BookPriv   *priv;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	return priv->version;
}

const gchar *
book_get_spec_file (Book *book)
{
	BookPriv   *priv;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	return priv->spec_file;
}

void
book_set_spec_file (Book *book, const gchar *spec_file)
{
	BookPriv   *priv;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	priv->spec_file = g_strdup (spec_file);
}

gchar *
book_get_path (Book *book)
{
	BookPriv   *priv;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	return gnome_vfs_uri_to_string (priv->base_uri, 0);
}

void
book_set_visible (Book *book, gboolean visible)
{
	BookPriv   *priv;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	priv->visible = visible;
}

gboolean
book_is_visible (Book *book)
{
	BookPriv   *priv;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	return priv->visible;
}

gboolean
book_contains (Book *book, const GnomeVFSURI *uri)
{
	BookPriv   *priv;
	
	g_return_val_if_fail (book != NULL, FALSE);
	g_return_val_if_fail (IS_BOOK (book), FALSE);
	g_return_val_if_fail (uri != NULL, FALSE);

	priv = book->priv;
	
	return gnome_vfs_uri_is_parent (priv->base_uri, uri, TRUE);
}

GSList *
book_get_functions (Book *book)
{
	BookPriv   *priv;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	return priv->functions;
}

BookNode *
book_get_root (Book *book)
{
	BookPriv   *priv;
	
	g_return_val_if_fail (book != NULL, FALSE);
	g_return_val_if_fail (IS_BOOK (book), FALSE);

	priv = book->priv;

	return priv->root;
}

void
book_open_document (Book *book, const Document *document)
{
	BookPriv   *priv;
	Document   *doc;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	g_return_if_fail (document != NULL);
	g_return_if_fail (document->book == book);

	priv = book->priv;

	doc = g_hash_table_lookup (priv->documents, document->link);
	
	if (document && doc == document) {
		priv->current_document = doc;
	}
}

Document *
book_find_document (Book *book, const gchar *url, gchar **anchor)
{
	BookPriv      *priv;
	Document      *document;
	gchar         *doc_url;
	const gchar   *book_rel_url;
	
	g_return_val_if_fail (book != NULL, NULL);
	g_return_val_if_fail (IS_BOOK (book), NULL);
	g_return_val_if_fail (url != NULL, NULL);

	d(g_print ("Trying to find document: %s\n", url));
	
	priv = book->priv;

	book_rel_url = book_url_get_book_relative (book, url);
	
	doc_url = util_url_split (book_rel_url, anchor);
	
	document = g_hash_table_lookup (priv->documents, doc_url);
	
	g_free (doc_url);
	
	return document;
}

BookNode *
book_find_node (Book *book, const Document *document, const gchar *anchor)
{
	BookPriv      *priv;
	BookNode      *book_node;
	
	g_return_val_if_fail (book != NULL, NULL);
	g_return_val_if_fail (IS_BOOK (book), NULL);
	g_return_val_if_fail (document != NULL, NULL);
	
	priv = book->priv;

	book_node = book_node_find (priv->root, document, anchor);
	
	if (!book_node) {
		book_node = book_node_find (priv->root, document, NULL);
	}
	
	return book_node;
}

gint
book_get_current_depth (Book *book)
{
	BookPriv   *priv;
	gchar      *ch;
	gint        depth;
	
	g_return_val_if_fail (book != NULL, 0);
	g_return_val_if_fail (IS_BOOK (book), 0);

	priv = book->priv;

	if (!priv->current_document) {
		return 0;
	}

	depth = 0;
	ch    = priv->current_document->link;

	while (ch = strchr (ch, '/')) {
		depth++;
		ch++;
	}
	
	return depth;
}

gint
book_compare_func (gconstpointer a, gconstpointer b)
{
	g_return_val_if_fail (a != NULL, 1);
	g_return_val_if_fail (IS_BOOK (a), 1);
	g_return_val_if_fail (b != NULL, 1);
	g_return_val_if_fail (IS_BOOK (b), 1);

	return strcoll (BOOK(a)->priv->title, BOOK(b)->priv->title);
}

void
book_set_base_url (Book        *book,
		   const gchar *url)
{
	BookPriv   *priv;
	gchar      *tmp_url = NULL;
	
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = book->priv;

	if (url[strlen (url) - 1] != '/') { 
		priv->base_uri = gnome_vfs_uri_new (url);
		return;
	}
		
	tmp_url = g_strndup (url, strlen (url) - 1);
	priv->base_uri = gnome_vfs_uri_new (tmp_url);
	g_free (tmp_url);
}

/* BookNode functions */

const gchar *
book_node_get_title (const BookNode *node)
{
 	g_return_if_fail (node != NULL);
	
	return node->title;
}

BookNode *
book_node_get_parent (const BookNode *node)
{
 	return node->parent;
}

GSList *
book_node_get_contents (const BookNode *node)
{
	g_return_if_fail (node != NULL);
	
	return node->contents;
}

Book *
document_get_book (const Document *document)
{
	g_return_val_if_fail (document != NULL, NULL);
	
	return document->book;
}


Book *
book_node_get_book (const BookNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	return document_get_book (node->document);
}

gboolean
book_node_is_chapter (const BookNode *node)
{
	return node->contents ? TRUE : FALSE;
}

GnomeVFSURI *
book_node_get_uri (const BookNode *node, const gchar *anchor)
{
	const gchar *local_anchor;
	
	g_return_val_if_fail (node != NULL, NULL);

	/* FIX: This is a work-around because of me being stupid /Hallski */
	if (anchor) {
		local_anchor = anchor;
	} else {
		local_anchor = node->anchor;
	}
	
	return document_get_uri (node->document, local_anchor);
}

Document *
book_node_get_document (const BookNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	return node->document;
}

const gchar *
book_node_get_anchor (const BookNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	return node->anchor;
}

GnomeVFSURI *
document_get_uri (const Document *document, const gchar *anchor)
{
	BookPriv      *priv;
	gchar         *url;
	GnomeVFSURI   *uri;
	gchar         *tst;
	
	g_return_val_if_fail (document != NULL, NULL);
	
	priv = document->book->priv;

	if (anchor) {
		url = g_strconcat (document->link, anchor, NULL);
		
		uri = gnome_vfs_uri_append_string (priv->base_uri, url);
		g_free (url);
	} else {
		uri = gnome_vfs_uri_append_string (priv->base_uri, 
						   document->link);
	}
	
	return uri;
}

