/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2012 Thomas Bechtold <toabctl@gnome.org>
 * Copyright (C) 2017 Sébastien Wilmet <swilmet@gnome.org>
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

#ifndef __DH_SETTINGS_H__
#define __DH_SETTINGS_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define DH_TYPE_SETTINGS                (dh_settings_get_type ())
#define DH_SETTINGS(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_SETTINGS, DhSettings))
#define DH_IS_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_SETTINGS))
#define DH_SETTINGS_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_SETTINGS, DhSettingsClass))
#define DH_IS_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_SETTINGS))
#define DH_SETTINGS_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_SETTINGS, DhSettingsClass))

typedef struct _DhSettings        DhSettings;
typedef struct _DhSettingsClass   DhSettingsClass;
typedef struct _DhSettingsPrivate DhSettingsPrivate;

struct _DhSettings {
        GObject parent;
        DhSettingsPrivate *priv;
};

struct _DhSettingsClass {
        GObjectClass parent;

        /* Signals */
        void (*fonts_changed) (DhSettings  *settings,
                               const gchar *font_name_fixed,
                               const gchar *font_name_variable);
};

GType           dh_settings_get_type                    (void) G_GNUC_CONST;

DhSettings *    dh_settings_get                         (void);

void            dh_settings_get_selected_fonts          (DhSettings  *self,
                                                         gchar      **font_name_fixed,
                                                         gchar      **font_name_variable);

GSettings *     dh_settings_peek_fonts_settings         (DhSettings *self);

GSettings *     dh_settings_peek_window_settings        (DhSettings *self);

GSettings *     dh_settings_peek_contents_settings      (DhSettings *self);

GSettings *     dh_settings_peek_paned_settings         (DhSettings *self);

GSettings *     dh_settings_peek_assistant_settings     (DhSettings *self);

G_END_DECLS

#endif /* __DH_SETTINGS_H__ */
