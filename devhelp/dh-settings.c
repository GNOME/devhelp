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

/* libdevhelp GSettings schema IDs */
#define SETTINGS_SCHEMA_ID_CONTENTS             "org.gnome.libdevhelp-3.contents"

struct _DhSettingsPrivate {
        GSettings *settings_contents;
};

/* DhSettings is a singleton. */
static DhSettings *singleton = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (DhSettings, dh_settings, G_TYPE_OBJECT);

static void
dh_settings_dispose (GObject *object)
{
        DhSettings *self = DH_SETTINGS (object);

        g_clear_object (&self->priv->settings_contents);

        G_OBJECT_CLASS (dh_settings_parent_class)->dispose (object);
}

static void
dh_settings_finalize (GObject *object)
{
        if (singleton == DH_SETTINGS (object))
                singleton = NULL;

        G_OBJECT_CLASS (dh_settings_parent_class)->finalize (object);
}

static void
dh_settings_class_init (DhSettingsClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_settings_dispose;
        object_class->finalize = dh_settings_finalize;
}

static void
dh_settings_init (DhSettings *self)
{
        self->priv = dh_settings_get_instance_private (self);

        /* The GSettings schemas provided by the libdevhelp are relocatable.
         * Different major versions of libdevhelp must be parallel-installable,
         * so the schema IDs must be different (they must contain the API/major
         * version). But for users to not lose all their settings when there is
         * a new major version of libdevhelp, the schemas – if still
         * compatible – are relocated to an old common path.
         *
         * If a schema becomes incompatible, the compatible keys can be migrated
         * with dconf, with the DhDconfMigration utility class.
         */
        self->priv->settings_contents = g_settings_new_with_path (SETTINGS_SCHEMA_ID_CONTENTS,
                                                                  /* Must be compatible with Devhelp app version 3.28. */
                                                                  "/org/gnome/devhelp/state/main/contents/");
}

DhSettings *
dh_settings_get_singleton (void)
{
        if (singleton == NULL)
                singleton = g_object_new (DH_TYPE_SETTINGS, NULL);

        return singleton;
}

void
_dh_settings_unref_singleton (void)
{
        if (singleton != NULL)
                g_object_unref (singleton);

        /* singleton is not set to NULL here, it is set to NULL in
         * dh_settings_finalize() (i.e. when we are sure that the ref count
         * reaches 0).
         */
}

GSettings *
dh_settings_peek_contents_settings (DhSettings *self)
{
        g_return_val_if_fail (DH_IS_SETTINGS (self), NULL);
        return self->priv->settings_contents;
}
