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

#include "dh-tab.h"
#include <glib/gi18n.h>
#include "tepl-info-bar.h"

struct _DhTabPrivate {
        TeplInfoBar *info_bar;
        DhWebView *web_view;
};

G_DEFINE_TYPE_WITH_PRIVATE (DhTab, dh_tab, GTK_TYPE_GRID)

static void
dh_tab_dispose (GObject *object)
{
        DhTab *tab = DH_TAB (object);

        tab->priv->info_bar = NULL;
        tab->priv->web_view = NULL;

        G_OBJECT_CLASS (dh_tab_parent_class)->dispose (object);
}

static void
dh_tab_class_init (DhTabClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_tab_dispose;
}

static gboolean
web_view_load_failed_cb (WebKitWebView   *web_view,
                         WebKitLoadEvent  load_event,
                         const gchar     *failing_uri,
                         GError          *error,
                         DhTab           *tab)
{
        /* Ignore cancellation errors, which happen when typing fast in the
         * search entry.
         */
        if (g_error_matches (error, WEBKIT_NETWORK_ERROR, WEBKIT_NETWORK_ERROR_CANCELLED))
                return GDK_EVENT_STOP;

        if (tab->priv->info_bar != NULL)
                gtk_widget_destroy (GTK_WIDGET (tab->priv->info_bar));

        tab->priv->info_bar = tepl_info_bar_new_simple (GTK_MESSAGE_ERROR,
                                                        _("Error opening the requested link."),
                                                        error->message);
        tepl_info_bar_add_close_button (tab->priv->info_bar);

        g_signal_connect (tab->priv->info_bar,
                          "destroy",
                          G_CALLBACK (gtk_widget_destroyed),
                          &tab->priv->info_bar);

        gtk_grid_attach_next_to (GTK_GRID (tab),
                                 GTK_WIDGET (tab->priv->info_bar),
                                 GTK_WIDGET (tab->priv->web_view),
                                 GTK_POS_TOP,
                                 1, 1);

        gtk_widget_show (GTK_WIDGET (tab->priv->info_bar));

        return GDK_EVENT_STOP;
}

static void
web_view_load_changed_cb (WebKitWebView   *web_view,
                          WebKitLoadEvent  load_event,
                          DhTab           *tab)
{
        if (load_event == WEBKIT_LOAD_STARTED &&
            tab->priv->info_bar != NULL) {
                /* The error is no longer relevant. */
                gtk_widget_destroy (GTK_WIDGET (tab->priv->info_bar));
        }
}

static void
dh_tab_init (DhTab *tab)
{
        tab->priv = dh_tab_get_instance_private (tab);

        gtk_orientable_set_orientation (GTK_ORIENTABLE (tab), GTK_ORIENTATION_VERTICAL);

        tab->priv->web_view = dh_web_view_new ();
        gtk_widget_show (GTK_WIDGET (tab->priv->web_view));
        gtk_container_add (GTK_CONTAINER (tab), GTK_WIDGET (tab->priv->web_view));

        g_signal_connect (tab->priv->web_view,
                          "load-failed",
                          G_CALLBACK (web_view_load_failed_cb),
                          tab);

        g_signal_connect (tab->priv->web_view,
                          "load-changed",
                          G_CALLBACK (web_view_load_changed_cb),
                          tab);
}

DhTab *
dh_tab_new (void)
{
        return g_object_new (DH_TYPE_TAB, NULL);
}

DhWebView *
dh_tab_get_web_view (DhTab *tab)
{
        g_return_val_if_fail (DH_IS_TAB (tab), NULL);

        return tab->priv->web_view;
}
