/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004-2008 Imendio AB
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
 */

#include "config.h"
#include <string.h>
#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <unistd.h>
#include <gdk/gdkx.h>
#endif

#include "dh-window.h"
#include "dh-link.h"
#include "dh-parser.h"
#include "dh-preferences.h"
#include "dh-assistant.h"
#include "dh-util.h"
#include "ige-conf.h"
#include "dh-base.h"
#include "dh-book-manager.h"

typedef struct {
        GSList        *windows;
        GSList        *assistants;
        DhBookManager *book_manager;
} DhBasePriv;

G_DEFINE_TYPE (DhBase, dh_base, G_TYPE_OBJECT);

#define GET_PRIVATE(instance) G_TYPE_INSTANCE_GET_PRIVATE \
  (instance, DH_TYPE_BASE, DhBasePriv)

static void dh_base_init           (DhBase      *base);
static void dh_base_class_init     (DhBaseClass *klass);

static DhBase *base_instance;

static void
base_finalize (GObject *object)
{
        G_OBJECT_CLASS (dh_base_parent_class)->finalize (object);
}

static void
base_dispose (GObject *object)
{
        DhBasePriv *priv;

        priv = GET_PRIVATE (object);

        if (priv->book_manager) {
                g_object_unref (priv->book_manager);
                priv->book_manager = NULL;
        }
}


static void
dh_base_class_init (DhBaseClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = base_finalize;
        object_class->dispose = base_dispose;

	g_type_class_add_private (klass, sizeof (DhBasePriv));
}

static void
dh_base_init (DhBase *base)
{
        DhBasePriv *priv = GET_PRIVATE (base);
        IgeConf    *conf;
        gchar      *path;

        conf = ige_conf_get ();
        path = dh_util_build_data_filename ("devhelp", "devhelp.defaults", NULL);
        ige_conf_add_defaults (conf, path);
        g_free (path);

        priv->book_manager = dh_book_manager_new ();
        dh_book_manager_populate (priv->book_manager);
}

static void
base_window_or_assistant_finalized_cb (DhBase   *base,
                                       gpointer  window_or_assistant)
{
        DhBasePriv *priv = GET_PRIVATE (base);

        priv->windows = g_slist_remove (priv->windows, window_or_assistant);
        priv->assistants = g_slist_remove (priv->assistants, window_or_assistant);

        if (priv->windows == NULL && priv->assistants == NULL) {
                gtk_main_quit ();
        }
}

DhBase *
dh_base_get (void)
{
        if (!base_instance) {
                base_instance = g_object_new (DH_TYPE_BASE, NULL);
        }

        return base_instance;
}

DhBase *
dh_base_new (void)
{
        if (base_instance) {
                g_error ("You can only have one DhBase instance.");
        }

        return dh_base_get ();
}

GtkWidget *
dh_base_new_window (DhBase *base)
{
        DhBasePriv *priv;
        GtkWidget  *window;

        g_return_val_if_fail (DH_IS_BASE (base), NULL);

        priv = GET_PRIVATE (base);

        window = dh_window_new (base);

        priv->windows = g_slist_prepend (priv->windows, window);

        g_object_weak_ref (G_OBJECT (window),
                           (GWeakNotify) base_window_or_assistant_finalized_cb,
                           base);

        return window;
}

GtkWidget *
dh_base_new_assistant (DhBase *base)
{
        DhBasePriv *priv;
        GtkWidget  *assistant;

        g_return_val_if_fail (DH_IS_BASE (base), NULL);

        priv = GET_PRIVATE (base);

        assistant = dh_assistant_new (base);

        priv->assistants = g_slist_prepend (priv->assistants, assistant);

        g_object_weak_ref (G_OBJECT (assistant),
                           (GWeakNotify) base_window_or_assistant_finalized_cb,
                           base);

        return assistant;
}

DhBookManager *
dh_base_get_book_manager (DhBase *base)
{
        DhBasePriv *priv;

        g_return_val_if_fail (DH_IS_BASE (base), NULL);

        priv = GET_PRIVATE (base);

        return priv->book_manager;
}

GtkWidget *
dh_base_get_window_on_current_workspace (DhBase *base)
{
        DhBasePriv *priv;

        g_return_val_if_fail (DH_IS_BASE (base), NULL);

        priv = GET_PRIVATE (base);

        if (!priv->windows) {
                return NULL;
        }
        return priv->windows->data;
}

GtkWidget *
dh_base_get_window (DhBase *base)
{
        GtkWidget *window;

        g_return_val_if_fail (DH_IS_BASE (base), NULL);

        window = dh_base_get_window_on_current_workspace (base);
        if (!window) {
                window = dh_base_new_window (base);
                gtk_window_present (GTK_WINDOW (window));
        }

        return window;
}

void
dh_base_quit (DhBase *base)
{
        DhBasePriv *priv = GET_PRIVATE (base);

        /* Make sure all of the windows get a chance to release their resources
         * properly.  As they get destroyed,
         * base_window_or_assistant_finalized_cb() will be called, and when the
         * last one is removed, we will quit */
        g_slist_foreach (priv->windows, (GFunc)gtk_widget_destroy, NULL);
        g_slist_foreach (priv->assistants, (GFunc)gtk_widget_destroy, NULL);
}
