/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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

#include <config.h>

#include <string.h>

#include "dh-window.h"
#include "dh-profile.h"
#include "dh-base.h"

struct _DhBasePriv {
	GSList *profiles;

	GSList *windows;
};

static void        base_init                  (DhBase         *base);
static void        base_class_init            (DhBaseClass    *klass);
static void        base_new_window_cb         (DhWindow       *window,
					       DhBase         *base);
static void        base_window_finalized_cb   (DhBase         *base,
					       DhWindow       *window);


static GObjectClass  *parent_class;

GType
dh_base_get_type (void)
{
	static GType type = 0;
        
	if (!type) {
		static const GTypeInfo info = {
			sizeof (DhBaseClass),
			NULL,
			NULL,
			(GClassInitFunc) base_class_init,
			NULL,
			NULL,
			sizeof (DhProfile),
			0,
			(GInstanceInitFunc) base_init,
		};
                
		type = g_type_register_static (G_TYPE_OBJECT,
					       "DhBase",
					       &info, 0);
	}

	return type;
}

static void
base_init (DhBase *base)
{
        DhBasePriv *priv;

        priv = g_new0 (DhBasePriv, 1);
        
	priv->profiles = NULL;
	priv->windows  = NULL;
        base->priv     = priv;
}

static void
base_class_init (DhBaseClass *klass)
{
	parent_class = gtk_type_class (G_TYPE_OBJECT);
}

static void
base_new_window_cb (DhWindow *window, DhBase *base)
{
	GtkWidget *new_window;
	
	g_return_if_fail (DH_IS_WINDOW (window));
	g_return_if_fail (DH_IS_BASE (base));
	
	new_window = dh_base_new_window (base, NULL);
	
	gtk_widget_show_all (new_window);
}

static void
base_window_finalized_cb (DhBase *base, DhWindow *window)
{
	DhBasePriv *priv;
	
	g_return_if_fail (DH_IS_BASE (base));
	
	priv = base->priv;
	
	priv->windows = g_slist_remove (priv->windows, window);

	if (g_slist_length (priv->windows) == 0) {
		bonobo_main_quit ();
	}
}

DhBase *
dh_base_new (void)
{
        DhBase     *base;
	DhBasePriv *priv;
	
        base = g_object_new (DH_TYPE_BASE, NULL);
	priv = base->priv;
	
	priv->profiles = dh_profiles_init ();

        return base;
}

GtkWidget *
dh_base_new_window (DhBase *base, DhProfile *profile)
{
	DhBasePriv *priv;
	GtkWidget  *window;
        
        g_return_val_if_fail (DH_IS_BASE (base), NULL);

	priv = base->priv;
        
	/* FIXME: Send in the profile submitted */
        window = dh_window_new (DH_PROFILE (priv->profiles->data));
        
	priv->windows = g_slist_prepend (priv->windows, window);

	g_object_weak_ref (G_OBJECT (window),
			   (GWeakNotify) base_window_finalized_cb,
			   base);
	
	/*g_signal_connect (window, "new_window_requested",
			  G_CALLBACK (base_new_window_cb),
			  base);*/

	gtk_widget_show_all (window);

	return window;
}

GSList *
dh_base_get_windows (DhBase *base)
{
	DhBasePriv          *priv;

	g_return_val_if_fail (DH_IS_BASE (base), NULL);
	
	priv = base->priv;
	
	return priv->windows;
}


