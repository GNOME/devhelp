/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Imendio AB
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
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "dh-window.h"
#include "dh-util.h"
#include "dh-assistant-view.h"
#include "dh-assistant.h"
#include "dh-settings.h"

typedef struct {
        DhApp         *application;
        GtkWidget     *main_box;
        GtkWidget     *view;
        DhSettings    *settings;
} DhAssistantPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhAssistant, dh_assistant, GTK_TYPE_APPLICATION_WINDOW);

static gboolean
assistant_key_press_event_cb (GtkWidget   *widget,
                              GdkEventKey *event,
                              DhAssistant *assistant)
{
        if (event->keyval == GDK_KEY_Escape) {
                gtk_widget_destroy (GTK_WIDGET (assistant));
                return TRUE;
        }

        return FALSE;
}

static void
assistant_view_open_uri_cb (DhAssistantView *view,
                            const char      *uri,
                            DhAssistant     *assistant)
{
        DhAssistantPrivate *priv;
        GtkWindow* window;

        priv = dh_assistant_get_instance_private (assistant);
        window = dh_app_peek_first_window (priv->application);
        _dh_window_display_uri (DH_WINDOW (window), uri);
}

static gboolean
window_configure_event_cb (GtkWidget         *window,
                           GdkEventConfigure *event,
                           DhAssistant       *assistant)
{
        DhAssistantPrivate *priv = dh_assistant_get_instance_private (assistant);

        dh_util_window_settings_save (GTK_WINDOW (assistant),
                                      dh_settings_peek_assistant_settings (priv->settings),
                                      FALSE);
        return FALSE;
}

static void
dh_assistant_dispose (GObject *object)
{
        DhAssistant *assistant = DH_ASSISTANT (object);
        DhAssistantPrivate *priv = dh_assistant_get_instance_private (assistant);

        g_clear_object (&priv->application);
        g_clear_object (&priv->settings);

        G_OBJECT_CLASS (dh_assistant_parent_class)->dispose (object);
}

static void
dh_assistant_class_init (DhAssistantClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_assistant_dispose;
}

static void
dh_assistant_init (DhAssistant *assistant)
{
        DhAssistantPrivate *priv = dh_assistant_get_instance_private (assistant);

        priv->settings = dh_settings_get ();
        priv->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_show (priv->main_box);
        gtk_container_add (GTK_CONTAINER (assistant), priv->main_box);

        /* i18n: Please don't translate "Devhelp". */
        gtk_window_set_title (GTK_WINDOW (assistant), _("Devhelp — Assistant"));
        gtk_window_set_icon_name (GTK_WINDOW (assistant), "devhelp");

        priv->view = dh_assistant_view_new ();

        g_signal_connect (priv->view, "open-uri",
                          G_CALLBACK (assistant_view_open_uri_cb),
                          assistant);

        g_signal_connect (assistant, "key-press-event",
                          G_CALLBACK (assistant_key_press_event_cb),
                          assistant);

        gtk_box_pack_start (GTK_BOX (priv->main_box),
                            priv->view, TRUE, TRUE, 0);
        gtk_widget_show (priv->view);

        dh_util_window_settings_restore (GTK_WINDOW (assistant),
                                         dh_settings_peek_assistant_settings (priv->settings),
                                         FALSE);

        g_signal_connect (GTK_WINDOW (assistant), "configure-event",
                          G_CALLBACK (window_configure_event_cb),
                          assistant);
}

GtkWidget *
dh_assistant_new (DhApp *application)
{
        GtkWidget          *assistant;
        DhAssistantPrivate *priv;

        assistant = g_object_new (DH_TYPE_ASSISTANT, NULL);

        priv = dh_assistant_get_instance_private (DH_ASSISTANT (assistant));
        priv->application = g_object_ref (application);

        dh_assistant_view_set_book_manager (DH_ASSISTANT_VIEW (priv->view),
                                            dh_app_peek_book_manager (application));

        return assistant;
}

gboolean
dh_assistant_search (DhAssistant *assistant,
                     const gchar *str)
{
        DhAssistantPrivate *priv;

        g_return_val_if_fail (DH_IS_ASSISTANT (assistant), FALSE);
        g_return_val_if_fail (str != NULL, FALSE);

        priv = dh_assistant_get_instance_private (assistant);

        if (dh_assistant_view_search (DH_ASSISTANT_VIEW (priv->view), str)) {
                gtk_widget_show (GTK_WIDGET (assistant));
                return TRUE;
        }

        return FALSE;
}
