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

#include "dh-notebook.h"
#include "dh-tab-label.h"

/* #DhNotebook is a subclass of #GtkNotebook. The content of the tabs are
 * #DhTab's, and the tab labels are #DhTabLabel's.
 */

G_DEFINE_TYPE (DhNotebook, dh_notebook, GTK_TYPE_NOTEBOOK)

static void
show_or_hide_tabs (GtkNotebook *notebook)
{
        gint n_pages;

        n_pages = gtk_notebook_get_n_pages (notebook);
        gtk_notebook_set_show_tabs (notebook, n_pages > 1);
}

static void
dh_notebook_page_added (GtkNotebook *notebook,
                        GtkWidget   *child,
                        guint        page_num)
{
        if (GTK_NOTEBOOK_CLASS (dh_notebook_parent_class)->page_added != NULL)
                GTK_NOTEBOOK_CLASS (dh_notebook_parent_class)->page_added (notebook, child, page_num);

        show_or_hide_tabs (notebook);
}

static void
dh_notebook_page_removed (GtkNotebook *notebook,
                          GtkWidget   *child,
                          guint        page_num)
{
        if (GTK_NOTEBOOK_CLASS (dh_notebook_parent_class)->page_removed != NULL)
                GTK_NOTEBOOK_CLASS (dh_notebook_parent_class)->page_removed (notebook, child, page_num);

        show_or_hide_tabs (notebook);
}

static void
dh_notebook_class_init (DhNotebookClass *klass)
{
        GtkNotebookClass *gtk_notebook_class = GTK_NOTEBOOK_CLASS (klass);

        gtk_notebook_class->page_added = dh_notebook_page_added;
        gtk_notebook_class->page_removed = dh_notebook_page_removed;
}

static void
dh_notebook_init (DhNotebook *notebook)
{
        gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
}

DhNotebook *
dh_notebook_new (void)
{
        return g_object_new (DH_TYPE_NOTEBOOK, NULL);
}

static void
web_view_open_new_tab_cb (DhWebView   *web_view,
                          const gchar *uri,
                          DhNotebook  *notebook)
{
        dh_notebook_open_new_tab (notebook, uri, FALSE);
}

void
dh_notebook_open_new_tab (DhNotebook  *notebook,
                          const gchar *uri,
                          gboolean     switch_focus)
{
        DhWebView *web_view;
        DhTab *tab;
        GtkWidget *label;
        gint page_num;

        g_return_if_fail (DH_IS_NOTEBOOK (notebook));

        web_view = dh_web_view_new ();
        gtk_widget_show (GTK_WIDGET (web_view));

        tab = dh_tab_new (web_view);
        gtk_widget_show (GTK_WIDGET (tab));

        g_signal_connect (web_view,
                          "open-new-tab",
                          G_CALLBACK (web_view_open_new_tab_cb),
                          notebook);

        label = dh_tab_label_new (tab);
        gtk_widget_show (label);

        page_num = gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                             GTK_WIDGET (tab),
                                             label);

        gtk_container_child_set (GTK_CONTAINER (notebook),
                                 GTK_WIDGET (tab),
                                 "tab-expand", TRUE,
                                 "reorderable", TRUE,
                                 NULL);

        if (switch_focus)
                gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page_num);

        if (uri != NULL)
                webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), uri);
        else
                webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), "about:blank");
}

/* Returns: (transfer none) (nullable): */
DhTab *
dh_notebook_get_active_tab (DhNotebook *notebook)
{
        gint page_num;

        g_return_val_if_fail (DH_IS_NOTEBOOK (notebook), NULL);

        page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
        if (page_num == -1)
                return NULL;

        return DH_TAB (gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), page_num));
}

/* Returns: (transfer none) (nullable): */
DhWebView *
dh_notebook_get_active_web_view (DhNotebook *notebook)
{
        DhTab *tab;

        g_return_val_if_fail (DH_IS_NOTEBOOK (notebook), NULL);

        tab = dh_notebook_get_active_tab (notebook);
        return tab != NULL ? dh_tab_get_web_view (tab) : NULL;
}

/* Returns: (transfer container): */
GList *
dh_notebook_get_all_web_views (DhNotebook *notebook)
{
        gint n_pages;
        gint page_num;
        GList *list = NULL;

        g_return_val_if_fail (DH_IS_NOTEBOOK (notebook), NULL);

        n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook));

        for (page_num = 0; page_num < n_pages; page_num++) {
                DhTab *tab;

                tab = DH_TAB (gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), page_num));
                list = g_list_prepend (list, dh_tab_get_web_view (tab));
        }

        return g_list_reverse (list);
}
