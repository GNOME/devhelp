/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dh-book-list-directory.h"
#include "dh-util-lib.h"

/**
 * SECTION:dh-book-list-directory
 * @Title: DhBookListDirectory
 * @Short_description: Subclass of #DhBookList containing the #DhBook's in one
 *   directory
 *
 * #DhBookListDirectory is a subclass of #DhBookList containing the #DhBook's in
 * #DhBookListDirectory:directory. In that directory, each book must be in a
 * direct sub-directory, with the Devhelp index file as a direct child of that
 * sub-directory.
 *
 * For example if #DhBookListDirectory:directory is "/usr/share/gtk-doc/html/",
 * and if there is an index file at
 * "/usr/share/gtk-doc/html/glib/glib.devhelp2", #DhBookListDirectory will
 * contain a #DhBook for that index file.
 *
 * Additionally the name of (1) the sub-directory and (2) the index file minus
 * its extension, must match ("glib" in the example).
 *
 * See #DhBook for the list of allowed Devhelp index file extensions
 * ("*.devhelp2" in the example).
 *
 * #DhBookListDirectory listens to the #DhBook #DhBook::deleted and
 * #DhBook::updated signals, to remove the #DhBook or to re-create it. And
 * #DhBookListDirectory contains a #GFileMonitor on the
 * #DhBookListDirectory:directory to add new #DhBook's when they are installed.
 * But note that those #GFileMonitor's are not guaranteed to work perfectly,
 * recreating the #DhBookListDirectory (or restarting the application) may be
 * needed to see all the index files after filesystem changes in
 * #DhBookListDirectory:directory.
 */

#define NEW_POSSIBLE_BOOK_TIMEOUT_SECS 5

typedef struct {
        DhBookListDirectory *list_directory; /* unowned */
        GFile *book_directory;
        guint timeout_id;
} NewPossibleBookData;

struct _DhBookListDirectoryPrivate {
        GFile *directory;
        GFileMonitor *directory_monitor;

        /* List of NewPossibleBookData* */
        GSList *new_possible_books_data;
};

enum {
        PROP_0,
        PROP_DIRECTORY,
        N_PROPERTIES
};

/* List of unowned DhBookListDirectory*. */
static GList *instances;

static GParamSpec *properties[N_PROPERTIES];

G_DEFINE_TYPE_WITH_PRIVATE (DhBookListDirectory, dh_book_list_directory, DH_TYPE_BOOK_LIST)

/* Prototypes */
static gboolean create_book_from_index_file (DhBookListDirectory *list_directory,
                                             GFile               *index_file);

static NewPossibleBookData *
new_possible_book_data_new (DhBookListDirectory *list_directory,
                            GFile               *book_directory)
{
        NewPossibleBookData *data;

        data = g_new0 (NewPossibleBookData, 1);
        data->list_directory = list_directory;
        data->book_directory = g_object_ref (book_directory);

        return data;
}

static void
new_possible_book_data_free (gpointer _data)
{
        NewPossibleBookData *data = _data;

        if (data == NULL)
                return;

        g_clear_object (&data->book_directory);

        if (data->timeout_id != 0)
                g_source_remove (data->timeout_id);

        g_free (data);
}

static void
book_deleted_cb (DhBook              *book,
                 DhBookListDirectory *list_directory)
{
        dh_book_list_remove_book (DH_BOOK_LIST (list_directory), book);
}

static void
book_updated_cb (DhBook              *book,
                 DhBookListDirectory *list_directory)
{
        GFile *index_file;

        /* Re-create the DhBook to parse again the index file. */

        index_file = dh_book_get_index_file (book);
        g_object_ref (index_file);

        dh_book_list_remove_book (DH_BOOK_LIST (list_directory), book);

        create_book_from_index_file (list_directory, index_file);
        g_object_unref (index_file);
}

/* Returns TRUE if "successful", FALSE if the next possible index file in the
 * book directory needs to be tried.
 */
static gboolean
create_book_from_index_file (DhBookListDirectory *list_directory,
                             GFile               *index_file)
{
        GList *books;
        GList *l;
        DhBook *book;

        books = dh_book_list_get_books (DH_BOOK_LIST (list_directory));

        /* Check if a DhBook at the same location has already been loaded. */
        for (l = books; l != NULL; l = l->next) {
                DhBook *cur_book = DH_BOOK (l->data);
                GFile *cur_index_file;

                cur_index_file = dh_book_get_index_file (cur_book);

                if (g_file_equal (index_file, cur_index_file))
                        return TRUE;
        }

        book = dh_book_new (index_file);
        if (book == NULL)
                return FALSE;

        /* Check if book with same ID was already loaded (we need to force
         * unique book IDs).
         */
        if (g_list_find_custom (books, book, (GCompareFunc)dh_book_cmp_by_id) != NULL) {
                g_object_unref (book);
                return TRUE;
        }

        g_signal_connect_object (book,
                                 "deleted",
                                 G_CALLBACK (book_deleted_cb),
                                 list_directory,
                                 0);

        g_signal_connect_object (book,
                                 "updated",
                                 G_CALLBACK (book_updated_cb),
                                 list_directory,
                                 0);

        dh_book_list_add_book (DH_BOOK_LIST (list_directory), book);
        g_object_unref (book);

        return TRUE;
}

