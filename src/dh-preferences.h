/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2008 Imendio AB
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DH_CONF_PATH "/apps/devhelp"
#define DH_CONF_USE_SYSTEM_FONTS      DH_CONF_PATH "/ui/use_system_fonts"
#define DH_CONF_VARIABLE_FONT         DH_CONF_PATH "/ui/variable_font"
#define DH_CONF_FIXED_FONT            DH_CONF_PATH "/ui/fixed_font"
#define DH_CONF_SYSTEM_VARIABLE_FONT  "/desktop/gnome/interface/font_name"
#define DH_CONF_SYSTEM_FIXED_FONT     "/desktop/gnome/interface/monospace_font_name"

void dh_preferences_setup_fonts (void);
void dh_preferences_show_dialog (GtkWindow *parent);

G_END_DECLS

#endif /* __DH_PREFERENCES_H__ */

