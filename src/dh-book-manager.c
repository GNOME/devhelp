/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
 * Copyright (C) 2012 Thomas Bechtold <toabctl@gnome.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "dh-book-manager.h"
#include "dh-book.h"
#include "dh-language.h"
#include "dh-link.h"
#include "dh-settings.h"
#include "dh-util.h"

/**
 * SECTION:dh-book-manager
 * @Title: DhBookManager
 * @Short_description: Aggregation of all #DhBook's
 *
 * #DhBookManager is a singleton class containing all the #DhBook's.
 */

#define NEW_POSSIBLE_BOOK_TIMEOUT_SECS 5

typedef struct {
        DhBookManager *book_manager;
        GFile *file;
} NewPossibleBookData;

typedef struct {
        /* The list of all DhBooks* found in the system */
        GList *books;

        /* GFile* -> GFileMonitor* */
        GHashTable *monitors;

        /* List of book IDs (gchar*) currently disabled */
        GSList *books_disabled;

        /* List of DhLanguage* with at least one book enabled */
        GList *languages;

        guint group_by_language : 1;
} DhBookManagerPrivate;

enum {
        BOOK_CREATED,
        BOOK_DELETED,
        BOOK_ENABLED,
        BOOK_DISABLED,
        LANGUAGE_ENABLED,
        LANGUAGE_DISABLED,
        N_SIGNALS
};

enum {
        PROP_0,
        PROP_GROUP_BY_LANGUAGE
};

static guint signals[N_SIGNALS] = { 0 };

static DhBookManager *singleton = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (DhBookManager, dh_book_manager, G_TYPE_OBJECT);

static gboolean create_book_from_index_file (DhBookManager *book_manager,
                                             GFile         *index_file);

