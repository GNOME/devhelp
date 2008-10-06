/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Imendio AB
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
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include "dh-window.h"
#include "dh-link.h"
#include "dh-assistant.h"

typedef struct {
        DhBase    *base;

        GtkWidget *main_box;
        GtkWidget *web_view;

        gchar     *current_search;
        DhLink    *current_link;
} DhAssistantPriv;

static void dh_assistant_class_init (DhAssistantClass *klass);
static void dh_assistant_init       (DhAssistant      *assistant);

G_DEFINE_TYPE (DhAssistant, dh_assistant, GTK_TYPE_WINDOW);

#define GET_PRIVATE(instance) G_TYPE_INSTANCE_GET_PRIVATE \
  (instance, DH_TYPE_ASSISTANT, DhAssistantPriv);

static WebKitNavigationResponse
assistant_navigation_requested_cb (WebKitWebView        *web_view,
                                   WebKitWebFrame       *frame,
                                   WebKitNetworkRequest *request,
                                   DhAssistant          *assistant)
{
        DhAssistantPriv *priv;
        const gchar     *uri;

        priv = GET_PRIVATE (assistant);

        uri = webkit_network_request_get_uri (request);

        if (strcmp (uri, "about:blank") == 0) {
                return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;
        }

        if (g_str_has_prefix (uri, "file://")) {
                GtkWidget *window;

                window = dh_base_get_window (priv->base);
                _dh_window_display_uri (DH_WINDOW (window), uri);
        }

        return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}

static gboolean
assistant_key_press_event_cb (GtkWidget   *widget,
                              GdkEventKey *event,
                              DhAssistant *assistant)
{
        if (event->keyval == GDK_Escape) {
                gtk_widget_destroy (GTK_WIDGET (assistant));
                return TRUE;
        }

        return FALSE;
}

static gboolean
assistant_button_press_event_cb (GtkWidget      *widget,
                                 GdkEventButton *event,
                                 gpointer        user_data)
{
        /* Block webkit's builtin context menu. */
        if (event->button != 1) {
                return TRUE;
        }

        return FALSE;
}

static void
assistant_finalize (GObject *object)
{
        DhAssistantPriv *priv = GET_PRIVATE (object);

        g_object_unref (priv->base);

        g_free (priv->current_search);

        if (priv->current_link) {
                dh_link_unref (priv->current_link);
        }

        G_OBJECT_CLASS (dh_assistant_parent_class)->finalize (object);
}

static void
dh_assistant_class_init (DhAssistantClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = assistant_finalize;

        g_type_class_add_private (klass, sizeof (DhAssistantPriv));
}

static void
dh_assistant_init (DhAssistant *assistant)
{
        DhAssistantPriv *priv = GET_PRIVATE (assistant);
        GtkWidget       *scrolled_window;

        priv->main_box = gtk_vbox_new (FALSE, 0);
        gtk_widget_show (priv->main_box);
        gtk_container_add (GTK_CONTAINER (assistant), priv->main_box);

        gtk_window_set_icon_name (GTK_WINDOW (assistant), "devhelp");
        gtk_window_set_default_size (GTK_WINDOW (assistant), 400, 400);

        priv->web_view = webkit_web_view_new ();

        g_signal_connect (priv->web_view, "navigation-requested",
                          G_CALLBACK (assistant_navigation_requested_cb),
                          assistant);
        g_signal_connect (priv->web_view, "button-press-event",
                          G_CALLBACK (assistant_button_press_event_cb),
                          assistant);

        g_signal_connect (priv->web_view, "key-press-event",
                          G_CALLBACK (assistant_key_press_event_cb),
                          assistant);

        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

        gtk_container_add (GTK_CONTAINER (scrolled_window), priv->web_view);

        gtk_widget_show_all (scrolled_window);

        webkit_web_view_open (WEBKIT_WEB_VIEW (priv->web_view),
                              "about:blank");

        gtk_box_pack_start (GTK_BOX (priv->main_box),
                            scrolled_window, TRUE, TRUE, 0);
}

GtkWidget *
dh_assistant_new (DhBase *base)
{
        DhAssistant     *assistant;
        DhAssistantPriv *priv;

        assistant = g_object_new (DH_TYPE_ASSISTANT, NULL);

        priv = GET_PRIVATE (assistant);

        priv->base = g_object_ref (base);

        return GTK_WIDGET (assistant);
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

static void
assistant_set_link (DhAssistant *assistant,
                    DhLink      *link)
{
        DhAssistantPriv *priv = GET_PRIVATE (assistant);
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

        if (priv->current_link == link) {
                return;
        }

        if (link) {
                dh_link_ref (link);
        }
        if (priv->current_link) {
                dh_link_unref (priv->current_link);
        }
        priv->current_link = link;

        if (!link) {
                webkit_web_view_open (WEBKIT_WEB_VIEW (priv->web_view),
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

                html = g_strdup_printf (
                        "<html>"
                        "<head>"
                        "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s\">"
                        "<script src=\"file://%s\"</script>"
                        "</head>"
                        "<body %s>"
                        "<div class=\"title\">%s: %s</div><div class=\"subtitle\">%s %s</div>"
                        "<div class=\"content\">%s</div>"
                        "</body>"
                        "</html>",
                        DATADIR "/devhelp/assistant/assistant.css",
                        DATADIR "/devhelp/assistant/assistant.js",
                        function,
                        dh_link_get_type_as_string (link),
                        dh_link_get_name (link),
                        _("Book:"),
                        dh_link_get_book_name (link),
                        buf);
                g_free (buf);

                /* We need to set a local base to be able to access
                 * the stylesheet and javascript, but we also have to
                 * set something that is not the same as the current
                 * page, otherwise link clicks won't go through the
                 * network request handler (which we need so we can
                 * forward then to a main devhelp window. The reason
                 * is that page-local anchor links are handled
                 * internally in webkit.
                 */
                tmp = g_path_get_dirname (filename);
                base = g_strconcat ("file://", tmp, "/fake", NULL);
                g_free (tmp);

                webkit_web_view_load_html_string (
                        WEBKIT_WEB_VIEW (priv->web_view),
                        html,
                        base);

                g_free (html);
                g_free (base);
        } else {
                webkit_web_view_open (WEBKIT_WEB_VIEW (priv->web_view),
                                      "about:blank");
        }

        g_mapped_file_free (file);
        g_free (filename);
}

gboolean
dh_assistant_search (DhAssistant *assistant,
                     const gchar *str)
{
        DhAssistantPriv *priv;
        GList           *keywords, *l;
        const gchar     *name;
        DhLink          *link;
        DhLink          *exact_link;
        DhLink          *prefix_link;

        g_return_val_if_fail (DH_IS_ASSISTANT (assistant), FALSE);
        g_return_val_if_fail (str != NULL, FALSE);

        priv = GET_PRIVATE (assistant);

        /* Filter out very short strings. */
        if (strlen (str) < 4) {
                return FALSE;
        }

        if (priv->current_search && strcmp (priv->current_search, str) == 0) {
                return FALSE;
        }
        g_free (priv->current_search);
        priv->current_search = g_strdup (str);

        keywords = dh_base_get_keywords (priv->base);

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
                assistant_set_link (assistant, exact_link);
        }
        else if (prefix_link) {
                /*g_print ("prefix hit: '%s' '%s'\n", prefix_link->name, str);*/
                assistant_set_link (assistant, prefix_link);
        } else {
                /*g_print ("no hit\n");*/
                /*assistant_set_link (assistant, NULL);*/
                return FALSE;
        }

        gtk_widget_show (GTK_WIDGET (assistant));

        return TRUE;
}
