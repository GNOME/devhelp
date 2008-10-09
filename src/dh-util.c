/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001      Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004,2008 Imendio AB
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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "ige-conf.h"
#include "dh-util.h"

static GladeXML *
get_glade_file (const gchar *filename,
                const gchar *root,
                const gchar *domain,
                const gchar *first_required_widget,
                va_list args)
{
        GladeXML   *gui;
        const char *name;
        GtkWidget **widget_ptr;

        gui = glade_xml_new (filename, root, domain);
        if (!gui) {
                g_warning ("Couldn't find necessary glade file '%s'", filename);
                return NULL;
        }

        for (name = first_required_widget; name; name = va_arg (args, char *)) {
                widget_ptr = va_arg (args, void *);
                *widget_ptr = glade_xml_get_widget (gui, name);

                if (!*widget_ptr) {
                        g_warning ("Glade file '%s' is missing widget '%s'.",
                                   filename, name);
                        continue;
                }
        }

        return gui;
}

GladeXML *
dh_util_glade_get_file (const gchar *filename,
                        const gchar *root,
                        const gchar *domain,
                        const gchar *first_required_widget,
                        ...)
{
        va_list   args;
        GladeXML *gui;

        va_start (args, first_required_widget);
        gui = get_glade_file (filename,
                              root,
                              domain,
                              first_required_widget,
                              args);
        va_end (args);

        return gui;
}

void
dh_util_glade_connect (GladeXML *gui,
                       gpointer  user_data,
                       gchar    *first_widget,
                       ...)
{
        va_list      args;
        const gchar *name;
        const gchar *signal;
        GtkWidget   *widget;
        gpointer    *callback;

        va_start (args, first_widget);

        for (name = first_widget; name; name = va_arg (args, char *)) {
                signal = va_arg (args, void *);
                callback = va_arg (args, void *);

                widget = glade_xml_get_widget (gui, name);
                if (!widget) {
                        g_warning ("Glade file is missing widget '%s', aborting",
                                   name);
                        continue;
                }

                g_signal_connect (widget,
                                  signal,
                                  G_CALLBACK (callback),
                                  user_data);
        }

        va_end (args);
}

gchar *
dh_util_build_data_filename (const gchar *first_part,
                             ...)
{
        const gchar  *datadir = NULL;
        va_list       args;
        const gchar  *part;
        gchar       **strv;
        gint          i;
        gchar        *ret;

        va_start (args, first_part);

#ifdef GDK_WINDOWING_QUARTZ
        datadir = g_getenv ("DEVHELP_DATADIR");
#endif

        if (datadir == NULL) {
                datadir = DATADIR;
        }

        /* 2 = 1 initial component + terminating NULL element. */
        strv = g_malloc (sizeof (gchar *) * 2);
        strv[0] = (gchar *) datadir;

        i = 1;
        for (part = first_part; part; part = va_arg (args, char *), i++) {
                /* +2 = 1 new element + terminating NULL element. */
                strv = g_realloc (strv, sizeof (gchar*) * (i + 2));
                strv[i] = (gchar *) part;
        }

        strv[i] = NULL;
        ret = g_build_filenamev (strv);
        g_free (strv);

        va_end (args);

        return ret;
}

typedef struct {
        gchar *name;
        guint  timeout_id;
} DhUtilStateItem;

static void
util_state_item_free (DhUtilStateItem *item)
{
        g_free (item->name);
        if (item->timeout_id) {
                g_source_remove (item->timeout_id);
        }
        g_slice_free (DhUtilStateItem, item);
}

static void
util_state_setup_widget (GtkWidget   *widget, 
                         const gchar *name)
{
        DhUtilStateItem *item;

        item = g_slice_new0 (DhUtilStateItem);
        item->name = g_strdup (name);

        g_object_set_data_full (G_OBJECT (widget),
                                "dh-util-state",
                                item,
                                (GDestroyNotify) util_state_item_free);
}

