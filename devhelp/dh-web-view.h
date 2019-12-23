/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_WEB_VIEW_H
#define DH_WEB_VIEW_H

#include <webkit2/webkit2.h>
#include <devhelp/dh-profile.h>

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

        /* Signals */
        void    (* open_new_tab)        (DhWebView   *view,
                                         const gchar *uri);

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_web_view_get_type            (void);

DhWebView *     dh_web_view_new                 (DhProfile *profile);

DhProfile *     dh_web_view_get_profile         (DhWebView *view);

const gchar *   dh_web_view_get_devhelp_title   (DhWebView *view);

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
