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

#include "dh-web-view.h"
#include <math.h>
#include <glib/gi18n.h>
#include "dh-settings-app.h"

struct _DhWebViewPrivate {
        gchar *search_text;
        gdouble total_scroll_delta_y;
};

static const gdouble zoom_levels[] = {
        0.5,            /* 50% */
        0.8408964152,   /* 75% */
        1.0,            /* 100% */
        1.1892071149,   /* 125% */
        1.4142135623,   /* 150% */
        1.6817928304,   /* 175% */
        2.0,            /* 200% */
        2.8284271247,   /* 300% */
        4.0             /* 400% */
};

static const guint n_zoom_levels = G_N_ELEMENTS (zoom_levels);

#define ZOOM_DEFAULT (zoom_levels[2])

G_DEFINE_TYPE_WITH_PRIVATE (DhWebView, dh_web_view, WEBKIT_TYPE_WEB_VIEW)

static gint
get_current_zoom_level_index (DhWebView *view)
{
        gdouble zoom_level;
        gdouble previous;
        guint i;

        zoom_level = webkit_web_view_get_zoom_level (WEBKIT_WEB_VIEW (view));

        previous = zoom_levels[0];
        for (i = 1; i < n_zoom_levels; i++) {
                gdouble current = zoom_levels[i];
                gdouble mean = sqrt (previous * current);

                if (zoom_level <= mean)
                        return i - 1;

                previous = current;
        }

        return n_zoom_levels - 1;
}

static void
bump_zoom_level (DhWebView *view,
                 gint       bump_amount)
{
        gint zoom_level_index;
        gdouble new_zoom_level;

        if (bump_amount == 0)
                return;

        zoom_level_index = get_current_zoom_level_index (view) + bump_amount;
        zoom_level_index = CLAMP (zoom_level_index, 0, (gint)n_zoom_levels - 1);
        new_zoom_level = zoom_levels[zoom_level_index];

        webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (view), new_zoom_level);
}

static gboolean
dh_web_view_scroll_event (GtkWidget      *widget,
                          GdkEventScroll *scroll_event)
{
        DhWebView *view = DH_WEB_VIEW (widget);
        gdouble delta_y;

        if ((scroll_event->state & GDK_CONTROL_MASK) == 0)
                goto chain_up;

        switch (scroll_event->direction) {
                case GDK_SCROLL_UP:
                        bump_zoom_level (view, 1);
                        return GDK_EVENT_STOP;

                case GDK_SCROLL_DOWN:
                        bump_zoom_level (view, -1);
                        return GDK_EVENT_STOP;

                case GDK_SCROLL_LEFT:
                case GDK_SCROLL_RIGHT:
                        break;

                case GDK_SCROLL_SMOOTH:
                        gdk_event_get_scroll_deltas ((GdkEvent *)scroll_event, NULL, &delta_y);
                        view->priv->total_scroll_delta_y += delta_y;

                        /* Avoiding direct float comparison.
                         * -1 and 1 are the thresholds for bumping the zoom level,
                         * which can be adjusted for taste.
                         */
                        if ((gint)view->priv->total_scroll_delta_y <= -1) {
                                view->priv->total_scroll_delta_y = 0.f;
                                bump_zoom_level (view, 1);
                        } else if ((gint)view->priv->total_scroll_delta_y >= 1) {
                                view->priv->total_scroll_delta_y = 0.f;
                                bump_zoom_level (view, -1);
                        }
                        return GDK_EVENT_STOP;

                default:
                        g_warn_if_reached ();
        }

chain_up:
        if (GTK_WIDGET_CLASS (dh_web_view_parent_class)->scroll_event == NULL)
                return GDK_EVENT_PROPAGATE;

        return GTK_WIDGET_CLASS (dh_web_view_parent_class)->scroll_event (widget, scroll_event);
}

static void
set_fonts (WebKitWebView *view,
           const gchar   *font_name_fixed,
           const gchar   *font_name_variable)
{
        PangoFontDescription *font_desc_fixed;
        PangoFontDescription *font_desc_variable;
        guint font_size_fixed;
        guint font_size_variable;
        guint font_size_fixed_px;
        guint font_size_variable_px;
        WebKitSettings *settings;

        g_return_if_fail (font_name_fixed != NULL);
        g_return_if_fail (font_name_variable != NULL);

        /* Get the font size. */
        font_desc_fixed = pango_font_description_from_string (font_name_fixed);
        font_desc_variable = pango_font_description_from_string (font_name_variable);
        font_size_fixed = pango_font_description_get_size (font_desc_fixed) / PANGO_SCALE;
        font_size_variable = pango_font_description_get_size (font_desc_variable) / PANGO_SCALE;
        font_size_fixed_px = webkit_settings_font_size_to_pixels (font_size_fixed);
        font_size_variable_px = webkit_settings_font_size_to_pixels (font_size_variable);

        /* Set the fonts. */
        settings = webkit_web_view_get_settings (view);
        webkit_settings_set_zoom_text_only (settings, TRUE);
        webkit_settings_set_monospace_font_family (settings, font_name_fixed);
        webkit_settings_set_default_monospace_font_size (settings, font_size_fixed_px);
        webkit_settings_set_serif_font_family (settings, font_name_variable);
        webkit_settings_set_default_font_size (settings, font_size_variable_px);

        g_debug ("Set font-fixed to '%s' (%i) and font-variable to '%s' (%i).",
                 font_name_fixed, font_size_fixed_px, font_name_variable, font_size_variable_px);

        pango_font_description_free (font_desc_fixed);
        pango_font_description_free (font_desc_variable);
}

