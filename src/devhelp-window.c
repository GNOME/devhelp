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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "devhelp-window.h"
static void devhelp_window_class_init          (DevHelpWindowClass   *klass);
static void devhelp_window_init                (DevHelpWindow        *index);
 
static void devhelp_window_destroy             (GtkObject            *object);

static GtkObjectClass *parent_class = NULL;

struct _DevHelpWindowPriv {
        GtkWidget   *index;
        GtkWidget   *search_list;
        GtkWidget   *search_entry;
        
};

GtkType
devhelp_window_get_type (void)
{
        static GtkType devhelp_window_type = 0;

        if (!devhelp_window_type) {
                static const GtkTypeInfo devhelp_window_info = {
                        "DevHelpWindow",
                        sizeof (DevHelpWindow),
                        sizeof (DevHelpWindowClass),
                        (GtkClassInitFunc)  devhelp_window_class_init,
                        (GtkObjectInitFunc) devhelp_window_init,
                        /* reserved_1 */ NULL,
                        /* reserved_2 */ NULL,
                        (GtkClassInitFunc) NULL,
                };

                devhelp_window_type = gtk_type_unique (bonobo_window_get_type (), 
                                                       &devhelp_window_info);
        }

        return devhelp_window_type;
}

static void
devhelp_window_class_init (DevHelpWindowClass *klass)
{
        GtkObjectClass   *object_class;
        
        parent_class = gtk_type_class (bonobo_window_get_type ());

        object_class = (GtkObjectClass *) klass;
        
        object_class->destroy = devhelp_window_destroy;
}

static void
devhelp_window_init (DevHelpWindow *window)
{
        DevHelpWindowPriv   *priv;

        priv = g_new0 (DevHelpWindowPriv, 1);
        
        window->priv = priv;
}

static void
devhelp_window_destroy (GtkObject *object)
{
        
}

GtkWidget *
devhelp_window_new (void)
{
        DevHelpWindow   *window;
        GtkWidget       *widget;

        window = gtk_type_new (TYPE_DEVHELP_WINDOW);
        
        widget = bonobo_window_construct (BONOBO_WINDOW (window), 
                                          "DevHelp", "DevHelp");

        gtk_window_set_policy (GTK_WINDOW (widget), TRUE, TRUE, FALSE);

        gtk_window_set_default_size (GTK_WINDOW (widget), 700, 500);

        gtk_widget_show_all (widget);
        
        return widget;
}