/* @book_directory is a directory containing a single book, with the index file
 * as a direct child.
 */
static void
create_book_from_book_directory (DhBookListDirectory *list_directory,
                                 GFile               *book_directory)
{
        GSList *possible_index_files;
        GSList *l;

        possible_index_files = _dh_util_get_possible_index_files (book_directory);

        for (l = possible_index_files; l != NULL; l = l->next) {
                GFile *index_file = G_FILE (l->data);

                if (create_book_from_index_file (list_directory, index_file))
                        break;
        }

        g_slist_free_full (possible_index_files, g_object_unref);
}

static gboolean
new_possible_book_timeout_cb (gpointer user_data)
{
        NewPossibleBookData *data = user_data;
        DhBookListDirectoryPrivate *priv = data->list_directory->priv;

        data->timeout_id = 0;

        create_book_from_book_directory (data->list_directory, data->book_directory);

        priv->new_possible_books_data = g_slist_remove (priv->new_possible_books_data, data);
        new_possible_book_data_free (data);

        return G_SOURCE_REMOVE;
}

static void
books_directory_changed_cb (GFileMonitor        *directory_monitor,
                            GFile               *file,
                            GFile               *other_file,
                            GFileMonitorEvent    event_type,
                            DhBookListDirectory *list_directory)
{
        DhBookListDirectoryPrivate *priv = list_directory->priv;
        NewPossibleBookData *data;

        /* With the GFileMonitor here we only handle events for new directories
         * created. Book deletions and updates are handled by the GFileMonitor
         * in each DhBook object.
         */
        if (event_type != G_FILE_MONITOR_EVENT_CREATED)
                return;

        data = new_possible_book_data_new (list_directory, file);

        /* We add a timeout of several seconds so that we give time to the whole
         * documentation to get installed. If we don't do this, we may end up
         * trying to add the new book when even the *.devhelp2 index file is not
         * installed yet.
         */
        data->timeout_id = g_timeout_add_seconds (NEW_POSSIBLE_BOOK_TIMEOUT_SECS,
                                                  new_possible_book_timeout_cb,
                                                  data);

        priv->new_possible_books_data = g_slist_prepend (priv->new_possible_books_data, data);
}

static void
monitor_books_directory (DhBookListDirectory *list_directory)
{
        GError *error = NULL;

        g_assert (list_directory->priv->directory_monitor == NULL);
        list_directory->priv->directory_monitor = g_file_monitor_directory (list_directory->priv->directory,
                                                                            G_FILE_MONITOR_NONE,
                                                                            NULL,
                                                                            &error);

        if (error != NULL) {
                gchar *parse_name;

                parse_name = g_file_get_parse_name (list_directory->priv->directory);

                g_warning ("Failed to create file monitor on directory “%s”: %s",
                           parse_name,
                           error->message);

                g_free (parse_name);
                g_clear_error (&error);
        }

        if (list_directory->priv->directory_monitor != NULL) {
                g_signal_connect_object (list_directory->priv->directory_monitor,
                                         "changed",
                                         G_CALLBACK (books_directory_changed_cb),
                                         list_directory,
                                         0);
        }
}

static void
find_books (DhBookListDirectory *list_directory)
{
        GFileEnumerator *enumerator;
        GError *error = NULL;

        enumerator = g_file_enumerate_children (list_directory->priv->directory,
                                                G_FILE_ATTRIBUTE_STANDARD_NAME,
                                                G_FILE_QUERY_INFO_NONE,
                                                NULL,
                                                &error);

        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
                g_clear_error (&error);
                goto out;
        }

        if (error != NULL) {
                gchar *parse_name;

                parse_name = g_file_get_parse_name (list_directory->priv->directory);

                g_warning ("Error when reading directory '%s': %s",
                           parse_name,
                           error->message);

                g_free (parse_name);
                g_clear_error (&error);
                goto out;
        }

        monitor_books_directory (list_directory);

        while (TRUE) {
                GFile *book_directory = NULL;

                g_file_enumerator_iterate (enumerator, NULL, &book_directory, NULL, &error);

                if (error != NULL) {
                        gchar *parse_name;

                        parse_name = g_file_get_parse_name (list_directory->priv->directory);

                        g_warning ("Error when enumerating directory '%s': %s",
                                   parse_name,
                                   error->message);

                        g_free (parse_name);
                        g_clear_error (&error);
                        break;
                }

                if (book_directory == NULL)
                        break;

                create_book_from_book_directory (list_directory, book_directory);
        }

