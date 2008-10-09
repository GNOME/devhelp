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
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include "dh-window.h"
#include "dh-link.h"
#include "dh-assistant.h"
#include "dh-assistant-view.h"

typedef struct {
        GtkWidget *main_box;
        GtkWidget *web_view;

        gchar     *current_search;
} DhAssistantPriv;

static void dh_assistant_class_init (DhAssistantClass *klass);
static void dh_assistant_init       (DhAssistant      *assistant);

G_DEFINE_TYPE (DhAssistant, dh_assistant, GTK_TYPE_WINDOW);

#define GET_PRIVATE(instance) G_TYPE_INSTANCE_GET_PRIVATE \
  (instance, DH_TYPE_ASSISTANT, DhAssistantPriv)

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

static void
assistant_finalize (GObject *object)
{
        DhAssistantPriv *priv = GET_PRIVATE (object);

        g_free (priv->current_search);

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

        priv->web_view = dh_assistant_view_new ();

        g_signal_connect (assistant, "key-press-event",
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

        dh_assistant_view_set_base (DH_ASSISTANT_VIEW (priv->web_view),
                                    base);

        return GTK_WIDGET (assistant);
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

        keywords = dh_base_get_keywords (dh_assistant_view_get_base (DH_ASSISTANT_VIEW (priv->web_view)));

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
                dh_assistant_view_set_link (DH_ASSISTANT_VIEW (GET_PRIVATE (assistant)->web_view),
                                            exact_link);
        }
        else if (prefix_link) {
                /*g_print ("prefix hit: '%s' '%s'\n", prefix_link->name, str);*/
                dh_assistant_view_set_link (DH_ASSISTANT_VIEW (GET_PRIVATE (assistant)->web_view),
                                            prefix_link);
        } else {
                /*g_print ("no hit\n");*/
                /*dh_assistant_view_set_link (DH_ASSISTANT_VIEW (GET_PRIVATE (assistant)->web_view),
                                            NULL);*/
                return FALSE;
        }

        gtk_widget_show (GTK_WIDGET (assistant));

        return TRUE;
}
