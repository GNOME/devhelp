/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
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
 * Author: Johan Dahlin <zilch.am@home.se>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <time.h>
#include <string.h>
#include <glib.h>
#include <gtkhtml/gtkhtml.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-paper.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomeui/gnome-uidefs.h>
#include <libgnomeui/gnome-paper-selector.h>
#include <libgnomeprint/gnome-printer.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-printer-dialog.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprint/gnome-print-master-preview.h>
#include "util.h"
#include "html-widget.h"

#define d(x)

#define READ_BUFFER_SIZE 8192

typedef struct {
	HtmlWidget            *html_widget;
	GnomeVFSAsyncHandle   *vfs_handle;
	GtkHTMLStream         *html_stream;
	gchar                 *anchor;
} HtmlReadData;

static void     html_widget_init           (HtmlWidget          *html_widget);
static void     html_widget_class_init     (GtkObjectClass      *klass);
static void     html_widget_destroy        (GtkObject           *object);
static void     html_widget_load_uri       (HtmlWidget          *html_widget,
					    const GnomeVFSURI   *uri,
					    GtkHTMLStream       *handle,
					    gboolean             page_load);
static void     html_widget_url_requested  (GtkHTML             *gtk_html,
					    const gchar         *url,
					    GtkHTMLStream       *handle);
static gboolean html_widget_is_new_uri     (HtmlWidget          *html_widget,
					    const GnomeVFSURI   *uri);
static void     html_read_data_free        (HtmlReadData        *read_data);
static void     html_widget_async_open_cb  (GnomeVFSAsyncHandle   *handle,
					    GnomeVFSResult         result,
					    HtmlReadData          *read_data);

static void     html_widget_async_read_cb  (GnomeVFSAsyncHandle   *handle,
					    GnomeVFSResult         result,
					    gpointer               buffer,
					    GnomeVFSFileSize       bytes_requested,
					    GnomeVFSFileSize       bytes_read,
					    HtmlReadData          *read_data);
static void     html_widget_async_close_cb (GnomeVFSAsyncHandle   *handle,
					    GnomeVFSResult         result,
					    HtmlReadData          *read_data);


struct _HtmlWidgetPriv {
	GnomeVFSURI     *current_uri;
	
	HtmlReadData    *page_load_data;
};

GtkType
html_widget_get_type (void)
{
	static GtkType html_widget_type = 0;
        
	if (!html_widget_type) {
		static const GtkTypeInfo html_widget_info = {
			"HtmlWidget",
			sizeof (HtmlWidget),
			sizeof (HtmlWidgetClass),
			(GtkClassInitFunc)  html_widget_class_init,
			(GtkObjectInitFunc) html_widget_init,
			NULL, /* -- Reserved -- */
			NULL, /* -- Reserved -- */
			(GtkClassInitFunc) NULL,
		};
                
		html_widget_type = gtk_type_unique (GTK_TYPE_HTML, &html_widget_info);
	}
		
	return html_widget_type;
}

static void
html_widget_init (HtmlWidget *html_widget)
{
	HtmlWidgetPriv   *priv;
	
	priv              = g_new0 (HtmlWidgetPriv, 1);
	priv->current_uri = NULL;
	
	html_widget->priv = priv;
}

static void
html_widget_destroy (GtkObject *object)
{
	HtmlWidget       *html_widget;
        HtmlWidgetPriv   *priv;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_HTML_WIDGET (object));
		
	html_widget = HTML_WIDGET (object);
	priv        = html_widget->priv;
	
	if (priv->page_load_data) {
		html_read_data_free (priv->page_load_data);
	}
	
	if (priv->current_uri) {
		gnome_vfs_uri_unref (priv->current_uri);
	}

	g_free (priv);
	
	html_widget->priv = NULL;
}

static void
html_widget_class_init (GtkObjectClass *klass)
{
	GtkHTMLClass   *gtk_html_class;

	gtk_html_class = (GtkHTMLClass *) klass;
	
	klass->destroy = html_widget_destroy;
	gtk_html_class->url_requested = html_widget_url_requested;
	
}

static gboolean
html_widget_is_new_uri (HtmlWidget *html_widget, const GnomeVFSURI *uri)
{
	HtmlWidgetPriv   *priv;
	
	g_return_val_if_fail (html_widget != NULL, TRUE);
	g_return_val_if_fail (IS_HTML_WIDGET (html_widget), TRUE);
	g_return_val_if_fail (uri != NULL, FALSE);
	
	priv = html_widget->priv;

	if (!priv->current_uri) {
		return TRUE;
	}
	
	if (gnome_vfs_uri_equal (priv->current_uri, uri)) {
		return FALSE;
	}

	return TRUE;
}

static void
html_read_data_free (HtmlReadData *read_data)
{
	if (read_data->anchor) {
		g_free (read_data->anchor);
	}

	g_free (read_data);
}

