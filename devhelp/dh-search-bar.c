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

#include "dh-search-bar.h"
#include "dh-web-view.h"

/**
 * SECTION:dh-search-bar
 * @Title: DhSearchBar
 * @Short_description: Subclass of #GtkSearchBar to search in #DhWebView's
 *
 * #DhSearchBar is a subclass of #GtkSearchBar, meant to be shown above a
 * #DhNotebook. There is only one #DhSearchBar for the whole #DhNotebook, it
 * applies the same search text to all the #DhWebView's (lazily, when the tab is
 * shown).
 *
 * (A different way to implement the search for the #DhWebView's would be to
 * have a different #GtkSearchEntry for each #DhWebView, with the
 * #GtkSearchEntry shown inside the #DhTab; in that case #DhSearchBar won't help
 * you).
 */

struct _DhSearchBarPrivate {
        DhNotebook *notebook;
        GtkSearchEntry *search_entry;
};

enum {
        PROP_0,
        PROP_NOTEBOOK,
        N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

G_DEFINE_TYPE_WITH_PRIVATE (DhSearchBar, dh_search_bar, GTK_TYPE_SEARCH_BAR)

static void
update_search_in_web_view (DhSearchBar *search_bar,
                           DhWebView   *view)
{
        const gchar *search_text = NULL;

        if (gtk_search_bar_get_search_mode (GTK_SEARCH_BAR (search_bar)))
                search_text = gtk_entry_get_text (GTK_ENTRY (search_bar->priv->search_entry));

        dh_web_view_set_search_text (view, search_text);
}

static void
update_search_in_active_web_view (DhSearchBar *search_bar)
{
        DhWebView *web_view;

        web_view = dh_notebook_get_active_web_view (search_bar->priv->notebook);
        if (web_view != NULL)
                update_search_in_web_view (search_bar, web_view);
}

static void
update_search_in_all_web_views (DhSearchBar *search_bar)
{
        GList *web_views;
        GList *l;

        web_views = dh_notebook_get_all_web_views (search_bar->priv->notebook);

        for (l = web_views; l != NULL; l = l->next) {
                DhWebView *web_view = DH_WEB_VIEW (l->data);
                update_search_in_web_view (search_bar, web_view);
        }

        g_list_free (web_views);
}

static void
search_previous_in_active_web_view (DhSearchBar *search_bar)
{
        DhWebView *web_view;

        web_view = dh_notebook_get_active_web_view (search_bar->priv->notebook);
        if (web_view == NULL)
                return;

        update_search_in_web_view (search_bar, web_view);
        dh_web_view_search_previous (web_view);
}

static void
search_next_in_active_web_view (DhSearchBar *search_bar)
{
        DhWebView *web_view;

        web_view = dh_notebook_get_active_web_view (search_bar->priv->notebook);
        if (web_view == NULL)
                return;

        update_search_in_web_view (search_bar, web_view);
        dh_web_view_search_next (web_view);
}

static void
search_mode_enabled_notify_cb (DhSearchBar *search_bar,
                               GParamSpec  *pspec,
                               gpointer     user_data)
{
        if (gtk_search_bar_get_search_mode (GTK_SEARCH_BAR (search_bar)))
                update_search_in_active_web_view (search_bar);
        else
                update_search_in_all_web_views (search_bar);
}

static void
search_changed_cb (GtkSearchEntry *entry,
                   DhSearchBar    *search_bar)
{
        /* Note that this callback is called after a small delay. */
        update_search_in_active_web_view (search_bar);
}

static void
previous_match_cb (GtkSearchEntry *entry,
                   DhSearchBar    *search_bar)
{
        search_previous_in_active_web_view (search_bar);
}

static void
next_match_cb (GtkSearchEntry *entry,
               DhSearchBar    *search_bar)
{
        search_next_in_active_web_view (search_bar);
}

static void
prev_button_clicked_cb (GtkButton   *prev_button,
                        DhSearchBar *search_bar)
{
        search_previous_in_active_web_view (search_bar);
}

static void
next_button_clicked_cb (GtkButton   *next_button,
                        DhSearchBar *search_bar)
{
        search_next_in_active_web_view (search_bar);
}

static void
notebook_switch_page_after_cb (GtkNotebook *notebook,
                               GtkWidget   *new_page,
                               guint        new_page_num,
                               DhSearchBar *search_bar)
{
        update_search_in_active_web_view (search_bar);
}

static void
dh_search_bar_constructed (GObject *object)
{
        DhSearchBar *search_bar = DH_SEARCH_BAR (object);
        GtkWidget *hgrid;
        GtkStyleContext *style_context;
        GtkWidget *prev_button;
        GtkWidget *next_button;

        if (G_OBJECT_CLASS (dh_search_bar_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (dh_search_bar_parent_class)->constructed (object);

        gtk_search_bar_set_show_close_button (GTK_SEARCH_BAR (search_bar), TRUE);

        hgrid = gtk_grid_new ();
        style_context = gtk_widget_get_style_context (hgrid);
        gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_LINKED);

        /* Search entry */
        search_bar->priv->search_entry = GTK_SEARCH_ENTRY (gtk_search_entry_new ());
        gtk_widget_set_size_request (GTK_WIDGET (search_bar->priv->search_entry), 300, -1);
        gtk_container_add (GTK_CONTAINER (hgrid),
                           GTK_WIDGET (search_bar->priv->search_entry));

        g_signal_connect (search_bar->priv->search_entry,
                          "search-changed",
                          G_CALLBACK (search_changed_cb),
                          search_bar);

        g_signal_connect (search_bar->priv->search_entry,
                          "previous-match",
                          G_CALLBACK (previous_match_cb),
                          search_bar);

        g_signal_connect (search_bar->priv->search_entry,
                          "next-match",
                          G_CALLBACK (next_match_cb),
                          search_bar);

        /* Prev/next buttons */
        prev_button = gtk_button_new_from_icon_name ("go-up-symbolic", GTK_ICON_SIZE_BUTTON);
        gtk_container_add (GTK_CONTAINER (hgrid), prev_button);

        next_button = gtk_button_new_from_icon_name ("go-down-symbolic", GTK_ICON_SIZE_BUTTON);
        gtk_container_add (GTK_CONTAINER (hgrid), next_button);

        g_signal_connect (prev_button,
                          "clicked",
                          G_CALLBACK (prev_button_clicked_cb),
                          search_bar);

        g_signal_connect (next_button,
                          "clicked",
                          G_CALLBACK (next_button_clicked_cb),
                          search_bar);

        /* Misc */
        g_signal_connect (search_bar,
                          "notify::search-mode-enabled",
                          G_CALLBACK (search_mode_enabled_notify_cb),
                          NULL);

        g_signal_connect_object (search_bar->priv->notebook,
                                 "switch-page",
                                 G_CALLBACK (notebook_switch_page_after_cb),
                                 search_bar,
                                 G_CONNECT_AFTER);

        gtk_widget_show_all (hgrid);
        gtk_container_add (GTK_CONTAINER (search_bar), hgrid);

        gtk_search_bar_connect_entry (GTK_SEARCH_BAR (search_bar),
                                      GTK_ENTRY (search_bar->priv->search_entry));
}

static void
dh_search_bar_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
        DhSearchBar *search_bar = DH_SEARCH_BAR (object);

        switch (prop_id) {
                case PROP_NOTEBOOK:
                        g_value_set_object (value, dh_search_bar_get_notebook (search_bar));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_search_bar_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
        DhSearchBar *search_bar = DH_SEARCH_BAR (object);

        switch (prop_id) {
                case PROP_NOTEBOOK:
                        g_assert (search_bar->priv->notebook == NULL);
                        search_bar->priv->notebook = g_object_ref_sink (g_value_get_object (value));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_search_bar_dispose (GObject *object)
{
        DhSearchBar *search_bar = DH_SEARCH_BAR (object);

        g_clear_object (&search_bar->priv->notebook);
        search_bar->priv->search_entry = NULL;

        G_OBJECT_CLASS (dh_search_bar_parent_class)->dispose (object);
}

static void
dh_search_bar_class_init (DhSearchBarClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->constructed = dh_search_bar_constructed;
        object_class->get_property = dh_search_bar_get_property;
        object_class->set_property = dh_search_bar_set_property;
        object_class->dispose = dh_search_bar_dispose;

        /**
         * DhSearchBar:notebook:
         *
         * The associated #DhNotebook. #DhSearchBar has a strong reference to
         * the #DhNotebook.
         *
         * Since: 3.30
         */
        properties[PROP_NOTEBOOK] =
                g_param_spec_object ("notebook",
                                     "notebook",
                                     "",
                                     DH_TYPE_NOTEBOOK,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
dh_search_bar_init (DhSearchBar *search_bar)
{
        search_bar->priv = dh_search_bar_get_instance_private (search_bar);
}

/**
 * dh_search_bar_new:
 * @notebook: a #DhNotebook.
 *
 * Returns: (transfer floating): a new #DhSearchBar.
 * Since: 3.30
 */
DhSearchBar *
dh_search_bar_new (DhNotebook *notebook)
{
        g_return_val_if_fail (DH_IS_NOTEBOOK (notebook), NULL);

        return g_object_new (DH_TYPE_SEARCH_BAR,
                             "notebook", notebook,
                             NULL);
}

/**
 * dh_search_bar_get_notebook:
 * @search_bar: a #DhSearchBar.
 *
 * Returns: (transfer none): the #DhSearchBar:notebook.
 * Since: 3.30
 */
DhNotebook *
dh_search_bar_get_notebook (DhSearchBar *search_bar)
{
        g_return_val_if_fail (DH_IS_SEARCH_BAR (search_bar), NULL);

        return search_bar->priv->notebook;
}

/**
 * dh_search_bar_grab_focus_to_search_entry:
 * @search_bar: a #DhSearchBar.
 *
 * Grabs the focus of #DhSearchBar search entry and selects its text
 */
void
dh_search_bar_grab_focus_to_search_entry (DhSearchBar *search_bar)
{
        g_return_if_fail (DH_IS_SEARCH_BAR (search_bar));

        gtk_widget_grab_focus (GTK_WIDGET (search_bar->priv->search_entry));
        gtk_editable_select_region (GTK_EDITABLE (search_bar->priv->search_entry), 0, -1);
}
