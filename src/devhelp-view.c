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
 *
 * Author: Mikael Hallendal <micke@codefactory.se>
 */

#include <stdio.h>
#include <string.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnome/gnome-i18n.h>

#include "devhelp-view.h"

#define d(x)

struct _DevHelpViewPriv {
        HtmlDocument *doc;
	gchar        *base_url;
	
	gboolean      first;
	gboolean      active;

	gint          stamp;
	GMutex       *stamp_mutex;
	GAsyncQueue  *thread_queue;
};

typedef struct {
	DevHelpView *view;
	gint         stamp;
	gchar       *url;
	gchar       *anchor;
} ReaderThreadData;

typedef enum {
	READER_QUEUE_TYPE_DATA,
	READER_QUEUE_TYPE_FINISHED,
	READER_QUEUE_TYPE_JUMP
} ReaderQueueType;

typedef struct {
	DevHelpView     *view;
	gint             stamp;
	gchar           *data;
	ReaderQueueType  type;
	gchar           *anchor;
	gchar           *url;
} ReaderQueueData;

static void     view_init                  (DevHelpView         *html);
static void     view_class_init            (DevHelpViewClass    *klass);

static gpointer view_reader_thread         (ReaderThreadData    *th_data);
static void     view_change_read_stamp     (DevHelpView         *view);
static void     view_url_requested_cb      (HtmlDocument        *doc,
					    const gchar         *uri,
					    HtmlStream          *stream,
					    gpointer             data);
static ReaderQueueData * 
view_q_data_new                            (DevHelpView         *view,
					    gint                 stamp, 
					    const gchar         *url,
					    const gchar         *anchor,
					    ReaderQueueType      type);
static void     view_stream_cancel         (HtmlStream          *stream, 
					    gpointer             user_data, 
					    gpointer             cancel_data);
static void     view_q_data_free           (ReaderQueueData     *q_data);
static void     view_link_clicked_cb       (HtmlDocument        *doc, 
					    const gchar         *url, 
					    gpointer             data);
static gboolean view_check_read_cancelled  (DevHelpView         *view,
					    gint                 stamp);
static gchar *  view_split_uri             (const gchar         *uri,
					    gchar              **anchor);

#define BUFFER_SIZE 16384

