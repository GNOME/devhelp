/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
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
#include <libgnomevfs/gnome-vfs.h>
#include <gnome-xml/parser.h>
#include <gnome-xml/xmlmemory.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "function-database.h"
#include "util.h"
#include "bookshelf.h"

#define d(x)

static void      bookshelf_init              (Bookshelf        *bookshelf);
static void      bookshelf_class_init        (GtkObjectClass   *klass);
static void      bookshelf_destroy           (GtkObject        *object);

static GSList *  bookshelf_read_books_dir    (GnomeVFSURI      *books_uri);

struct _BookshelfPriv {
	GSList             *books;
	FunctionDatabase   *fd;

	Book               *current_book;
};

struct _XMLBook {
	gchar    *name;
	gchar    *path;
	gboolean  visible;
};

GtkType
bookshelf_get_type (void)
{
	static GtkType bookshelf_type = 0;
        
	if (!bookshelf_type) {
		static const GtkTypeInfo bookshelf_info = {
			"Bookshelf",
			sizeof (Bookshelf),
			sizeof (BookshelfClass),
			(GtkClassInitFunc)  bookshelf_class_init,
			(GtkObjectInitFunc) bookshelf_init,
			NULL, /* -- Reserved -- */
			NULL, /* -- Reserved -- */
			(GtkClassInitFunc) NULL,
		};
                
		bookshelf_type = gtk_type_unique (GTK_TYPE_OBJECT,
						  &bookshelf_info);
	}

	return bookshelf_type;
}

static void
bookshelf_init (Bookshelf *bookshelf)
{
	BookshelfPriv   *priv;
        
	priv            = g_new0 (BookshelfPriv, 1);
	priv->books     = NULL;
	bookshelf->priv = priv;
}

static void
bookshelf_class_init (GtkObjectClass *klass)
{
	klass->destroy = bookshelf_destroy;
}

static void
bookshelf_destroy (GtkObject *object)
{
	Bookshelf       *bookshelf;
	BookshelfPriv   *priv;
        
	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_BOOKSHELF (object));
        
	bookshelf = BOOKSHELF (object);
	priv      = bookshelf->priv;

	/* FIX: Free priv data */
        
	g_free (priv);
        
	bookshelf->priv = NULL;
}

void
bookshelf_write_xml (Bookshelf     *bookshelf, 
		     const gchar   *filename)
{
	BookshelfPriv   *priv;
	Book            *book;
	FILE            *fp;
	GSList          *node;
	const gchar     *name;
	const gchar     *version;	
	const gchar     *path;
	gboolean         visible;
	
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	
	priv = bookshelf->priv;

	if (filename == NULL) {
		fp = stdout;
	} else {
		fp = fopen (filename, "w");

		if (fp == NULL) {
			g_warning (_("Failed to open file %s for writing."), filename);
			return;
		}
	}
	
	fprintf (fp, "<?xml version=\"1.0\"?>\n\n");
	fprintf (fp, "<booklist>\n");

	for (node = priv->books; node; node = node->next) {
		book = BOOK (node->data);
		
		name    = book_get_name (book);
		version = book_get_version (book);		
		path    = book_get_path (book);
		visible = book_is_visible (book);
		
		fprintf (fp, "  <book name=\"%s\" ", name);
		
		if (version != NULL) {
			fprintf (fp, "version=\"%s\" ", 
				 version);
		}
		
		fprintf (fp, "visible=\"%d\" path=\"%s\"/>\n",
			 visible == TRUE ? 1 : 0,
			 path);
	}
	
	fprintf (fp, "</booklist>\n");

	if (fp != stdout) {
		fclose (fp);
	}
}

GList*
bookshelf_read_xml (Bookshelf *bookshelf, const gchar *filename)
{
	BookshelfPriv   *priv;
	XMLBook         *book;
	GList           *list;
	xmlDocPtr        doc;
	xmlNode         *root_node, *cur;
	gchar           *xml_str;
	gchar           *visible;
	
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	
	priv = bookshelf->priv;
		
	doc = xmlParseFile (filename);

	if (!doc) {
		return NULL;
	}
	
	root_node = xmlDocGetRootElement (doc);
	
	if (!root_node) {
		g_warning (_("Empty document: %s"), filename);
		xmlFreeDoc (doc);
		return NULL;
	}
	
	if (xmlStrcmp (root_node->name, (const xmlChar *) "booklist")) {
		g_warning (_("Document wrong type, got %s, expected 'booklist': %s"), 
			   root_node->name, filename);
		xmlFreeDoc (doc);
		return NULL;
	}
	
	cur = root_node->xmlChildrenNode;
	list = NULL;
 	while (cur) {
		if (!xmlStrcmp (cur->name, (const xmlChar *) "book")) {
			book = g_new (XMLBook, 1);
			book->name = xmlGetProp (cur, "name");
			book->path = xmlGetProp (cur, "path");
			visible = xmlGetProp (cur, "visible");
			if (visible != NULL) {
				book->visible = atoi (visible);
			} else {
				book->visible = TRUE;
			}
			xmlFree (visible);
			list = g_list_append (list, book);
		}
		
		cur = cur->next;
	}
	
	/* TODO: lot's of free() in this func */
	return list;
}