static void
update_fonts (DhWebView *view)
{
        DhSettingsApp *settings;
        gchar *font_fixed = NULL;
        gchar *font_variable = NULL;

        settings = dh_settings_app_get_singleton ();
        dh_settings_app_get_selected_fonts (settings, &font_fixed, &font_variable);

        set_fonts (WEBKIT_WEB_VIEW (view), font_fixed, font_variable);

        g_free (font_fixed);
        g_free (font_variable);
}

static void
settings_fonts_changed_cb (DhSettingsApp *settings,
                           DhWebView     *view)
{
        update_fonts (view);
}

static void
dh_web_view_constructed (GObject *object)
{
        DhWebView *view = DH_WEB_VIEW (object);
        WebKitSettings *settings;
        DhSettingsApp *settings_app;

        if (G_OBJECT_CLASS (dh_web_view_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (dh_web_view_parent_class)->constructed (object);

        /* Disable some things we have no need for. */
        settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (view));
        webkit_settings_set_enable_html5_database (settings, FALSE);
        webkit_settings_set_enable_html5_local_storage (settings, FALSE);
        webkit_settings_set_enable_plugins (settings, FALSE);

        settings_app = dh_settings_app_get_singleton ();
        g_signal_connect_object (settings_app,
                                 "fonts-changed",
                                 G_CALLBACK (settings_fonts_changed_cb),
                                 view,
                                 0);

        update_fonts (view);
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
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        object_class->constructed = dh_web_view_constructed;
        object_class->finalize = dh_web_view_finalize;

        widget_class->scroll_event = dh_web_view_scroll_event;
}

static void
dh_web_view_init (DhWebView *view)
{
        view->priv = dh_web_view_get_instance_private (view);
        view->priv->total_scroll_delta_y = 0.f;

        gtk_widget_set_hexpand (GTK_WIDGET (view), TRUE);
        gtk_widget_set_vexpand (GTK_WIDGET (view), TRUE);
}

DhWebView *
dh_web_view_new (void)
{
        return g_object_new (DH_TYPE_WEB_VIEW, NULL);
}

const gchar *
dh_web_view_get_devhelp_title (DhWebView *view)
{
        const gchar *title;

        g_return_val_if_fail (DH_IS_WEB_VIEW (view), NULL);

        title = webkit_web_view_get_title (WEBKIT_WEB_VIEW (view));

        if (title == NULL || title[0] == '\0')
                title = _("Empty Page");

        return title;
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

gboolean
dh_web_view_can_zoom_in (DhWebView *view)
{
        gint zoom_level_index;

        g_return_val_if_fail (DH_IS_WEB_VIEW (view), FALSE);

        zoom_level_index = get_current_zoom_level_index (view);
        return zoom_level_index < ((gint)n_zoom_levels - 1);
}

gboolean
dh_web_view_can_zoom_out (DhWebView *view)
{
        gint zoom_level_index;

        g_return_val_if_fail (DH_IS_WEB_VIEW (view), FALSE);

        zoom_level_index = get_current_zoom_level_index (view);
        return zoom_level_index > 0;
}

gboolean
dh_web_view_can_reset_zoom (DhWebView *view)
{
        gint zoom_level_index;

        g_return_val_if_fail (DH_IS_WEB_VIEW (view), FALSE);

        zoom_level_index = get_current_zoom_level_index (view);
        return zoom_levels[zoom_level_index] != ZOOM_DEFAULT;
}

void
dh_web_view_zoom_in (DhWebView *view)
{
        g_return_if_fail (DH_IS_WEB_VIEW (view));

        bump_zoom_level (view, 1);
}

void
dh_web_view_zoom_out (DhWebView *view)
{
        g_return_if_fail (DH_IS_WEB_VIEW (view));

        bump_zoom_level (view, -1);
}

void
dh_web_view_reset_zoom (DhWebView *view)
{
        g_return_if_fail (DH_IS_WEB_VIEW (view));

        webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (view), ZOOM_DEFAULT);
}
