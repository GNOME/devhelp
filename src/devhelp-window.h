/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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
 * Author: Mikael Hallendal <micke@codefactory.se>
 */

#ifndef __DEVHELP_WINDOW_H__
#define __DEVHELP_WINDOW_H__

#include <glib-object.h>
#include <bonobo/bonobo-window.h>
#include <gtk/gtktypeutils.h>
#include <gtk/gtkwidget.h>

#define TYPE_DEVHELP_WINDOW		(devhelp_window_get_type ())
#define DEVHELP_WINDOW(obj)		(GTK_CHECK_CAST ((obj), TYPE_DEVHELP_WINDOW, DevHelpWindow))
#define DEVHELP_WINDOW_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), TYPE_DEVHELP_WINDOW, DevHelpWindowClass))
#define IS_DEVHELP_WINDOW(obj)		(GTK_CHECK_TYPE ((obj), TYPE_DEVHELP_WINDOW))
#define IS_DEVHELP_WINDOW_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), TYPE_DEVHELP_WINDOW))

typedef struct _DevHelpWindow       DevHelpWindow;
typedef struct _DevHelpWindowClass  DevHelpWindowClass;
typedef struct _DevHelpWindowPriv   DevHelpWindowPriv;

struct _DevHelpWindow
{
        BonoboWindow         parent;
        
        DevHelpWindowPriv   *priv;
};

struct _DevHelpWindowClass
{
        BonoboWindowClass    parent_class;

        /* Signals */
};

GtkType          devhelp_window_get_type        (void);
GtkWidget *      devhelp_window_new             ();

void             devhelp_window_search          (DevHelpWindow   *window,
						 const gchar     *str);

#endif /* __DEVHELP_WINDOW_H__ */