GtkWidget *
html_widget_new (void)
{
	HtmlWidget *html_widget;
	gchar      *str;

	html_widget = gtk_type_new (HTML_WIDGET_TYPE);
	gtk_html_construct (GTK_WIDGET (html_widget));

	str = g_strdup_printf ("<html><head></head><body><h1>DevHelp</h1><p>%s</p></body></html>",
			       _("Select a subject in the contents to the left "
				 "or switch to the search pane to find what you are looking for."
				 "<p>Use <b>Shift Up/Down</b> to navigate the tree to the left, and "
				 "<b>Shift Left/Right</b> to expand and collapse the books in the tree."));
	
	gtk_html_load_from_string (GTK_HTML (html_widget), str, strlen (str));
	g_free (str);

	return GTK_WIDGET (html_widget);
}

void
html_widget_open_uri (HtmlWidget          *html_widget,
		      const GnomeVFSURI   *uri)
{
	HtmlWidgetPriv   *priv;
	gboolean          load = FALSE;
	gchar            *anchor;

	g_return_if_fail (html_widget != NULL);
	g_return_if_fail (IS_HTML_WIDGET (html_widget));
	g_return_if_fail (uri != NULL);
	
	priv = html_widget->priv;

	if (priv->page_load_data) {
		if (html_widget_is_new_uri (html_widget, uri)) {
			gnome_vfs_async_cancel (priv->page_load_data->vfs_handle);
			gnome_vfs_async_close (priv->page_load_data->vfs_handle,
					       (GnomeVFSAsyncCloseCallback) html_widget_async_close_cb,
					       priv->page_load_data);
			priv->page_load_data = NULL;
			load = TRUE;
		} else {
			if (priv->page_load_data->anchor) {
				g_free (priv->page_load_data->anchor);
				priv->page_load_data->anchor =
					util_uri_get_anchor (uri);
				load = FALSE;
			}
		}
	} else {
		if (html_widget_is_new_uri (html_widget, uri)) {
			load = TRUE;
		} else {
			anchor = util_uri_get_anchor (uri);
			if (anchor) {
				g_print ("Jumping to %s\n", anchor);
				
				gtk_html_jump_to_anchor (GTK_HTML (html_widget),
							 anchor + 1);
				g_free (anchor);
				load = FALSE;
			} else {
				load = TRUE;
			}
		}
	}
	
	/* Load the page */
	if (load) {
		html_widget_load_uri (html_widget, uri,
				      gtk_html_begin (GTK_HTML (html_widget)),
				      TRUE);
	} 
}

static void
html_widget_async_open_cb (GnomeVFSAsyncHandle   *handle,
			   GnomeVFSResult         result,
			   HtmlReadData          *read_data)
{
	HtmlWidgetPriv   *priv;
 	gchar            *buffer;
	gchar            *str_uri;

	g_return_if_fail (read_data != NULL);

	d(puts(__FUNCTION__));
	
	priv        = read_data->html_widget->priv;

	if (result != GNOME_VFS_OK) {
		str_uri = gnome_vfs_uri_to_string (priv->current_uri,
						   GNOME_VFS_URI_HIDE_NONE);
		
		g_warning (_("Couldn't open uri: %s"), str_uri);
		
		g_free (str_uri);
		
		if (priv->page_load_data) {
			if (priv->page_load_data == read_data) {
				gtk_html_end (GTK_HTML (read_data->html_widget),
					      read_data->html_stream,
					      GTK_HTML_STREAM_OK);
				priv->page_load_data = NULL;
				gnome_vfs_uri_unref (priv->current_uri);
				priv->current_uri = NULL;
			}

			html_read_data_free (read_data);
		}
		return;
	}
	
	buffer = g_new (gchar, READ_BUFFER_SIZE);
	
	gnome_vfs_async_read (handle, 
			      buffer,
			      READ_BUFFER_SIZE - 1,
			      (GnomeVFSAsyncReadCallback)html_widget_async_read_cb,
			      read_data);
}

static void
html_widget_async_read_cb (GnomeVFSAsyncHandle   *handle,
 			   GnomeVFSResult         result,
			   gpointer               buffer,
			   GnomeVFSFileSize       bytes_requested,
			   GnomeVFSFileSize       bytes_read,
			   HtmlReadData          *read_data)
{
	g_return_if_fail (read_data != NULL);
	
	d(puts(__FUNCTION__));
	
	if (result != GNOME_VFS_OK) {
		gnome_vfs_async_close (handle, 
				       (GnomeVFSAsyncCloseCallback) html_widget_async_close_cb,
				       read_data);
	} 

	if (bytes_read > 0) {
		gtk_html_write (GTK_HTML (read_data->html_widget), 
				read_data->html_stream,
				buffer, 
				bytes_read);

		gnome_vfs_async_read (handle, 
				      buffer,
				      READ_BUFFER_SIZE,
				      (GnomeVFSAsyncReadCallback) html_widget_async_read_cb,
				      read_data);
	} 
}


