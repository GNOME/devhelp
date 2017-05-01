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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "dh-book-manager.h"
#include "dh-link.h"
#include "dh-book.h"
#include "dh-language.h"
#include "dh-settings.h"

#define NEW_POSSIBLE_BOOK_TIMEOUT_SECS 5

typedef struct {
        DhBookManager *book_manager;
        GFile         *file;
} NewPossibleBookData;

typedef struct {
        /* The list of all DhBooks* found in the system */
        GList      *books;

        /* GFile* -> GFileMonitor* */
        GHashTable *monitors;

        /* List of book names (gchar*) currently disabled */
        GSList     *books_disabled;

        /* List of DhLanguage* with at least one book enabled */
        GList      *languages;

        /* Whether books should be grouped by language */
        guint       group_by_language : 1;
} DhBookManagerPrivate;

enum {
        BOOK_CREATED,
        BOOK_DELETED,
        BOOK_ENABLED,
        BOOK_DISABLED,
        LANGUAGE_ENABLED,
        LANGUAGE_DISABLED,
        LAST_SIGNAL
};

enum {
        PROP_0,
        PROP_GROUP_BY_LANGUAGE
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (DhBookManager, dh_book_manager, G_TYPE_OBJECT);

static void    book_manager_add_from_filepath (DhBookManager *book_manager,
                                               const gchar   *book_path);
static void    book_manager_add_from_dir      (DhBookManager *book_manager,
                                               const gchar   *dir_path);
static void    book_manager_inc_language      (DhBookManager *book_manager,
                                               const gchar   *language_name);
static void    book_manager_dec_language      (DhBookManager *book_manager,
                                               const gchar   *language_name);
static void    populate                       (DhBookManager *book_manager);

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
        DhBookManagerPrivate *priv;

        priv = dh_book_manager_get_instance_private (DH_BOOK_MANAGER (object));

        g_list_free_full (priv->languages, g_object_unref);

        g_slist_free_full (priv->books_disabled, g_free);

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
book_manager_load_books_disabled (DhBookManager *book_manager)
{
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        DhSettings *settings;
        gchar **books_disabled_strv;
        gchar **i;

        settings = dh_settings_get_singleton ();
        books_disabled_strv = g_settings_get_strv (
                dh_settings_peek_contents_settings (settings),
                "books-disabled");

        for (i = books_disabled_strv; *i != NULL; i++) {
                gchar *book = *i;
                priv->books_disabled = g_slist_prepend (priv->books_disabled, book);
        }

        priv->books_disabled = g_slist_reverse (priv->books_disabled);

        g_free (books_disabled_strv);
}

static void
dh_book_manager_init (DhBookManager *book_manager)
{
        DhSettings *settings;

        book_manager_load_books_disabled (book_manager);

        settings = dh_settings_get_singleton ();
        g_settings_bind (dh_settings_peek_contents_settings (settings),
                         "group-books-by-language",
                         book_manager,
                         "group-by-language",
                         G_SETTINGS_BIND_DEFAULT);

        populate (book_manager);
}

static void
book_manager_store_books_disabled (DhBookManager *book_manager)
{
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        DhSettings *settings;
        GVariantBuilder *builder;
        GVariant *variant;
        GSList *l;

        builder = g_variant_builder_new (G_VARIANT_TYPE_STRING_ARRAY);

        for (l = priv->books_disabled; l != NULL; l = l->next) {
                gchar *book = l->data;
                g_variant_builder_add (builder, "s", book);
        }

        variant = g_variant_builder_end (builder);
        g_variant_builder_unref (builder);

        settings = dh_settings_get_singleton ();
        g_settings_set_value (dh_settings_peek_contents_settings (settings),
                              "books-disabled",
                              variant);
}

static gboolean
book_manager_is_book_disabled_in_conf (DhBookManager *book_manager,
                                       DhBook        *book)
{
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        const gchar *name;
        GSList *l;

        name = dh_book_get_name (book);

        for (l = priv->books_disabled; l != NULL; l = l->next) {
                gchar *cur_name = l->data;

                if (g_strcmp0 (name, cur_name) == 0)
                        return TRUE;
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

static void
populate (DhBookManager *book_manager)
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
                        return book_path;
                }
                g_free (book_path);
        }
        return NULL;
}

static gboolean
book_manager_new_possible_book_cb (gpointer user_data)
{
        NewPossibleBookData *data = user_data;
        gchar               *file_path;
        gchar               *file_basename;
        gchar               *book_path;

        file_path = g_file_get_path (data->file);
        file_basename = g_file_get_basename (data->file);

        /* Compute book path, will return NULL if it's not a proper path */
        book_path = book_manager_get_book_path (file_path, file_basename);
        if (book_path) {
                /* Add book from filepath */
                book_manager_add_from_filepath (data->book_manager,
                                                book_path);
                g_free (book_path);
        }

        g_free (file_path);
        g_free (file_basename);
        g_object_unref (data->book_manager);
        g_object_unref (data->file);
        g_slice_free (NewPossibleBookData, data);

        return G_SOURCE_REMOVE;
}

