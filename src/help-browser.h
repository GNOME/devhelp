/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
 * Copyright (C) 2001 Richard Hult <rhult@codefactory.se>
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
 * Author: Richard Hult <rhult@codefactory.se>
 */

#ifndef __HELP_BROWSER_H__
#define __HELP_BROWSER_H__

#include <gtk/gtkobject.h>
#include <gtk/gtktypeutils.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/bonobo-xobject.h>

#include "GNOME_DevHelp.h"

#define HELP_BROWSER_TYPE        (help_browser_get_type ())
#define HELP_BROWSER(o)          (GTK_CHECK_CAST ((o), HELP_BROWSER_TYPE, HelpBrowser))
#define HELP_BROWSER_CLASS(k)    (GTK_CHECK_FOR_CAST((k), HELP_BROWSER_TYPE, HelpBrowserClass))
#define IS_HELP_BROWSER(o)       (GTK_CHECK_TYPE ((o), HELP_BROWSER_TYPE))
#define IS_HELP_BROWSER_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), HELP_BROWSER_TYPE))

typedef struct _HelpBrowser HelpBrowser;

struct _HelpBrowser {
	BonoboXObject        parent;
};

typedef struct {
        BonoboXObjectClass                 parent_class;
        POA_GNOME_DevHelp_HelpBrowser__epv epv;
} HelpBrowserClass;


GtkType         help_browser_get_type (void);
HelpBrowser    *help_browser_new      (void);


#endif /* __HELP_BROWSER_H__ */


