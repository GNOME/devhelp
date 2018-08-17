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

#pragma once

#include <gio/gio.h>
#include <devhelp/dh-book.h>

G_BEGIN_DECLS

#define DH_TYPE_SETTINGS                (dh_settings_get_type ())
G_DECLARE_DERIVABLE_TYPE (DhSettings, dh_settings, DH, SETTINGS, GObject)

struct _DhSettingsClass {
        GObjectClass parent;

        /* Signals */
        void (* books_disabled_changed) (DhSettings *settings);
        void (* fonts_changed)          (DhSettings *settings);

        /* Padding for future expansion */
        gpointer padding[12];
};

G_GNUC_INTERNAL
DhSettings  *_dh_settings_new                          (const gchar  *contents_path,
                                                        const gchar  *fonts_path);
DhSettings  *dh_settings_get_default                   (void);
G_GNUC_INTERNAL
void         _dh_settings_unref_default                (void);
void         dh_settings_bind_all                      (DhSettings   *settings);
gboolean     dh_settings_get_group_books_by_language   (DhSettings   *settings);
void         dh_settings_set_group_books_by_language   (DhSettings   *settings,
                                                        gboolean      group_books_by_language);
void         dh_settings_bind_group_books_by_language  (DhSettings   *settings);
gboolean     dh_settings_is_book_enabled               (DhSettings   *settings,
                                                        DhBook       *book);
void         dh_settings_set_book_enabled              (DhSettings   *settings,
                                                        DhBook       *book,
                                                        gboolean      enabled);
void         dh_settings_freeze_books_disabled_changed (DhSettings   *settings);
void         dh_settings_thaw_books_disabled_changed   (DhSettings   *settings);
void         dh_settings_get_selected_fonts            (DhSettings   *settings,
                                                        gchar       **variable_font,
                                                        gchar       **fixed_font);
gboolean     dh_settings_get_use_system_fonts          (DhSettings   *settings);
void         dh_settings_set_use_system_fonts          (DhSettings   *settings,
                                                        gboolean      use_system_fonts);
const gchar *dh_settings_get_variable_font             (DhSettings   *settings);
void         dh_settings_set_variable_font             (DhSettings   *settings,
                                                        const gchar  *variable_font);
const gchar *dh_settings_get_fixed_font                (DhSettings   *settings);
void         dh_settings_set_fixed_font                (DhSettings   *settings,
                                                        const gchar  *fixed_font);
void         dh_settings_bind_fonts                    (DhSettings   *settings);

G_END_DECLS

