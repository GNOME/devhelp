/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
 * Copyright (C) 2012 Thomas Bechtold <toabctl@gnome.org>
 * Copyright (C) 2017 Sébastien Wilmet <swilmet@gnome.org>
 *
 * Devhelp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Devhelp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Devhelp.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "dh-book-manager.h"
#include "dh-book.h"
#include "dh-settings.h"
#include "dh-util-lib.h"

/**
 * SECTION:dh-book-manager
 * @Title: DhBookManager
 * @Short_description: Aggregation of all #DhBook's
 *
 * #DhBookManager is a singleton class containing all the #DhBook's.
 */

/* TODO: Re-architect DhBookManager and DhBook.
 *
 * DhBookManager and DhBook are not very flexible:
 * 1. Whether a DhBook is enabled or disabled is hard-coded into the DhBook
 * objects. It's bound to the "books-disabled" GSetting.
 *
 * 2. The list of directories where DhBookManager searches the books is more or
 * less hard-coded inside DhBookManager (it's just configurable with XDG env
 * variables, see the populate() function, it's documented in the README). It
 * would be nice to have total control over which directories are searched,
 * without duplicating them if two different "views" have a directory in common
 * (especially not duplicating GFileMonitor's).
 *
 * Ideas:
 * - Create a DhBookSelection class (or set of classes, with maybe an interface
 *   or base class), and remove the "enabled" property from DhBook. The
 *   books-disabled GSetting would be implemented by one implementation of
 *   DhBookSelection. A :book-selection property could be added to some classes,
 *   and if that property is NULL take the "default selection" (by default the
 *   one for the books-disabled GSetting). Another possible name: DhBookFilter
 *   (or have both).
 *
 *   Have ::book-added and ::book-removed signals. A single ::changed signal is
 *   I think not appropriate: for example for DhBookTree, a full repopulate
 *   could be done when ::changed is emitted, but in that case DhBookTree would
 *   loose its selection.
 *
 * - Factor out a DhBookListDirectory class, finding and monitoring a list of
 *   books in one directory. The constructor would roughly be
 *   find_books_in_dir(). DhBookManager would just contain a list of
 *   DhBookListDirectory objects, ensuring that there are no duplicates. A list
 *   of DhBookListDirectory's could be added to a DhBookSelection, and it's
 *   DhBookSelection which applies priorities. So two different DhBookSelection
 *   objects could apply different priorities between the directories.
 *   Ensuring that a book ID is unique would be done by each DhBookListDirectory
 *   object, and also by DhBookSelection; which means that Devhelp would use
 *   more memory since some DhBooks would not be freed since they are contained
 *   in different DhBookListDirectory objects, but the index files anyway needed
 *   to be parsed to know the book ID, so it's not a big issue.
 *
 * Relevant bugzilla tickets:
 * - https://bugzilla.gnome.org/show_bug.cgi?id=784491
 *   "BookManager: allow custom search paths for documentation"
 *
 *   For gnome-builder needs.
 *
 * - https://bugzilla.gnome.org/show_bug.cgi?id=792068
 *   "Make it work with Flatpak"
 *
 *   The directories probably need to be adjusted.
 *
 * - https://bugzilla.gnome.org/show_bug.cgi?id=761284
 *   "Have the latest stable/unstable GNOME API references"
 *
 *   The books can be downloaded in different directories, one directory for
 *   "GNOME stable" and another directory for "GNOME unstable" (or for specific
 *   versions). Switching between versions would just be a matter of changing
 *   the DhBookSelection.
 *
 * - https://bugzilla.gnome.org/show_bug.cgi?id=118423
 *   "Individual bookshelfs"
 *
 * - https://bugzilla.gnome.org/show_bug.cgi?id=764441
 *   "Implement language switching feature"
 *
 *   Basically have the same book ID/name available for different programming
 *   languages. DhBookSelection could filter by programming language. Out of
 *   scope for now, because language switching is implemented in JavaScript for
 *   hot-doc, and gtk-doc doesn't support yet producing documentation for
 *   different programming languages.
 */

#define NEW_POSSIBLE_BOOK_TIMEOUT_SECS 5

typedef struct {
        DhBookManager *book_manager; /* unowned */
        GFile *book_directory;
        guint timeout_id;
} NewPossibleBookData;

typedef struct {
        /* The list of all DhBooks* found in the system */
        GList *books;

        /* GFile* -> GFileMonitor* */
        GHashTable *monitors;

        /* List of NewPossibleBookData* */
        GSList *new_possible_books_data;

        /* List of book IDs (gchar*) currently disabled */
        GList *books_disabled;

        guint group_by_language : 1;
} DhBookManagerPrivate;

