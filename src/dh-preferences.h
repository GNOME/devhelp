/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2006 Imendio AB
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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __DH_PREFERENCES_H__
#define __DH_PREFERENCES_H__

#include <gtk/gtkwindow.h>

#define GCONF_PATH "/apps/devhelp"

#define GCONF_MAIN_WINDOW_MAXIMIZED "/apps/devhelp/ui/main_window_maximized"
#define GCONF_MAIN_WINDOW_WIDTH     "/apps/devhelp/ui/main_window_width"
#define GCONF_MAIN_WINDOW_HEIGHT    "/apps/devhelp/ui/main_window_height"
#define GCONF_MAIN_WINDOW_POS_X     "/apps/devhelp/ui/main_window_position_x"
#define GCONF_MAIN_WINDOW_POS_Y     "/apps/devhelp/ui/main_window_position_y"
#define GCONF_PANED_LOCATION        "/apps/devhelp/ui/paned_location"
#define GCONF_SELECTED_TAB          "/apps/devhelp/ui/selected_tab"
#define GCONF_USE_SYSTEM_FONTS      "/apps/devhelp/ui/use_system_fonts"
#define GCONF_ADVANCED_OPTIONS      "/apps/devhelp/ui/show_advanced_search_options"
#define GCONF_VARIABLE_FONT         "/apps/devhelp/ui/variable_font"
#define GCONF_FIXED_FONT            "/apps/devhelp/ui/fixed_font"
#define GCONF_SYSTEM_VARIABLE_FONT  "/desktop/gnome/interface/font_name"
#define GCONF_SYSTEM_FIXED_FONT     "/desktop/gnome/interface/monospace_font_name"

void dh_preferences_init        (void);
void dh_preferences_setup_fonts (void);
void dh_preferences_show_dialog (GtkWindow *parent);

#endif /* __DH_PREFERENCES_H__ */

