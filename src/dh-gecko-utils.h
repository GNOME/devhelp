/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio AB
 * Copyright (C) 2004 Marco Pesenti Gritti
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
 */

#ifndef __DH_GECKO_UTILS_H__
#define __DH_GECKO_UTILS_H__

#include <gtkmozembed.h>

G_BEGIN_DECLS

enum {
	DH_GECKO_PREF_FONT_VARIABLE,
	DH_GECKO_PREF_FONT_FIXED
};

void dh_gecko_utils_set_font                  (gint         font_type,
					       const gchar *fontname);
void dh_gecko_utils_init                      (void);
void dh_gecko_utils_shutdown                  (void);
gint dh_gecko_utils_get_mouse_event_button    (gpointer     event);
gint dh_gecko_utils_get_mouse_event_modifiers (gpointer     event);
void dh_gecko_utils_copy_selection            (GtkMozEmbed *embed);

G_END_DECLS

#endif /* __DH_HTML_H__ */

