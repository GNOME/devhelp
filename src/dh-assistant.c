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
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "dh-assistant.h"
#include "dh-window.h"
#include "dh-util.h"
#include "dh-assistant-view.h"
#include "dh-settings.h"

typedef struct {
        GtkWidget     *view;
        DhSettings    *settings;
} DhAssistantPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhAssistant, dh_assistant, GTK_TYPE_APPLICATION_WINDOW);

static void
assistant_view_open_uri_cb (DhAssistantView *view,
                            const char      *uri,
                            DhAssistant     *assistant)
{
        DhApp *app;
        GtkWindow *window;

        app = DH_APP (gtk_window_get_application (GTK_WINDOW (assistant)));

        window = dh_app_peek_first_window (app);
        _dh_window_display_uri (DH_WINDOW (window), uri);
}

static gboolean
dh_assistant_key_press_event (GtkWidget   *widget,
                              GdkEventKey *event)
{
        if (event->keyval == GDK_KEY_Escape) {
                gtk_widget_destroy (widget);
                return GDK_EVENT_STOP;
        }

        return GTK_WIDGET_CLASS (dh_assistant_parent_class)->key_press_event (widget, event);
}

static gboolean
dh_assistant_configure_event (GtkWidget         *widget,
                              GdkEventConfigure *event)
{
        DhAssistant *assistant = DH_ASSISTANT (widget);
        DhAssistantPrivate *priv = dh_assistant_get_instance_private (assistant);

        dh_util_window_settings_save (GTK_WINDOW (widget),
                                      dh_settings_peek_assistant_settings (priv->settings),
                                      FALSE);
        return GTK_WIDGET_CLASS (dh_assistant_parent_class)->configure_event (widget, event);
}

static void
dh_assistant_dispose (GObject *object)
{
        DhAssistant *assistant = DH_ASSISTANT (object);
        DhAssistantPrivate *priv = dh_assistant_get_instance_private (assistant);

        g_clear_object (&priv->settings);

        G_OBJECT_CLASS (dh_assistant_parent_class)->dispose (object);
}

static void
dh_assistant_class_init (DhAssistantClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        object_class->dispose = dh_assistant_dispose;

        widget_class->key_press_event = dh_assistant_key_press_event;
        widget_class->configure_event = dh_assistant_configure_event;

        /* Bind class to template */
        gtk_widget_class_set_template_from_resource (widget_class,
                                                     "/org/gnome/devhelp/dh-assistant.ui");
        gtk_widget_class_bind_template_child_private (widget_class, DhAssistant, view);
}

static void
dh_assistant_init (DhAssistant *assistant)
{
        DhAssistantPrivate *priv = dh_assistant_get_instance_private (assistant);

        gtk_widget_init_template (GTK_WIDGET (assistant));

        priv->settings = dh_settings_get ();

        g_signal_connect (priv->view, "open-uri",
                          G_CALLBACK (assistant_view_open_uri_cb),
                          assistant);

        dh_util_window_settings_restore (GTK_WINDOW (assistant),
                                         dh_settings_peek_assistant_settings (priv->settings),
                                         FALSE);
}

GtkWidget *
dh_assistant_new (DhApp *application)
{
        GtkWidget          *assistant;
        DhAssistantPrivate *priv;

        assistant = g_object_new (DH_TYPE_ASSISTANT, "application", application, NULL);

        priv = dh_assistant_get_instance_private (DH_ASSISTANT (assistant));
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