static void
html_widget_async_close_cb (GnomeVFSAsyncHandle   *handle,
			    GnomeVFSResult         result,
			    HtmlReadData          *read_data)
{
	HtmlWidgetPriv   *priv;
	
	g_return_if_fail (read_data != NULL);

	d(puts(__FUNCTION__));
	
	priv = read_data->html_widget->priv;
	
	if (priv->page_load_data && priv->page_load_data == read_data) {
		gtk_html_end (GTK_HTML (read_data->html_widget),
			      read_data->html_stream, 
			      GTK_HTML_STREAM_OK);
		priv->page_load_data = NULL;
	}
	
	if (read_data->anchor) {
		gtk_html_jump_to_anchor (GTK_HTML (read_data->html_widget), 
					 read_data->anchor + 1);
	}
	
	html_read_data_free (read_data);
}

static void
html_widget_load_uri (HtmlWidget          *html_widget,
		      const GnomeVFSURI   *uri,
		      GtkHTMLStream       *handle,
		      gboolean             page_load)
{
	HtmlWidgetPriv        *priv;
	HtmlReadData          *read_data;
	
	g_return_if_fail (html_widget != NULL);
	g_return_if_fail (IS_HTML_WIDGET (html_widget));
	g_return_if_fail (uri != NULL);
	
	priv = html_widget->priv;

	if (priv->current_uri) {
		gnome_vfs_uri_unref (priv->current_uri);
	}
	
	priv->current_uri = gnome_vfs_uri_dup (uri);

	read_data = g_new0 (HtmlReadData, 1); 
	read_data->html_widget = html_widget;
	read_data->html_stream = handle;
	read_data->anchor      = util_uri_get_anchor (uri);

	if (page_load) {
		if (priv->page_load_data) {
			html_read_data_free (priv->page_load_data);
		}
		
		priv->page_load_data = read_data;
	}
	
	gnome_vfs_async_open_uri (&read_data->vfs_handle, 
				  (GnomeVFSURI *) uri,
				  GNOME_VFS_OPEN_READ,
				  (GnomeVFSAsyncOpenCallback)html_widget_async_open_cb, 
				  read_data);
}

/* html "url_requested" 
 *
 */
static void
html_widget_url_requested (GtkHTML         *gtk_html,
			   const gchar     *url,
			   GtkHTMLStream   *handle)
{
	HtmlWidget       *html_widget;
	HtmlWidgetPriv   *priv;
	GnomeVFSURI      *uri;
	
	g_return_if_fail (gtk_html != NULL);
	g_return_if_fail (IS_HTML_WIDGET (gtk_html));
	g_return_if_fail (handle != NULL);
	g_return_if_fail (url != NULL);

	html_widget = HTML_WIDGET (gtk_html);
	priv        = html_widget->priv;
	uri         = util_uri_relative_new (url, priv->current_uri);
	
	html_widget_load_uri (html_widget, uri, handle, FALSE);

	gnome_vfs_uri_unref (uri);
}

void
html_widget_print (HtmlWidget *html_widget)
{
	GtkWidget         *dialog;
	gchar             *paper_name;
	GnomePrintMaster  *print_master;
	GnomePrintContext *ctx;
	const GnomePaper  *paper;
	gboolean           preview, landscape;
	int                btn;
	
	paper_name = NULL;

	preview = FALSE;
	dialog = gnome_print_dialog_new (_("Print Help"), 0);
	gtk_window_set_wmclass (GTK_WINDOW (dialog),
				"Print",
				"DevHelp");
	
	btn = gnome_dialog_run (GNOME_DIALOG (dialog));
	switch (btn) {
	case -1:
		return;
		
	case GNOME_PRINT_CANCEL:
		gtk_widget_destroy (dialog);
		return;
		
	case GNOME_PRINT_PREVIEW:
		preview = TRUE;
		break;
	default:
		break;
	};

	landscape = FALSE;
	
	print_master = gnome_print_master_new_from_dialog (
		GNOME_PRINT_DIALOG (dialog));
	
	/* Get the paper metrics. */
	if (paper_name) {
		paper = gnome_paper_with_name (paper_name);
	} else {
		paper = gnome_paper_with_name (gnome_paper_name_default ());
	}
	
	gnome_print_master_set_paper (print_master, paper);
	
	ctx = gnome_print_master_get_context (print_master);
	
	gtk_html_print (GTK_HTML (html_widget), ctx);
	
	gnome_print_master_close (print_master);
	
	if (preview) {
		GnomePrintMasterPreview *preview;
		
		preview = gnome_print_master_preview_new_with_orientation (
			print_master, _("Print Preview"), landscape);
		gtk_window_set_wmclass (GTK_WINDOW (preview),
					"PrintPreview",
					"DevHelp");
		gtk_widget_show (GTK_WIDGET (preview));
	} else {
		int result;
		
		result = gnome_print_master_print (print_master);
		
		if (result == -1) {
			g_warning (_("Printing failed."));
		}
	}
	
	/* Done. */
	gtk_object_unref (GTK_OBJECT (print_master));
	gtk_widget_destroy (dialog);
}
