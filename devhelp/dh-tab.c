/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dh-tab.h"

/**
 * SECTION:dh-tab
 * @Title: DhTab
 * @Short_description: Subclass of #GtkGrid containing a #DhWebView
 *
 * #DhTab is a subclass of #GtkGrid, it is meant to be the content of one tab in
 * a #DhNotebook. It contains by default only one element: a #DhWebView. But
 * applications can add more widgets to the #GtkGrid.
 */

struct _DhTabPrivate {
        DhWebView *web_view;
};

enum {
        PROP_0,
        PROP_WEB_VIEW,
        N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

G_DEFINE_TYPE_WITH_PRIVATE (DhTab, dh_tab, GTK_TYPE_GRID)

static void
set_web_view (DhTab     *tab,
              DhWebView *web_view)
{
        if (web_view == NULL)
                return;

        g_return_if_fail (DH_IS_WEB_VIEW (web_view));

        g_assert (tab->priv->web_view == NULL);
        tab->priv->web_view = g_object_ref_sink (web_view);

        gtk_container_add (GTK_CONTAINER (tab), GTK_WIDGET (tab->priv->web_view));
}

static void
dh_tab_get_property (GObject    *object,
                     guint       prop_id,
                     GValue     *value,
                     GParamSpec *pspec)
{
        DhTab *tab = DH_TAB (object);

        switch (prop_id) {
                case PROP_WEB_VIEW:
                        g_value_set_object (value, dh_tab_get_web_view (tab));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_tab_set_property (GObject      *object,
                     guint         prop_id,
                     const GValue *value,
                     GParamSpec   *pspec)
{
        DhTab *tab = DH_TAB (object);

        switch (prop_id) {
                case PROP_WEB_VIEW:
                        set_web_view (tab, g_value_get_object (value));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_tab_constructed (GObject *object)
{
        DhTab *tab = DH_TAB (object);

        if (G_OBJECT_CLASS (dh_tab_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (dh_tab_parent_class)->constructed (object);

        if (tab->priv->web_view == NULL) {
                DhWebView *web_view;

                web_view = dh_web_view_new (NULL);
                gtk_widget_show (GTK_WIDGET (web_view));
                set_web_view (tab, web_view);
        }
}

static void
dh_tab_dispose (GObject *object)
{
        DhTab *tab = DH_TAB (object);

        g_clear_object (&tab->priv->web_view);

        G_OBJECT_CLASS (dh_tab_parent_class)->dispose (object);
}

static void
dh_tab_class_init (DhTabClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = dh_tab_get_property;
        object_class->set_property = dh_tab_set_property;
        object_class->constructed = dh_tab_constructed;
        object_class->dispose = dh_tab_dispose;

        /**
         * DhTab:web-view:
         *
         * The #DhWebView of the tab. If set to %NULL a #DhWebView is created
         * with the default #DhProfile.
         *
         * Since: 3.30
         */
        properties[PROP_WEB_VIEW] =
                g_param_spec_object ("web-view",
                                     "web-view",
                                     "",
                                     DH_TYPE_WEB_VIEW,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
dh_tab_init (DhTab *tab)
{
        tab->priv = dh_tab_get_instance_private (tab);

        gtk_orientable_set_orientation (GTK_ORIENTABLE (tab), GTK_ORIENTATION_VERTICAL);
}

/**
 * dh_tab_new:
 * @web_view: (nullable): a #DhWebView, or %NULL to create a #DhWebView with the
 *   default #DhProfile.
 *
 * Returns: (transfer floating): a new #DhTab.
 * Since: 3.30
 */
DhTab *
dh_tab_new (DhWebView *web_view)
{
        g_return_val_if_fail (web_view == NULL || DH_IS_WEB_VIEW (web_view), NULL);

        return g_object_new (DH_TYPE_TAB,
                             "web-view", web_view,
                             NULL);
}

/**
 * dh_tab_get_web_view:
 * @tab: a #DhTab.
 *
 * Returns: (transfer none): the #DhTab:web-view.
 * Since: 3.30
 */
DhWebView *
dh_tab_get_web_view (DhTab *tab)
{
        g_return_val_if_fail (DH_IS_TAB (tab), NULL);

        return tab->priv->web_view;
}
