/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@codefactory.se>
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

#include "dh-util.h"
#include "dh-html.h"

#define d(x) 

struct _DhHtmlPriv {
        HtmlDocument *doc;
	gchar        *base_url;
	
	gboolean      first;
	gboolean      active;

	gint          stamp;
	GMutex       *stamp_mutex;
	GAsyncQueue  *thread_queue;
};

typedef struct {
	DhHtml         *html;
	gint            stamp;
	GnomeVFSHandle *handle;
	gchar          *anchor;
} ReaderThreadData;

typedef enum {
	READER_QUEUE_TYPE_DATA,
	READER_QUEUE_TYPE_FINISHED,
	READER_QUEUE_TYPE_JUMP
} ReaderQueueType;

typedef struct {
	DhHtml          *html;
	gint             stamp;
	gchar           *data;
	gint             len;
	ReaderQueueType  type;
	gchar           *anchor;
} ReaderQueueData;

static void     html_init                  (DhHtml              *html);
static void     html_class_init            (DhHtmlClass         *klass);

static gpointer html_reader_thread         (ReaderThreadData    *th_data);
static void     html_change_read_stamp     (DhHtml              *html);
static void     html_url_requested_cb      (HtmlDocument        *doc,
					    const gchar         *uri,
					    HtmlStream          *stream,
					    gpointer             data);
static ReaderQueueData * 
html_q_data_new                            (DhHtml              *html,
					    gint                 stamp, 
					    const gchar         *anchor,
					    ReaderQueueType      type);
static void     html_stream_cancelled      (HtmlStream          *stream, 
					    gpointer             user_data, 
					    gpointer             cancel_data);
static void     html_q_data_free           (ReaderQueueData     *q_data);
static void     html_link_clicked_cb       (HtmlDocument        *doc, 
					    const gchar         *url, 
					    gpointer             data);
static gboolean html_check_read_cancelled  (DhHtml              *html,
					    gint                 stamp);
static gchar *  html_split_uri             (const gchar         *uri,
					    gchar              **anchor);
static gchar *  html_get_full_uri          (DhHtml              *html,
					    const gchar         *url);

#define BUFFER_SIZE 16384

