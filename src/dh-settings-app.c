/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2012 Thomas Bechtold <toabctl@gnome.org>
 * Copyright (C) 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
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

#include "dh-settings-app.h"

/* Devhelp GSettings schema IDs */
#define SETTINGS_SCHEMA_ID_WINDOW               "org.gnome.devhelp.state.main.window"
#define SETTINGS_SCHEMA_ID_PANED                "org.gnome.devhelp.state.main.paned"
#define SETTINGS_SCHEMA_ID_ASSISTANT            "org.gnome.devhelp.state.assistant.window"
#define SETTINGS_SCHEMA_ID_FONTS                "org.gnome.devhelp.fonts"

/* Provided by the gsettings-desktop-schemas module. */
#define SETTINGS_SCHEMA_ID_DESKTOP_INTERFACE    "org.gnome.desktop.interface"
#define SYSTEM_FIXED_FONT_KEY                   "monospace-font-name"
#define SYSTEM_VARIABLE_FONT_KEY                "font-name"

struct _DhSettingsAppPrivate {
        GSettings *settings_window;
        GSettings *settings_paned;
        GSettings *settings_assistant;
        GSettings *settings_fonts;
        GSettings *settings_desktop_interface;
};

enum {
        FONTS_CHANGED,
        N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

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
        g_clear_object (&self->priv->settings_fonts);
        g_clear_object (&self->priv->settings_desktop_interface);

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

        signals[FONTS_CHANGED] =
                g_signal_new ("fonts-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DhSettingsAppClass, fonts_changed),
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              2,
                              G_TYPE_STRING,
                              G_TYPE_STRING);
}

static void
fonts_changed_cb (GSettings *gsettings,
                  gchar     *key,
                  gpointer   user_data)
{
        DhSettingsApp *self = DH_SETTINGS_APP (user_data);
        gchar *fixed_font = NULL;
        gchar *variable_font = NULL;

        dh_settings_app_get_selected_fonts (self, &fixed_font, &variable_font);

        g_signal_emit (self, signals[FONTS_CHANGED], 0, fixed_font, variable_font);

        g_free (fixed_font);
        g_free (variable_font);
}

static void
dh_settings_app_init (DhSettingsApp *self)
{
        self->priv = dh_settings_app_get_instance_private (self);

        self->priv->settings_window = g_settings_new (SETTINGS_SCHEMA_ID_WINDOW);
        self->priv->settings_paned = g_settings_new (SETTINGS_SCHEMA_ID_PANED);
        self->priv->settings_assistant = g_settings_new (SETTINGS_SCHEMA_ID_ASSISTANT);
        self->priv->settings_fonts = g_settings_new (SETTINGS_SCHEMA_ID_FONTS);
        self->priv->settings_desktop_interface = g_settings_new (SETTINGS_SCHEMA_ID_DESKTOP_INTERFACE);

        g_signal_connect_object (self->priv->settings_fonts,
                                 "changed",
                                 G_CALLBACK (fonts_changed_cb),
                                 self,
                                 0);

        g_signal_connect_object (self->priv->settings_desktop_interface,
                                 "changed::" SYSTEM_FIXED_FONT_KEY,
                                 G_CALLBACK (fonts_changed_cb),
                                 self,
                                 0);

        g_signal_connect_object (self->priv->settings_desktop_interface,
                                 "changed::" SYSTEM_VARIABLE_FONT_KEY,
                                 G_CALLBACK (fonts_changed_cb),
                                 self,
                                 0);
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

GSettings *
dh_settings_app_peek_fonts_settings (DhSettingsApp *self)
{
        g_return_val_if_fail (DH_IS_SETTINGS_APP (self), NULL);
        return self->priv->settings_fonts;
}

void
dh_settings_app_get_selected_fonts (DhSettingsApp  *self,
                                    gchar         **font_name_fixed,
                                    gchar         **font_name_variable)
{
        gboolean use_system_font;

        g_return_if_fail (DH_IS_SETTINGS_APP (self));
        g_return_if_fail (font_name_fixed != NULL && *font_name_fixed == NULL);
        g_return_if_fail (font_name_variable != NULL && *font_name_variable == NULL);

        use_system_font = g_settings_get_boolean (self->priv->settings_fonts, "use-system-fonts");

        if (use_system_font) {
                *font_name_fixed = g_settings_get_string (self->priv->settings_desktop_interface,
                                                          SYSTEM_FIXED_FONT_KEY);
                *font_name_variable = g_settings_get_string (self->priv->settings_desktop_interface,
                                                             SYSTEM_VARIABLE_FONT_KEY);
        } else {
                *font_name_fixed = g_settings_get_string (self->priv->settings_fonts,
                                                          "fixed-font");
                *font_name_variable = g_settings_get_string (self->priv->settings_fonts,
                                                             "variable-font");
        }
}
