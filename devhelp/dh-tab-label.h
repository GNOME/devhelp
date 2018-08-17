/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
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
#include <devhelp/dh-tab.h>

G_BEGIN_DECLS

#define DH_TYPE_TAB_LABEL             (dh_tab_label_get_type ())
G_DECLARE_FINAL_TYPE (DhTabLabel, dh_tab_label, DH, TAB_LABEL, GtkGrid)

GtkWidget *dh_tab_label_new     (DhTab      *tab);
DhTab     *dh_tab_label_get_tab (DhTabLabel *tab_label);

G_END_DECLS

