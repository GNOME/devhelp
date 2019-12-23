/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2012 Thomas Bechtold <toabctl@gnome.org>
 * SPDX-FileCopyrightText: 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_SETTINGS_H
#define DH_SETTINGS_H

#include <gio/gio.h>
#include <devhelp/dh-book.h>

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
        void (* books_disabled_changed) (DhSettings *settings);
        void (* fonts_changed)          (DhSettings *settings);

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_settings_get_type                            (void) G_GNUC_CONST;

G_GNUC_INTERNAL
DhSettings *    _dh_settings_new                                (const gchar *contents_path,
                                                                 const gchar *fonts_path);

DhSettings *    dh_settings_get_default                         (void);

G_GNUC_INTERNAL
void            _dh_settings_unref_default                      (void);

void            dh_settings_bind_all                            (DhSettings *settings);

gboolean        dh_settings_get_group_books_by_language         (DhSettings *settings);

void            dh_settings_set_group_books_by_language         (DhSettings *settings,
                                                                 gboolean    group_books_by_language);

void            dh_settings_bind_group_books_by_language        (DhSettings *settings);

gboolean        dh_settings_is_book_enabled                     (DhSettings *settings,
                                                                 DhBook     *book);

void            dh_settings_set_book_enabled                    (DhSettings *settings,
                                                                 DhBook     *book,
                                                                 gboolean    enabled);

void            dh_settings_freeze_books_disabled_changed       (DhSettings *settings);

void            dh_settings_thaw_books_disabled_changed         (DhSettings *settings);

void            dh_settings_get_selected_fonts                  (DhSettings  *settings,
                                                                 gchar      **variable_font,
                                                                 gchar      **fixed_font);

gboolean        dh_settings_get_use_system_fonts                (DhSettings *settings);

void            dh_settings_set_use_system_fonts                (DhSettings *settings,
                                                                 gboolean    use_system_fonts);

const gchar *   dh_settings_get_variable_font                   (DhSettings *settings);

void            dh_settings_set_variable_font                   (DhSettings  *settings,
                                                                 const gchar *variable_font);

const gchar *   dh_settings_get_fixed_font                      (DhSettings *settings);

void            dh_settings_set_fixed_font                      (DhSettings  *settings,
                                                                 const gchar *fixed_font);

void            dh_settings_bind_fonts                          (DhSettings *settings);

G_END_DECLS

#endif /* DH_SETTINGS_H */
