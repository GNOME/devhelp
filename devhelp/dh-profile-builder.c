/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
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

#include "dh-profile-builder.h"

/**
 * SECTION:dh-profile-builder
 * @Title: DhProfileBuilder
 * @Short_description: Builds #DhProfile objects
 *
 * #DhProfileBuilder permits to build #DhProfile objects.
 */

/* API design:
 *
 * It follows the builder pattern, see:
 * https://blogs.gnome.org/otte/2018/02/03/builders/
 * but it is implemented in a simpler way, to have less boilerplate.
 */

struct _DhProfileBuilder {
        GObject parent_instance;
};

typedef struct {
        DhSettings *settings;
        DhBookList *book_list;
} DhProfileBuilderPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhProfileBuilder, dh_profile_builder, G_TYPE_OBJECT)

static void
dh_profile_builder_dispose (GObject *object)
{
        DhProfileBuilder *builder = DH_PROFILE_BUILDER (object);
        DhProfileBuilderPrivate *priv = dh_profile_builder_get_instance_private (builder);

        g_clear_object (&priv->settings);
        g_clear_object (&priv->book_list);

        G_OBJECT_CLASS (dh_profile_builder_parent_class)->dispose (object);
}

static void
dh_profile_builder_class_init (DhProfileBuilderClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_profile_builder_dispose;
}

static void
dh_profile_builder_init (DhProfileBuilder *builder)
{
}

/**
 * dh_profile_builder_new:
 *
 * Returns: (transfer full): a new #DhProfileBuilder.
 * Since: 3.30
 */
DhProfileBuilder *
dh_profile_builder_new (void)
{
        return g_object_new (DH_TYPE_PROFILE_BUILDER, NULL);
}

/**
 * dh_profile_builder_set_settings:
 * @builder: a #DhProfileBuilder.
 * @settings: a #DhSettings.
 *
 * Sets the #DhSettings object.
 *
 * If you don't call this function, the default #DhSettings object as returned
 * by dh_settings_get_default() will be used.
 *
 * Since: 3.30
 */
void
dh_profile_builder_set_settings (DhProfileBuilder *builder,
                                 DhSettings       *settings)
{
        DhProfileBuilderPrivate *priv = dh_profile_builder_get_instance_private (builder);

        g_return_if_fail (DH_IS_PROFILE_BUILDER (builder));
        g_return_if_fail (DH_IS_SETTINGS (settings));

        g_set_object (&priv->settings, settings);
}

/**
 * dh_profile_builder_set_book_list:
 * @builder: a #DhProfileBuilder.
 * @book_list: a #DhBookList.
 *
 * Sets the #DhBookList object.
 *
 * If you don't call this function, the default #DhBookList object as returned
 * by dh_book_list_get_default() will be used.
 *
 * Since: 3.30
 */
void
dh_profile_builder_set_book_list (DhProfileBuilder *builder,
                                  DhBookList       *book_list)
{
        DhProfileBuilderPrivate *priv = dh_profile_builder_get_instance_private (builder);

        g_return_if_fail (DH_IS_PROFILE_BUILDER (builder));
        g_return_if_fail (DH_IS_BOOK_LIST (book_list));

        g_set_object (&priv->book_list, book_list);
}

/**
 * dh_profile_builder_create_object:
 * @builder: a #DhProfileBuilder.
 *
 * Returns: (transfer full): the newly created #DhProfile object.
 * Since: 3.30
 */
DhProfile *
dh_profile_builder_create_object (DhProfileBuilder *builder)
{
        DhProfileBuilderPrivate *priv = dh_profile_builder_get_instance_private (builder);

        g_return_val_if_fail (DH_IS_PROFILE_BUILDER (builder), NULL);

        /* Set default values if needed.
         * Use all the set functions to test them, to have the same code paths
         * as if the set functions were already called.
         */
        if (priv->settings == NULL)
                dh_profile_builder_set_settings (builder, dh_settings_get_default ());

        if (priv->book_list == NULL)
                dh_profile_builder_set_book_list (builder, dh_book_list_get_default ());

        return _dh_profile_new (priv->settings,
                                priv->book_list);
}
