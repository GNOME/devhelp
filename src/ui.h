/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
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
 * Author: Johan Dahlin <zilch.am@home.se>
 */

#ifndef __UI_H__
#define __UI_H__

#include "main.h"
#include "book-node.h"

DevHelp*          devhelp_create_ui             (void);

void              devhelp_search                (DevHelp          *devhelp, 
						 const gchar      *string);
 
void              devhelp_ui_show_sidebar       (DevHelp          *devhelp,
						 gboolean          show);

void              devhelp_ui_set_zoom_level     (DevHelp          *devhelp,
						 gint              level);

void              devhelp_ui_set_autocompletion (DevHelp          *devhelp,
						 gboolean          value);

/* FIXME: Rename. */
void              gtk_clist_set_contents        (GtkCList         *clist, 
						 GSList           *list);

gboolean          gtk_clist_if_exact_go_there   (GtkCList         *clist, 
						 const gchar      *filename);

#endif /* __UI_H__ */

