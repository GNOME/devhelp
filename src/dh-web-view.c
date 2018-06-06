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

/* #DhWebView is a subclass of #WebKitWebView, to have a higher-level API for
 * some features.
 */

struct _DhWebViewPrivate {
        DhProfile *profile;
        gchar *search_text;
        gdouble total_scroll_delta_y;
};

enum {
        PROP_0,
        PROP_PROFILE,
        N_PROPERTIES
};

enum {
        SIGNAL_OPEN_NEW_TAB,
        N_SIGNALS
};

static GParamSpec *properties[N_PROPERTIES];
static guint signals[N_SIGNALS];

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

static gboolean
dh_web_view_button_press_event (GtkWidget      *widget,
				GdkEventButton *event)
{
        WebKitWebView *view = WEBKIT_WEB_VIEW (widget);

        switch (event->button) {
                /* Some mice emit button presses when the scroll wheel is tilted
                 * to the side. Web browsers use them to navigate in history.
                 */
                case 8:
                        webkit_web_view_go_back (view);
                        return GDK_EVENT_STOP;
                case 9:
                        webkit_web_view_go_forward (view);
                        return GDK_EVENT_STOP;

                default:
                        break;
        }

        if (GTK_WIDGET_CLASS (dh_web_view_parent_class)->button_press_event == NULL)
                return GDK_EVENT_PROPAGATE;

        return GTK_WIDGET_CLASS (dh_web_view_parent_class)->button_press_event (widget, event);
}

static gboolean
dh_web_view_load_failed (WebKitWebView   *web_view,
                         WebKitLoadEvent  load_event,
                         const gchar     *failing_uri,
                         GError          *error)
{
        /* Ignore cancellation errors, which happen when typing fast in the
         * search entry.
         */
        if (g_error_matches (error, WEBKIT_NETWORK_ERROR, WEBKIT_NETWORK_ERROR_CANCELLED))
                return GDK_EVENT_STOP;

        if (WEBKIT_WEB_VIEW_CLASS (dh_web_view_parent_class)->load_failed == NULL)
                return GDK_EVENT_PROPAGATE;

        return WEBKIT_WEB_VIEW_CLASS (dh_web_view_parent_class)->load_failed (web_view,
                                                                              load_event,
                                                                              failing_uri,
                                                                              error);
}

static gchar *
find_equivalent_local_uri (const gchar *uri)
{
        gchar **components;
        guint n_components;
        const gchar *book_id;
        const gchar *relative_url;
        DhBookList *book_list;
        GList *books;
        GList *book_node;
        gchar *local_uri = NULL;

        g_return_val_if_fail (uri != NULL, NULL);

        components = g_strsplit (uri, "/", 0);
        n_components = g_strv_length (components);

        if ((g_str_has_prefix (uri, "http://library.gnome.org/devel/") ||
             g_str_has_prefix (uri, "https://library.gnome.org/devel/")) &&
            n_components >= 7) {
                book_id = components[4];
                relative_url = components[6];
        } else if ((g_str_has_prefix (uri, "http://developer.gnome.org/") ||
                    g_str_has_prefix (uri, "https://developer.gnome.org/")) &&
                   n_components >= 6) {
                /* E.g. http://developer.gnome.org/gio/stable/ch02.html */
                book_id = components[3];
                relative_url = components[5];
        } else {
                goto out;
        }

        book_list = dh_book_list_get_default ();
        books = dh_book_list_get_books (book_list);

        for (book_node = books; book_node != NULL; book_node = book_node->next) {
                DhBook *cur_book = DH_BOOK (book_node->data);
                GList *links;
                GList *link_node;

                if (g_strcmp0 (dh_book_get_id (cur_book), book_id) != 0)
                        continue;

                links = dh_book_get_links (cur_book);

                for (link_node = links; link_node != NULL; link_node = link_node->next) {
                        DhLink *cur_link = link_node->data;

                        if (dh_link_match_relative_url (cur_link, relative_url)) {
                                local_uri = dh_link_get_uri (cur_link);
                                goto out;
                        }
                }
        }

out:
        g_strfreev (components);
        return local_uri;
}