static gchar *
util_state_get_key (const gchar *name,
                    const gchar *key)
{
        return g_strdup_printf ("/apps/devhelp/state/%s/%s", name, key);
}

static void
util_state_schedule_save (GtkWidget   *widget,
                          GSourceFunc  func)

{
        DhUtilStateItem *item;

        item = g_object_get_data (G_OBJECT (widget), "dh-util-state");
        if (item->timeout_id) {
		g_source_remove (item->timeout_id);
	}

	item->timeout_id = g_timeout_add (500,
                                          func,
                                          widget);
}

static void
util_state_save_window (GtkWindow   *window,
                        const gchar *name)
{
        gchar          *key;
        GdkWindowState  state;
        gboolean        maximized;
        gint            width, height;
        gint            x, y;

        state = gdk_window_get_state (GTK_WIDGET (window)->window);
        if (state & GDK_WINDOW_STATE_MAXIMIZED) {
                maximized = TRUE;
        } else {
                maximized = FALSE;
        }

        key = util_state_get_key (name, "maximized");
        ige_conf_set_bool (ige_conf_get (), key, maximized);
        g_free (key);

        /* If maximized don't save the size and position. */
        if (maximized) {
                return;
        }

        gtk_window_get_size (GTK_WINDOW (window), &width, &height);

        key = util_state_get_key (name, "width");
        ige_conf_set_int (ige_conf_get (), key, width);
        g_free (key);

        key = util_state_get_key (name, "height");
        ige_conf_set_int (ige_conf_get (), key, height);
        g_free (key);

        gtk_window_get_position (GTK_WINDOW (window), &x, &y);

        key = util_state_get_key (name, "x_position");
        ige_conf_set_int (ige_conf_get (), key, x);
        g_free (key);

        key = util_state_get_key (name, "y_position");
        ige_conf_set_int (ige_conf_get (), key, y);
        g_free (key);
}

static void
util_state_restore_window (GtkWindow   *window,
                           const gchar *name)
{
        gchar     *key;
        gboolean   maximized;
        gint       width, height;
        gint       x, y;
        GdkScreen *screen;
        gint       max_width, max_height;

        key = util_state_get_key (name, "width");
        ige_conf_get_int (ige_conf_get (), key, &width);
        g_free (key);

        key = util_state_get_key (name, "height");
        ige_conf_get_int (ige_conf_get (), key, &height);
        g_free (key);

        key = util_state_get_key (name, "x_position");
        ige_conf_get_int (ige_conf_get (), key, &x);
        g_free (key);

        key = util_state_get_key (name, "y_position");
        ige_conf_get_int (ige_conf_get (), key, &y);
        g_free (key);

        if (width > 1 && height > 1) {
                screen = gtk_widget_get_screen (GTK_WIDGET (window));
                max_width = gdk_screen_get_width (screen);
                max_height = gdk_screen_get_height (screen);

                width = CLAMP (width, 0, max_width);
                height = CLAMP (height, 0, max_height);

                x = CLAMP (x, 0, max_width - width);
                y = CLAMP (y, 0, max_height - height);

                gtk_window_set_default_size (window, width, height);
        }

        gtk_window_move (window, x, y);

        key = util_state_get_key (name, "maximized");
        ige_conf_get_bool (ige_conf_get (), key, &maximized);
        g_free (key);

        if (maximized) {
                gtk_window_maximize (window);
        }
}

static gboolean
util_state_window_timeout_cb (gpointer window)
{
        DhUtilStateItem *item;

        item = g_object_get_data (window, "dh-util-state");
        if (item) {
                item->timeout_id = 0;
                util_state_save_window (window, item->name);
        }

	return FALSE;
}

static gboolean
util_state_window_configure_event_cb (GtkWidget         *window,
                                      GdkEventConfigure *event,
                                      gpointer           user_data)
{
	util_state_schedule_save (window, util_state_window_timeout_cb);
	return FALSE;
}