static void
book_manager_booklist_monitor_event_cb (GFileMonitor      *file_monitor,
                                        GFile             *file,
                                        GFile             *other_file,
                                        GFileMonitorEvent  event_type,
                                        gpointer           user_data)
{
        DhBookManager       *book_manager = user_data;
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
                               book_manager_new_possible_book_cb,
                               data);
}

static void
book_manager_monitor_path (DhBookManager *book_manager,
                           const gchar   *path)
{
        GFileMonitor      *file_monitor;
        GFile             *file;
        DhBookManagerPrivate *priv;

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
                book_dir_path = g_build_filename (dir_path,
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

static void
book_manager_book_deleted_cb (DhBook   *book,
                              gpointer  user_data)
{
        DhBookManager     *book_manager = user_data;
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        GList *li;

        /* Look for the item we want to remove */
        li = g_list_find (priv->books, book);
        if (li) {
                /* Decrement language count */
                book_manager_dec_language (book_manager,
                                           dh_book_get_language (book));

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
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        GSList            *li;

        li = book_manager_find_book_in_disabled_list (priv->books_disabled,
                                                      book);
        /* When setting as enabled a given book, we should have it in the
         * disabled books list! */
        g_assert (li != NULL);
        priv->books_disabled = g_slist_delete_link (priv->books_disabled, li);
        book_manager_store_books_disabled (book_manager);

        /* Increment language count */
        book_manager_inc_language (book_manager,
                                   dh_book_get_language (book));

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
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        GSList            *li;

        li = book_manager_find_book_in_disabled_list (priv->books_disabled,
                                                      book);
        /* When setting as disabled a given book, we shouldn't have it in the
         * disabled books list! */
        g_assert (li == NULL);
        priv->books_disabled = g_slist_append (priv->books_disabled,
                                               g_strdup (dh_book_get_name (book)));
        book_manager_store_books_disabled (book_manager);

        /* Decrement language count */
        book_manager_dec_language (book_manager,
                                   dh_book_get_language (book));

        g_signal_emit (book_manager,
                       signals[BOOK_DISABLED],
                       0,
                       book);
}

static void
book_manager_add_from_filepath (DhBookManager *book_manager,
                                const gchar   *book_path)
{
        DhBookManagerPrivate *priv;
        DhBook            *book;
        gboolean           book_enabled;

        g_return_if_fail (book_manager);
        g_return_if_fail (book_path);

        priv = dh_book_manager_get_instance_private (book_manager);

        /* Allocate new book struct */
        book = dh_book_new (book_path);
        if (book == NULL)
                return;

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

        /* Set the proper enabled/disabled state, depending on conf */
        book_enabled = !book_manager_is_book_disabled_in_conf (book_manager, book);
        dh_book_set_enabled (book, book_enabled);

        /* Store language if enabled */
        if (book_enabled) {
                book_manager_inc_language (book_manager,
                                           dh_book_get_language (book));
        }

        /* Get notifications of book being deleted or updated */
        g_signal_connect_object (book,
                                 "deleted",
                                 G_CALLBACK (book_manager_book_deleted_cb),
                                 book_manager,
                                 0);
        g_signal_connect_object (book,
                                 "updated",
                                 G_CALLBACK (book_manager_book_updated_cb),
                                 book_manager,
                                 0);
        g_signal_connect_object (book,
                                 "enabled",
                                 G_CALLBACK (book_manager_book_enabled_cb),
                                 book_manager,
                                 0);
        g_signal_connect_object (book,
                                 "disabled",
                                 G_CALLBACK (book_manager_book_disabled_cb),
                                 book_manager,
                                 0);

        /* Emit signal to notify others */
        g_signal_emit (book_manager,
                       signals[BOOK_CREATED],
                       0,
                       book);
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

        g_return_val_if_fail (book_manager, NULL);

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

        g_return_val_if_fail (book_manager, FALSE);

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

        g_return_if_fail (book_manager);

        priv = dh_book_manager_get_instance_private (book_manager);

        priv->group_by_language = group_by_language;
        g_object_notify (G_OBJECT (book_manager), "group-by-language");
}

static void
book_manager_inc_language (DhBookManager *book_manager,
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
book_manager_dec_language (DhBookManager *book_manager,
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

/**
 * dh_book_manager_new:
 *
 * Returns: a new #DhBookManager object.
 */
DhBookManager *
dh_book_manager_new (void)
{
        return g_object_new (DH_TYPE_BOOK_MANAGER, NULL);
}
