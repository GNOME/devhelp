/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
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

#include "dh-link.h"
#include "dh-parser.h"
#include "dh-book-manager.h"

/* Structure defining basic contents to store about every book */
typedef struct {
        /* File path of the book */
        gchar    *path;
        /* Enable or disabled? */
        gboolean  enabled;
        /* Generated book tree */
        GNode    *tree;
        /* Generated list of keywords in the book */
        GList    *keywords;
} DhBook;

typedef struct {
        /* The list of all books found in the system */
        GList *books;
} DhBookManagerPriv;

G_DEFINE_TYPE (DhBookManager, dh_book_manager, G_TYPE_OBJECT);

#define GET_PRIVATE(instance) G_TYPE_INSTANCE_GET_PRIVATE \
        (instance, DH_TYPE_BOOK_MANAGER, DhBookManagerPriv);

static void    dh_book_manager_init       (DhBookManager      *book_manager);
static void    dh_book_manager_class_init (DhBookManagerClass *klass);

static void    book_manager_add_from_filepath     (DhBookManager *book_manager,
                                                   const gchar   *book_path);
static void    book_manager_add_from_dir          (DhBookManager *book_manager,
                                                   const gchar   *dir_path);

#ifdef GDK_WINDOWING_QUARTZ
static void    book_manager_add_from_xcode_docset (DhBookManager *book_manager,
                                                   const gchar   *dir_path);
#endif

static DhBook *book_new                   (const gchar  *book_path);
static void    book_free                  (DhBook       *book);
static gint    book_cmp                   (const DhBook *a,
                                           const DhBook *b);

static void
book_manager_finalize (GObject *object)
{
        DhBookManagerPriv *priv;
        GList *walker;

        priv = GET_PRIVATE (object);

        walker = priv->books;
        while (walker) {
                book_free ((DhBook *)walker->data);
                walker = g_list_next (walker);
        }
        g_list_free (priv->books);

        G_OBJECT_CLASS (dh_book_manager_parent_class)->finalize (object);
}

static void
dh_book_manager_class_init (DhBookManagerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = book_manager_finalize;

	g_type_class_add_private (klass, sizeof (DhBookManagerPriv));
}

static void
dh_book_manager_init (DhBookManager *book_manager)
{
        DhBookManagerPriv *priv = GET_PRIVATE (book_manager);

        /* Empty list of books */
        priv->books = NULL;
}

static gchar *
book_manager_get_book_path (const gchar *base_path,
                            const gchar *name)
{
        static const gchar *suffixes[] = {
                "devhelp2",
                "devhelp2.gz",
                "devhelp",
                "devhelp.gz",
                NULL
        };
        gchar *tmp;
        gchar *book_path;
        guint  i;

        for (i = 0; suffixes[i]; i++) {
                tmp = g_build_filename (base_path, name, name, NULL);
                book_path = g_strconcat (tmp, ".", suffixes[i], NULL);
                g_free (tmp);

                if (g_file_test (book_path, G_FILE_TEST_EXISTS)) {
                        return book_path;;
                }
                g_free (book_path);
        }
        return NULL;
}

static void
book_manager_add_from_dir (DhBookManager *book_manager,
                           const gchar   *dir_path)
{
        GError      *error = NULL;
        GDir        *dir;
        const gchar *name;

        g_return_if_fail (book_manager);
        g_return_if_fail (dir_path);

        /* Open directory */
        dir = g_dir_open (dir_path, 0, &error);
        if (!dir) {
                g_warning ("Failed to open directory '%s': %s",
                           dir_path, error->message);
                g_error_free (error);
                return;
        }

        /* And iterate it */
        while ((name = g_dir_read_name (dir)) != NULL) {
                gchar *book_path;

                book_path = book_manager_get_book_path (dir_path, name);
                if (book_path) {
                        /* Add book from filepath */
                        book_manager_add_from_filepath (book_manager,
                                                        book_path);
                        g_free (book_path);
                }
        }

        g_dir_close (dir);
}

