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

struct _DhTabPrivate {
        GtkInfoBar *info_bar;
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

static void
dh_tab_init (DhTab *tab)
{
        tab->priv = dh_tab_get_instance_private (tab);

        gtk_orientable_set_orientation (GTK_ORIENTABLE (tab), GTK_ORIENTATION_VERTICAL);

        /* GtkInfoBar */
        tab->priv->info_bar = GTK_INFO_BAR (gtk_info_bar_new ());
        gtk_widget_set_no_show_all (GTK_WIDGET (tab->priv->info_bar), TRUE);
        gtk_info_bar_set_show_close_button (tab->priv->info_bar, TRUE);
        gtk_info_bar_set_message_type (tab->priv->info_bar, GTK_MESSAGE_ERROR);

        g_signal_connect (tab->priv->info_bar,
                          "response",
                          G_CALLBACK (gtk_widget_hide),
                          NULL);

        /* DhWebView */
        tab->priv->web_view = dh_web_view_new ();
        gtk_widget_show (GTK_WIDGET (tab->priv->web_view));

        gtk_container_add (GTK_CONTAINER (tab), GTK_WIDGET (tab->priv->info_bar));
        gtk_container_add (GTK_CONTAINER (tab), GTK_WIDGET (tab->priv->web_view));
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

GtkInfoBar *
dh_tab_get_info_bar (DhTab *tab)
{
        g_return_val_if_fail (DH_IS_TAB (tab), NULL);

        return tab->priv->info_bar;
}
