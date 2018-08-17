/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
 * Copyright (C) 2017 Sébastien Wilmet <swilmet@gnome.org>
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
#include "dh-window.h"

G_BEGIN_DECLS

#define DH_TYPE_APP         (dh_app_get_type ())

G_DECLARE_DERIVABLE_TYPE (DhApp, dh_app, DH, APP, GtkApplication)

struct _DhAppClass {
        GtkApplicationClass parent_class;
};

DhApp    *dh_app_new                    (void);
DhWindow *dh_app_get_active_main_window (DhApp    *app,
                                         gboolean  create_if_none);

G_END_DECLS
