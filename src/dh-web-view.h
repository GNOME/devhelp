/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DH_WEB_VIEW_H
#define DH_WEB_VIEW_H

#include <webkit2/webkit2.h>

G_BEGIN_DECLS

#define DH_TYPE_WEB_VIEW             (dh_web_view_get_type ())
#define DH_WEB_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_WEB_VIEW, DhWebView))
#define DH_WEB_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_WEB_VIEW, DhWebViewClass))
#define DH_IS_WEB_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_WEB_VIEW))
#define DH_IS_WEB_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_WEB_VIEW))
#define DH_WEB_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_WEB_VIEW, DhWebViewClass))

typedef struct _DhWebView         DhWebView;
typedef struct _DhWebViewClass    DhWebViewClass;
typedef struct _DhWebViewPrivate  DhWebViewPrivate;

struct _DhWebView {
        WebKitWebView parent;

        DhWebViewPrivate *priv;
};

struct _DhWebViewClass {
        WebKitWebViewClass parent_class;

	/* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_web_view_get_type            (void);

DhWebView *     dh_web_view_new                 (void);

void            dh_web_view_set_search_text     (DhWebView   *view,
                                                 const gchar *search_text);

void            dh_web_view_search_next         (DhWebView *view);

void            dh_web_view_search_previous     (DhWebView *view);

gboolean        dh_web_view_can_zoom_in         (DhWebView *view);

gboolean        dh_web_view_can_zoom_out        (DhWebView *view);

gboolean        dh_web_view_can_reset_zoom      (DhWebView *view);

void            dh_web_view_zoom_in             (DhWebView *view);

void            dh_web_view_zoom_out            (DhWebView *view);

void            dh_web_view_reset_zoom          (DhWebView *view);

G_END_DECLS

#endif /* DH_WEB_VIEW_H */
