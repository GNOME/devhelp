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

#ifndef DH_SETTINGS_APP_H
#define DH_SETTINGS_APP_H

#include <gio/gio.h>

G_BEGIN_DECLS

#define DH_TYPE_SETTINGS_APP                (dh_settings_app_get_type ())
#define DH_SETTINGS_APP(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_SETTINGS_APP, DhSettingsApp))
#define DH_IS_SETTINGS_APP(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_SETTINGS_APP))
#define DH_SETTINGS_APP_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_SETTINGS_APP, DhSettingsAppClass))
#define DH_IS_SETTINGS_APP_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_SETTINGS_APP))
#define DH_SETTINGS_APP_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_SETTINGS_APP, DhSettingsAppClass))

typedef struct _DhSettingsApp        DhSettingsApp;
typedef struct _DhSettingsAppClass   DhSettingsAppClass;
typedef struct _DhSettingsAppPrivate DhSettingsAppPrivate;

struct _DhSettingsApp {
        GObject parent;
        DhSettingsAppPrivate *priv;
};

struct _DhSettingsAppClass {
        GObjectClass parent;

        /* Signals */
        void (*fonts_changed) (DhSettingsApp *settings,
                               const gchar   *font_name_fixed,
                               const gchar   *font_name_variable);
};

GType           dh_settings_app_get_type                    (void);

DhSettingsApp * dh_settings_app_get_singleton               (void);

void            dh_settings_app_unref_singleton             (void);

GSettings *     dh_settings_app_peek_window_settings        (DhSettingsApp *self);

GSettings *     dh_settings_app_peek_paned_settings         (DhSettingsApp *self);

GSettings *     dh_settings_app_peek_assistant_settings     (DhSettingsApp *self);

GSettings *     dh_settings_app_peek_fonts_settings         (DhSettingsApp *self);

void            dh_settings_app_get_selected_fonts          (DhSettingsApp  *self,
                                                             gchar         **font_name_fixed,
                                                             gchar         **font_name_variable);

G_END_DECLS

#endif /* DH_SETTINGS_APP_H */
