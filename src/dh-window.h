/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2005 Imendio AB
 * Copyright (C) 2017-2018 Sébastien Wilmet <swilmet@gnome.org>
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DH_TYPE_WINDOW         (dh_window_get_type ())

G_DECLARE_DERIVABLE_TYPE (DhWindow, dh_window, DH, WINDOW, GtkApplicationWindow)

struct _DhWindowClass {
        GtkApplicationWindowClass parent_class;
};


GtkWidget *dh_window_new         (GtkApplication *application);
void       dh_window_search      (DhWindow       *window,
                                  const gchar    *str);
void       dh_window_display_uri (DhWindow       *window,
                                  const gchar    *uri);

G_END_DECLS

