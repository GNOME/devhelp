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
#include <glib.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-messagebox.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libxml/parser.h>
#include "function-database.h"
#include "util.h"
#include "bookshelf.h"

#define d(x)

static void      bookshelf_init              (Bookshelf        *bookshelf);
static void      bookshelf_class_init        (GObjectClass     *klass);
static void      bookshelf_destroy           (GObject          *object);

static GSList *  bookshelf_read_books_dir    (GnomeVFSURI      *books_uri);

static GSList *  bookshelf_read_xml          (Bookshelf        *bookshelf,
					      const gchar      *filename);


struct _BookshelfPriv {
	GSList             *books;
	FunctionDatabase   *fd;

	Book               *current_book;
	
	const gchar        *filename;
	GSList             *xml_books;
};

enum {
	BOOK_ADDED,
	BOOK_REMOVED,
	LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

GType
bookshelf_get_type (void)
{
	static GType bookshelf_type = 0;
        
	if (!bookshelf_type) {
		static const GTypeInfo bookshelf_info = {
			sizeof (BookshelfClass),
			NULL,
			NULL,
			(GClassInitFunc)  bookshelf_class_init,
			NULL,
			NULL,
			sizeof (Bookshelf),
			0,
			(GInstanceInitFunc) bookshelf_init,
		};
                
		bookshelf_type = g_type_register_static (G_TYPE_OBJECT,
							 "Bookshelf",
							 &bookshelf_info, 0);
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
bookshelf_class_init (GObjectClass *klass)
{
	klass->finalize = bookshelf_destroy;

	signals[BOOK_ADDED] = 
		g_signal_new ("book_added",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (BookshelfClass,
						book_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);

	signals[BOOK_REMOVED] = 
		g_signal_new ("book_removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (BookshelfClass,
					       book_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);
}

static void
bookshelf_destroy (GObject *object)
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

static GSList *
bookshelf_read_books_dir (GnomeVFSURI *books_uri)
{
	GSList                    *list;
	gchar                     *book_name;
	GList                     *dir_list, *node;
	GnomeVFSFileInfo          *info;
//	GnomeVFSDirectoryFilter   *filter;
	GnomeVFSResult             result;
	gchar                     *str_uri;
	
	g_return_val_if_fail (books_uri != NULL, NULL);

	list = NULL;
#if 0
	filter = gnome_vfs_directory_filter_new (GNOME_VFS_DIRECTORY_FILTER_NONE,
						 GNOME_VFS_DIRECTORY_FILTER_DEFAULT &
						 GNOME_VFS_DIRECTORY_FILTER_NODIRS |
						 GNOME_VFS_DIRECTORY_FILTER_NOPARENTDIR | 
						 GNOME_VFS_DIRECTORY_FILTER_NOSELFDIR,
						 NULL);
#endif	
	str_uri = gnome_vfs_uri_to_string (books_uri, GNOME_VFS_URI_HIDE_NONE);
	result  = gnome_vfs_directory_list_load (&dir_list, str_uri,
						 GNOME_VFS_FILE_INFO_DEFAULT);
//						 filter);
	g_free (str_uri);
//	gnome_vfs_directory_filter_destroy (filter);

	/* If no books are found. */
	if (result == GNOME_VFS_ERROR_NOT_FOUND) {
		return NULL;
	} 
	else if (result != GNOME_VFS_OK) {
		g_warning (_("Problems when reading books: %s\n"), 
			   gnome_vfs_result_to_string (result));
		return NULL;
	}

	for (node = dir_list; node; node = node->next) {
		int len;
		info = (GnomeVFSFileInfo *) node->data;
		if (strlen (info->name) <= 8) {
			continue;
		} else if (strncmp (info->name + (strlen (info->name)-8), ".devhelp", 8) != 0) {
			continue;
		}
		g_message ("info->name: %s", info->name);
		book_name = g_strdup (info->name);
		list = g_slist_prepend (list, book_name);
	}

	return list;
}

Bookshelf *
bookshelf_new (FunctionDatabase *fd)
{
	Bookshelf       *bookshelf;
	BookshelfPriv   *priv;
	const gchar     *home_dir;
	gchar           *filename;
	gchar           *user_dir;
	
	bookshelf = g_object_new (TYPE_BOOKSHELF, NULL);
	priv      = bookshelf->priv;
	priv->fd  = fd;

	home_dir = g_get_home_dir ();

	filename = g_strdup_printf ("%s/.devhelp/books.xml", home_dir);
	priv->xml_books = bookshelf_read_xml (bookshelf, filename);
	priv->filename = filename;

        function_database_freeze (priv->fd);

	/* First add user directory */
        user_dir = g_strdup_printf ("%s/.devhelp", home_dir);
	bookshelf_add_directory (bookshelf, user_dir);
        g_free (user_dir);

	/* Then add global directories */
	
	/* If we have a non-standard datadir */
	if (strcmp (DATA_DIR, "/usr/share") &&
	    strcmp (DATA_DIR, "/usr/local/share")) {
		g_message ("Adding %s", DATA_DIR);
		bookshelf_add_directory (bookshelf, DATA_DIR"/devhelp");
	}
	
	bookshelf_add_directory (bookshelf, "/usr/share/devhelp"); 
	bookshelf_add_directory (bookshelf, "/usr/local/share/devhelp"); 

        function_database_thaw (priv->fd);
	
	return bookshelf;
}

FunctionDatabase * 
bookshelf_get_function_database (Bookshelf *bookshelf)
{
	BookshelfPriv   *priv;
	
	g_return_val_if_fail (bookshelf != NULL, NULL);
	g_return_val_if_fail (IS_BOOKSHELF (bookshelf), NULL);
	
	priv = bookshelf->priv;
	
	return priv->fd;
}

GSList * 
bookshelf_get_hidden_books (Bookshelf *bookshelf)
{
	BookshelfPriv     *priv;
	GSList            *list;
	GSList            *hidden;
	XMLBook           *book;
	const GnomeVFSURI *book_uri;
	
	g_return_val_if_fail (bookshelf != NULL, NULL);
	g_return_val_if_fail (IS_BOOKSHELF (bookshelf), NULL);
	
	priv = bookshelf->priv;

	hidden = NULL;
	for (list = priv->xml_books; list; list = list->next) {
		book = (XMLBook*)list->data;
		/* Is the book visible? */
		if (book->visible == TRUE) {
			continue;
		}
		hidden = g_slist_append (hidden, book);
	}
	
	return hidden;
}

void 
bookshelf_hide_book (Bookshelf *bookshelf, Book *book)
{
	BookshelfPriv   *priv;
	XMLBook         *xml_book;
	
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));

	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = bookshelf->priv;

	xml_book = g_new (XMLBook, 1);
	xml_book->name      = book_get_name (book);
	xml_book->version   = book_get_version (book);	
	xml_book->spec_path = book_get_spec_file (book);
	xml_book->visible   = FALSE;
			
	bookshelf_remove_book (bookshelf, book);
	priv->xml_books = g_slist_append (priv->xml_books, xml_book);
}

void
bookshelf_show_book (Bookshelf *bookshelf, XMLBook *xml_book)
{
	BookshelfPriv *priv;
	GnomeVFSURI   *book_uri;
	Book          *book;
	gchar         *tmp;
	gchar         *dirname;	
	gchar         *base_url;
	
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));

