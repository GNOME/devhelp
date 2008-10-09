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

#include "dh-assistant-view.h"

#include <webkit/webkit.h>

struct _DhAssistantView {
        WebKitWebView      base_instance;
};

struct _DhAssistantViewClass {
        WebKitWebViewClass base_class;
};

G_DEFINE_TYPE (DhAssistantView, dh_assistant_view, WEBKIT_TYPE_WEB_VIEW);

static void
dh_assistant_view_init (DhAssistantView* self)
{}

static void
dh_assistant_view_class_init (DhAssistantViewClass* self_class)
{}

GtkWidget*
dh_assistant_view_new (void)
{
        return g_object_new (DH_TYPE_ASSISTANT_VIEW, NULL);
}