static gboolean
dh_web_view_decide_policy (WebKitWebView            *web_view,
                           WebKitPolicyDecision     *policy_decision,
                           WebKitPolicyDecisionType  type)
{
        WebKitNavigationPolicyDecision *navigation_decision;
        WebKitNavigationAction *navigation_action;
        const gchar *uri;
        gchar *local_uri = NULL;
        gint button;
        gint state;
        gboolean open_new_tab = FALSE;

        if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
                goto chain_up;

        navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (policy_decision);
        navigation_action = webkit_navigation_policy_decision_get_navigation_action (navigation_decision);
        uri = webkit_uri_request_get_uri (webkit_navigation_action_get_request (navigation_action));
        if (uri == NULL) {
                g_warn_if_reached ();
                goto chain_up;
        }

        /* middle click or ctrl-click -> new tab */
        button = webkit_navigation_action_get_mouse_button (navigation_action);
        state = webkit_navigation_action_get_modifiers (navigation_action);
        open_new_tab = (button == 2 || (button == 1 && state == GDK_CONTROL_MASK));

        if (g_str_equal (uri, "about:blank"))
                goto handle_open_new_tab;

        local_uri = find_equivalent_local_uri (uri);
        if (local_uri != NULL && !open_new_tab) {
                webkit_policy_decision_ignore (policy_decision);
                webkit_web_view_load_uri (web_view, local_uri);
                g_free (local_uri);
                return GDK_EVENT_STOP;
        }

        if (local_uri == NULL && !g_str_has_prefix (uri, "file://")) {
                GtkWidget *toplevel;
                GtkWindow *window = NULL;
                GError *error = NULL;

                webkit_policy_decision_ignore (policy_decision);

                toplevel = gtk_widget_get_toplevel (GTK_WIDGET (web_view));
                if (GTK_IS_WINDOW (toplevel))
                        window = GTK_WINDOW (toplevel);

                gtk_show_uri_on_window (window, uri, GDK_CURRENT_TIME, &error);

                if (error != NULL) {
                        g_warning ("Error when opening URI “%s” externally: %s",
                                   uri,
                                   error->message);
                        g_clear_error (&error);
                }

                return GDK_EVENT_STOP;
        }

handle_open_new_tab:
        if (open_new_tab) {
                webkit_policy_decision_ignore (policy_decision);
                g_signal_emit (web_view,
                               signals[SIGNAL_OPEN_NEW_TAB], 0,
                               local_uri != NULL ? local_uri : uri);
                g_free (local_uri);
                return GDK_EVENT_STOP;
        }

chain_up:
        g_free (local_uri);

        if (WEBKIT_WEB_VIEW_CLASS (dh_web_view_parent_class)->decide_policy == NULL)
                return GDK_EVENT_PROPAGATE;

        return WEBKIT_WEB_VIEW_CLASS (dh_web_view_parent_class)->decide_policy (web_view,
                                                                                policy_decision,
                                                                                type);
}

static void
set_profile (DhWebView *view,
             DhProfile *profile)
{
        if (profile == NULL)
                return;

        g_return_if_fail (DH_IS_PROFILE (profile));

        g_assert (view->priv->profile == NULL);
        view->priv->profile = g_object_ref (profile);
}