enum {
        SIGNAL_BOOK_CREATED,
        SIGNAL_BOOK_DELETED,
        SIGNAL_BOOK_ENABLED,
        SIGNAL_BOOK_DISABLED,
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

static NewPossibleBookData *
new_possible_book_data_new (DhBookManager *book_manager,
                            GFile         *book_directory)
{
        NewPossibleBookData *data;

        data = g_new0 (NewPossibleBookData, 1);
        data->book_manager = book_manager;
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

        g_slist_free_full (priv->new_possible_books_data, new_possible_book_data_free);
        priv->new_possible_books_data = NULL;

        G_OBJECT_CLASS (dh_book_manager_parent_class)->dispose (object);
}

static void
dh_book_manager_finalize (GObject *object)
{
        DhBookManager *book_manager = DH_BOOK_MANAGER (object);
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);

        g_list_free_full (priv->books_disabled, g_free);

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
        signals[SIGNAL_BOOK_CREATED] =
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
        signals[SIGNAL_BOOK_DELETED] =
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
        signals[SIGNAL_BOOK_ENABLED] =
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
        signals[SIGNAL_BOOK_DISABLED] =
                g_signal_new ("book-disabled",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1,
                              DH_TYPE_BOOK);

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

        settings = dh_settings_get_default ();
        contents_settings = dh_settings_peek_contents_settings (settings);
        books_disabled_strv = g_settings_get_strv (contents_settings, "books-disabled");

        if (books_disabled_strv == NULL)
                return;

        for (i = 0; books_disabled_strv[i] != NULL; i++) {
                gchar *book_id = books_disabled_strv[i];
                priv->books_disabled = g_list_prepend (priv->books_disabled, book_id);
        }

        priv->books_disabled = g_list_reverse (priv->books_disabled);

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
        GList *l;

        builder = g_variant_builder_new (G_VARIANT_TYPE_STRING_ARRAY);

        for (l = priv->books_disabled; l != NULL; l = l->next) {
                const gchar *book_id = l->data;
                g_variant_builder_add (builder, "s", book_id);
        }

        variant = g_variant_builder_end (builder);
        g_variant_builder_unref (builder);

        settings = dh_settings_get_default ();
        contents_settings = dh_settings_peek_contents_settings (settings);
        g_settings_set_value (contents_settings, "books-disabled", variant);
}

static GList *
find_book_in_disabled_list (GList  *books_disabled,
                            DhBook *book)
{
        const gchar *book_id;
        GList *node;

        book_id = dh_book_get_id (book);

        for (node = books_disabled; node != NULL; node = node->next) {
                const gchar *cur_book_id = node->data;

                if (g_strcmp0 (book_id, cur_book_id) == 0)
                        return node;
        }

        return NULL;
}

static gboolean
is_book_disabled_in_conf (DhBookManager *book_manager,
                          DhBook        *book)
{
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);

        return find_book_in_disabled_list (priv->books_disabled, book) != NULL;
}

static void
remove_book (DhBookManager *book_manager,
             DhBook        *book)
{
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        GList *node;

        node = g_list_find (priv->books, book);

        if (node != NULL) {
                g_signal_emit (book_manager,
                               signals[SIGNAL_BOOK_DELETED],
                               0,
                               book);

                priv->books = g_list_delete_link (priv->books, node);
                g_object_unref (book);
        }
}

static void
book_deleted_cb (DhBook        *book,
                 DhBookManager *book_manager)
{
        remove_book (book_manager, book);
}

static void
book_updated_cb (DhBook        *book,
                 DhBookManager *book_manager)
{
        GFile *index_file;

        /* Re-create the DhBook to parse again the index file. */

        index_file = dh_book_get_index_file (book);
        g_object_ref (index_file);

        remove_book (book_manager, book);

        create_book_from_index_file (book_manager, index_file);
        g_object_unref (index_file);
}

static void
book_enabled_cb (DhBook        *book,
                 DhBookManager *book_manager)
{
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        GList *node;
        gchar *book_id;

        node = find_book_in_disabled_list (priv->books_disabled, book);

        /* When setting as enabled a given book, we should have it in the
         * disabled books list!
         */
        g_return_if_fail (node != NULL);

        book_id = node->data;
        g_free (book_id);
        priv->books_disabled = g_list_delete_link (priv->books_disabled, node);

        store_books_disabled (book_manager);

        g_signal_emit (book_manager,
                       signals[SIGNAL_BOOK_ENABLED],
                       0,
                       book);
}

