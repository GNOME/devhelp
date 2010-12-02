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
#include "dh-util.h"
#include "dh-book.h"
#include "dh-book-manager.h"
#include "dh-marshal.h"

typedef struct {
        /* The list of all DhBooks found in the system */
        GList      *books;
        /* HT with the monitors setup */
        GHashTable *monitors;
        /* List of book names currently disabled */
        GSList     *books_disabled;
} DhBookManagerPriv;

enum {
        BOOK_CREATED,
        BOOK_DELETED,
        BOOK_ENABLED,
        BOOK_DISABLED,
        LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (DhBookManager, dh_book_manager, G_TYPE_OBJECT);

#define GET_PRIVATE(instance) G_TYPE_INSTANCE_GET_PRIVATE       \
        (instance, DH_TYPE_BOOK_MANAGER, DhBookManagerPriv)

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

static void
book_manager_finalize (GObject *object)
{
        DhBookManagerPriv *priv;
        GList             *l;
        GSList            *sl;

        priv = GET_PRIVATE (object);

        /* Destroy all books */
        for (l = priv->books; l; l = g_list_next (l)) {
                g_object_unref (l->data);
        }
        g_list_free (priv->books);

        /* Destroy the monitors HT */
        if (priv->monitors) {
                g_hash_table_destroy (priv->monitors);
        }

        /* Clean the list of books disabled */
        for (sl = priv->books_disabled; sl; sl = g_slist_next (sl)) {
                g_free (sl->data);
        }
        g_slist_free (priv->books_disabled);

        G_OBJECT_CLASS (dh_book_manager_parent_class)->finalize (object);
}

static void
dh_book_manager_class_init (DhBookManagerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = book_manager_finalize;

        signals[BOOK_CREATED] =
                g_signal_new ("book-created",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL,
                              _dh_marshal_VOID__OBJECT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_OBJECT);
        signals[BOOK_DELETED] =
                g_signal_new ("book-deleted",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL,
                              _dh_marshal_VOID__OBJECT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_OBJECT);
        signals[BOOK_ENABLED] =
                g_signal_new ("book-enabled",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL,
                              _dh_marshal_VOID__OBJECT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_OBJECT);
        signals[BOOK_DISABLED] =
                g_signal_new ("book-disabled",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL,
                              _dh_marshal_VOID__OBJECT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_OBJECT);

	g_type_class_add_private (klass, sizeof (DhBookManagerPriv));
}

static void
dh_book_manager_init (DhBookManager *book_manager)
{
        DhBookManagerPriv *priv = GET_PRIVATE (book_manager);

        priv->books = NULL;
        priv->monitors = NULL;

        priv->books_disabled = dh_util_state_load_books_disabled ();
}

static gboolean
book_manager_is_book_disabled_in_conf (DhBookManager *book_manager,
                                       DhBook        *book)
{
        DhBookManagerPriv *priv = GET_PRIVATE (book_manager);
        GSList            *li;

        for (li = priv->books_disabled; li; li = g_slist_next (li)) {
                if (g_strcmp0 (dh_book_get_name (book),
                               (const gchar *)li->data) == 0) {
                        return TRUE;
                }
        }

        return FALSE;
}

static void
book_manager_add_books_in_data_dir (DhBookManager *book_manager,
                                    const gchar   *data_dir)
{
        gchar *dir;

        dir = g_build_filename (data_dir, "gtk-doc", "html", NULL);
        book_manager_add_from_dir (book_manager, dir);
        g_free (dir);

        dir = g_build_filename (data_dir, "devhelp", "books", NULL);
        book_manager_add_from_dir (book_manager, dir);
        g_free (dir);
}

void
dh_book_manager_populate (DhBookManager *book_manager)
{
        const gchar * const * system_dirs;

        book_manager_add_books_in_data_dir (book_manager,
                                            g_get_user_data_dir ());

        system_dirs = g_get_system_data_dirs ();
        while (*system_dirs) {
                book_manager_add_books_in_data_dir (book_manager,
                                                    *system_dirs);
                system_dirs++;
        }

#ifdef GDK_WINDOWING_QUARTZ
        book_manager_add_from_xcode_docset (
                book_manager,
                "/Library/Developer/Shared/Documentation/DocSets");
#endif
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
                tmp = g_build_filename (base_path, name, NULL);
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
book_manager_booklist_monitor_event_cb (GFileMonitor      *file_monitor,
                                        GFile             *file,
                                        GFile             *other_file,
                                        GFileMonitorEvent  event_type,
                                        gpointer	   user_data)
{
        DhBookManager *book_manager = user_data;
        GError        *error = NULL;
        GFileInfo     *file_info;
        gchar         *file_path;
        gchar         *file_basename;
        gchar         *book_path;

        /* In the book manager we only handle events for new directories
         * created. Books removed or updated are handled by the book objects
         * themselves */
        if (event_type != G_FILE_MONITOR_EVENT_CREATED) {
                return;
        }

        /* Get file info */
        file_info = g_file_query_info (file,
                                       G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                       G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                       NULL,
                                       &error);
        if (!file_info) {
                gchar *file_uri;

                file_uri = g_file_get_uri (file);
                g_warning ("Couldn't query file type of '%s': %s",
                           file_uri,
                           error ? error->message : "unknown error");
                g_free (file_uri);
                g_clear_error (&error);
                return;
        }

        /* The created file must be a directory! */
        if (g_file_info_get_file_type (file_info) != G_FILE_TYPE_DIRECTORY) {
                g_object_unref (file_info);
                return;
        }
        g_object_unref (file_info);

        file_path = g_file_get_path (file);
        file_basename = g_file_get_basename (file);

        /* Compute book path */
        book_path = book_manager_get_book_path (file_path, file_basename);
        if (book_path) {
                /* Add book from filepath */
                book_manager_add_from_filepath (book_manager,
                                                book_path);
                g_free (book_path);
        }

        g_free (file_path);
        g_free (file_basename);
}

static void
book_manager_monitor_path (DhBookManager *book_manager,
                           const gchar   *path)
{
        GFileMonitor      *file_monitor;
        GFile             *file;
        DhBookManagerPriv *priv;

        priv = GET_PRIVATE (book_manager);

        file = g_file_new_for_path (path);

        /* If monitor already exists, do not re-add it */
        if (priv->monitors &&
            g_hash_table_lookup (priv->monitors, file)) {
                return;
        }

        /* Create new monitor for the given directory */
        file_monitor = g_file_monitor_directory (file,
                                                 G_FILE_MONITOR_NONE,
                                                 NULL,
                                                 NULL);
        if (file_monitor) {
                /* Setup changed signal callback */
                g_signal_connect (file_monitor, "changed",
                                  G_CALLBACK (book_manager_booklist_monitor_event_cb),
                                  book_manager);

                /* Create HT if not already there */
                if (G_UNLIKELY (!priv->monitors)) {
                        priv->monitors = g_hash_table_new_full (g_file_hash,
                                                                (GEqualFunc) g_file_equal,
                                                                (GDestroyNotify) g_object_unref,
                                                                (GDestroyNotify) g_object_unref);
                }

                /* Add the directory to the monitors HT */
                g_hash_table_insert (priv->monitors,
                                     g_object_ref (file),
                                     file_monitor);
        } else {
                g_warning ("Couldn't setup to monitor changes on directory '%s'",
                           path);
        }

        g_object_unref (file);
}

static void
book_manager_add_from_dir (DhBookManager *book_manager,
                           const gchar   *dir_path)
{
        GDir        *dir;
        const gchar *name;

        g_return_if_fail (book_manager);
        g_return_if_fail (dir_path);

        /* Open directory */
        dir = g_dir_open (dir_path, 0, NULL);
        if (!dir) {
                return;
        }

        /* Monitor the directory for changes */
        book_manager_monitor_path (book_manager, dir_path);

        /* And iterate it */
        while ((name = g_dir_read_name (dir)) != NULL) {
                gchar *book_dir_path;
                gchar *book_path;

                /* Build the path of the directory where the final
                 * devhelp book resides */
                book_dir_path = g_build_filename (G_DIR_SEPARATOR_S,
                                                  dir_path,
                                                  name,
                                                  NULL);

                book_path = book_manager_get_book_path (book_dir_path, name);
                if (book_path) {
                        /* Add book from filepath */
                        book_manager_add_from_filepath (book_manager,
                                                        book_path);
                        g_free (book_path);
                }
                g_free (book_dir_path);
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
        GDir        *dir;
        const gchar *name;

        g_return_if_fail (book_manager);
        g_return_if_fail (dir_path);

        if (!seems_docset_dir (dir_path)) {
                return;
        }

        /* Open directory */
        dir = g_dir_open (dir_path, 0, NULL);
        if (!dir) {
                return;
        }

        /* Monitor the directory for changes (if it works on MacOSX,
         * not sure if GIO implements GFileMonitor based on FSEvents
         * or what */
        book_manager_monitor_path (book_manager, dir_path);

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
book_manager_book_deleted_cb (DhBook   *book,
                              gpointer  user_data)
{
        DhBookManager     *book_manager = user_data;
        DhBookManagerPriv *priv = GET_PRIVATE (book_manager);
        GList *li;

        /* Look for the item we want to remove */
        li = g_list_find (priv->books, book);
        if (li) {
                g_debug ("Deleting book '%s' from the book manager list",
                         dh_book_get_title (book));

                /* Emit signal to notify others */
                g_signal_emit (book_manager,
                               signals[BOOK_DELETED],
                               0,
                               book);

                /* Remove the item and unref our reference */
                priv->books = g_list_delete_link (priv->books, li);
                g_object_unref (book);
        }
}

static void
book_manager_book_updated_cb (DhBook   *book,
                              gpointer  user_data)
{
        DhBookManager *book_manager = user_data;
        gchar         *book_path;

        /* When we update a book, we need to delete it and then
         * create it again. */
        book_path = g_strdup (dh_book_get_path (book));
        book_manager_book_deleted_cb (book, book_manager);
        book_manager_add_from_filepath (book_manager, book_path);
        g_free (book_path);
}

static GSList *
book_manager_find_book_in_disabled_list (GSList *books_disabled,
                                         DhBook *book)
{
        GSList *li;

        for (li = books_disabled; li; li = g_slist_next (li)) {
                if (g_strcmp0 (dh_book_get_name (book),
                               (const gchar *)li->data) == 0) {
                        return li;
                }
        }
        return NULL;
}

static void
book_manager_book_enabled_cb (DhBook   *book,
                              gpointer  user_data)
{
        DhBookManager     *book_manager = user_data;
        DhBookManagerPriv *priv = GET_PRIVATE (book_manager);
        GSList            *li;

        li = book_manager_find_book_in_disabled_list (priv->books_disabled,
                                                      book);
        /* When setting as enabled a given book, we should have it in the
         * disabled books list! */
        g_assert (li != NULL);
        priv->books_disabled = g_slist_delete_link (priv->books_disabled, li);
        dh_util_state_store_books_disabled (priv->books_disabled);

        g_signal_emit (book_manager,
                       signals[BOOK_ENABLED],
                       0,
                       book);
}

static void
book_manager_book_disabled_cb (DhBook   *book,
                               gpointer  user_data)
{
        DhBookManager     *book_manager = user_data;
        DhBookManagerPriv *priv = GET_PRIVATE (book_manager);
        GSList            *li;

        li = book_manager_find_book_in_disabled_list (priv->books_disabled,
                                                      book);
        /* When setting as disabled a given book, we shouldn't have it in the
         * disabled books list! */
        g_assert (li == NULL);
        priv->books_disabled = g_slist_append (priv->books_disabled,
                                               g_strdup (dh_book_get_name (book)));
        dh_util_state_store_books_disabled (priv->books_disabled);

        g_signal_emit (book_manager,
                       signals[BOOK_DISABLED],
                       0,
                       book);
}

static void
book_manager_add_from_filepath (DhBookManager *book_manager,
                                const gchar   *book_path)
{
        DhBookManagerPriv *priv;
        DhBook            *book;

        g_return_if_fail (book_manager);
        g_return_if_fail (book_path);

        priv = GET_PRIVATE (book_manager);

        /* Allocate new book struct */
        book = dh_book_new (book_path);

        /* Check if book with same path was already loaded in the manager */
        if (g_list_find_custom (priv->books,
                                book,
                                (GCompareFunc)dh_book_cmp_by_path)) {
                g_object_unref (book);
                return;
        }

        /* Check if book with same bookname was already loaded in the manager
         * (we need to force unique book names) */
        if (g_list_find_custom (priv->books,
                                book,
                                (GCompareFunc)dh_book_cmp_by_name)) {
                g_object_unref (book);
                return;
        }

        /* Add the book to the book list */
        priv->books = g_list_insert_sorted (priv->books,
                                            book,
                                            (GCompareFunc)dh_book_cmp_by_title);

        g_debug ("Adding book '%s' to the book manager list",
                 dh_book_get_title (book));

        /* Set the proper enabled/disabled state, depending on conf */
        dh_book_set_enabled (book,
                             !book_manager_is_book_disabled_in_conf (book_manager,
                                                                     book));

        /* Get notifications of book being deleted or updated */
        g_signal_connect (book,
                          "deleted",
                          G_CALLBACK (book_manager_book_deleted_cb),
                          book_manager);
        g_signal_connect (book,
                          "updated",
                          G_CALLBACK (book_manager_book_updated_cb),
                          book_manager);
        g_signal_connect (book,
                          "enabled",
                          G_CALLBACK (book_manager_book_enabled_cb),
                          book_manager);
        g_signal_connect (book,
                          "disabled",
                          G_CALLBACK (book_manager_book_disabled_cb),
                          book_manager);

        /* Emit signal to notify others */
        g_signal_emit (book_manager,
                       signals[BOOK_CREATED],
                       0,
                       book);
}

GList *
dh_book_manager_get_books (DhBookManager *book_manager)
{
        g_return_val_if_fail (book_manager, NULL);

        return GET_PRIVATE (book_manager)->books;
}

DhBook *
dh_book_manager_get_book_by_name (DhBookManager *book_manager,
                                  const gchar   *name)
{
        GList  *l;

        g_return_val_if_fail (book_manager, NULL);

        l = g_list_find_custom (GET_PRIVATE (book_manager)->books,
                                name,
                                (GCompareFunc)dh_book_cmp_by_name_str);

        return l ? l->data : NULL;
}

DhBook *
dh_book_manager_get_book_by_path (DhBookManager *book_manager,
                                  const gchar   *path)
{
        GList  *l;

        g_return_val_if_fail (book_manager, NULL);

        l = g_list_find_custom (GET_PRIVATE (book_manager)->books,
                                path,
                                (GCompareFunc)dh_book_cmp_by_path_str);

        return l ? l->data : NULL;
}

DhBookManager *
dh_book_manager_new (void)
{
        return g_object_new (DH_TYPE_BOOK_MANAGER, NULL);
}

