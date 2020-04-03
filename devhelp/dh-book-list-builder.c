/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"
#include "dh-book-list-builder.h"
#include "dh-book-list-directory.h"
#include "dh-book-list-simple.h"

/**
 * SECTION:dh-book-list-builder
 * @Title: DhBookListBuilder
 * @Short_description: Builds #DhBookList objects
 *
 * #DhBookListBuilder permits to build #DhBookList objects.
 */

/* API design:
 *
 * It follows the builder pattern, see:
 * https://blogs.gnome.org/otte/2018/02/03/builders/
 * but it is implemented in a simpler way, to have less boilerplate.
 */

struct _DhBookListBuilderPrivate {
        /* List of DhBookList*. */
        GList *sub_book_lists;

        DhSettings *settings;
};

G_DEFINE_TYPE_WITH_PRIVATE (DhBookListBuilder, dh_book_list_builder, G_TYPE_OBJECT)

static void
dh_book_list_builder_dispose (GObject *object)
{
        DhBookListBuilder *builder = DH_BOOK_LIST_BUILDER (object);

        g_list_free_full (builder->priv->sub_book_lists, g_object_unref);
        builder->priv->sub_book_lists = NULL;

        g_clear_object (&builder->priv->settings);

        G_OBJECT_CLASS (dh_book_list_builder_parent_class)->dispose (object);
}

static void
dh_book_list_builder_class_init (DhBookListBuilderClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_book_list_builder_dispose;
}

static void
dh_book_list_builder_init (DhBookListBuilder *builder)
{
        builder->priv = dh_book_list_builder_get_instance_private (builder);
}

/**
 * dh_book_list_builder_new:
 *
 * Returns: (transfer full): a new #DhBookListBuilder.
 * Since: 3.30
 */
DhBookListBuilder *
dh_book_list_builder_new (void)
{
        return g_object_new (DH_TYPE_BOOK_LIST_BUILDER, NULL);
}

/**
 * dh_book_list_builder_add_sub_book_list:
 * @builder: a #DhBookListBuilder.
 * @sub_book_list: a #DhBookList.
 *
 * Adds @sub_book_list.
 *
 * The #DhBookList object that will be created with
 * dh_book_list_builder_create_object() will contain all the sub-#DhBookList's
 * added with this function (and it will listen to their signals). The
 * sub-#DhBookList's must be added in order of decreasing priority (the first
 * sub-#DhBookList added has the highest priority). The priority is used in case
 * of book ID conflicts (see dh_book_get_id()).
 *
 * Since: 3.30
 */
void
dh_book_list_builder_add_sub_book_list (DhBookListBuilder *builder,
                                        DhBookList        *sub_book_list)
{
        g_return_if_fail (DH_IS_BOOK_LIST_BUILDER (builder));
        g_return_if_fail (DH_IS_BOOK_LIST (sub_book_list));

        builder->priv->sub_book_lists = g_list_append (builder->priv->sub_book_lists,
                                                       g_object_ref (sub_book_list));
}

static void
add_book_list_directory (DhBookListBuilder *builder,
			 const gchar       *directory_path)
{
        GFile *directory;
        DhBookListDirectory *sub_book_list;

        directory = g_file_new_for_path (directory_path);
        sub_book_list = dh_book_list_directory_new (directory);
        g_object_unref (directory);

        dh_book_list_builder_add_sub_book_list (builder, DH_BOOK_LIST (sub_book_list));
        g_object_unref (sub_book_list);
}

static void
add_default_sub_book_lists_in_data_dir (DhBookListBuilder *builder,
					const gchar       *data_dir)
{
        gchar *dir;

        g_return_if_fail (data_dir != NULL);

        dir = g_build_filename (data_dir, "gtk-doc", "html", NULL);
        add_book_list_directory (builder, dir);
        g_free (dir);

        dir = g_build_filename (data_dir, "devhelp", "books", NULL);
        add_book_list_directory (builder, dir);
        g_free (dir);
}

/**
 * dh_book_list_builder_add_default_sub_book_lists:
 * @builder: a #DhBookListBuilder.
 *
 * Creates the default #DhBookListDirectory's and adds them to @builder with
 * dh_book_list_builder_add_sub_book_list().
 *
 * It creates and adds a #DhBookListDirectory for the following directories (in
 * that order):
 * - `$XDG_DATA_HOME/gtk-doc/html/`
 * - `$XDG_DATA_HOME/devhelp/books/`
 * - For each directory in `$XDG_DATA_DIRS`:
 *   - `$xdg_data_dir/gtk-doc/html/`
 *   - `$xdg_data_dir/devhelp/books/`
 *
 * See g_get_user_data_dir() and g_get_system_data_dirs().
 *
 * Additionally, if the libdevhelp has been compiled with the `flatpak_build`
 * option, it creates and adds a #DhBookListDirectory for the following
 * directories (in that order, after the above ones):
 * - `/run/host/usr/share/gtk-doc/html/`
 * - `/run/host/usr/share/devhelp/books/`
 *
 * The exact list of directories is subject to change, it is not part of the
 * API.
 *
 * Since: 3.30
 */
void
dh_book_list_builder_add_default_sub_book_lists (DhBookListBuilder *builder)
{
        const gchar * const *system_dirs;
        gint i;

        g_return_if_fail (DH_IS_BOOK_LIST_BUILDER (builder));

        add_default_sub_book_lists_in_data_dir (builder, g_get_user_data_dir ());

        system_dirs = g_get_system_data_dirs ();
        g_return_if_fail (system_dirs != NULL);

        for (i = 0; system_dirs[i] != NULL; i++)
                add_default_sub_book_lists_in_data_dir (builder, system_dirs[i]);

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
        add_default_sub_book_lists_in_data_dir (builder, "/run/host/usr/share");
#endif
}

/**
 * dh_book_list_builder_read_books_disabled_setting:
 * @builder: a #DhBookListBuilder.
 * @settings: (nullable): a #DhSettings, or %NULL.
 *
 * Sets the #DhSettings object from which to read the "books-disabled"
 * #GSettings key. If @settings is %NULL or if this function isn't called, then
 * the #DhBookList object that will be created with
 * dh_book_list_builder_create_object() will not read a "books-disabled"
 * setting.
 *
 * With #DhBookListBuilder it is not possible to read the "books-disabled"
 * settings from several #DhSettings objects and combine them. Only the last
 * call to this function is taken into account when creating the #DhBookList
 * with dh_book_list_builder_create_object().
 *
 * Since: 3.30
 */
void
dh_book_list_builder_read_books_disabled_setting (DhBookListBuilder *builder,
                                                  DhSettings        *settings)
{
        g_return_if_fail (DH_IS_BOOK_LIST_BUILDER (builder));
        g_return_if_fail (settings == NULL || DH_IS_SETTINGS (settings));

        g_set_object (&builder->priv->settings, settings);
}

/**
 * dh_book_list_builder_create_object:
 * @builder: a #DhBookListBuilder.
 *
 * Creates the #DhBookList. It actually creates a subclass of #DhBookList, but
 * the subclass is not exposed to the public API.
 *
 * Returns: (transfer full): the newly created #DhBookList object.
 * Since: 3.30
 */
DhBookList *
dh_book_list_builder_create_object (DhBookListBuilder *builder)
{
        g_return_val_if_fail (DH_IS_BOOK_LIST_BUILDER (builder), NULL);

        return _dh_book_list_simple_new (builder->priv->sub_book_lists,
                                         builder->priv->settings);
}