	priv = bookshelf->priv;

	book_uri = gnome_vfs_uri_new (xml_book->spec_path); 
	book = book_new (book_uri, priv->fd);
	book_set_visible (book, TRUE);
	
	tmp = gnome_vfs_uri_extract_dirname (book_uri);
	dirname = g_strndup (tmp, strlen (tmp)-6);
	base_url = g_strdup_printf ("%s/books/%s",
				    dirname,
				    book_get_name_full (book));
	book_set_base_url (book, base_url);
	g_free (dirname);
	
	priv->xml_books = g_slist_remove (priv->xml_books, xml_book);
	bookshelf_add_book (bookshelf, book);
}


static GSList *
bookshelf_read_xml (Bookshelf *bookshelf, const gchar *filename)
{
	BookshelfPriv   *priv;
	XMLBook         *book;
	GSList          *list;
	xmlDocPtr        doc;
	xmlNode         *root_node, *cur;
	gchar           *visible;
	
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	
	priv = bookshelf->priv;

	if (g_file_test (filename, G_FILE_TEST_EXISTS) == FALSE) {
		return NULL;
	}
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
			book->spec_path = xmlGetProp (cur, "spec");
			book->name      = xmlGetProp (cur, "name");
			book->version   = xmlGetProp (cur, "version");
			//book->book_path = xmlGetProp (cur, "path");
			visible       = xmlGetProp (cur, "visible");
			if (visible != NULL) {
				book->visible = atoi (visible);
			} else {
				book->visible = TRUE;
			}
			xmlFree (visible);

			/* Okay, we found an old spec file.
			 * Remove it and return NULL (no books)
			 */
			if (book->spec_path == NULL) {
				gchar *tmp;
				GtkWidget *dialog;
				
				tmp = g_strdup_printf ("rm -f %s", filename);
				system (tmp);
				g_free (tmp);

				tmp = g_strdup_printf (_("I found an old version of your spec file (%s)\n"
							 "I removed it, so you have to hide the books you want to be hidden again."),
						       filename);
				
				dialog = gnome_message_box_new (tmp, GNOME_MESSAGE_BOX_INFO,
								_("Ok"), NULL);
				gnome_dialog_run (GNOME_DIALOG (dialog));
				g_free (tmp);

				return NULL;
			}
			
			if (gnome_vfs_uri_exists (gnome_vfs_uri_new (book->spec_path))) {
				list = g_slist_append (list, book);
			} else {
				g_message ("Hidden book %s does not exist",
					   book->name);
			}
		}

		cur = cur->next;
	}
	
	/* TODO: lot's of free() in this func */
	return list;
}

