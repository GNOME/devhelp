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
        gchar *search_text;
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
        DhWebView *view = DH_WEB_VIEW (object);

        g_free (view->priv->search_text);

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

/*
 * dh_web_view_set_search_text:
 * @view: a #DhWebView.
 * @search_text: (nullable): the search string, or %NULL.
 *
 * A more convenient API (for Devhelp needs) than #WebKitFindController. If
 * @search_text is not empty, it calls webkit_find_controller_search() if not
 * already done. If @search_text is empty or %NULL, it calls
 * webkit_find_controller_search_finish().
 */
void
dh_web_view_set_search_text (DhWebView   *view,
                             const gchar *search_text)
{
        WebKitFindController *find_controller;

        g_return_if_fail (DH_IS_WEB_VIEW (view));

        if (g_strcmp0 (view->priv->search_text, search_text) == 0)
                return;

        g_free (view->priv->search_text);
        view->priv->search_text = g_strdup (search_text);

        find_controller = webkit_web_view_get_find_controller (WEBKIT_WEB_VIEW (view));

        if (search_text != NULL && search_text[0] != '\0') {
                /* If webkit_find_controller_search() is called a second time
                 * with the same parameters it's not a NOP, it launches a new
                 * search, apparently, which screws up search_next() and
                 * search_previous(). So we must call it only once for the same
                 * search string.
                 */
                webkit_find_controller_search (find_controller,
                                               search_text,
                                               WEBKIT_FIND_OPTIONS_WRAP_AROUND |
                                               WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE,
                                               G_MAXUINT);
        } else {
                /* It's fine to call it several times. But unfortunately it
                 * doesn't change the WebKitFindController:text property. So we
                 * must store our own search_text.
                 */
                webkit_find_controller_search_finish (find_controller);
        }
}

void
dh_web_view_search_next (DhWebView *view)
{
        WebKitFindController *find_controller;

        g_return_if_fail (DH_IS_WEB_VIEW (view));

        if (view->priv->search_text == NULL || view->priv->search_text[0] == '\0')
                return;

        find_controller = webkit_web_view_get_find_controller (WEBKIT_WEB_VIEW (view));
        webkit_find_controller_search_next (find_controller);
}

void
dh_web_view_search_previous (DhWebView *view)
{
        WebKitFindController *find_controller;

        g_return_if_fail (DH_IS_WEB_VIEW (view));

        if (view->priv->search_text == NULL || view->priv->search_text[0] == '\0')
                return;

        find_controller = webkit_web_view_get_find_controller (WEBKIT_WEB_VIEW (view));
        webkit_find_controller_search_previous (find_controller);
}