enum {
	URI_SELECTED,
	LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

GType
devhelp_view_get_type (void)
{
        static GType view_type = 0;

        if (!view_type)
        {
                static const GTypeInfo view_info =
                        {
                                sizeof (DevHelpViewClass),
                                NULL,
                                NULL,
                                (GClassInitFunc) view_class_init,
                                NULL,
                                NULL,
                                sizeof (DevHelpView),
                                0,
                                (GInstanceInitFunc) view_init,
                        };
                
                view_type = g_type_register_static (HTML_TYPE_VIEW,
                                                    "DevHelpView", 
                                                    &view_info, 0);
        }
        
        return view_type;
}

static void
view_init (DevHelpView *view)
{
        DevHelpViewPriv *priv;
        
        priv = g_new0 (DevHelpViewPriv, 1);

        priv->doc          = html_document_new ();
        priv->base_url     = NULL;
	priv->active       = FALSE;
	priv->first        = TRUE;
	priv->stamp_mutex  = g_mutex_new ();
	priv->thread_queue = g_async_queue_new ();
	
        html_view_set_document (HTML_VIEW (view), priv->doc);
        
        g_signal_connect (G_OBJECT (priv->doc), "link_clicked",
                          G_CALLBACK (view_link_clicked_cb), view);
        
        g_signal_connect (G_OBJECT (priv->doc), "request_url",
                          G_CALLBACK (view_url_requested_cb), view);

        view->priv = priv;
}

static void
view_class_init (DevHelpViewClass *klass)
{
	signals[URI_SELECTED] = 
		g_signal_new ("uri_selected",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (DevHelpViewClass, uri_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);
}

static gpointer
view_reader_thread (ReaderThreadData *th_data)
{
	DevHelpView      *view;
	DevHelpViewPriv  *priv;
	gint              stamp;
	GnomeVFSHandle   *handle;
	GnomeVFSResult    result;
	ReaderQueueData  *q_data;
	gchar             buffer[BUFFER_SIZE];
	GnomeVFSFileSize  n;
	
	g_return_if_fail (th_data != NULL);

	view  = th_data->view;
	priv  = view->priv;
	stamp = th_data->stamp;
	
	d(g_print ("file_start\n"));

	g_mutex_lock (priv->stamp_mutex);
	
	if (view_check_read_cancelled (view, stamp)) {
		
		g_mutex_unlock (priv->stamp_mutex);

		return;
	}

	g_mutex_unlock (priv->stamp_mutex);

	result = gnome_vfs_open (&handle, 
				 th_data->url,
				 GNOME_VFS_OPEN_READ);
	
	if (result != GNOME_VFS_OK) {
		/* FIXME: Signal error */
		return;
	}

	while (TRUE) {
		result = gnome_vfs_read (handle, buffer, BUFFER_SIZE, &n);
		
		/* FIXME: Do some error checking */
		if (result != GNOME_VFS_OK) {
			break;
		}
		
		q_data = view_q_data_new (view, stamp, 
					  th_data->url, th_data->anchor,
					  READER_QUEUE_TYPE_DATA);

		q_data->data = g_strdup (buffer);
		
		g_async_queue_push (priv->thread_queue, q_data);

		g_mutex_lock (priv->stamp_mutex);
		
		if (view_check_read_cancelled (view, stamp)) {
		
			g_mutex_unlock (priv->stamp_mutex);

			return;
		}
		
		g_mutex_unlock (priv->stamp_mutex);
	}
	
	q_data = view_q_data_new (view, stamp, 
				  th_data->url, th_data->anchor,
				  READER_QUEUE_TYPE_FINISHED);

	g_async_queue_push (priv->thread_queue, q_data);

	return NULL;
}

static void
view_change_read_stamp (DevHelpView *view)
{
	DevHelpViewPriv *priv;
	
	g_return_if_fail (DEVHELP_IS_VIEW (view));
	
	priv = view->priv;

	if ((priv->stamp++) >= G_MAXINT) {
		priv->stamp = 1;
	}
}

static gboolean
view_idle_check_queue (ReaderThreadData *th_data)
{
	DevHelpView     *view;
	DevHelpViewPriv *priv;
	ReaderQueueData *q_data = NULL;
	gint             len;
	
	g_return_val_if_fail (th_data != NULL, FALSE);
	
	view = th_data->view;
	priv = view->priv;

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
			view_q_data_free (q_data);
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
						     view_stream_cancel,
						     view);
			priv->first = FALSE;
		} else {
			d(g_print ("Adding data to open document\n"));
		}

		len = strlen (q_data->data);
		
		if (len > 0) {
			html_document_write_stream (priv->doc, 
						    q_data->data, len);
		}
		
		break;
		
	case READER_QUEUE_TYPE_FINISHED:
		if (!priv->first) {
			html_document_close_stream (priv->doc);
			gtk_adjustment_set_value (gtk_layout_get_vadjustment (GTK_LAYOUT (view)),
						  0);
		}

		if (q_data->anchor) {
			d(g_print ("Jumping to anchor: %s\n", q_data->anchor));
			
			html_view_jump_to_anchor (HTML_VIEW (q_data->view),
						  q_data->anchor);
		}

		gdk_window_set_cursor (GTK_WIDGET (view)->window, NULL);
		gtk_widget_grab_focus (GTK_WIDGET (view));
		break;
	default:
		g_assert_not_reached ();
	}
	
	view_q_data_free (q_data);

	g_mutex_unlock (priv->stamp_mutex);

	return TRUE;
}

static void
view_url_requested_cb (HtmlDocument *doc,
		       const gchar  *uri,
		       HtmlStream   *stream,
		       gpointer      data)
{
        DevHelpView     *view;
        DevHelpViewPriv *priv;
	GnomeVFSURI  *vfs_uri;

        d(puts(__FUNCTION__));

        view = DEVHELP_VIEW (data);
        priv = view->priv;

	g_return_if_fail (HTML_IS_DOCUMENT(doc));
	g_return_if_fail (stream != NULL);

	html_stream_set_cancel_func (stream, 
				     view_stream_cancel,
				     view);
	
	/* Read this ... */
}