void
bookshelf_write_xml (Bookshelf *bookshelf)
{
	BookshelfPriv   *priv;
	XMLBook         *book;
	FILE            *fp;
	GSList          *node;
	
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	
	priv = bookshelf->priv;

	if (priv->filename == NULL) {
		fp = stdout;
	} else {
		fp = fopen (priv->filename, "w");

		if (fp == NULL) {
			g_warning (_("Failed to open file %s for writing."), priv->filename);
			return;
		}
	}
	
	fprintf (fp, "<?xml version=\"1.0\"?>\n\n");
	fprintf (fp, "<booklist>\n");

	for (node = priv->xml_books; node; node = node->next) {
		book = (XMLBook*)node->data;

		if (book->visible == TRUE) {
			continue;
		}
		
		fprintf (fp, "  <book ");

		fprintf (fp, "name=\"%s\" ", book->name);
		fprintf (fp, "spec=\"%s\" ", book->spec_path);

		if (book->version != NULL) {
			fprintf (fp, "version=\"%s\" ", book->version);
		}
		fprintf (fp, "visible=\"0\"/>\n");
	}
	
	fprintf (fp, "</booklist>\n");

	if (fp != stdout) {
		fclose (fp);
	}
}

static guint
version_strcmp (Book *book1, Book *book2)
{
	const gchar *version1 = book_get_version (book1);
	const gchar *version2 = book_get_version (book2);
	
	if (version1 == NULL && book2 == NULL) {
		return 0;
	} 
	else if (version1 == NULL || version2 == NULL) {
		return -1;
	} else {
		return strcmp (version1, version2);
	}
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
	if (bookshelf_have_book (bookshelf, book)) {
		return FALSE;
	}
			
	priv->books        = g_slist_prepend (priv->books, book);
	priv->current_book = book;
      
	g_signal_emit (G_OBJECT (bookshelf),
		       signals[BOOK_ADDED],
		       0,
                       book);

	return TRUE;
}

