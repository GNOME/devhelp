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

/* API design:
 *
 * Follow the builder pattern, see:
 * https://blogs.gnome.org/otte/2018/02/03/builders/
 * but implement it in a simpler way, to have less boilerplate.
 */

struct _DhSettingsBuilderPrivate {
        gchar *contents_path;
};

G_DEFINE_TYPE_WITH_PRIVATE (DhSettingsBuilder, dh_settings_builder, G_TYPE_OBJECT)

static void
dh_settings_builder_finalize (GObject *object)
{
        DhSettingsBuilder *builder = DH_SETTINGS_BUILDER (object);

        g_free (builder->priv->contents_path);

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
        builder->priv = dh_settings_builder_get_instance_private (builder);
}

DhSettingsBuilder *
dh_settings_builder_new (void)
{
        return g_object_new (DH_TYPE_SETTINGS_BUILDER, NULL);
}

void
dh_settings_builder_set_contents_path (DhSettingsBuilder *builder,
                                       const gchar       *contents_path)
{
        g_return_if_fail (DH_IS_SETTINGS_BUILDER (builder));
        g_return_if_fail (contents_path != NULL);

        g_free (builder->priv->contents_path);
        builder->priv->contents_path = g_strdup (contents_path);
}

DhSettings *
dh_settings_builder_create_object (DhSettingsBuilder *builder)
{
        g_return_val_if_fail (DH_IS_SETTINGS_BUILDER (builder), NULL);

        return _dh_settings_new (builder->priv->contents_path);
}