static void
dh_web_view_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
        DhWebView *view = DH_WEB_VIEW (object);

        switch (prop_id) {
                case PROP_PROFILE:
                        g_value_set_object (value, dh_web_view_get_profile (view));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_web_view_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
        DhWebView *view = DH_WEB_VIEW (object);

        switch (prop_id) {
                case PROP_PROFILE:
                        set_profile (view, g_value_get_object (value));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
set_fonts (WebKitWebView *view,
           const gchar   *font_name_variable,
           const gchar   *font_name_fixed)
{
        PangoFontDescription *font_desc_variable;
        PangoFontDescription *font_desc_fixed;
        guint font_size_variable;
        guint font_size_fixed;
        guint font_size_variable_px;
        guint font_size_fixed_px;
        WebKitSettings *settings;

        g_return_if_fail (font_name_variable != NULL);
        g_return_if_fail (font_name_fixed != NULL);

        /* Get the font size. */
        font_desc_variable = pango_font_description_from_string (font_name_variable);
        font_desc_fixed = pango_font_description_from_string (font_name_fixed);
        font_size_variable = pango_font_description_get_size (font_desc_variable) / PANGO_SCALE;
        font_size_fixed = pango_font_description_get_size (font_desc_fixed) / PANGO_SCALE;
        font_size_variable_px = webkit_settings_font_size_to_pixels (font_size_variable);
        font_size_fixed_px = webkit_settings_font_size_to_pixels (font_size_fixed);

        /* Set the fonts. */
        settings = webkit_web_view_get_settings (view);
        webkit_settings_set_zoom_text_only (settings, TRUE);
        webkit_settings_set_serif_font_family (settings, font_name_variable);
        webkit_settings_set_default_font_size (settings, font_size_variable_px);
        webkit_settings_set_monospace_font_family (settings, font_name_fixed);
        webkit_settings_set_default_monospace_font_size (settings, font_size_fixed_px);

        pango_font_description_free (font_desc_variable);
        pango_font_description_free (font_desc_fixed);
}

static void
update_fonts (DhWebView *view)
{
        DhSettings *settings;
        gchar *variable_font = NULL;
        gchar *fixed_font = NULL;

        settings = dh_settings_get_default ();
        dh_settings_get_selected_fonts (settings, &variable_font, &fixed_font);

        set_fonts (WEBKIT_WEB_VIEW (view), variable_font, fixed_font);

        g_free (variable_font);
        g_free (fixed_font);
}

static void
settings_fonts_changed_cb (DhSettings *settings,
                           DhWebView  *view)
{
        update_fonts (view);
}

static void
dh_web_view_constructed (GObject *object)
{
        DhWebView *view = DH_WEB_VIEW (object);
        WebKitSettings *webkit_settings;
        DhSettings *dh_settings;

        if (G_OBJECT_CLASS (dh_web_view_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (dh_web_view_parent_class)->constructed (object);

        /* Disable some things we have no need for. */
        webkit_settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (view));
        webkit_settings_set_enable_html5_database (webkit_settings, FALSE);
        webkit_settings_set_enable_html5_local_storage (webkit_settings, FALSE);
        webkit_settings_set_enable_plugins (webkit_settings, FALSE);

        if (view->priv->profile == NULL)
                set_profile (view, dh_profile_get_default ());

        dh_settings = dh_settings_get_default ();
        g_signal_connect_object (dh_settings,
                                 "fonts-changed",
                                 G_CALLBACK (settings_fonts_changed_cb),
                                 view,
                                 0);

        update_fonts (view);
}

static void
dh_web_view_dispose (GObject *object)
{
        DhWebView *view = DH_WEB_VIEW (object);

        g_clear_object (&view->priv->profile);

        G_OBJECT_CLASS (dh_web_view_parent_class)->dispose (object);
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
        WebKitWebViewClass *webkit_class = WEBKIT_WEB_VIEW_CLASS (klass);

        object_class->get_property = dh_web_view_get_property;
        object_class->set_property = dh_web_view_set_property;
        object_class->constructed = dh_web_view_constructed;
        object_class->dispose = dh_web_view_dispose;
        object_class->finalize = dh_web_view_finalize;

        widget_class->scroll_event = dh_web_view_scroll_event;
        widget_class->button_press_event = dh_web_view_button_press_event;

        webkit_class->load_failed = dh_web_view_load_failed;
        webkit_class->decide_policy = dh_web_view_decide_policy;

        properties[PROP_PROFILE] =
                g_param_spec_object ("profile",
                                     "profile",
                                     "",
                                     DH_TYPE_PROFILE,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_properties (object_class, N_PROPERTIES, properties);

        /**
         * DhWebView::open-new-tab:
         * @view: the #DhWebView emitting the signal.
         * @uri: the URI to open.
         *
         * The ::open-new-tab signal is emitted when a URI needs to be opened in
         * a new #DhWebView.
         */
        signals[SIGNAL_OPEN_NEW_TAB] =
                g_signal_new ("open-new-tab",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DhWebViewClass, open_new_tab),
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
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
dh_web_view_new (DhProfile *profile)
{
        g_return_val_if_fail (profile == NULL || DH_IS_PROFILE (profile), NULL);

        return g_object_new (DH_TYPE_WEB_VIEW,
                             "profile", profile,
                             NULL);
}

DhProfile *
dh_web_view_get_profile (DhWebView *view)
{
        g_return_val_if_fail (DH_IS_WEB_VIEW (view), NULL);

        return view->priv->profile;
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