static XMLBook*
xml_spec_get_book_uri (GSList *xml_books, const GnomeVFSURI *book_uri)
{
	const gchar *str_uri;
	GSList      *node;
	XMLBook     *xml_book;
	
	str_uri = gnome_vfs_uri_to_string (book_uri, GNOME_VFS_URI_HIDE_NONE);
	
	for (node = xml_books; node; node = node->next) {
		xml_book = (XMLBook*) node->data;
			
		if (!strcmp (xml_book->spec_path, str_uri)) {
			return xml_book;
		}
	}
	
	return NULL;
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
	gchar           *home_dir;
	GSList          *node2;
	XMLBook         *xml_book;
	gboolean         skip;
	
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	
	priv = bookshelf->priv;
	
	book_directory = g_strdup_printf ("%s/specs", directory);
		
	book_dir_uri = gnome_vfs_uri_new (book_directory);
	if (!gnome_vfs_uri_exists (book_dir_uri)) {
		return;
	}
	
	books = bookshelf_read_books_dir (book_dir_uri);
        
	skip = FALSE;
	for (node = books; node; node = node->next) {
		book_file_name = g_strdup_printf (node->data);
		
		book_uri = gnome_vfs_uri_append_path (book_dir_uri,
						      book_file_name);
		
		book = book_new (book_uri, priv->fd);

		xml_book = xml_spec_get_book_uri (priv->xml_books, book_uri);
		if (xml_book != NULL) {
			gchar *basename_str;
			gchar *book_uri_str;

			basename_str = (gchar*)basename (xml_book->spec_path);
			book_uri_str = (gchar*)basename (gnome_vfs_uri_to_string (book_uri,
										  GNOME_VFS_URI_HIDE_NONE));
			if (!strcmp (basename_str, book_uri_str)) {
				book_set_base_url (book, xml_book->spec_path);
			}
			
			if (xml_book->visible == FALSE) {
				skip = TRUE;
			}
		} else {
			gchar *book_dir = g_strdup_printf ("file://%s/books/%s",
							   directory,
							   book_get_name_full (book));
			book_set_base_url (book, book_dir);
			g_free (book_dir);
		}
		g_free (book_file_name);

		if (skip == TRUE) {
			/* TODO: g_free (book) ? */
			skip = FALSE;
			continue;
		}
		
		bookshelf_add_book (bookshelf, book);
		
		g_free (node->data);
	}
	
	priv->books = g_slist_sort (priv->books, book_compare_func);
	gnome_vfs_uri_unref (book_dir_uri);
	g_slist_free (books);
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

void
bookshelf_remove_book (Bookshelf *bookshelf, Book *book)
{
	BookshelfPriv   *priv;
	GSList          *node;
	
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));

	priv = bookshelf->priv;
	
	priv->books = g_slist_remove (priv->books, book);

	g_signal_emit (G_OBJECT (bookshelf),
		       signals[BOOK_REMOVED],
		       GPOINTER_TO_INT (book));
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
bookshelf_find_node (Bookshelf        *bookshelf, 
		     const Document   *document, 
		     const gchar      *anchor)
{
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	g_return_if_fail (document != NULL);

	return book_find_node (document_get_book (document), document, anchor);
}

gboolean
bookshelf_have_book (Bookshelf *bookshelf,
		     Book      *book)
{
	BookshelfPriv *priv;
	GSList        *node;
	
	g_return_if_fail (bookshelf != NULL);
	g_return_if_fail (IS_BOOKSHELF (bookshelf));
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));

	priv = bookshelf->priv;
	/* Does the bookshelf have the book in the visible part? */
	for (node = priv->books; node; node = node->next) {
		Book *tmp = BOOK (node->data);
			
		if (strcmp (book_get_name_full (tmp),
			    book_get_name_full (book)) == 0) {
			return TRUE;
		}
	}

	/* ... or in the hidden? */
	for (node = priv->xml_books; node; node = node->next) {
		XMLBook *tmp = (XMLBook*) node->data;

		if (strcmp (tmp->name, book_get_name (book)) == 0 &&
		    strcmp (tmp->version, book_get_version (book)) == 0) {
			return TRUE;
		}
	}
	
	return FALSE;
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
	gchar            *doc_url;
	
	g_return_val_if_fail (bookshelf != NULL, NULL);
	g_return_val_if_fail (IS_BOOKSHELF (bookshelf), NULL);
	
	priv    = bookshelf->priv;

	if (!util_uri_is_relative (url)) {
 		uri = gnome_vfs_uri_new (url); 
		book = bookshelf_find_book_by_uri (bookshelf, uri);
		
		if (book) {
 			doc_url = util_url_split (url, anchor);
			document = book_find_document (book, doc_url, NULL);
			g_free (doc_url);
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
