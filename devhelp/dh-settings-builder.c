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

#include "dh-settings-builder.h"

/**
 * SECTION:dh-settings-builder
 * @Title: DhSettingsBuilder
 * @Short_description: Builds #DhSettings objects
 *
 * #DhSettingsBuilder permits to build #DhSettings objects.
 *
 * The #GSettings schemas provided by the libdevhelp are relocatable. So the
 * paths need to be provided. If a path for a certain schema is not provided to
 * the #DhSettingsBuilder with the set function, the default path for that
 * schema will be used. The default paths are the paths common with the Devhelp
 * application.
 *
 * Why are the schemas relocatable? Because different major versions of
 * libdevhelp must be parallel-installable, so the schema IDs must necessarily
 * be different (they must contain the API/major version), but for users to not
 * lose all their settings when there is a new major version of libdevhelp, the
 * schemas – if still compatible – can be relocated to an old common path
 * (common to several major versions of libdevhelp or to an application). If a
 * schema becomes incompatible, the compatible keys can be migrated individually
 * with dconf (see the DhDconfMigration utility class in the libdevhelp source
 * code).
 */

/* API design:
 *
 * It follows the builder pattern, see:
 * https://blogs.gnome.org/otte/2018/02/03/builders/
 * but it is implemented in a simpler way, to have less boilerplate.
 */

struct _DhSettingsBuilder {
        GObject parent_instance;

        gchar *contents_path;
        gchar *fonts_path;
};

G_DEFINE_TYPE (DhSettingsBuilder, dh_settings_builder, G_TYPE_OBJECT)

static void
dh_settings_builder_finalize (GObject *object)
{
        DhSettingsBuilder *builder = DH_SETTINGS_BUILDER (object);

        g_free (builder->contents_path);
        g_free (builder->fonts_path);

        G_OBJECT_CLASS (dh_settings_builder_parent_class)->finalize (object);
}

static void
dh_settings_builder_class_init (DhSettingsBuilderClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = dh_settings_builder_finalize;
}

static void
dh_settings_builder_init (DhSettingsBuilder *builder)
{
        builder = dh_settings_builder_get_instance_private (builder);
}

/**
 * dh_settings_builder_new:
 *
 * Returns: (transfer full): a new #DhSettingsBuilder.
 * Since: 3.30
 */
DhSettingsBuilder *
dh_settings_builder_new (void)
{
        return g_object_new (DH_TYPE_SETTINGS_BUILDER, NULL);
}

/**
 * dh_settings_builder_set_contents_path:
 * @builder: a #DhSettingsBuilder.
 * @contents_path: the path for the "contents" schema.
 *
 * Sets the path for the "contents" schema.
 *
 * If you don't call this function, the default path for this schema will be
 * used.
 *
 * Since: 3.30
 */
void
dh_settings_builder_set_contents_path (DhSettingsBuilder *builder,
                                       const gchar       *contents_path)
{
        g_return_if_fail (DH_IS_SETTINGS_BUILDER (builder));
        g_return_if_fail (contents_path != NULL);

        g_free (builder->contents_path);
        builder->contents_path = g_strdup (contents_path);
}

/**
 * dh_settings_builder_set_fonts_path:
 * @builder: a #DhSettingsBuilder.
 * @fonts_path: the path for the "fonts" schema.
 *
 * Sets the path for the "fonts" schema.
 *
 * If you don't call this function, the default path for this schema will be
 * used.
 *
 * Since: 3.30
 */
void
dh_settings_builder_set_fonts_path (DhSettingsBuilder *builder,
                                    const gchar       *fonts_path)
{
        g_return_if_fail (DH_IS_SETTINGS_BUILDER (builder));
        g_return_if_fail (fonts_path != NULL);

        g_free (builder->fonts_path);
        builder->fonts_path = g_strdup (fonts_path);
}

/**
 * dh_settings_builder_create_object:
 * @builder: a #DhSettingsBuilder.
 *
 * Returns: (transfer full): the newly created #DhSettings object.
 * Since: 3.30
 */
DhSettings *
dh_settings_builder_create_object (DhSettingsBuilder *builder)
{
        g_return_val_if_fail (DH_IS_SETTINGS_BUILDER (builder), NULL);

        /* Set default paths if needed.
         * Use all the set functions to test them, to have the same code paths
         * as if the set functions were already called.
         */
        if (builder->contents_path == NULL) {
                // Must be compatible with Devhelp app version 3.28:
                dh_settings_builder_set_contents_path (builder, "/org/gnome/devhelp/state/main/contents/");
        }
        if (builder->fonts_path == NULL) {
                // Must be compatible with Devhelp app version 3.28:
                dh_settings_builder_set_fonts_path (builder, "/org/gnome/devhelp/fonts/");
        }

        return _dh_settings_new (builder->contents_path,
                                 builder->fonts_path);
}
