/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2001-2002 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2004,2008 Imendio AB
 * SPDX-FileCopyrightText: 2015, 2017 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_UTIL_APP_H
#define DH_UTIL_APP_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

void            dh_util_window_settings_save            (GtkWindow *window,
                                                         GSettings *settings);

void            dh_util_window_settings_restore         (GtkWindow *gtk_window,
                                                         GSettings *settings);

gboolean        dh_util_is_devel_build                  (void);

G_END_DECLS

#endif /* DH_UTIL_APP_H */