static gboolean
util_state_paned_timeout_cb (gpointer paned)
{
        DhUtilStateItem *item;

        item = g_object_get_data (paned, "dh-util-state");
        if (item) {
                gchar *key;

                item->timeout_id = 0;

                key = util_state_get_key (item->name, "position");
                ige_conf_set_int (ige_conf_get (),
                                  key,
                                  gtk_paned_get_position (paned));
                g_free (key);
        }

	return FALSE;
}

static gboolean
util_state_paned_changed_cb (GtkWidget *paned,
                             gpointer   user_data)
{
	util_state_schedule_save (paned, util_state_paned_timeout_cb);
	return FALSE;
}

void
dh_util_state_manage_window (GtkWindow   *window,
                             const gchar *name)
{
        util_state_setup_widget (GTK_WIDGET (window), name);

        g_signal_connect (window, "configure-event",
                          G_CALLBACK (util_state_window_configure_event_cb),
                          NULL);

        util_state_restore_window (window, name);
}

void
dh_util_state_manage_paned (GtkPaned    *paned,
                            const gchar *name)
{
        gchar *key;
        gint   position;

        util_state_setup_widget (GTK_WIDGET (paned), name);

        key = util_state_get_key (name, "position");
        if (ige_conf_get_int (ige_conf_get (), key, &position)) {
                gtk_paned_set_position (paned, position);
        }
        g_free (key);

        g_signal_connect (paned, "notify::position",
                          G_CALLBACK (util_state_paned_changed_cb),
                          NULL);
}

static gboolean
util_state_notebook_timeout_cb (gpointer notebook)
{
        DhUtilStateItem *item;

        item = g_object_get_data (notebook, "dh-util-state");
        if (item) {
                GtkWidget   *page;
                const gchar *page_name;

                item->timeout_id = 0;

                page = gtk_notebook_get_nth_page (
                        notebook, 
                        gtk_notebook_get_current_page (notebook));
                page_name = dh_util_state_get_notebook_page_name (page);
                if (page_name) {
                        gchar *key;

                        key = util_state_get_key (item->name, "selected_tab");
                        ige_conf_set_string (ige_conf_get (), key, page_name);
                        g_free (key);
                }
        }

	return FALSE;
}

static void
util_state_notebook_switch_page_cb (GtkWidget       *notebook,
                                    GtkNotebookPage *page,
                                    guint            page_num,
                                    gpointer         user_data)
{
	util_state_schedule_save (notebook, util_state_notebook_timeout_cb);
}

void
dh_util_state_set_notebook_page_name (GtkWidget   *page,
                                      const gchar *page_name)
{
        g_object_set_data_full (G_OBJECT (page),
                                "dh-util-state-tab-name",
                                g_strdup (page_name),
                                g_free);
}

const gchar *
dh_util_state_get_notebook_page_name (GtkWidget *page)
{
        return g_object_get_data (G_OBJECT (page),
                                  "dh-util-state-tab-name");
}

void
dh_util_state_manage_notebook (GtkNotebook *notebook,
                               const gchar *name,
                               const gchar *default_tab)
{
        gchar     *key;
        gchar     *tab;
        gint       i;

        util_state_setup_widget (GTK_WIDGET (notebook), name);

        key = util_state_get_key (name, "selected_tab");
        if (!ige_conf_get_string (ige_conf_get (), key, &tab)) {
                tab = g_strdup (default_tab);
        }
        g_free (key);

        for (i = 0; i < gtk_notebook_get_n_pages (notebook); i++) {
                GtkWidget   *page;
                const gchar *page_name;

                page = gtk_notebook_get_nth_page (notebook, i);
                page_name = dh_util_state_get_notebook_page_name (page);
                if (page_name && strcmp (page_name, tab) == 0) {
                        gtk_notebook_set_current_page (notebook, i);
                        gtk_widget_grab_focus (page);
                        break;
                }
        }

        g_free (tab);

        g_signal_connect (notebook, "switch-page",
                          G_CALLBACK (util_state_notebook_switch_page_cb),
                          NULL);
}
