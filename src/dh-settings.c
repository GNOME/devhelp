/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
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
#include "dh-settings.h"

G_DEFINE_TYPE (DhSettings, dh_settings, G_TYPE_OBJECT);

#define DH_SETTINGS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DH_TYPE_SETTINGS, DhSettingsPrivate))

/* schema-ids for settings we need */
#define SETTINGS_SCHEMA_ID_DESKTOP_INTERFACE "org.gnome.desktop.interface"
#define SETTINGS_SCHEMA_ID_FONTS "org.gnome.devhelp.fonts"
#define SETTINGS_SCHEMA_ID_WINDOW "org.gnome.devhelp.state.main.window"
#define SETTINGS_SCHEMA_ID_CONTENTS "org.gnome.devhelp.state.main.contents"
#define SETTINGS_SCHEMA_ID_PANED "org.gnome.devhelp.state.main.paned"
#define SETTINGS_SCHEMA_ID_ASSISTANT "org.gnome.devhelp.state.assistant.window"

/* singleton object - all consumers of DhSettings get the same object (refcounted) */
static DhSettings *singleton = NULL;

/* Prototypes */
static void fonts_changed_cb (GSettings *settings, gchar *key, gpointer user_data);


struct _DhSettingsPrivate {
        GSettings *settings_desktop_interface;
        GSettings *settings_fonts;
        GSettings *settings_window;
        GSettings *settings_contents;
        GSettings *settings_paned;
        GSettings *settings_assistant;
};

enum {
        FONTS_CHANGED,
        LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };


static void
dh_settings_init (DhSettings *self)
{
        self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, DH_TYPE_SETTINGS, DhSettingsPrivate);

        self->priv->settings_desktop_interface = g_settings_new (SETTINGS_SCHEMA_ID_DESKTOP_INTERFACE);
        self->priv->settings_fonts = g_settings_new (SETTINGS_SCHEMA_ID_FONTS);
        self->priv->settings_window = g_settings_new (SETTINGS_SCHEMA_ID_WINDOW);
        self->priv->settings_contents = g_settings_new (SETTINGS_SCHEMA_ID_CONTENTS);
        self->priv->settings_paned = g_settings_new (SETTINGS_SCHEMA_ID_PANED);
        self->priv->settings_assistant = g_settings_new (SETTINGS_SCHEMA_ID_ASSISTANT);

        /* setup GSettings notifications */
        g_signal_connect (self->priv->settings_fonts,
                          "changed",
                          G_CALLBACK (fonts_changed_cb), self);
}


static void
dispose (GObject *object)
{
        DhSettings *self = DH_SETTINGS (object);
        g_clear_object (&self->priv->settings_desktop_interface);
        g_clear_object (&self->priv->settings_fonts);
        g_clear_object (&self->priv->settings_window);
        g_clear_object (&self->priv->settings_contents);
        g_clear_object (&self->priv->settings_paned);
        g_clear_object (&self->priv->settings_assistant);

        G_OBJECT_CLASS (dh_settings_parent_class)->dispose (object);
}


static void
finalize (GObject *object)
{
        singleton = NULL;

        /* Chain up to the parent class */
        G_OBJECT_CLASS (dh_settings_parent_class)->finalize(object);
}


static void
dh_settings_class_init (DhSettingsClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (object_class, sizeof (DhSettingsPrivate));
        object_class->dispose = dispose;
        object_class->finalize = finalize;

        signals[FONTS_CHANGED] =
                g_signal_new ("fonts-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DhSettingsClass, fonts_changed),
                              NULL, NULL,
                              g_cclosure_marshal_generic,
                              G_TYPE_NONE,
                              2,
                              G_TYPE_STRING,
                              G_TYPE_STRING);
}

static void
fonts_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
        DhSettings *self = DH_SETTINGS (user_data);
        gchar *fixed_font = NULL;
        gchar *variable_font = NULL;
        dh_settings_get_selected_fonts (self, &fixed_font, &variable_font);
        //emit signal - font changed
        g_signal_emit (self, signals[FONTS_CHANGED], 0, fixed_font, variable_font);
        g_free (fixed_font);
        g_free (variable_font);
}

/*******************************************************************************
 * Public Methods
 ******************************************************************************/
DhSettings *
dh_settings_get (void)
{
        if (!singleton) {
                singleton = DH_SETTINGS (g_object_new (DH_TYPE_SETTINGS, NULL));
        } else {
                g_object_ref (singleton);
        }
        g_assert (singleton);
        return singleton;
}

void
dh_settings_get_selected_fonts (DhSettings *self, gchar **font_name_fixed, gchar **font_name_variable)
{
        gboolean use_system_font;

        g_return_if_fail (font_name_fixed != NULL && *font_name_fixed == NULL);
        g_return_if_fail (font_name_variable != NULL && *font_name_variable == NULL);

        use_system_font = g_settings_get_boolean (self->priv->settings_fonts, "use-system-fonts");

        if (use_system_font) {
                *font_name_fixed = g_settings_get_string (
                        self->priv->settings_desktop_interface,
                        "monospace-font-name");
                *font_name_variable = g_settings_get_string (
                        self->priv->settings_desktop_interface,
                        "font-name");
        } else {
                *font_name_fixed = g_settings_get_string (
                        self->priv->settings_fonts, "fixed-font");
                *font_name_variable = g_settings_get_string (
                        self->priv->settings_fonts, "variable-font");
        }
}

GSettings *
dh_settings_peek_fonts_settings (DhSettings *self)
{
        return self->priv->settings_fonts;
}

GSettings *
dh_settings_peek_window_settings (DhSettings *self)
{
        return self->priv->settings_window;
}

GSettings *
dh_settings_peek_contents_settings (DhSettings *self)
{
        return self->priv->settings_contents;
}

GSettings *
dh_settings_peek_paned_settings (DhSettings *self)
{
        return self->priv->settings_paned;
}

GSettings *
dh_settings_peek_assistant_settings (DhSettings *self)
{
        return self->priv->settings_assistant;
}
