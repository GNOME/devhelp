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

#ifndef DH_SETTINGS_H
#define DH_SETTINGS_H

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

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_settings_get_type                            (void) G_GNUC_CONST;

G_GNUC_INTERNAL
DhSettings *    _dh_settings_new                                (const gchar *contents_path);

DhSettings *    dh_settings_get_default                         (void);

G_GNUC_INTERNAL
void            _dh_settings_unref_default                      (void);

GSettings *     dh_settings_peek_contents_settings              (DhSettings *settings);

gboolean        dh_settings_get_group_books_by_language         (DhSettings *settings);

void            dh_settings_set_group_books_by_language         (DhSettings *settings,
                                                                 gboolean    group_books_by_language);

void            dh_settings_bind_group_books_by_language        (DhSettings *settings);

G_END_DECLS

#endif /* DH_SETTINGS_H */
