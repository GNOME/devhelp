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

#ifndef __DEVHELP_CONTROLLER_H__
#define __DEVHELP_CONTROLLER_H__

#include <glib.h>
#include <gtk/gtkobject.h>
#include <gtk/gtktypeutils.h>
#include <bonobo/bonobo-xobject.h>
#include "GNOME_DevHelp.h"
#include "bookshelf.h"

#define TYPE_DEVHELP_CONTROLLER		   (devhelp_controller_get_type ())
#define DEVHELP_CONTROLLER(obj)		   (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DEVHELP_CONTROLLER, DevHelpController))
#define DEVHELP_CONTROLLER_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_DEVHELP_CONTROLLER, DevHelpControllerClass))
#define IS_DEVHELP_CONTROLLER(obj)	   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_DEVHELP_CONTROLLER))
#define IS_DEVHELP_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_DEVHELP_CONTROLLER))

typedef struct _DevHelpController       DevHelpController;
typedef struct _DevHelpControllerClass  DevHelpControllerClass;
typedef struct _DevHelpControllerPriv   DevHelpControllerPriv;

struct _DevHelpController
{
	BonoboXObject            parent;
        
        DevHelpControllerPriv   *priv;
};

struct _DevHelpControllerClass
{
        BonoboXObjectClass                  parent_class;
	
	POA_GNOME_DevHelp_Controller__epv   epv;
};

GtkType                  devhelp_controller_get_type    (void);
DevHelpController *      devhelp_controller_new         ();

#endif /* __DEVHELP_CONTROLLER_H__ */
