/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004, 2008 Imendio AB
 * Copyright (C) 2015, 2017 Sébastien Wilmet <swilmet@gnome.org>
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

#include "config.h"
#include "dh-util.h"
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <math.h>

gchar *
dh_util_build_data_filename (const gchar *first_part,
                             ...)
{
        gchar        *datadir = NULL;
        va_list       args;
        const gchar  *part;
        gchar       **strv;
        gint          i;
        gchar        *ret;

        va_start (args, first_part);

        if (datadir == NULL) {
                datadir = g_strdup (DATADIR);
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

        g_free (datadir);

        va_end (args);

        return ret;
}

/* We're only going to expect ASCII strings here, so there's no point in
 * playing with g_unichar_totitle() and such.
 * Note that we modify the string in place.
 */
void
dh_util_ascii_strtitle (gchar *str)
{
        gboolean word_start;

        if (!str)
                return;

        word_start = TRUE;
        while (*str != '\0') {
                if (g_ascii_isalpha (*str)) {
                        *str = (word_start ?
                                g_ascii_toupper (*str) :
                                g_ascii_tolower (*str));
                        word_start = FALSE;
                } else {
                        word_start = TRUE;
                }
                str++;
        }
}

gchar *
dh_util_create_data_uri_for_filename (const gchar *filename,
                                      const gchar *mime_type)
{
        gchar *data;
        gsize  data_length;
        gchar *base64;
        gchar *uri;

        if (!g_file_get_contents (filename, &data, &data_length, NULL))
                return NULL;

        base64 = g_base64_encode ((const guchar *)data, data_length);
        g_free (data);

        uri = g_strdup_printf ("data:%s;charset=utf8;base64,%s", mime_type, base64);
        g_free(base64);

        return uri;
}

/* set the given fonts on the given view */
void
dh_util_view_set_font (WebKitWebView *view, const gchar *font_name_fixed, const gchar *font_name_variable)
{
        /* get the font size */
        PangoFontDescription *font_desc_fixed = pango_font_description_from_string (font_name_fixed);
        PangoFontDescription *font_desc_variable = pango_font_description_from_string (font_name_variable);
        guint font_size_fixed = pango_font_description_get_size (font_desc_fixed) / PANGO_SCALE;
        guint font_size_variable = pango_font_description_get_size (font_desc_variable) / PANGO_SCALE;
        guint font_size_fixed_px = webkit_settings_font_size_to_pixels (font_size_fixed);
        guint font_size_variable_px = webkit_settings_font_size_to_pixels (font_size_variable);

        pango_font_description_free (font_desc_fixed);
        pango_font_description_free (font_desc_variable);

        /* set the fonts */
        g_object_set (webkit_web_view_get_settings (view),
                      "zoom-text-only", TRUE,
                      "monospace-font-family", font_name_fixed,
                      "default-monospace-font-size", font_size_fixed_px,
                      "serif-font-family", font_name_variable,
                      "default-font-size", font_size_variable_px,
                      NULL);
        g_debug ("Set font-fixed to '%s' (%i) and font-variable to '%s' (%i).",
                 font_name_fixed, font_size_fixed_px, font_name_variable, font_size_variable_px);
}

static void
introspect_window_gsettings (GSettings *window_settings,
                             gboolean  *has_required_keys,
                             gboolean  *has_maximized_key)
{
        GSettingsSchema *schema = NULL;

        g_object_get (window_settings,
                      "settings-schema", &schema,
                      NULL);

        *has_required_keys = (g_settings_schema_has_key (schema, "width") &&
                              g_settings_schema_has_key (schema, "height"));

        *has_maximized_key = g_settings_schema_has_key (schema, "maximized");

        g_settings_schema_unref (schema);
}

void
dh_util_window_settings_save (GtkWindow *window,
                              GSettings *settings)
{
        gboolean has_required_keys;
        gboolean has_maximized_key;
        gint width;
        gint height;

        g_return_if_fail (GTK_IS_WINDOW (window));
        g_return_if_fail (G_IS_SETTINGS (settings));

        introspect_window_gsettings (settings, &has_required_keys, &has_maximized_key);
        g_return_if_fail (has_required_keys);

        if (has_maximized_key) {
                GdkWindowState state;
                gboolean maximized;

                state = gdk_window_get_state (gtk_widget_get_window (GTK_WIDGET (window)));
                maximized = (state & GDK_WINDOW_STATE_MAXIMIZED) != 0;

                g_settings_set_boolean (settings, "maximized", maximized);

                /* If maximized don't save the size. */
                if (maximized)
                        return;
        }

        /* Store the dimensions */
        gtk_window_get_size (GTK_WINDOW (window), &width, &height);
        g_settings_set_int (settings, "width", width);
        g_settings_set_int (settings, "height", height);
}

/* This should be called when @gtk_window is realized (i.e. its GdkWindow is
 * created) but not yet mapped (i.e. gtk_widget_show() has not yet been called,
 * so that when it is shown it already has the good size).
 */
void
dh_util_window_settings_restore (GtkWindow *gtk_window,
                                 GSettings *settings)
{
        gboolean has_required_keys;
        gboolean has_maximized_key;
        gint width;
        gint height;

        g_return_if_fail (GTK_IS_WINDOW (gtk_window));
        g_return_if_fail (gtk_widget_get_realized (GTK_WIDGET (gtk_window)));
        g_return_if_fail (G_IS_SETTINGS (settings));

        introspect_window_gsettings (settings, &has_required_keys, &has_maximized_key);
        g_return_if_fail (has_required_keys);

        width = g_settings_get_int (settings, "width");
        height = g_settings_get_int (settings, "height");

        if (width > 1 && height > 1) {
                GdkDisplay *display;
                GdkWindow *gdk_window;
                GdkMonitor *monitor;
                GdkRectangle monitor_workarea;
                gint max_width;
                gint max_height;

                display = gtk_widget_get_display (GTK_WIDGET (gtk_window));
                /* To get the GdkWindow the widget must be realized. */
                gdk_window = gtk_widget_get_window (GTK_WIDGET (gtk_window));
                monitor = gdk_display_get_monitor_at_window (display, gdk_window);
                gdk_monitor_get_workarea (monitor, &monitor_workarea);

                max_width = monitor_workarea.width;
                max_height = monitor_workarea.height;

                width = CLAMP (width, 0, max_width);
                height = CLAMP (height, 0, max_height);

                gtk_window_set_default_size (gtk_window, width, height);
        }

        if (has_maximized_key && g_settings_get_boolean (settings, "maximized"))
                gtk_window_maximize (gtk_window);
}

/* Adds q2 onto the end of q1, and frees q2. */
void
dh_util_queue_concat (GQueue *q1,
                      GQueue *q2)
{
        g_return_if_fail (q1 != NULL);

        if (q2 == NULL)
                return;

        if (q1->head == NULL) {
                g_assert_cmpint (q1->length, ==, 0);
                g_assert (q1->tail == NULL);

                q1->head = q2->head;
                q1->tail = q2->tail;
                q1->length = q2->length;
        } else if (q2->head != NULL) {
                g_assert_cmpint (q1->length, >, 0);
                g_assert_cmpint (q2->length, >, 0);
                g_assert (q1->tail != NULL);
                g_assert (q2->tail != NULL);

                q1->tail->next = q2->head;
                q2->head->prev = q1->tail;

                q1->tail = q2->tail;
                q1->length += q2->length;
        } else {
                g_assert_cmpint (q2->length, ==, 0);
                g_assert (q2->tail == NULL);
        }

        q2->head = NULL;
        q2->tail = NULL;
        q2->length = 0;
        g_queue_free (q2);
}

static gboolean
unref_node_link (GNode    *node,
                 gpointer  data)
{
        dh_link_unref (node->data);
        return FALSE;
}

void
_dh_util_free_book_tree (GNode *book_tree)
{
        if (book_tree == NULL)
                return;

        g_node_traverse (book_tree,
                         G_IN_ORDER,
                         G_TRAVERSE_ALL,
                         -1,
                         unref_node_link,
                         NULL);

        g_node_destroy (book_tree);
}
