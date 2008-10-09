/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Imendio AB
 * Copyright (C) 2008 Sven Herzberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "config.h"
#include <string.h>
#include <glib/gi18n-lib.h>
#include <webkit/webkit.h>
#include "dh-assistant-view.h"
#include "dh-link.h"
#include "dh-util.h"
#include "dh-window.h"

struct _DhAssistantView {
        WebKitWebView  parent_instance;

        DhBase        *base;
        DhLink        *link;
        gchar         *current_search;
};

struct _DhAssistantViewClass {
        WebKitWebViewClass parent_class;
};

static void dh_assistant_view_set_link (DhAssistantView *view,
                                        DhLink          *link);

G_DEFINE_TYPE (DhAssistantView, dh_assistant_view, WEBKIT_TYPE_WEB_VIEW);

static void
dh_assistant_view_init (DhAssistantView *view)
{
}

static void
view_finalize (GObject *object)
{
        DhAssistantView *view = DH_ASSISTANT_VIEW (object);

        if (view->link) {
                g_object_unref (view->link);
        }

        if (view->base) {
                g_object_unref (view->base);
        }

        g_free (view->current_search);

        G_OBJECT_CLASS (dh_assistant_view_parent_class)->finalize (object);
}

static WebKitNavigationResponse
assistant_navigation_requested (WebKitWebView        *web_view,
                                WebKitWebFrame       *frame,
                                WebKitNetworkRequest *request)
{
        DhAssistantView *view;
        const gchar     *uri;

        view = DH_ASSISTANT_VIEW (web_view);

        uri = webkit_network_request_get_uri (request);

        if (strcmp (uri, "about:blank") == 0) {
                return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;
        }

        if (g_str_has_prefix (uri, "file://")) {
                GtkWidget *window;

                window = dh_base_get_window (view->base);
                _dh_window_display_uri (DH_WINDOW (window), uri);
        }

        return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}

static gboolean
assistant_button_press_event (GtkWidget      *widget,
                              GdkEventButton *event)
{
        /* Block webkit's builtin context menu. */
        if (event->button != 1) {
                return TRUE;
        }

        return FALSE;
}

static void
dh_assistant_view_class_init (DhAssistantViewClass* view_class)
{
        GObjectClass       *object_class = G_OBJECT_CLASS (view_class);
        GtkWidgetClass     *widget_class = GTK_WIDGET_CLASS (view_class);
        WebKitWebViewClass *web_view_class = WEBKIT_WEB_VIEW_CLASS (view_class);

        object_class->finalize = view_finalize;

        widget_class->button_press_event = assistant_button_press_event;

        web_view_class->navigation_requested = assistant_navigation_requested;
}

DhBase*
dh_assistant_view_get_base (DhAssistantView *view)
{
        g_return_val_if_fail (DH_IS_ASSISTANT_VIEW (view), NULL);

        return view->base;
}

GtkWidget*
dh_assistant_view_new (void)
{
        return g_object_new (DH_TYPE_ASSISTANT_VIEW, NULL);
}

gboolean
dh_assistant_view_search (DhAssistantView *view,
                          const gchar     *str)
{
        GList           *keywords, *l;
        const gchar     *name;
        DhLink          *link;
        DhLink          *exact_link;
        DhLink          *prefix_link;

        g_return_val_if_fail (DH_IS_ASSISTANT_VIEW (view), FALSE);
        g_return_val_if_fail (str, FALSE);

        /* Filter out very short strings. */
        if (strlen (str) < 4) {
                return FALSE;
        }

        if (view->current_search && strcmp (view->current_search, str) == 0) {
                return FALSE;
        }
        g_free (view->current_search);
        view->current_search = g_strdup (str);

        keywords = dh_base_get_keywords (dh_assistant_view_get_base (view));

        prefix_link = NULL;
        exact_link = NULL;
        for (l = keywords; l && exact_link == NULL; l = l->next) {
                DhLinkType type;

                link = l->data;

                type = dh_link_get_link_type (link);

                if (type == DH_LINK_TYPE_BOOK ||
                    type == DH_LINK_TYPE_PAGE ||
                    type == DH_LINK_TYPE_KEYWORD) {
                        continue;
                }

                name = dh_link_get_name (link);
                if (strcmp (name, str) == 0) {
                        exact_link = link;
                }
                else if (g_str_has_prefix (name, str)) {
                        /* Prefer shorter prefix matches. */
                        if (!prefix_link) {
                                prefix_link = link;
                        }
                        else if (strlen (dh_link_get_name (prefix_link)) > strlen (name)) {
                                prefix_link = link;
                        }
                }
        }

        if (exact_link) {
                /*g_print ("exact hit: '%s' '%s'\n", exact_link->name, str);*/
                dh_assistant_view_set_link (view,
                                            exact_link);
        }
        else if (prefix_link) {
                /*g_print ("prefix hit: '%s' '%s'\n", prefix_link->name, str);*/
                dh_assistant_view_set_link (view,
                                            prefix_link);
        } else {
                /*g_print ("no hit\n");*/
                /*dh_assistant_view_set_link (view,
                                            NULL);*/
                return FALSE;
        }

        return TRUE;
}

