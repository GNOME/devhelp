/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Ricard Hult <rhult@codefactory.se>
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
 *
 * Author: Ricard Hult <rhult@codefactory.se>
 */

#ifndef __PREFERENCES_H__
#define __PREFERENCES_H__

#include <gconf/gconf-client.h>

/* Work around circular includes. */
typedef struct _Preferences Preferences;

#include "main.h"

typedef struct {
	const gchar *label;
	gint         data;
} OptionMenuData;

#define ZOOM_TINY_INDEX   0
#define	ZOOM_SMALL_INDEX  1
#define	ZOOM_MEDIUM_INDEX 2
#define	ZOOM_LARGE_INDEX  3
#define	ZOOM_HUGE_INDEX   4

#define AUTOCOMP_SLOW_INDEX   0
#define AUTOCOMP_MEDIUM_INDEX 1
#define AUTOCOMP_FAST_INDEX   2

extern const OptionMenuData zoom_levels[];
extern const OptionMenuData autocompletion_speeds[];

Preferences *preferences_new (DevHelp *devhelp);
void         preferences_set_sidebar_visible (Preferences *preferences, gboolean value);
gboolean     preferences_get_sidebar_visible (Preferences *preferences);
void         preferences_set_zoom_level (Preferences *preferences, gint value);
gint         preferences_get_zoom_level (Preferences *preferences);
void         preferences_set_sidebar_position (Preferences *preferences, gint width);
gint         preferences_get_sidebar_position (Preferences *preferences);
void         preferences_set_autocompletion (Preferences *preferences, gboolean value);
gboolean     preferences_get_autocompletion (Preferences *preferences);
void         preferences_set_autocompletion_speed (Preferences *preferences, gint value);
gint         preferences_get_autocompletion_speed (Preferences *preferences);


#endif /* __PREFERENCES_H__ */


