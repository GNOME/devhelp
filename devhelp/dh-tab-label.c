/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2018-2019 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dh-tab-label.h"
#include "dh-web-view.h"

/**
 * SECTION:dh-tab-label
 * @Title: DhTabLabel
 * @Short_description: A #DhTab label, used by #DhNotebook
 *
 * The #DhTabLabel widget is used for the tab labels in #DhNotebook.
 *
 * It contains the title as returned by dh_web_view_get_devhelp_title(), plus a
 * close button.
 */

struct _DhTabLabelPrivate {
        /* Weak ref */
        DhTab *tab;

        GtkLabel *label;
};

enum {
        PROP_0,
        PROP_TAB,
        N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

G_DEFINE_TYPE_WITH_PRIVATE (DhTabLabel, dh_tab_label, GTK_TYPE_GRID)

static void
update_label (DhTabLabel *tab_label)
{
        DhWebView *web_view;
        const gchar *title;

        if (tab_label->priv->tab == NULL)
                return;

        web_view = dh_tab_get_web_view (tab_label->priv->tab);
        title = dh_web_view_get_devhelp_title (web_view);
        gtk_label_set_text (tab_label->priv->label, title);
}

static void
web_view_title_notify_cb (DhWebView  *web_view,
                          GParamSpec *pspec,
                          DhTabLabel *tab_label)
{
        update_label (tab_label);
}

static void
set_tab (DhTabLabel *tab_label,
         DhTab      *tab)
{
        DhWebView *web_view;

        if (tab == NULL)
                return;

        g_return_if_fail (DH_IS_TAB (tab));

        g_assert (tab_label->priv->tab == NULL);
        g_set_weak_pointer (&tab_label->priv->tab, tab);

        web_view = dh_tab_get_web_view (tab);
        g_signal_connect_object (web_view,
                                 "notify::title",
                                 G_CALLBACK (web_view_title_notify_cb),
                                 tab_label,
                                 0);

        update_label (tab_label);
}

static void
dh_tab_label_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
        DhTabLabel *tab_label = DH_TAB_LABEL (object);

        switch (prop_id) {
                case PROP_TAB:
                        g_value_set_object (value, dh_tab_label_get_tab (tab_label));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_tab_label_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
        DhTabLabel *tab_label = DH_TAB_LABEL (object);

        switch (prop_id) {
                case PROP_TAB:
                        set_tab (tab_label, g_value_get_object (value));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_tab_label_dispose (GObject *object)
{
        DhTabLabel *tab_label = DH_TAB_LABEL (object);

        g_clear_weak_pointer (&tab_label->priv->tab);

        G_OBJECT_CLASS (dh_tab_label_parent_class)->dispose (object);
}

static void
dh_tab_label_class_init (DhTabLabelClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = dh_tab_label_get_property;
        object_class->set_property = dh_tab_label_set_property;
        object_class->dispose = dh_tab_label_dispose;

        /**
         * DhTabLabel:tab:
         *
         * The associated #DhTab. #DhTabLabel has a weak reference to the
         * #DhTab.
         *
         * Since: 3.30
         */
        properties[PROP_TAB] =
                g_param_spec_object ("tab",
                                     "tab",
                                     "",
                                     DH_TYPE_TAB,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static GtkWidget *
create_close_button (void)
{
        GtkWidget *close_button;
        GtkStyleContext *style_context;

        close_button = gtk_button_new_from_icon_name ("window-close-symbolic",
                                                      GTK_ICON_SIZE_BUTTON);
        gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);
        gtk_widget_set_focus_on_click (close_button, FALSE);

        style_context = gtk_widget_get_style_context (close_button);
        gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_FLAT);

        return close_button;
}

static void
close_button_clicked_cb (GtkButton  *close_button,
                         DhTabLabel *tab_label)
{
        if (tab_label->priv->tab != NULL)
                gtk_widget_destroy (GTK_WIDGET (tab_label->priv->tab));
}

static void
dh_tab_label_init (DhTabLabel *tab_label)
{
        GtkWidget *close_button;

        tab_label->priv = dh_tab_label_get_instance_private (tab_label);

        gtk_grid_set_column_spacing (GTK_GRID (tab_label), 4);

        /* Label */

        tab_label->priv->label = GTK_LABEL (gtk_label_new (NULL));
        gtk_widget_set_hexpand (GTK_WIDGET (tab_label->priv->label), TRUE);
        gtk_widget_set_vexpand (GTK_WIDGET (tab_label->priv->label), TRUE);
        gtk_widget_set_halign (GTK_WIDGET (tab_label->priv->label), GTK_ALIGN_CENTER);
        gtk_label_set_ellipsize (tab_label->priv->label, PANGO_ELLIPSIZE_END);

        gtk_widget_show (GTK_WIDGET (tab_label->priv->label));
        gtk_container_add (GTK_CONTAINER (tab_label),
                           GTK_WIDGET (tab_label->priv->label));

        /* Close button */

        close_button = create_close_button ();

        g_signal_connect (close_button,
                          "clicked",
                          G_CALLBACK (close_button_clicked_cb),
                          tab_label);

        gtk_widget_show (close_button);
        gtk_container_add (GTK_CONTAINER (tab_label), close_button);
}

/**
 * dh_tab_label_new:
 * @tab: the associated #DhTab.
 *
 * Returns: (transfer floating): a new #DhTabLabel.
 * Since: 3.30
 */
GtkWidget *
dh_tab_label_new (DhTab *tab)
{
        g_return_val_if_fail (DH_IS_TAB (tab), NULL);

        return g_object_new (DH_TYPE_TAB_LABEL,
                             "tab", tab,
                             NULL);
}

/**
 * dh_tab_label_get_tab:
 * @tab_label: a #DhTabLabel.
 *
 * Returns: (transfer none) (nullable): the #DhTabLabel:tab.
 * Since: 3.30
 */
DhTab *
dh_tab_label_get_tab (DhTabLabel *tab_label)
{
        g_return_val_if_fail (DH_IS_TAB_LABEL (tab_label), NULL);

        return tab_label->priv->tab;
}
