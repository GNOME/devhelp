/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2012 Thomas Bechtold <toabctl@gnome.org>
 * Copyright (C) 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
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

#include "dh-settings.h"
#include "dh-settings-builder.h"

/**
 * SECTION:dh-settings
 * @Title: DhSettings
 * @Short_description: Access to the libdevhelp #GSettings objects
 *
 * #DhSettings permits to have access to the #GSettings objects that are part of
 * the libdevhelp. To have the documentation about the available keys and their
 * types, read the `*.gschema.xml` file.
 */

/* API design:
 *
 * There is no dh_settings_set_default() function. Because what if the default
 * instance is changed when the app is already initialized? There is no signal
 * to be notified. So there is only one default DhSettings possible, and it is
 * defined by the libdevhelp (not the app).
 *
 * TODO implement DhProfile and add "profile" properties to classes that need to
 * access the DhSettings. DhProfile would contain a DhSettings object, plus a
 * DhBookSelection.
 */

/* libdevhelp GSettings schema IDs */
#define SETTINGS_SCHEMA_ID_CONTENTS             "org.gnome.libdevhelp-3.contents"

struct _DhSettingsPrivate {
        GSettings *gsettings_contents;

        guint group_books_by_language : 1;
};

enum {
        PROP_0,
        PROP_GROUP_BOOKS_BY_LANGUAGE,
        N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];
static DhSettings *default_instance = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (DhSettings, dh_settings, G_TYPE_OBJECT);

static void
dh_settings_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
        DhSettings *settings = DH_SETTINGS (object);

        switch (prop_id) {
                case PROP_GROUP_BOOKS_BY_LANGUAGE:
                        g_value_set_boolean (value, dh_settings_get_group_books_by_language (settings));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_settings_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
        DhSettings *settings = DH_SETTINGS (object);

        switch (prop_id) {
                case PROP_GROUP_BOOKS_BY_LANGUAGE:
                        dh_settings_set_group_books_by_language (settings, g_value_get_boolean (value));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_settings_dispose (GObject *object)
{
        DhSettings *settings = DH_SETTINGS (object);

        g_clear_object (&settings->priv->gsettings_contents);

        G_OBJECT_CLASS (dh_settings_parent_class)->dispose (object);
}

static void
dh_settings_finalize (GObject *object)
{
        if (default_instance == DH_SETTINGS (object))
                default_instance = NULL;

        G_OBJECT_CLASS (dh_settings_parent_class)->finalize (object);
}

static void
dh_settings_class_init (DhSettingsClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = dh_settings_get_property;
        object_class->set_property = dh_settings_set_property;
        object_class->dispose = dh_settings_dispose;
        object_class->finalize = dh_settings_finalize;

        /**
         * DhSettings:group-books-by-language:
         *
         * Whether books should be grouped by programming language in the UI.
         *
         * Since: 3.30
         */
        properties[PROP_GROUP_BOOKS_BY_LANGUAGE] =
                g_param_spec_boolean ("group-books-by-language",
                                      "Group books by language",
                                      "",
                                      FALSE,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT |
                                      G_PARAM_STATIC_STRINGS);

        g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
dh_settings_init (DhSettings *settings)
{
        settings->priv = dh_settings_get_instance_private (settings);
}

DhSettings *
_dh_settings_new (const gchar *contents_path)
{
        DhSettings *object;

        g_return_val_if_fail (contents_path != NULL, NULL);

        object = g_object_new (DH_TYPE_SETTINGS, NULL);
        object->priv->gsettings_contents = g_settings_new_with_path (SETTINGS_SCHEMA_ID_CONTENTS,
                                                                     contents_path);

        return object;
}

/**
 * dh_settings_get_default:
 *
 * Gets the default #DhSettings object. It has the default #GSettings paths (see
 * #DhSettingsBuilder) and dh_settings_bind_all() has been called.
 *
 * Returns: (transfer none): the default #DhSettings object.
 * Since: 3.30
 */
DhSettings *
dh_settings_get_default (void)
{
        if (default_instance == NULL) {
                DhSettingsBuilder *builder;

                builder = dh_settings_builder_new ();
                default_instance = dh_settings_builder_create_object (builder);
                dh_settings_bind_all (default_instance);
                g_object_unref (builder);
        }

        return default_instance;
}

void
_dh_settings_unref_default (void)
{
        if (default_instance != NULL)
                g_object_unref (default_instance);

        /* default_instance is not set to NULL here, it is set to NULL in
         * dh_settings_finalize() (i.e. when we are sure that the ref count
         * reaches 0).
         */
}

/**
 * dh_settings_peek_contents_settings:
 * @settings: a #DhSettings.
 *
 * Returns: (transfer none): the #GSettings for the "contents" schema.
 * Since: 3.30
 */
GSettings *
dh_settings_peek_contents_settings (DhSettings *settings)
{
        g_return_val_if_fail (DH_IS_SETTINGS (settings), NULL);
        return settings->priv->gsettings_contents;
}

/**
 * dh_settings_bind_all:
 * @settings: a #DhSettings.
 *
 * Binds all the #DhSettings properties to their corresponding #GSettings keys.
 *
 * Since: 3.30
 */
void
dh_settings_bind_all (DhSettings *settings)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));

        dh_settings_bind_group_books_by_language (settings);
}

/**
 * dh_settings_get_group_books_by_language:
 * @settings: a #DhSettings.
 *
 * Returns: the value of the #DhSettings:group-books-by-language property.
 * Since: 3.30
 */
gboolean
dh_settings_get_group_books_by_language (DhSettings *settings)
{
        g_return_val_if_fail (DH_IS_SETTINGS (settings), FALSE);

        return settings->priv->group_books_by_language;
}

/**
 * dh_settings_set_group_books_by_language:
 * @settings: a #DhSettings.
 * @group_books_by_language: the new value.
 *
 * Sets the #DhSettings:group-books-by-language property.
 *
 * Since: 3.30
 */
void
dh_settings_set_group_books_by_language (DhSettings *settings,
                                         gboolean    group_books_by_language)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));

        group_books_by_language = group_books_by_language != FALSE;

        if (settings->priv->group_books_by_language != group_books_by_language) {
                settings->priv->group_books_by_language = group_books_by_language;
                g_object_notify_by_pspec (G_OBJECT (settings), properties[PROP_GROUP_BOOKS_BY_LANGUAGE]);
        }
}

/**
 * dh_settings_bind_group_books_by_language:
 * @settings: a #DhSettings.
 *
 * Binds the #DhSettings:group-books-by-language property to the corresponding
 * #GSettings key.
 *
 * Since: 3.30
 */
void
dh_settings_bind_group_books_by_language (DhSettings *settings)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));

        g_settings_bind (settings->priv->gsettings_contents, "group-books-by-language",
                         settings, "group-books-by-language",
                         G_SETTINGS_BIND_DEFAULT |
                         G_SETTINGS_BIND_NO_SENSITIVITY);
}
