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

#include "dh-web-view.h"

struct _DhWebViewPrivate {
        gint something;
};

G_DEFINE_TYPE_WITH_PRIVATE (DhWebView, dh_web_view, WEBKIT_TYPE_WEB_VIEW)

static void
dh_web_view_constructed (GObject *object)
{
        WebKitWebView *view = WEBKIT_WEB_VIEW (object);
        WebKitSettings *settings;

        if (G_OBJECT_CLASS (dh_web_view_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (dh_web_view_parent_class)->constructed (object);

        /* Disable some things we have no need for. */
        settings = webkit_web_view_get_settings (view);
        webkit_settings_set_enable_html5_database (settings, FALSE);
        webkit_settings_set_enable_html5_local_storage (settings, FALSE);
        webkit_settings_set_enable_plugins (settings, FALSE);
}

static void
dh_web_view_finalize (GObject *object)
{

        G_OBJECT_CLASS (dh_web_view_parent_class)->finalize (object);
}

static void
dh_web_view_class_init (DhWebViewClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->constructed = dh_web_view_constructed;
        object_class->finalize = dh_web_view_finalize;
}

static void
dh_web_view_init (DhWebView *view)
{
        view->priv = dh_web_view_get_instance_private (view);
}

DhWebView *
dh_web_view_new (void)
{
        return g_object_new (DH_TYPE_WEB_VIEW, NULL);
}
