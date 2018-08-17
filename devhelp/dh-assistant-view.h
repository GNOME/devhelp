/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2008 Sven Herzberg
 *
 * Devhelp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Devhelp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Devhelp.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <webkit2/webkit2.h>
#include <devhelp/dh-link.h>

G_BEGIN_DECLS

#define DH_TYPE_ASSISTANT_VIEW         (dh_assistant_view_get_type ())
G_DECLARE_DERIVABLE_TYPE (DhAssistantView, dh_assistant_view, DH, ASSISTANT_VIEW, WebKitWebView)

struct _DhAssistantViewClass {
        WebKitWebViewClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GtkWidget *dh_assistant_view_new      (void);
gboolean   dh_assistant_view_set_link (DhAssistantView *view,
                                       DhLink          *link);
gboolean   dh_assistant_view_search   (DhAssistantView *view,
                                       const gchar     *str);
G_END_DECLS

