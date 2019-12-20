/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2012 Thomas Bechtold <toabctl@gnome.org>
 * SPDX-FileCopyrightText: 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
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
};

GType           dh_settings_app_get_type                    (void);

DhSettingsApp * dh_settings_app_get_singleton               (void);

void            dh_settings_app_unref_singleton             (void);

GSettings *     dh_settings_app_peek_window_settings        (DhSettingsApp *self);

GSettings *     dh_settings_app_peek_paned_settings         (DhSettingsApp *self);

GSettings *     dh_settings_app_peek_assistant_settings     (DhSettingsApp *self);

G_END_DECLS

#endif /* DH_SETTINGS_APP_H */