out:
        g_clear_object (&enumerator);
}

static void
set_directory (DhBookListDirectory *list_directory,
               GFile               *directory)
{
        g_assert (list_directory->priv->directory == NULL);
        g_return_if_fail (G_IS_FILE (directory));

        list_directory->priv->directory = g_object_ref (directory);
        find_books (list_directory);
}

static void
dh_book_list_directory_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
        DhBookListDirectory *list_directory = DH_BOOK_LIST_DIRECTORY (object);

        switch (prop_id) {
                case PROP_DIRECTORY:
                        g_value_set_object (value, dh_book_list_directory_get_directory (list_directory));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_book_list_directory_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
        DhBookListDirectory *list_directory = DH_BOOK_LIST_DIRECTORY (object);

        switch (prop_id) {
                case PROP_DIRECTORY:
                        set_directory (list_directory, g_value_get_object (value));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_book_list_directory_dispose (GObject *object)
{
        DhBookListDirectory *list_directory = DH_BOOK_LIST_DIRECTORY (object);

        g_clear_object (&list_directory->priv->directory);
        g_clear_object (&list_directory->priv->directory_monitor);

        g_slist_free_full (list_directory->priv->new_possible_books_data, new_possible_book_data_free);
        list_directory->priv->new_possible_books_data = NULL;

        G_OBJECT_CLASS (dh_book_list_directory_parent_class)->dispose (object);
}

static void
dh_book_list_directory_finalize (GObject *object)
{
        DhBookListDirectory *list_directory = DH_BOOK_LIST_DIRECTORY (object);

        instances = g_list_remove (instances, list_directory);

        G_OBJECT_CLASS (dh_book_list_directory_parent_class)->finalize (object);
}

static void
dh_book_list_directory_class_init (DhBookListDirectoryClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = dh_book_list_directory_get_property;
        object_class->set_property = dh_book_list_directory_set_property;
        object_class->dispose = dh_book_list_directory_dispose;
        object_class->finalize = dh_book_list_directory_finalize;

        /**
         * DhBookListDirectory:directory:
         *
         * The directory, as a #GFile, containing a set of Devhelp books.
         *
         * Since: 3.30
         */
        properties[PROP_DIRECTORY] =
                g_param_spec_object ("directory",
                                     "Directory",
                                     "",
                                     G_TYPE_FILE,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
dh_book_list_directory_init (DhBookListDirectory *list_directory)
{
        list_directory->priv = dh_book_list_directory_get_instance_private (list_directory);

        instances = g_list_prepend (instances, list_directory);
}

/**
 * dh_book_list_directory_new:
 * @directory: the #DhBookListDirectory:directory.
 *
 * Returns a #DhBookListDirectory for @directory.
 *
 * If a #DhBookListDirectory instance is still alive for @directory (according
 * to g_file_equal()), the same instance is returned with the reference count
 * increased by one, to avoid data duplication. If no #DhBookListDirectory
 * instance already exists for @directory, this function returns a new instance
 * with a reference count of one (so it's the responsibility of the caller to
 * keep the object alive if wanted, to avoid destroying and re-creating the same
 * #DhBookListDirectory repeatedly).
 *
 * Returns: (transfer full): a #DhBookListDirectory for @directory.
 * Since: 3.30
 */
DhBookListDirectory *
dh_book_list_directory_new (GFile *directory)
{
        GList *l;

        g_return_val_if_fail (G_IS_FILE (directory), NULL);

        for (l = instances; l != NULL; l = l->next) {
                DhBookListDirectory *cur_list_directory = DH_BOOK_LIST_DIRECTORY (l->data);

                if (cur_list_directory->priv->directory != NULL &&
                    g_file_equal (cur_list_directory->priv->directory, directory))
                        return g_object_ref (cur_list_directory);
        }

        return g_object_new (DH_TYPE_BOOK_LIST_DIRECTORY,
                             "directory", directory,
                             NULL);
}

/**
 * dh_book_list_directory_get_directory:
 * @list_directory: a #DhBookListDirectory.
 *
 * Returns: (transfer none): the #DhBookListDirectory:directory.
 * Since: 3.30
 */
GFile *
dh_book_list_directory_get_directory (DhBookListDirectory *list_directory)
{
        g_return_val_if_fail (DH_IS_BOOK_LIST_DIRECTORY (list_directory), NULL);

        return list_directory->priv->directory;
}