static ReaderQueueData * 
view_q_data_new (DevHelpView     *view,
		 gint             stamp, 
		 const gchar     *url,
		 const gchar     *anchor,
		 ReaderQueueType  type)
{
	ReaderQueueData *q_data;
	
	q_data         = g_new0 (ReaderQueueData, 1);
	q_data->view   = g_object_ref (view);
	q_data->stamp  = stamp;
	q_data->type   = type;
	q_data->data   = NULL;
	q_data->url    = g_strdup (url);
	
	if (anchor) {
		q_data->anchor = g_strdup (anchor);
	}

	return q_data;
}


static void
view_stream_cancel (HtmlStream *stream, 
		    gpointer    user_data,  
		    gpointer    cancel_data)
{
        d(puts(__FUNCTION__));

	/* What to do here? */
}

static void
view_q_data_free (ReaderQueueData *q_data)
{
        DevHelpViewPriv *priv;
        
}

static void
view_link_clicked_cb (HtmlDocument *doc, const gchar *url, gpointer data)
{
	DevHelpView     *view;
        DevHelpViewPriv *priv;
	
        view = DEVHELP_VIEW (data);
        priv = view->priv;

	if (priv->base_url) {
		d(g_print ("Link '%s' pressed relative to: %s\n", 
			   url,
			   priv->base_url));
        } else {
        }

	g_signal_emit (view, signals[URI_SELECTED], 0, GPOINTER_TO_INT (url));
}

static gboolean
view_check_read_cancelled (DevHelpView *view, gint stamp)
{
	DevHelpViewPriv *priv;
	
	g_return_val_if_fail (DEVHELP_IS_VIEW (view), TRUE);
	
	priv = view->priv;

	d(g_print ("check_cancelled\n"));
	
	if (priv->stamp != stamp) {
		return TRUE;
	}
	
	return FALSE;
}

static gchar *
view_split_uri (const gchar *uri, gchar **anchor)
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

GtkWidget *
devhelp_view_new (void)
{
        DevHelpView     *view;
        DevHelpViewPriv *priv;
        
        d(puts(__FUNCTION__));

        view = g_object_new (DEVHELP_TYPE_VIEW, NULL);
        priv = view->priv;
        
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

        return GTK_WIDGET (view);
}

void
devhelp_view_open_uri (DevHelpView *view, 
		       const gchar *str_uri)
{
        DevHelpViewPriv  *priv;
	ReaderThreadData *th_data;
	GdkCursor        *cursor;
	gchar            *url;
	gchar            *anchor = NULL;
	
        d(puts(__FUNCTION__));
	
	g_return_if_fail (DEVHELP_IS_VIEW (view));
	g_return_if_fail (str_uri != NULL);

        priv = view->priv;

	d(g_print ("Opening URI: %s\n", str_uri));

	url = view_split_uri (str_uri, &anchor);
	
	if (priv->base_url) {
 		if (g_ascii_strcasecmp (priv->base_url, url) == 0 &&
 		    priv->first != TRUE) {
 			if (anchor) {
 				html_view_jump_to_anchor (HTML_VIEW (view),
 							  anchor);
 			} else {
				gtk_adjustment_set_value (gtk_layout_get_vadjustment (GTK_LAYOUT (view)),
							  0);
			}

			return;
 		}

		g_free (priv->base_url);
	}

        priv->base_url = url;

	g_mutex_lock (priv->stamp_mutex);

	th_data = g_new0 (ReaderThreadData, 1);
	
	view_change_read_stamp (view);
	
	th_data->stamp = priv->stamp;
	
	g_mutex_unlock (priv->stamp_mutex);

	priv->first = TRUE;
	
	th_data->view   = g_object_ref (view);
	th_data->url    = g_strdup (url);
	
	if (anchor) {
		th_data->anchor = anchor;
	} else {
		th_data->anchor = NULL;
	}
	
	g_timeout_add (100, (GSourceFunc) view_idle_check_queue, th_data);

	cursor = gdk_cursor_new (GDK_WATCH);
	
	gdk_window_set_cursor (GTK_WIDGET (view)->window, cursor);
	gdk_cursor_unref (cursor);
	
	g_thread_create_full ((GThreadFunc) view_reader_thread, th_data,
			      2 * BUFFER_SIZE,
			      TRUE,
			      FALSE, G_THREAD_PRIORITY_NORMAL,
			      NULL);
}
