/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Ricard Hult <rhult@codefactory.se>
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
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

#include <glib-object.h>
#include <gconf/gconf-client.h>

#define TYPE_PREFERENCES		(preferences_get_type ())
#define PREFERENCES(o)  		(G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_PREFERENCES, Preferences))
#define PREFERENCES_CLASS(k)    	(G_TYPE_CHECK_CLASS_CAST ((k), TYPE_PREFERENCES, PreferencesClass))
#define IS_PREFERENCES(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_PREFERENCES))
#define IS_PREFERENCES_CLASS(k) 	(G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_PREFERENCES))
#define PREFERENCES_GET_CLASS(o)        (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_PREFERENCES, PreferencesClass))

typedef struct _Preferences        Preferences;
typedef struct _PreferencesClass   PreferencesClass;
typedef struct _PreferencesPriv    PreferencesPriv;

struct _Preferences {
	GObject          parent;
	
	PreferencesPriv *priv;
};

struct _PreferencesClass {
	GObjectClass     parent_class;
	
	/* Signals */
	
	void (*sidebar_visible_changed)   (Preferences   *prefs,
					   gboolean       visible);
	
	void (*sidebar_position_changed)  (Preferences   *prefs,
					   gint           width);
	
	void (*zoom_level_changed)        (Preferences   *prefs,
					   gint           value);
};

typedef struct {
	const gchar *label;
	gint         data;
} OptionMenuData;

#define ZOOM_TINY_INDEX   0
#define	ZOOM_SMALL_INDEX  1
#define	ZOOM_MEDIUM_INDEX 2
#define	ZOOM_LARGE_INDEX  3
#define	ZOOM_HUGE_INDEX   4

extern const OptionMenuData zoom_levels[];

GType          preferences_get_type             (void);
Preferences *  preferences_new                  (void);
void           preferences_open_dialog          (Preferences   *prefs);

#endif /* __PREFERENCES_H__ */


