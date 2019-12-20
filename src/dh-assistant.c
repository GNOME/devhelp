/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * SPDX-FileCopyrightText: 2008 Imendio AB
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dh-assistant.h"
#include <devhelp/devhelp.h>
#include "dh-settings-app.h"
#include "dh-util-app.h"
#include "dh-window.h"

typedef struct {
        GtkWidget *view;
} DhAssistantPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhAssistant, dh_assistant, GTK_TYPE_APPLICATION_WINDOW);

static void
assistant_view_open_uri_cb (DhAssistantView *view,
                            const char      *uri,
                            DhAssistant     *assistant)
{
        DhApp *app;
        DhWindow *window;

        app = DH_APP (gtk_window_get_application (GTK_WINDOW (assistant)));

        window = dh_app_get_active_main_window (app, TRUE);
        _dh_window_display_uri (window, uri);
}

static gboolean
dh_assistant_key_press_event (GtkWidget   *widget,
                              GdkEventKey *event)
{
        DhAssistant *assistant = DH_ASSISTANT (widget);

        if (event->keyval == GDK_KEY_Escape) {
                gtk_window_close (GTK_WINDOW (assistant));
                return GDK_EVENT_STOP;
        }

        return GTK_WIDGET_CLASS (dh_assistant_parent_class)->key_press_event (widget, event);
}

static gboolean
dh_assistant_delete_event (GtkWidget   *widget,
                           GdkEventAny *event)
{
        DhSettingsApp *settings;

        settings = dh_settings_app_get_singleton ();
        dh_util_window_settings_save (GTK_WINDOW (widget),
                                      dh_settings_app_peek_assistant_settings (settings));

        if (GTK_WIDGET_CLASS (dh_assistant_parent_class)->delete_event == NULL)
                return GDK_EVENT_PROPAGATE;

        return GTK_WIDGET_CLASS (dh_assistant_parent_class)->delete_event (widget, event);
}

static void
dh_assistant_class_init (DhAssistantClass *klass)
{
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        widget_class->key_press_event = dh_assistant_key_press_event;
        widget_class->delete_event = dh_assistant_delete_event;

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

        g_signal_connect (priv->view, "open-uri",
                          G_CALLBACK (assistant_view_open_uri_cb),
                          assistant);
}

DhAssistant *
dh_assistant_new (DhApp *application)
{
        DhAssistant *assistant;
        DhSettingsApp *settings;

        assistant = g_object_new (DH_TYPE_ASSISTANT,
                                  "application", application,
                                  NULL);

        settings = dh_settings_app_get_singleton ();
        gtk_widget_realize (GTK_WIDGET (assistant));
        dh_util_window_settings_restore (GTK_WINDOW (assistant),
                                         dh_settings_app_peek_assistant_settings (settings));

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
