/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@codefactory.se>
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

#ifndef __DH_CONTROLLER_H__
#define __DH_CONTROLLER_H__

#include <glib.h>
#include <gtk/gtkobject.h>
#include <gtk/gtktypeutils.h>
#include <bonobo/bonobo-xobject.h>
#include "GNOME_DevHelp.h"
#include "dh-bookshelf.h"

#define DH_TYPE_CONTROLLER		   (dh_controller_get_type ())
#define DH_CONTROLLER(obj)		   (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_CONTROLLER, DhController))
#define DH_CONTROLLER_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_CONTROLLER, DhControllerClass))
#define DH_IS_CONTROLLER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_CONTROLLER))
#define DH_IS_CONTROLLER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_CONTROLLER))

typedef struct _DhController       DhController;
typedef struct _DhControllerClass  DhControllerClass;
typedef struct _DhControllerPriv   DhControllerPriv;

struct _DhController
{
	BonoboXObject            parent;
        
        DhControllerPriv   *priv;
};

struct _DhControllerClass
{
        BonoboXObjectClass             parent_class;
	
	POA_GNOME_DevHelp_Controller__epv   epv;
};

GtkType             dh_controller_get_type    (void);
DhController *      dh_controller_new         ();

#endif /* __DH_CONTROLLER_H__ */