enum {
	URI_SELECTED,
	LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

GType
dh_html_get_type (void)
{
        static GType type = 0;

        if (!type)
        {
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
                
                type = g_type_register_static (HTML_TYPE_VIEW,
					       "DhHtml", 
					       &info, 0);
        }
        
        return type;
}

static void
html_init (DhHtml *html)
{
        DhHtmlPriv *priv;
        
        priv = g_new0 (DhHtmlPriv, 1);

        priv->doc          = html_document_new ();
        priv->base_url     = NULL;
	priv->active       = FALSE;
	priv->first        = TRUE;
	priv->stamp_mutex  = g_mutex_new ();
	priv->thread_queue = g_async_queue_new ();
	
        html_view_set_document (HTML_VIEW (html), priv->doc);
        
        g_signal_connect (G_OBJECT (priv->doc), "link_clicked",
                          G_CALLBACK (html_link_clicked_cb), html);
        
        g_signal_connect (G_OBJECT (priv->doc), "request_url",
                          G_CALLBACK (html_url_requested_cb), html);

        html->priv = priv;
}

static void
html_class_init (DhHtmlClass *klass)
{
	signals[URI_SELECTED] = 
		g_signal_new ("uri_selected",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (DhHtmlClass, uri_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);
}

static gpointer
html_reader_thread (ReaderThreadData *th_data)
{
	DhHtml      *html;
	DhHtmlPriv  *priv;
	gint              stamp;
	GnomeVFSHandle   *handle;
	GnomeVFSResult    result;
	ReaderQueueData  *q_data;
	gchar             buffer[BUFFER_SIZE];
	GnomeVFSFileSize  n;
	
	g_return_if_fail (th_data != NULL);

	html  = th_data->html;
	priv  = html->priv;
	stamp = th_data->stamp;
	
	d(g_print ("file_start\n"));

	g_mutex_lock (priv->stamp_mutex);
	
	if (html_check_read_cancelled (html, stamp)) {
		
		g_mutex_unlock (priv->stamp_mutex);

		return;
	}

	g_mutex_unlock (priv->stamp_mutex);

	handle = th_data->handle;

	while (TRUE) {
		result = gnome_vfs_read (handle, buffer, BUFFER_SIZE, &n);
		
		/* FIXME: Do some error checking */
		if (result != GNOME_VFS_OK) {
			break;
		}
		
		q_data = html_q_data_new (html, stamp, 
					  th_data->anchor,
					  READER_QUEUE_TYPE_DATA);

		q_data->data = g_strdup (buffer);
		q_data->len = n;
		
		g_async_queue_push (priv->thread_queue, q_data);

		g_mutex_lock (priv->stamp_mutex);
		
		if (html_check_read_cancelled (html, stamp)) {
		
			g_mutex_unlock (priv->stamp_mutex);

			return;
		}
		
		g_mutex_unlock (priv->stamp_mutex);
	}
	
	q_data = html_q_data_new (html, stamp, 
				  th_data->anchor,
				  READER_QUEUE_TYPE_FINISHED);

	g_async_queue_push (priv->thread_queue, q_data);

	return NULL;
}

static void
html_change_read_stamp (DhHtml *html)
{
	DhHtmlPriv *priv;
	
	g_return_if_fail (DH_IS_HTML (html));
	
	priv = html->priv;

	if ((priv->stamp++) >= G_MAXINT) {
		priv->stamp = 1;
	}
}

static gboolean
html_idle_check_queue (ReaderThreadData *th_data)
{
	DhHtml     *html;
	DhHtmlPriv *priv;
	ReaderQueueData *q_data = NULL;
	gint             len;
	
	g_return_val_if_fail (th_data != NULL, FALSE);
	
	html = th_data->html;
	priv = html->priv;

	if (!g_mutex_trylock (priv->stamp_mutex)) {
		return TRUE;
	}
	
	if (th_data->stamp != priv->stamp) {
		g_mutex_unlock (priv->stamp_mutex);
		return FALSE;
	}
	
	q_data = (ReaderQueueData *) 
		g_async_queue_try_pop (priv->thread_queue);
	
	if (q_data) {
		if (priv->stamp != q_data->stamp) {
			html_q_data_free (q_data);
			q_data = NULL;
			g_mutex_unlock (priv->stamp_mutex);
			return TRUE;
		}
	} else {
		g_mutex_unlock (priv->stamp_mutex);
		return TRUE;
	}
	
	switch (q_data->type) {
	case READER_QUEUE_TYPE_DATA:
		if (priv->first) {
			d(g_print ("New document\n"));
			html_document_clear (priv->doc);
			html_document_open_stream (priv->doc, "text/html");
			html_stream_set_cancel_func (priv->doc->current_stream,
						     html_stream_cancelled,
						     html);
			priv->first = FALSE;
		} else {
			d(g_print ("Adding data to open document\n"));
		}

		len = q_data->len;
		
		if (len > 0) {
			html_document_write_stream (priv->doc, 
						    q_data->data, len);
		}
		
		break;
		
	case READER_QUEUE_TYPE_FINISHED:
		if (!priv->first) {
			html_document_close_stream (priv->doc);
			gtk_adjustment_set_value (gtk_layout_get_vadjustment (GTK_LAYOUT (html)),
						  0);
		}

		if (q_data->anchor) {
			d(g_print ("Jumping to anchor: %s\n", q_data->anchor));
			
			html_view_jump_to_anchor (HTML_VIEW (q_data->html),
						  q_data->anchor);
		}

		gdk_window_set_cursor (GTK_WIDGET (html)->window, NULL);
/* 		gtk_widget_grab_focus (GTK_WIDGET (html)); */
		break;
	default:
		g_assert_not_reached ();
	}
	
	html_q_data_free (q_data);

	g_mutex_unlock (priv->stamp_mutex);

	return TRUE;
}

static void
html_url_requested_cb (HtmlDocument *doc,
		       const gchar  *url,
		       HtmlStream   *stream,
		       gpointer      data)
{
        DhHtml           *html;
        DhHtmlPriv       *priv;
	GnomeVFSHandle   *handle;
	GnomeVFSResult    result;
	gchar             buffer[BUFFER_SIZE];
	GnomeVFSFileSize  read_len;
	gchar            *full_uri;
	
        d(puts(__FUNCTION__));

        html = DH_HTML (data);
        priv = html->priv;

	g_return_if_fail (HTML_IS_DOCUMENT(doc));
	g_return_if_fail (stream != NULL);

	html_stream_set_cancel_func (stream, 
				     html_stream_cancelled,
				     html);
	
	full_uri = html_get_full_uri (html, url);
	
	result = gnome_vfs_open (&handle, full_uri, GNOME_VFS_OPEN_READ);

	if (result != GNOME_VFS_OK) {
		g_warning ("Failed to open: %s", full_uri);
		g_free (full_uri);

		return;
	}

	g_free (full_uri);

	while (gnome_vfs_read (handle, buffer, BUFFER_SIZE, &read_len) ==
	       GNOME_VFS_OK) {
		html_stream_write (stream, buffer, read_len);
	}

	gnome_vfs_close (handle);
}

static ReaderQueueData * 
html_q_data_new (DhHtml          *html,
		 gint             stamp, 
		 const gchar     *anchor,
		 ReaderQueueType  type)
{
	ReaderQueueData *q_data;
	
	q_data         = g_new0 (ReaderQueueData, 1);
	q_data->html   = g_object_ref (html);
	q_data->stamp  = stamp;
	q_data->type   = type;
	q_data->data   = NULL;
	
	if (anchor) {
		q_data->anchor = g_strdup (anchor);
	}

	return q_data;
}


static void
html_stream_cancelled (HtmlStream *stream, 
		       gpointer    user_data,  
		       gpointer    cancel_data)
{
        d(puts(__FUNCTION__));

	/* What to do here? */
}

static void
html_q_data_free (ReaderQueueData *q_data)
{
        DhHtmlPriv *priv;
        
}

static void
html_link_clicked_cb (HtmlDocument *doc, const gchar *url, gpointer data)
{
	DhHtml     *html;
        DhHtmlPriv *priv;
	gchar      *full_uri;
	
        html = DH_HTML (data);
        priv = html->priv;

	full_uri = html_get_full_uri (html, url);
	
	d(g_print ("Full URI: %s\n", full_uri));
	
	g_signal_emit (html, signals[URI_SELECTED], 0, full_uri);

	g_free (full_uri);
}

static gboolean
html_check_read_cancelled (DhHtml *html, gint stamp)
{
	DhHtmlPriv *priv;
	
	g_return_val_if_fail (DH_IS_HTML (html), TRUE);
	
	priv = html->priv;

	d(g_print ("check_cancelled\n"));
	
	if (priv->stamp != stamp) {
		return TRUE;
	}
	
	return FALSE;
}

static gchar *
html_split_uri (const gchar *uri, gchar **anchor)
{
	gchar *ret_val;
	gchar *ch;
	
	if ((ch = strchr (uri, '?')) || (ch = strchr (uri, '#'))) {
		if (anchor) {
			*anchor = g_strdup (ch + 1);
		}
		
		ret_val = g_strndup (uri, ch - uri);
	} else {
		ret_val = g_strdup (uri);
	}

	return ret_val;
}

static gchar *
html_get_full_uri (DhHtml *html, const gchar *url)
{
	DhHtmlPriv *priv;
	
	priv = html->priv;
	
	if (priv->base_url) {
		if (dh_util_uri_is_relative (url)) {
			return dh_util_uri_relative_new (url, priv->base_url);
		}
	}
	
	return g_strdup (url);
}

GtkWidget *
dh_html_new (void)
{
        DhHtml     *html;
        DhHtmlPriv *priv;
        
        d(puts(__FUNCTION__));

        html = g_object_new (DH_TYPE_HTML, NULL);
        priv = html->priv;
        
	html_document_open_stream (priv->doc, "text/html");
	{
		int len; 
		gchar *text = g_strdup_printf ("<html><head></head><body bgcolor=\"white\"><h1>DevHelp</h1><p>%s</p></body></html>",
	                                       _("Select a subject in the contents to the left "
						 "or switch to the search pane to find what you are looking for."
						 "<p>Use <b>Shift Up/Down</b> to navigate the tree to the left, and "
						 "<b>Shift Left/Right</b> to expand and collapse the books in the tree."));
		len = strlen (text);
                
		html_document_write_stream (priv->doc, text, len);
	   
	        g_free (text);
	}
        
	html_document_close_stream (priv->doc);

        return GTK_WIDGET (html);
}

void
dh_html_open_uri (DhHtml      *html, 
		  const gchar *str_uri)
{
        DhHtmlPriv       *priv;
	ReaderThreadData *th_data;
	GdkCursor        *cursor;
	gchar            *url;
	gchar            *anchor = NULL;
	GnomeVFSResult    result;
	GnomeVFSHandle   *handle;
	
        d(puts(__FUNCTION__));
	
	g_return_if_fail (DH_IS_HTML (html));
	g_return_if_fail (str_uri != NULL);

        priv = html->priv;

	d(g_print ("Opening URI: %s\n", str_uri));

	url = html_split_uri (str_uri, &anchor);

	if (priv->base_url) {
 		if (g_ascii_strcasecmp (priv->base_url, url) == 0 &&
 		    priv->first != TRUE) {
 			if (anchor) {
 				html_view_jump_to_anchor (HTML_VIEW (html),
 							  anchor);
 			} else {
				gtk_adjustment_set_value (gtk_layout_get_vadjustment (GTK_LAYOUT (html)),
							  0);
			}

			return;
 		}
	}

	result = gnome_vfs_open (&handle, url, GNOME_VFS_OPEN_READ);

	if (result != GNOME_VFS_OK) {
		g_print ("Error opening url '%s'\n", url);
		g_free (url);
		return;
	}
	
	g_free (priv->base_url);
        priv->base_url = url;

	g_mutex_lock (priv->stamp_mutex);

	th_data = g_new0 (ReaderThreadData, 1);
	
	html_change_read_stamp (html);
	
	th_data->stamp = priv->stamp;
	
	g_mutex_unlock (priv->stamp_mutex);

	priv->first = TRUE;
	
	th_data->html   = g_object_ref (html);
	th_data->handle = handle;
	
	if (anchor) {
		th_data->anchor = anchor;
	} else {
		th_data->anchor = NULL;
	}
	
	g_timeout_add (100, (GSourceFunc) html_idle_check_queue, th_data);

	cursor = gdk_cursor_new (GDK_WATCH);
	
	gdk_window_set_cursor (GTK_WIDGET (html)->window, cursor);
	gdk_cursor_unref (cursor);
	
	g_thread_create_full ((GThreadFunc) html_reader_thread, th_data,
			      2 * BUFFER_SIZE,
			      TRUE,
			      FALSE, G_THREAD_PRIORITY_NORMAL,
			      NULL);
}