gboolean
bookshelf_add_book (Bookshelf *bookshelf, Book* book)
{
	BookshelfPriv   *priv;
	Book            *book2;
	
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));	
	
	priv = bookshelf->priv;

	/* Is the book already installed? */
	book2 = bookshelf_find_book_by_name (bookshelf, book_get_name (book));
	if (book2 != NULL && strcmp (book_get_version (book),
				     book_get_version (book2)) == 0) {
		return FALSE;
	}

	priv->books        = g_slist_prepend (priv->books, book);
	priv->current_book = book;
	
	return TRUE;
}

void
bookshelf_add_directory (Bookshelf *bookshelf, const gchar *directory)
{
	BookshelfPriv   *priv;
	GnomeVFSURI     *book_uri;
	GnomeVFSURI     *book_dir_uri;
	Book            *book;
	GSList          *books, *node;
	gchar           *book_file_name;
	gchar           *book_directory;
	gchar           *xml_filename;
	GList           *xml_books, *node2;
	XMLBook         *xml_book;
	
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	
	priv = bookshelf->priv;
	
	xml_filename = g_strdup_printf ("%s/books.xml", directory);
	xml_books = bookshelf_read_xml (bookshelf, xml_filename);
	book_directory = g_strdup_printf ("%s/specs", directory);
		
	book_dir_uri = gnome_vfs_uri_new (book_directory);
	books = bookshelf_read_books_dir (book_dir_uri);
        
	for (node = books; node; node = node->next) {
		book_file_name = g_strdup_printf (node->data);
		
		book_uri = gnome_vfs_uri_append_path (book_dir_uri,
						      book_file_name);
		g_free (book_file_name);
		
		book = book_new (book_uri, priv->fd);

		/* if book in xml-list */
		for (node2 = xml_books; node2; node2 = node2->next) {
			xml_book = (XMLBook*) node2->data;
			if (strcmp (xml_book->name, (gchar*)book_get_name (book)) == 0) {
				book_set_base_url (book, xml_book->path);
				book_set_visible (book, xml_book->visible);
			}
		}

		bookshelf_add_book (bookshelf, book);

		g_free (node->data);
	}

	priv->books = g_slist_sort (priv->books, book_compare_func);
	gnome_vfs_uri_unref (book_dir_uri);
	g_slist_free (books);
}

static GSList *
bookshelf_read_books_dir (GnomeVFSURI *books_uri)
{
	GSList                    *list;
	gchar                     *book_name;
	GList                     *dir_list, *node;
	GnomeVFSFileInfo          *info;
	GnomeVFSDirectoryFilter   *filter;
	GnomeVFSResult             result;
	gchar                     *str_uri;
	
	g_return_val_if_fail (books_uri != NULL, NULL);

	list = NULL;

	filter = gnome_vfs_directory_filter_new (GNOME_VFS_DIRECTORY_FILTER_NONE,
						 GNOME_VFS_DIRECTORY_FILTER_DEFAULT &
						 GNOME_VFS_DIRECTORY_FILTER_NODIRS |
						 GNOME_VFS_DIRECTORY_FILTER_NOPARENTDIR | 
						 GNOME_VFS_DIRECTORY_FILTER_NOSELFDIR,
						 NULL);
	
	str_uri = gnome_vfs_uri_to_string (books_uri, GNOME_VFS_URI_HIDE_NONE);
	result  = gnome_vfs_directory_list_load (&dir_list, str_uri,
						 GNOME_VFS_FILE_INFO_DEFAULT,
						 filter);
	g_free (str_uri);
	gnome_vfs_directory_filter_destroy (filter);

	/* If no books are found. */
	if (result == GNOME_VFS_ERROR_NOT_FOUND) {
		return NULL;
	} else if (result != GNOME_VFS_OK) {
		g_warning (_("Problems when reading books: %s\n"), 
			   gnome_vfs_result_to_string (result));
		return NULL;
	}

	for (node = dir_list; node; node = node->next) {
		info = (GnomeVFSFileInfo *) node->data;
		book_name = g_strdup (info->name);
		list = g_slist_prepend (list, book_name);
	}

	return list;
}

