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

#include "dh-settings-app.h"

/* Devhelp GSettings schema IDs */
#define SETTINGS_SCHEMA_ID_WINDOW               "org.gnome.devhelp.state.main.window"
#define SETTINGS_SCHEMA_ID_PANED                "org.gnome.devhelp.state.main.paned"
#define SETTINGS_SCHEMA_ID_ASSISTANT            "org.gnome.devhelp.state.assistant.window"

struct _DhSettingsAppPrivate {
        GSettings *settings_window;
        GSettings *settings_paned;
        GSettings *settings_assistant;
};

/* DhSettingsApp is a singleton. */
static DhSettingsApp *singleton = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (DhSettingsApp, dh_settings_app, G_TYPE_OBJECT);

static void
dh_settings_app_dispose (GObject *object)
{
        DhSettingsApp *self = DH_SETTINGS_APP (object);

        g_clear_object (&self->priv->settings_window);
        g_clear_object (&self->priv->settings_paned);
        g_clear_object (&self->priv->settings_assistant);

        G_OBJECT_CLASS (dh_settings_app_parent_class)->dispose (object);
}

static void
dh_settings_app_finalize (GObject *object)
{
        if (singleton == DH_SETTINGS_APP (object))
                singleton = NULL;

        G_OBJECT_CLASS (dh_settings_app_parent_class)->finalize (object);
}

static void
dh_settings_app_class_init (DhSettingsAppClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_settings_app_dispose;
        object_class->finalize = dh_settings_app_finalize;
}

static void
dh_settings_app_init (DhSettingsApp *self)
{
        self->priv = dh_settings_app_get_instance_private (self);

        self->priv->settings_window = g_settings_new (SETTINGS_SCHEMA_ID_WINDOW);
        self->priv->settings_paned = g_settings_new (SETTINGS_SCHEMA_ID_PANED);
        self->priv->settings_assistant = g_settings_new (SETTINGS_SCHEMA_ID_ASSISTANT);
}

DhSettingsApp *
dh_settings_app_get_singleton (void)
{
        if (singleton == NULL)
                singleton = g_object_new (DH_TYPE_SETTINGS_APP, NULL);

        return singleton;
}

void
dh_settings_app_unref_singleton (void)
{
        if (singleton != NULL)
                g_object_unref (singleton);

        /* singleton is not set to NULL here, it is set to NULL in
         * dh_settings_app_finalize() (i.e. when we are sure that the ref count
         * reaches 0).
         */
}

GSettings *
dh_settings_app_peek_window_settings (DhSettingsApp *self)
{
        g_return_val_if_fail (DH_IS_SETTINGS_APP (self), NULL);
        return self->priv->settings_window;
}

GSettings *
dh_settings_app_peek_paned_settings (DhSettingsApp *self)
{
        g_return_val_if_fail (DH_IS_SETTINGS_APP (self), NULL);
        return self->priv->settings_paned;
}

GSettings *
dh_settings_app_peek_assistant_settings (DhSettingsApp *self)
{
        g_return_val_if_fail (DH_IS_SETTINGS_APP (self), NULL);
        return self->priv->settings_assistant;
}
