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

#ifndef __PREFERENCES_DIALOG_H__
#define __PREFERENCES_DIALOG_H__

#include <gtk/gtkwidget.h>
#include "preferences.h"

typedef struct _PreferencesDialog PreferencesDialog;

#define PREFERENCES_DIALOG(x) ((PreferencesDialog *)(x))

GtkWidget * preferences_dialog_new (Preferences   *prefs);

#endif /* __PREFERENCES_DIALOG_H__ */