Bookshelf *
bookshelf_new (const gchar* default_dir, FunctionDatabase *fd)
{
	Bookshelf       *bookshelf;
	BookshelfPriv   *priv;

	bookshelf = gtk_type_new (TYPE_BOOKSHELF);
	priv      = bookshelf->priv;
	priv->fd  = fd;

	bookshelf_add_directory (bookshelf, default_dir);
	
	return bookshelf;
}

Book *
bookshelf_find_book_by_title (Bookshelf *bookshelf, const gchar *title)
{
	BookshelfPriv   *priv;
	Book            *book;
	GSList          *node;

	g_return_val_if_fail (bookshelf != NULL, NULL);
	g_return_val_if_fail (IS_BOOKSHELF (bookshelf), NULL);
	g_return_val_if_fail (title != NULL, NULL);

	priv = bookshelf->priv;
	
	for (node = priv->books; node; node = node->next) {
		book = BOOK (node->data);
		
		if (!strcmp (title, book_get_title (book))) {
			return book;
		}
	}
	
	return NULL;
}

Book *
bookshelf_find_book_by_uri (Bookshelf *bookshelf, const GnomeVFSURI *uri)
{
	BookshelfPriv   *priv;
	Book            *book;
	GSList          *node;

	g_return_val_if_fail (bookshelf != NULL, NULL);
	g_return_val_if_fail (IS_BOOKSHELF (bookshelf), NULL);
	g_return_val_if_fail (uri != NULL, NULL);

	priv = bookshelf->priv;
	
	for (node = priv->books; node; node = node->next) {
		book = BOOK (node->data);
		
		if (book_contains (book, uri)) {
			return book;
		}
	}
	
	return NULL;
}

Book *
bookshelf_find_book_by_name (Bookshelf *bookshelf, const gchar *name) 
{
	BookshelfPriv   *priv;
	Book            *book;
	GSList          *node;
	const gchar     *book_name;

	g_return_val_if_fail (bookshelf != NULL, NULL);
	g_return_val_if_fail (IS_BOOKSHELF (bookshelf), NULL);
	g_return_val_if_fail (name != NULL, NULL);

	priv = bookshelf->priv;
	
	for (node = priv->books; node; node = node->next) {
		book = BOOK (node->data);
		
		book_name = book_get_name (book);
		
		if (book_name && !strcmp (book_name, name)) {
			return book;
		}
	}

	return NULL;
}

GSList *
bookshelf_get_books (Bookshelf *bookshelf)
{
	BookshelfPriv   *priv;
        
	g_return_val_if_fail (bookshelf != NULL, NULL);
	g_return_val_if_fail (IS_BOOKSHELF (bookshelf), NULL);
        
	priv = bookshelf->priv;
        
	return priv->books;
}

void
bookshelf_open_document (Bookshelf *bookshelf, const Document *document)
{
	BookshelfPriv   *priv;
	Book            *book;
	
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	g_return_if_fail (document != NULL);
        
	priv = bookshelf->priv;

	book = document_get_book (document);
	
	priv->current_book = book;

	book_open_document (book, document);
}

BookNode *
bookshelf_find_node (Bookshelf     *bookshelf, 
		     Document      *document, 
		     const gchar   *anchor)
{
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	g_return_if_fail (document != NULL);

	return book_find_node (document_get_book (document), document, anchor);
}

Document *
bookshelf_find_document (Bookshelf      *bookshelf, 
			 const gchar    *url, 
			 gchar         **anchor)
{
	BookshelfPriv    *priv;
	Book             *book;
	Document         *document = NULL;
	gint              depth;
	gint              un_depth;
	GnomeVFSURI      *uri;
	gchar            *book_name;
	
	g_return_val_if_fail (bookshelf != NULL, NULL);
	g_return_val_if_fail (IS_BOOKSHELF (bookshelf), NULL);
	
	priv    = bookshelf->priv;

	if (!util_uri_is_relative (url)) {
		uri = gnome_vfs_uri_new (url);
		book = bookshelf_find_book_by_uri (bookshelf, uri);
		
		if (book) {
			
/* 			book_node = book_find_node_by_uri (book, uri); */
		}
		
		gnome_vfs_uri_unref (uri);
	} else {
		if (!priv->current_book) {
			return NULL;
		}

		depth    = book_get_current_depth (priv->current_book);
		un_depth = util_url_get_un_depth (url);
		
		if (depth >= un_depth) {
			document = book_find_document (priv->current_book, 
						       url, anchor);
		} else {
			book_name = util_url_get_book_name (url);
			
			if (book_name) {
				book = bookshelf_find_book_by_name (bookshelf,
								    book_name);
				g_free (book_name);
				
				if (book) {
					document = book_find_document (book,
								       url, 
								       anchor);
				}
			}
		}
	}
	
	return document;
}