static void
dh_book_manager_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
        DhBookManager *book_manager = DH_BOOK_MANAGER (object);

        switch (prop_id)
        {
        case PROP_GROUP_BY_LANGUAGE:
                g_value_set_boolean (value, dh_book_manager_get_group_by_language (book_manager));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
dh_book_manager_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
        DhBookManager *book_manager = DH_BOOK_MANAGER (object);

        switch (prop_id)
        {
        case PROP_GROUP_BY_LANGUAGE:
                dh_book_manager_set_group_by_language (book_manager, g_value_get_boolean (value));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
dh_book_manager_dispose (GObject *object)
{
        DhBookManagerPrivate *priv;

        priv = dh_book_manager_get_instance_private (DH_BOOK_MANAGER (object));

        g_list_free_full (priv->books, g_object_unref);
        priv->books = NULL;

        if (priv->monitors != NULL) {
                g_hash_table_destroy (priv->monitors);
                priv->monitors = NULL;
        }

        G_OBJECT_CLASS (dh_book_manager_parent_class)->dispose (object);
}

static void
dh_book_manager_finalize (GObject *object)
{
        DhBookManager *book_manager = DH_BOOK_MANAGER (object);
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);

        g_list_free_full (priv->languages, g_object_unref);
        g_slist_free_full (priv->books_disabled, g_free);

        if (singleton == book_manager)
                singleton = NULL;

        G_OBJECT_CLASS (dh_book_manager_parent_class)->finalize (object);
}

static void
dh_book_manager_class_init (DhBookManagerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = dh_book_manager_get_property;
        object_class->set_property = dh_book_manager_set_property;
        object_class->dispose = dh_book_manager_dispose;
        object_class->finalize = dh_book_manager_finalize;

        /**
         * DhBookManager::book-created:
         * @book_manager: the #DhBookManager.
         * @book: the created #DhBook.
         */
        signals[BOOK_CREATED] =
                g_signal_new ("book-created",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1,
                              DH_TYPE_BOOK);

        /**
         * DhBookManager::book-deleted:
         * @book_manager: the #DhBookManager.
         * @book: the deleted #DhBook.
         */
        signals[BOOK_DELETED] =
                g_signal_new ("book-deleted",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1,
                              DH_TYPE_BOOK);

        /**
         * DhBookManager::book-enabled:
         * @book_manager: the #DhBookManager.
         * @book: the enabled #DhBook.
         */
        signals[BOOK_ENABLED] =
                g_signal_new ("book-enabled",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1,
                              DH_TYPE_BOOK);

        /**
         * DhBookManager::book-disabled:
         * @book_manager: the #DhBookManager.
         * @book: the disabled #DhBook.
         */
        signals[BOOK_DISABLED] =
                g_signal_new ("book-disabled",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1,
                              DH_TYPE_BOOK);

        /**
         * DhBookManager::language-enabled:
         * @book_manager: the #DhBookManager.
         * @language_name: the enabled programming language name.
         */
        signals[LANGUAGE_ENABLED] =
                g_signal_new ("language-enabled",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);

        /**
         * DhBookManager::language-disabled:
         * @book_manager: the #DhBookManager.
         * @language_name: the disabled programming language name.
         */
        signals[LANGUAGE_DISABLED] =
                g_signal_new ("language-disabled",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);

        /**
         * DhBookManager:group-by-language:
         *
         * Whether books should be grouped by programming language.
         */
        g_object_class_install_property (object_class,
                                         PROP_GROUP_BY_LANGUAGE,
                                         g_param_spec_boolean ("group-by-language",
                                                               "Group by language",
                                                               "",
                                                               FALSE,
                                                               G_PARAM_READWRITE |
                                                               G_PARAM_STATIC_STRINGS));
}

static void
load_books_disabled (DhBookManager *book_manager)
{
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        DhSettings *settings;
        GSettings *contents_settings;
        gchar **books_disabled_strv;
        gint i;

        g_assert (priv->books_disabled == NULL);

        settings = dh_settings_get_singleton ();
        contents_settings = dh_settings_peek_contents_settings (settings);
        books_disabled_strv = g_settings_get_strv (contents_settings, "books-disabled");

        if (books_disabled_strv == NULL)
                return;

        for (i = 0; books_disabled_strv[i] != NULL; i++) {
                gchar *book_id = books_disabled_strv[i];
                priv->books_disabled = g_slist_prepend (priv->books_disabled, book_id);
        }

        priv->books_disabled = g_slist_reverse (priv->books_disabled);

        g_free (books_disabled_strv);
}

static void
store_books_disabled (DhBookManager *book_manager)
{
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        DhSettings *settings;
        GSettings *contents_settings;
        GVariantBuilder *builder;
        GVariant *variant;
        GSList *l;

        builder = g_variant_builder_new (G_VARIANT_TYPE_STRING_ARRAY);

        for (l = priv->books_disabled; l != NULL; l = l->next) {
                const gchar *book_id = l->data;
                g_variant_builder_add (builder, "s", book_id);
        }

        variant = g_variant_builder_end (builder);
        g_variant_builder_unref (builder);

        settings = dh_settings_get_singleton ();
        contents_settings = dh_settings_peek_contents_settings (settings);
        g_settings_set_value (contents_settings, "books-disabled", variant);
}

static void
inc_language (DhBookManager *book_manager,
              const gchar   *language_name)
{
        GList *li;
        DhLanguage *language;
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);

        li = g_list_find_custom (priv->languages,
                                 language_name,
                                 (GCompareFunc)dh_language_compare_by_name);

        /* If already in list, increase count */
        if (li) {
                dh_language_inc_n_books_enabled (li->data);
                return;
        }

        /* Add new element to list if not found. Language must start with
         * with n_books_enabled=1. */
        language = dh_language_new (language_name);
        dh_language_inc_n_books_enabled (language);
        priv->languages = g_list_prepend (priv->languages,
                                          language);
        /* Emit signal to notify others */
        g_signal_emit (book_manager,
                       signals[LANGUAGE_ENABLED],
                       0,
                       language_name);
}

static void
dec_language (DhBookManager *book_manager,
              const gchar   *language_name)
{
        GList *li;
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);

        /* Language must exist in list */
        li = g_list_find_custom (priv->languages,
                                 language_name,
                                 (GCompareFunc)dh_language_compare_by_name);
        g_assert (li != NULL);

        /* If language count reaches zero, remove from list */
        if (dh_language_dec_n_books_enabled (li->data)) {
                g_object_unref (li->data);
                priv->languages = g_list_delete_link (priv->languages, li);

                /* Emit signal to notify others */
                g_signal_emit (book_manager,
                               signals[LANGUAGE_DISABLED],
                               0,
                               language_name);
        }
}