static void
book_disabled_cb (DhBook        *book,
                  DhBookManager *book_manager)
{
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        GList *node;
        const gchar *book_id;

        node = find_book_in_disabled_list (priv->books_disabled, book);

        /* When setting as disabled a given book, we shouldn't have it in the
         * disabled books list!
         */
        g_return_if_fail (node == NULL);

        book_id = dh_book_get_id (book);
        priv->books_disabled = g_list_append (priv->books_disabled,
                                              g_strdup (book_id));
        store_books_disabled (book_manager);

        g_signal_emit (book_manager,
                       signals[SIGNAL_BOOK_DISABLED],
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
                       signals[SIGNAL_BOOK_CREATED],
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

        possible_index_files = _dh_util_get_possible_index_files (book_directory);

        for (l = possible_index_files; l != NULL; l = l->next) {
                GFile *index_file = G_FILE (l->data);

                if (create_book_from_index_file (book_manager, index_file))
                        break;
        }

        g_slist_free_full (possible_index_files, g_object_unref);
}

static gboolean
new_possible_book_timeout_cb (gpointer user_data)
{
        NewPossibleBookData *data = user_data;
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (data->book_manager);

        data->timeout_id = 0;

        create_book_from_directory (data->book_manager, data->book_directory);

        priv->new_possible_books_data = g_slist_remove (priv->new_possible_books_data, data);
        new_possible_book_data_free (data);

        return G_SOURCE_REMOVE;
}

static void
books_directory_changed_cb (GFileMonitor      *directory_monitor,
                            GFile             *file,
                            GFile             *other_file,
                            GFileMonitorEvent  event_type,
                            DhBookManager     *book_manager)
{
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        NewPossibleBookData *data;

        /* With the GFileMonitor here we only handle events for new directories
         * created. Book deletions and updates are handled by the GFileMonitor
         * in each DhBook object.
         */
        if (event_type != G_FILE_MONITOR_EVENT_CREATED)
                return;

        data = new_possible_book_data_new (book_manager, file);

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
monitor_books_directory (DhBookManager *book_manager,
                         GFile         *books_directory)
{
        DhBookManagerPrivate *priv = dh_book_manager_get_instance_private (book_manager);
        GFileMonitor *directory_monitor;
        GError *error = NULL;

        /* If monitor already exists, do not re-create it. */
        if (priv->monitors != NULL &&
            g_hash_table_lookup (priv->monitors, books_directory) != NULL) {
                return;
        }

        directory_monitor = g_file_monitor_directory (books_directory,
                                                      G_FILE_MONITOR_NONE,
                                                      NULL,
                                                      &error);

        if (error != NULL) {
                gchar *parse_name;

                parse_name = g_file_get_parse_name (books_directory);

                g_warning ("Failed to create file monitor on directory “%s”: %s",
                           parse_name,
                           error->message);

                g_free (parse_name);
                g_clear_error (&error);
        }

        if (directory_monitor != NULL) {
                if (G_UNLIKELY (priv->monitors == NULL)) {
                        priv->monitors = g_hash_table_new_full (g_file_hash,
                                                                (GEqualFunc) g_file_equal,
                                                                g_object_unref,
                                                                g_object_unref);
                }

                g_hash_table_insert (priv->monitors,
                                     g_object_ref (books_directory),
                                     directory_monitor);

                g_signal_connect_object (directory_monitor,
                                         "changed",
                                         G_CALLBACK (books_directory_changed_cb),
                                         book_manager,
                                         0);
        }
}

static void
find_books_in_dir (DhBookManager *book_manager,
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

        monitor_books_directory (book_manager, directory);

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
find_books_in_data_dir (DhBookManager *book_manager,
                        const gchar   *data_dir)
{
        gchar *dir;

        g_return_if_fail (data_dir != NULL);

        dir = g_build_filename (data_dir, "gtk-doc", "html", NULL);
        find_books_in_dir (book_manager, dir);
        g_free (dir);

        dir = g_build_filename (data_dir, "devhelp", "books", NULL);
        find_books_in_dir (book_manager, dir);
        g_free (dir);
}

static void
populate (DhBookManager *book_manager)
{
        const gchar * const *system_dirs;
        gint i;

        find_books_in_data_dir (book_manager, g_get_user_data_dir ());

        system_dirs = g_get_system_data_dirs ();
        g_return_if_fail (system_dirs != NULL);

        for (i = 0; system_dirs[i] != NULL; i++) {
                find_books_in_data_dir (book_manager, system_dirs[i]);
        }

        /* For Flatpak, to see the books installed on the host by traditional
         * Linux distro packages.
         *
         * It is not a good idea to add the directory to XDG_DATA_DIRS, see:
         * https://github.com/flatpak/flatpak/issues/1299
         * "all sorts of things will break if we add all host config to each
         * app, which is totally opposite to the entire point of flatpak."
         * "i don't think XDG_DATA_DIRS is the right thing, because all sorts of
         * libraries will start reading files from there, like dconf, dbus,
         * service files, mimetypes, etc. It would be preferable to have
         * something that targeted just gtk-doc files."
         *
         * So instead of adapting XDG_DATA_DIRS, add the directory here, with
         * the path hard-coded.
         *
         * https://bugzilla.gnome.org/show_bug.cgi?id=792068
         */
#ifdef FLATPAK_BUILD
        find_books_in_data_dir (book_manager, "/run/host/usr/share");
#endif
}

static void
dh_book_manager_init (DhBookManager *book_manager)
{
        DhSettings *settings;
        GSettings *contents_settings;

        load_books_disabled (book_manager);

        settings = dh_settings_get_default ();
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