#ifdef GDK_WINDOWING_QUARTZ
static gboolean
seems_docset_dir (const gchar *path)
{
        gchar    *tmp;
        gboolean  seems_like_devhelp = FALSE;

        g_return_val_if_fail (path, FALSE);

        /* Do some sanity checking on the directory first so we don't have
         * to go through several hundreds of files in every docset.
         */
        tmp = g_build_filename (path, "style.css", NULL);
        if (g_file_test (tmp, G_FILE_TEST_EXISTS)) {
                gchar *tmp;

                tmp = g_build_filename (path, "index.sgml", NULL);
                if (g_file_test (tmp, G_FILE_TEST_EXISTS)) {
                        seems_like_devhelp = TRUE;
                }
                g_free (tmp);
        }
        g_free (tmp);

        return seems_like_devhelp;
}

static void
book_manager_add_from_xcode_docset (DhBookManager *book_manager,
                                    const gchar   *dir_path)
{
        GError      *error = NULL;
        GDir        *dir;
        const gchar *name;

        g_return_if_fail (book_manager);
        g_return_if_fail (dir_path);

        if (!seems_docset_dir (dir_path)) {
                return;
        }

        /* Open directory */
        dir = g_dir_open (dir_path, 0, &error);
        if (!dir) {
                g_warning ("Failed to open directory '%s': %s",
                           dir_path, error->message);
                g_error_free (error);
                return;
        }

        /* And iterate it, looking for files ending with .devhelp2 */
        while ((name = g_dir_read_name (dir)) != NULL) {
                if (g_strcmp0 (strrchr (name, '.'),
                               ".devhelp2") == 0) {
                        gchar *book_path;

                        book_path = g_build_filename (path, name, NULL);
                        /* Add book from filepath */
                        book_manager_add_from_filepath (book_manager,
                                                        book_path);
                        g_free (book_path);
                }
        }

        g_dir_close (dir);
}
#endif

static void
book_manager_add_from_filepath (DhBookManager *book_manager,
                                const gchar   *book_path)
{
        DhBookManagerPriv *priv = GET_PRIVATE (book_manager);
        DhBook *book;
        GError *error = NULL;

        g_return_if_fail (book_manager);
        g_return_if_fail (book_path);

        /* Allocate new book struct */
        book = book_new (book_path);

        /* Check if book was already loaded in the manager */
        if (g_list_find_custom (priv->books,
                                book,
                                (GCompareFunc)book_cmp)) {
                book_free (book);
                return;
        }

        /* Parse file storing contents in the book struct */
        if (!dh_parser_read_file  (book_path,
                                   book->tree,
                                   &book->keywords,
                                   &error)) {
                g_warning ("Failed to read '%s': %s",
                           book_path, error->message);
                g_error_free (error);

                /* Deallocate the book, as we are not going to add it
                 *  in the manager */
                book_free (book);
                return;
        }

        /* Add the book to the book list */
        priv->books = g_list_insert_sorted (priv->books,
                                            book,
                                            (GCompareFunc)book_cmp);
}


DhBookManager *
dh_book_manager_new (void)
{
        return g_object_new (DH_TYPE_BOOK_MANAGER, NULL);
}

/* Single book creation/destruction/management */

static DhBook *
book_new (const gchar *book_path)
{
        DhBook *book;

        g_return_val_if_fail (book_path, NULL);

        /* Allocate and initialize new book struct, by default
         *  enabled */
        book = g_malloc0 (sizeof (*book));
        book->path = g_strdup (book_path);
        book->enabled = TRUE;
        book->tree = g_node_new (NULL);
        book->keywords = NULL;

        return book;
}

static void
unref_node_link (GNode *node,
                 gpointer data)
{
        dh_link_unref (node->data);
}

static void
book_free (DhBook *book)
{
        g_return_if_fail (book);

        if (book->tree) {
                g_node_traverse (book->tree,
                                 G_IN_ORDER,
                                 G_TRAVERSE_ALL,
                                 -1,
                                 (GNodeTraverseFunc)unref_node_link,
                                 NULL);
                g_node_destroy (book->tree);
        }

        if (book->keywords) {
                g_list_foreach (book->keywords, (GFunc)dh_link_unref, NULL);
                g_list_free (book->keywords);
        }

        g_free (book->path);
        g_free (book);
}

static gint book_cmp (const DhBook *a,
                      const DhBook *b)
{
        return (a && b) ? g_strcmp0 (a->path, b->path) : -1;
}