static gboolean
is_book_disabled_in_conf (DhBookManager *book_manager,
                          DhBook        *book)
{
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        const gchar *book_id;
        GSList *l;

        book_id = dh_book_get_id (book);

        for (l = priv->books_disabled; l != NULL; l = l->next) {
                gchar *cur_book_id = l->data;

                if (g_strcmp0 (book_id, cur_book_id) == 0)
                        return TRUE;
        }

        return FALSE;
}

static void
book_deleted_cb (DhBook   *book,
                 gpointer  user_data)
{
        DhBookManager *book_manager = user_data;
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        GList *li;

        /* Look for the item we want to remove */
        li = g_list_find (priv->books, book);
        if (li) {
                /* Decrement language count */
                dec_language (book_manager, dh_book_get_language (book));

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
book_updated_cb (DhBook   *book,
                 gpointer  user_data)
{
        DhBookManager *book_manager = user_data;
        GFile *index_file;

        /* When we update a book, we need to delete it and then create it again. */

        index_file = dh_book_get_index_file (book);
        g_object_ref (index_file);

        book_deleted_cb (book, book_manager);

        create_book_from_index_file (book_manager, index_file);
        g_object_unref (index_file);
}

static GSList *
find_book_in_disabled_list (GSList *books_disabled,
                            DhBook *book)
{
        GSList *li;

        for (li = books_disabled; li; li = g_slist_next (li)) {
                if (g_strcmp0 (dh_book_get_id (book),
                               (const gchar *)li->data) == 0) {
                        return li;
                }
        }

        return NULL;
}

static void
book_enabled_cb (DhBook   *book,
                 gpointer  user_data)
{
        DhBookManager *book_manager = user_data;
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        GSList *li;

        li = find_book_in_disabled_list (priv->books_disabled, book);
        /* When setting as enabled a given book, we should have it in the
         * disabled books list! */
        g_assert (li != NULL);
        priv->books_disabled = g_slist_delete_link (priv->books_disabled, li);
        store_books_disabled (book_manager);

        /* Increment language count */
        inc_language (book_manager, dh_book_get_language (book));

        g_signal_emit (book_manager,
                       signals[BOOK_ENABLED],
                       0,
                       book);
}

static void
book_disabled_cb (DhBook   *book,
                  gpointer  user_data)
{
        DhBookManager *book_manager = user_data;
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        GSList *li;

        li = find_book_in_disabled_list (priv->books_disabled, book);
        /* When setting as disabled a given book, we shouldn't have it in the
         * disabled books list! */
        g_assert (li == NULL);
        priv->books_disabled = g_slist_append (priv->books_disabled,
                                               g_strdup (dh_book_get_id (book)));
        store_books_disabled (book_manager);

        /* Decrement language count */
        dec_language (book_manager, dh_book_get_language (book));

        g_signal_emit (book_manager,
                       signals[BOOK_DISABLED],
                       0,
                       book);
}

/* Returns TRUE if "successful", FALSE if the next possible index file in the
 * book directory needs to be tried.
 */
static gboolean
create_book_from_index_file (DhBookManager *book_manager,
                             GFile         *index_file)
{
        DhBookManagerPrivate *priv;
        DhBook *book;
        gboolean book_enabled;
        GList *l;

        priv = dh_book_manager_get_instance_private (book_manager);

        /* Check if a DhBook at the same location has already been loaded. */
        for (l = priv->books; l != NULL; l = l->next) {
                DhBook *cur_book = DH_BOOK (l->data);
                GFile *cur_index_file;

                cur_index_file = dh_book_get_index_file (cur_book);

                if (g_file_equal (index_file, cur_index_file))
                        return TRUE;
        }

        book = dh_book_new (index_file);
        if (book == NULL)
                return FALSE;

        /* Check if book with same ID was already loaded in the manager (we need
         * to force unique book IDs).
         */
        if (g_list_find_custom (priv->books,
                                book,
                                (GCompareFunc)dh_book_cmp_by_id)) {
                g_object_unref (book);
                return TRUE;
        }

        priv->books = g_list_insert_sorted (priv->books,
                                            book,
                                            (GCompareFunc)dh_book_cmp_by_title);

        book_enabled = !is_book_disabled_in_conf (book_manager, book);
        dh_book_set_enabled (book, book_enabled);

        if (book_enabled)
                inc_language (book_manager, dh_book_get_language (book));

        g_signal_connect_object (book,
                                 "deleted",
                                 G_CALLBACK (book_deleted_cb),
                                 book_manager,
                                 0);

        g_signal_connect_object (book,
                                 "updated",
                                 G_CALLBACK (book_updated_cb),
                                 book_manager,
                                 0);

        g_signal_connect_object (book,
                                 "enabled",
                                 G_CALLBACK (book_enabled_cb),
                                 book_manager,
                                 0);

        g_signal_connect_object (book,
                                 "disabled",
                                 G_CALLBACK (book_disabled_cb),
                                 book_manager,
                                 0);

        g_signal_emit (book_manager,
                       signals[BOOK_CREATED],
                       0,
                       book);

        return TRUE;
}

static void
create_book_from_directory (DhBookManager *book_manager,
                            GFile         *book_directory)
{
        GSList *possible_index_files;
        GSList *l;

        possible_index_files = dh_util_get_possible_index_files (book_directory);

        for (l = possible_index_files; l != NULL; l = l->next) {
                GFile *index_file = G_FILE (l->data);

                if (create_book_from_index_file (book_manager, index_file))
                        break;
        }

        g_slist_free_full (possible_index_files, g_object_unref);
}

static gboolean
new_possible_book_cb (gpointer user_data)
{
        NewPossibleBookData *data = user_data;

        create_book_from_directory (data->book_manager, data->file);

        g_object_unref (data->book_manager);
        g_object_unref (data->file);
        g_slice_free (NewPossibleBookData, data);
        return G_SOURCE_REMOVE;
}

static void
booklist_monitor_event_cb (GFileMonitor      *file_monitor,
                           GFile             *file,
                           GFile             *other_file,
                           GFileMonitorEvent  event_type,
                           gpointer           user_data)
{
        DhBookManager *book_manager = user_data;
        NewPossibleBookData *data;

        /* In the book manager we only handle events for new directories
         * created. Books removed or updated are handled by the book objects
         * themselves */
        if (event_type != G_FILE_MONITOR_EVENT_CREATED) {
                return;
        }

        data = g_slice_new (NewPossibleBookData);
        data->book_manager = g_object_ref (book_manager);
        data->file = g_object_ref (file);

        /* We add a timeout of several seconds so that we give time to the
         * whole documentation to get installed. If we don't do this, we may
         * end up trying to add the new book when even the .devhelp file is
         * not installed yet */
        g_timeout_add_seconds (NEW_POSSIBLE_BOOK_TIMEOUT_SECS,
                               new_possible_book_cb,
                               data);
}

static void
monitor_path (DhBookManager *book_manager,
              const gchar   *path)
{
        DhBookManagerPrivate *priv;
        GFileMonitor *file_monitor;
        GFile *file;

        priv = dh_book_manager_get_instance_private (book_manager);

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
                                  G_CALLBACK (booklist_monitor_event_cb),
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
add_books_in_dir (DhBookManager *book_manager,
                  const gchar   *dir_path)
{
        GFile *directory;
        GFileEnumerator *enumerator;
        GError *error = NULL;

        g_return_if_fail (dir_path != NULL);

        directory = g_file_new_for_path (dir_path);

        enumerator = g_file_enumerate_children (directory,
                                                G_FILE_ATTRIBUTE_STANDARD_NAME,
                                                G_FILE_QUERY_INFO_NONE,
                                                NULL,
                                                &error);

        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
                g_clear_error (&error);
                goto out;
        }

        if (error != NULL) {
                g_warning ("Error when reading directory '%s': %s",
                           dir_path,
                           error->message);
                g_clear_error (&error);
                goto out;
        }

        /* Monitor the directory for changes */
        monitor_path (book_manager, dir_path);

        while (TRUE) {
                GFile *book_directory = NULL;

                g_file_enumerator_iterate (enumerator, NULL, &book_directory, NULL, &error);

                if (error != NULL) {
                        g_warning ("Error when enumerating directory '%s': %s",
                                   dir_path,
                                   error->message);
                        g_clear_error (&error);
                        break;
                }

                if (book_directory == NULL)
                        break;

                create_book_from_directory (book_manager, book_directory);
        }

out:
        g_object_unref (directory);
        g_clear_object (&enumerator);
}

static void
add_books_in_data_dir (DhBookManager *book_manager,
                       const gchar   *data_dir)
{
        gchar *dir;

        g_return_if_fail (data_dir != NULL);

        dir = g_build_filename (data_dir, "gtk-doc", "html", NULL);
        add_books_in_dir (book_manager, dir);
        g_free (dir);

        dir = g_build_filename (data_dir, "devhelp", "books", NULL);
        add_books_in_dir (book_manager, dir);
        g_free (dir);
}

static void
populate (DhBookManager *book_manager)
{
        const gchar * const *system_dirs;
        gint i;

        add_books_in_data_dir (book_manager, g_get_user_data_dir ());

        system_dirs = g_get_system_data_dirs ();
        g_return_if_fail (system_dirs != NULL);

        for (i = 0; system_dirs[i] != NULL; i++) {
                add_books_in_data_dir (book_manager, system_dirs[i]);
        }
}

static void
dh_book_manager_init (DhBookManager *book_manager)
{
        DhSettings *settings;
        GSettings *contents_settings;

        load_books_disabled (book_manager);

        settings = dh_settings_get_singleton ();
        contents_settings = dh_settings_peek_contents_settings (settings);
        g_settings_bind (contents_settings, "group-books-by-language",
                         book_manager, "group-by-language",
                         G_SETTINGS_BIND_DEFAULT);

        populate (book_manager);
}

/**
 * dh_book_manager_new:
 *
 * Returns: (transfer full): the #DhBookManager singleton instance. You need to
 * unref it when no longer needed.
 * Deprecated: 3.26: Call dh_book_manager_get_singleton() instead.
 */
DhBookManager *
dh_book_manager_new (void)
{
        return g_object_ref (dh_book_manager_get_singleton ());
}

/**
 * dh_book_manager_get_singleton:
 *
 * Returns: (transfer none): the #DhBookManager singleton instance.
 * Since: 3.26
 */
DhBookManager *
dh_book_manager_get_singleton (void)
{
        if (singleton == NULL)
                singleton = g_object_new (DH_TYPE_BOOK_MANAGER, NULL);

        return singleton;
}

void
_dh_book_manager_unref_singleton (void)
{
        if (singleton != NULL)
                g_object_unref (singleton);

        /* singleton is not set to NULL here, it is set to NULL in
         * dh_book_manager_finalize() (i.e. when we are sure that the ref count
         * reaches 0).
         */
}

/**
 * dh_book_manager_populate:
 * @book_manager: a #DhBookManager.
 *
 * Populates the #DhBookManager with all books found on the system and user
 * directories.
 *
 * Deprecated: 3.26: The #DhBookManager is now automatically populated when the
 * object is created, there is no need to call this function anymore.
 */
void
dh_book_manager_populate (DhBookManager *book_manager)
{
}

/**
 * dh_book_manager_get_books:
 * @book_manager: a #DhBookManager.
 *
 * Returns: (element-type DhBook) (transfer none): the list of all #DhBook's
 * found.
 */
GList *
dh_book_manager_get_books (DhBookManager *book_manager)
{
        DhBookManagerPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK_MANAGER (book_manager), NULL);

        priv = dh_book_manager_get_instance_private (book_manager);

        return priv->books;
}

/**
 * dh_book_manager_get_group_by_language:
 * @book_manager: a #DhBookManager.
 *
 * Returns: whether the books should be grouped by programming language.
 */
gboolean
dh_book_manager_get_group_by_language (DhBookManager *book_manager)
{
        DhBookManagerPrivate *priv;

        g_return_val_if_fail (DH_IS_BOOK_MANAGER (book_manager), FALSE);

        priv = dh_book_manager_get_instance_private (book_manager);

        return priv->group_by_language;
}

/**
 * dh_book_manager_set_group_by_language:
 * @book_manager: a #DhBookManager.
 * @group_by_language: the new value.
 *
 * Sets whether the books should be grouped by programming language.
 */
void
dh_book_manager_set_group_by_language (DhBookManager *book_manager,
                                       gboolean       group_by_language)
{
        DhBookManagerPrivate *priv;

        g_return_if_fail (DH_IS_BOOK_MANAGER (book_manager));

        priv = dh_book_manager_get_instance_private (book_manager);

        group_by_language = group_by_language != FALSE;

        if (priv->group_by_language != group_by_language) {
                priv->group_by_language = group_by_language;
                g_object_notify (G_OBJECT (book_manager), "group-by-language");
        }
}
