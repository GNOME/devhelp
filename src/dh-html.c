/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004 Imendio hB
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

#include <stdio.h>
#include <string.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-init.h>
#include <gtkmozembed.h>

#include "dh-util.h"
#include "dh-marshal.h"
#include "dh-gecko-utils.h"
#include "dh-preferences.h"
#include "dh-html.h"

#define d(x) 

struct _DhHtmlPriv {
	GtkMozEmbed *gecko;
};

static void     html_class_init               (DhHtmlClass         *klass);
static void     html_init                     (DhHtml              *html);
static void     html_title_cb                 (GtkMozEmbed         *embed,
					       DhHtml              *html);
static void     html_location_cb              (GtkMozEmbed         *embed, 
					       DhHtml              *html);

enum {
	TITLE_CHANGED,
	LOCATION_CHANGED,
	LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

GType
dh_html_get_type (void)
{
        static GType type = 0;

        if (!type) {
                static const GTypeInfo info =
                        {
                                sizeof (DhHtmlClass),
                                NULL,
                                NULL,
				(GClassInitFunc) html_class_init,
                                NULL,
                                NULL,
                                sizeof (DhHtml),
                                0,
                                (GInstanceInitFunc) html_init,
                        };
                
                type = g_type_register_static (G_TYPE_OBJECT,
					       "DhHtml", 
					       &info, 0);
        }
        
        return type;
}

static void
html_class_init (DhHtmlClass *klass)
{
	signals[TITLE_CHANGED] = 
		g_signal_new ("title-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      dh_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1, G_TYPE_STRING);

	signals[LOCATION_CHANGED] =
		g_signal_new ("location-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      dh_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1, G_TYPE_STRING);
}



static void
html_init (DhHtml *html)
{
        DhHtmlPriv *priv;
        
	priv = g_new0 (DhHtmlPriv, 1);

	priv->gecko = (GtkMozEmbed *) gtk_moz_embed_new ();

	g_signal_connect (priv->gecko, "title",
			  G_CALLBACK (html_title_cb),
			  html);
	g_signal_connect (priv->gecko, "location",
			  G_CALLBACK (html_location_cb),
			  html);

        html->priv = priv;
}

static void
html_title_cb (GtkMozEmbed *embed, DhHtml *html)
{
	char *new_title;

	new_title = gtk_moz_embed_get_title (embed);
	if (new_title && *new_title != '\0') {
		g_signal_emit (html, signals[TITLE_CHANGED], 0, new_title);
	}
	g_free (new_title);
}

static void
html_location_cb (GtkMozEmbed *embed, DhHtml *html)
{
	DhHtmlPriv *priv;
	char       *location;

	priv = html->priv;
	
	location = gtk_moz_embed_get_location (embed);
	g_signal_emit (html, signals[LOCATION_CHANGED], 0, location);
	g_free (location);
}

DhHtml *
dh_html_new (void)
{
        DhHtml *html;

        d(puts(__FUNCTION__));

        html = g_object_new (DH_TYPE_HTML, NULL);
 
        return html;
}

void
dh_html_clear (DhHtml *html)
{
	DhHtmlPriv  *priv;
	static const char *data = "<html><body bgcolor=\"white\"></body></html>";
	
	g_return_if_fail (DH_IS_HTML (html));

	priv = html->priv;

	gtk_moz_embed_render_data (priv->gecko, data, strlen (data),
				   "file:///", "text/html");
}

void
dh_html_open_uri (DhHtml *html, const gchar *str_uri)
{
        DhHtmlPriv *priv;
	gchar      *full_uri;
	
        d(puts(__FUNCTION__));
	
	g_return_if_fail (DH_IS_HTML (html));
	g_return_if_fail (str_uri != NULL);

        priv = html->priv;
	
	if (str_uri[0] == '/') {
		full_uri = g_strdup_printf ("file://%s", str_uri);
	} else {
		full_uri = (gchar *) str_uri;
	}
	
	gtk_moz_embed_load_url (priv->gecko, full_uri);

	if (full_uri != str_uri) {
		g_free (full_uri);
	}
}

GtkWidget *
dh_html_get_widget (DhHtml *html)
{
	g_return_val_if_fail (DH_IS_HTML (html), NULL);
	
	gtk_widget_show (GTK_WIDGET (html->priv->gecko));

	return GTK_WIDGET (html->priv->gecko);
}

gboolean
dh_html_can_go_forward (DhHtml *html)
{
	DhHtmlPriv *priv;
	
	g_return_val_if_fail (DH_IS_HTML (html), FALSE);

	priv = html->priv;

	return gtk_moz_embed_can_go_forward (priv->gecko);
}

gboolean
dh_html_can_go_back (DhHtml *html)
{
	DhHtmlPriv *priv;
	
	g_return_val_if_fail (DH_IS_HTML (html), FALSE);

	priv = html->priv;

	return gtk_moz_embed_can_go_back (priv->gecko);
}

void
dh_html_go_forward (DhHtml *html)
{
	DhHtmlPriv *priv;
	
	g_return_if_fail (DH_IS_HTML (html));

	priv = html->priv;

	gtk_moz_embed_go_forward (priv->gecko);
}

void
dh_html_go_back (DhHtml *html)
{
	DhHtmlPriv *priv;
	
	g_return_if_fail (DH_IS_HTML (html));

	priv = html->priv;
	
	gtk_moz_embed_go_back (priv->gecko);
}



