/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* This file is part of Devhelp
 *
 * AUTHORS
 *     Sven Herzberg  <sven@imendio.com>
 *
 * Copyright (C) 2008  Sven Herzberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <string.h>
#include <webkit/webkit.h>

#include "dh-assistant-view.h"
#include "dh-window.h"

struct _DhAssistantView {
        WebKitWebView      base_instance;
        /* private - move to a private structure before publishing this struct */
        DhBase            *base;
        DhLink            *link;
};

struct _DhAssistantViewClass {
        WebKitWebViewClass base_class;
};

G_DEFINE_TYPE (DhAssistantView, dh_assistant_view, WEBKIT_TYPE_WEB_VIEW);

static void
dh_assistant_view_init (DhAssistantView* self)
{}

static void
view_finalize (GObject *object)
{
        DhAssistantView* self = (DhAssistantView*) object;

        if (self->link) {
                g_object_unref (self->link);
        }

        if (self->base) {
                g_object_unref (self->base);
        }

        G_OBJECT_CLASS (dh_assistant_view_parent_class)->finalize (object);
}

static WebKitNavigationResponse
assistant_navigation_requested_cb (WebKitWebView        *web_view,
                                   WebKitWebFrame       *frame,
                                   WebKitNetworkRequest *request)
{
        DhAssistantView *self;
        const gchar     *uri;

        self = DH_ASSISTANT_VIEW (web_view);

        uri = webkit_network_request_get_uri (request);

        if (strcmp (uri, "about:blank") == 0) {
                return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;
        }

        if (g_str_has_prefix (uri, "file://")) {
                GtkWidget *window;

                window = dh_base_get_window (self->base);
                _dh_window_display_uri (DH_WINDOW (window), uri);
        }

        return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}

static gboolean
assistant_button_press_event_cb (GtkWidget      *widget,
                                 GdkEventButton *event)
{
        /* Block webkit's builtin context menu. */
        if (event->button != 1) {
                return TRUE;
        }

        return FALSE;
}

static void
dh_assistant_view_class_init (DhAssistantViewClass* self_class)
{
        GObjectClass       *object_class = G_OBJECT_CLASS (self_class);
        GtkWidgetClass     *widget_class = GTK_WIDGET_CLASS (self_class);
        WebKitWebViewClass *web_view_class = WEBKIT_WEB_VIEW_CLASS (self_class);

        object_class->finalize = view_finalize;

        widget_class->button_press_event = assistant_button_press_event_cb;

        web_view_class->navigation_requested = assistant_navigation_requested_cb;
}

DhBase*
dh_assistant_view_get_base (DhAssistantView *self)
{
        g_return_val_if_fail (DH_IS_ASSISTANT_VIEW (self), NULL);

        return self->base;
}

DhLink*
dh_assistant_view_get_link (DhAssistantView *self)
{
        g_return_val_if_fail (DH_IS_ASSISTANT_VIEW (self), NULL);

        return self->link;
}

GtkWidget*
dh_assistant_view_new (void)
{
        return g_object_new (DH_TYPE_ASSISTANT_VIEW, NULL);
}

void
dh_assistant_view_set_base (DhAssistantView *view,
                            DhBase          *base)
{
        g_return_if_fail (DH_IS_ASSISTANT_VIEW (view));
        g_return_if_fail (DH_IS_BASE (base));

        view->base = g_object_ref (base);
}

void
dh_assistant_view_set_link (DhAssistantView *self,
                            DhLink          *link)
{
        g_return_if_fail (DH_IS_ASSISTANT_VIEW (self));

        if (self->link == link) {
                return;
        }

        if (self->link) {
                dh_link_unref (self->link);
                self->link = NULL;
        }

        if (link) {
                link = dh_link_ref (link);
        }
}