void
dh_assistant_view_set_base (DhAssistantView *view,
                            DhBase          *base)
{
        g_return_if_fail (DH_IS_ASSISTANT_VIEW (view));
        g_return_if_fail (DH_IS_BASE (base));

        view->base = g_object_ref (base);
}

static const gchar *
find_in_buffer (const gchar *buffer,
                const gchar *key,
                gsize        length,
                gsize        key_length)
{
        gsize m = 0;
        gsize i = 0;

        while (i < length) {
                if (key[m] == buffer[i]) {
                        m++;
                        if (m == key_length) {
                                return buffer + i - m + 1;
                        }
                } else {
                        m = 0;
                }
                i++;
        }

        return NULL;
}

void
dh_assistant_view_set_link (DhAssistantView *view,
                            DhLink          *link)
{
        gchar           *uri;
        const gchar     *anchor;
        gchar           *filename;
        GMappedFile     *file;
        const gchar     *contents;
        gsize            length;
        gchar           *key;
        gsize            key_length;
        const gchar     *start;
        const gchar     *end;

        g_return_if_fail (DH_IS_ASSISTANT_VIEW (view));

        if (view->link == link) {
                return;
        }

        if (view->link) {
                dh_link_unref (view->link);
                view->link = NULL;
        }

        if (link) {
                link = dh_link_ref (link);
        } else {
                webkit_web_view_open (WEBKIT_WEB_VIEW (view),
                                      "about:blank");
                return;
        }

        uri = dh_link_get_uri (link);
        anchor = strrchr (uri, '#');
        if (anchor) {
                filename = g_strndup (uri, anchor - uri);
                anchor++;
                g_free (uri);
        } else {
                g_free (uri);
                return;
        }

        file = g_mapped_file_new (filename, FALSE, NULL);
        if (!file) {
                g_free (filename);
                return;
        }

        contents = g_mapped_file_get_contents (file);
        length = g_mapped_file_get_length (file);

        key = g_strdup_printf ("<a name=\"%s\"", anchor);
        key_length = strlen (key);

        start = find_in_buffer (contents, key, length, key_length);
        g_free (key);

        end = NULL;

        if (start) {
                const gchar *start_key;
                const gchar *end_key;

                length -= start - contents;

                start_key = "<pre class=\"programlisting\">";

                start = find_in_buffer (start,
                                        start_key,
                                        length,
                                        strlen (start_key));

                end_key = "<div class=\"refsect";

                if (start) {
                        end = find_in_buffer (start, end_key,
                                              length - strlen (start_key),
                                              strlen (end_key));
                }
        }

        if (start && end) {
                gchar       *buf;
                gboolean     break_line;
                const gchar *function;
                gchar       *stylesheet;
                gchar       *javascript;
                gchar       *html;
                gchar       *tmp;
                gchar       *base;

                buf = g_strndup (start, end-start);

                /* Try to reformat function signatures so they take less
                 * space and look nicer. Don't reformat things that don't
                 * look like functions.
                 */
                switch (dh_link_get_link_type (link)) {
                case DH_LINK_TYPE_FUNCTION:
                        break_line = TRUE;
                        function = "onload=\"reformatSignature()\"";
                        break;
                case DH_LINK_TYPE_MACRO:
                        break_line = TRUE;
                        function = "onload=\"cleanupSignature()\"";
                        break;
                default:
                        break_line = FALSE;
                        function = "";
                        break;
                }

                if (break_line) {
                        gchar *name;

                        name = strstr (buf, dh_link_get_name (link));
                        if (name && name > buf) {
                                name[-1] = '\n';
                        }
                }

                stylesheet = dh_util_build_data_filename ("devhelp",
                                                          "assistant",
                                                          "assistant.css",
                                                          NULL);
                javascript = dh_util_build_data_filename ("devhelp",
                                                          "assistant",
                                                          "assistant.js",
                                                          NULL);
                
                html = g_strdup_printf (
                        "<html>"
                        "<head>"
                        "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s\">"
                        "<script src=\"file://%s\"</script>"
                        "</head>"
                        "<body %s>"
                        "<div class=\"title\">%s: <a href=\"%s\">%s</a></div>"
                        "<div class=\"subtitle\">%s %s</div>"
                        "<div class=\"content\">%s</div>"
                        "</body>"
                        "</html>",
                        stylesheet,
                        javascript,
                        function,
                        dh_link_get_type_as_string (link),
                        dh_link_get_uri (link),
                        dh_link_get_name (link),
                        _("Book:"),
                        dh_link_get_book_name (link),
                        buf);
                g_free (buf);

                g_free (stylesheet);
                g_free (javascript);

                /* We need to set a local base to be able to access the
                 * stylesheet and javascript, but we also have to set
                 * something that is not the same as the current page,
                 * otherwise link clicks won't go through the network
                 * request handler (which we need so we can forward then to
                 * a main devhelp window. The reason is that page-local
                 * anchor links are handled internally in webkit.
                 */
                tmp = g_path_get_dirname (filename);
                base = g_strconcat ("file://", tmp, "/fake", NULL);
                g_free (tmp);

                webkit_web_view_load_html_string (
                        WEBKIT_WEB_VIEW (view),
                        html,
                        base);

                g_free (html);
                g_free (base);
        } else {
                webkit_web_view_open (WEBKIT_WEB_VIEW (view),
                                      "about:blank");
        }

        g_mapped_file_free (file);
        g_free (filename);
}
