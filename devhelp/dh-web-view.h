/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
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
#include <devhelp/dh-profile.h>

G_BEGIN_DECLS

#define DH_TYPE_WEB_VIEW             (dh_web_view_get_type ())
G_DECLARE_DERIVABLE_TYPE (DhWebView, dh_web_view, DH, WEB_VIEW, WebKitWebView)

struct _DhWebViewClass {
        WebKitWebViewClass parent_class;

        /* Signals */
        void    (* open_new_tab)        (DhWebView   *view,
                                         const gchar *uri);

        /* Padding for future expansion */
        gpointer padding[12];
};

DhWebView   *dh_web_view_new               (DhProfile   *profile);
DhProfile   *dh_web_view_get_profile       (DhWebView   *view);
const gchar *dh_web_view_get_devhelp_title (DhWebView   *view);
void         dh_web_view_set_search_text   (DhWebView   *view,
                                            const gchar *search_text);
void         dh_web_view_search_next       (DhWebView   *view);
void         dh_web_view_search_previous   (DhWebView   *view);
gboolean     dh_web_view_can_zoom_in       (DhWebView   *view);
gboolean     dh_web_view_can_zoom_out      (DhWebView   *view);
gboolean     dh_web_view_can_reset_zoom    (DhWebView   *view);
void         dh_web_view_zoom_in           (DhWebView   *view);
void         dh_web_view_zoom_out          (DhWebView   *view);
void         dh_web_view_reset_zoom        (DhWebView   *view);

G_END_DECLS

