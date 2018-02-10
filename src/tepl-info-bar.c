/* Code copied from Tepl:
 * https://wiki.gnome.org/Projects/Tepl
 * Do not modify, modify upstream and then copy it back here.
 */

/*
 * This file is part of Tepl, a text editor library.
 *
 * Copyright 2016, 2017 - Sébastien Wilmet <swilmet@gnome.org>
 *
 * Tepl is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * Tepl is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "tepl-info-bar.h"

/*
 * SECTION:info-bar
 * @Short_description: Subclass of GtkInfoBar
 * @Title: TeplInfoBar
 *
 * #TeplInfoBar is a subclass of #GtkInfoBar with a vertical action area and
 * functions to ease the creation of info bars.
 */

typedef struct _TeplInfoBarPrivate TeplInfoBarPrivate;

struct _TeplInfoBarPrivate
{
	/* Left: icon. Right: content_vgrid. */
	GtkGrid *content_hgrid;

	/* Contains primary/secondary messages. */
	GtkGrid *content_vgrid;

	guint close_button_added : 1;
};

G_DEFINE_TYPE_WITH_PRIVATE (TeplInfoBar, tepl_info_bar, GTK_TYPE_INFO_BAR)

static void
tepl_info_bar_response (GtkInfoBar *gtk_info_bar,
			gint        response_id)
{
	TeplInfoBar *info_bar = TEPL_INFO_BAR (gtk_info_bar);
	TeplInfoBarPrivate *priv = tepl_info_bar_get_instance_private (info_bar);

	if (response_id == GTK_RESPONSE_CLOSE &&
	    priv->close_button_added)
	{
		gtk_widget_destroy (GTK_WIDGET (info_bar));

		/* No need to chain up, the widget is destroyed. */
		return;
	}

	if (GTK_INFO_BAR_CLASS (tepl_info_bar_parent_class)->response != NULL)
	{
		GTK_INFO_BAR_CLASS (tepl_info_bar_parent_class)->response (gtk_info_bar,
									   response_id);
	}
}

static void
tepl_info_bar_class_init (TeplInfoBarClass *klass)
{
	GtkInfoBarClass *info_bar_class = GTK_INFO_BAR_CLASS (klass);

	info_bar_class->response = tepl_info_bar_response;
}

static void
tepl_info_bar_init (TeplInfoBar *info_bar)
{
	TeplInfoBarPrivate *priv;
	GtkWidget *action_area;
	GtkWidget *content_area;

	priv = tepl_info_bar_get_instance_private (info_bar);

	_tepl_info_bar_set_size_request (GTK_INFO_BAR (info_bar));

	/* Change the buttons orientation to be vertical.
	 *
	 * With a small window, if 3 or more buttons are shown horizontally,
	 * there is a ridiculous amount of space for the text. And it can get
	 * worse since the button labels are translatable, in other languages it
	 * can take even more place. If the buttons are packed vertically, there
	 * is no problem.
	 *
	 * The GtkInfoBar implementation comes originally from gedit, and the
	 * action area was vertical. Then IIRC a GNOME designer decided that the
	 * action area must be horizontal, making the gedit info bars look
	 * weird... So, come back to the original design.
	 */
	action_area = gtk_info_bar_get_action_area (GTK_INFO_BAR (info_bar));
	if (GTK_IS_ORIENTABLE (action_area))
	{
		gtk_orientable_set_orientation (GTK_ORIENTABLE (action_area),
						GTK_ORIENTATION_VERTICAL);
	}
	else
	{
		g_warning ("Failed to set vertical orientation to the GtkInfoBar action area.");
	}

	/* hgrid */
	priv->content_hgrid = GTK_GRID (gtk_grid_new ());
	gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->content_hgrid),
					GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_set_column_spacing (priv->content_hgrid, 16);
	gtk_widget_show (GTK_WIDGET (priv->content_hgrid));

	content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar));
	gtk_container_add (GTK_CONTAINER (content_area),
			   GTK_WIDGET (priv->content_hgrid));

	/* vgrid */
	priv->content_vgrid = GTK_GRID (gtk_grid_new ());
	gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->content_vgrid),
					GTK_ORIENTATION_VERTICAL);
	gtk_grid_set_row_spacing (priv->content_vgrid, 6);
	gtk_widget_show (GTK_WIDGET (priv->content_vgrid));

	gtk_container_add (GTK_CONTAINER (priv->content_hgrid),
			   GTK_WIDGET (priv->content_vgrid));
}

/**
 * tepl_info_bar_new:
 *
 * Returns: a new #TeplInfoBar.
 * Since: 1.0
 */
TeplInfoBar *
tepl_info_bar_new (void)
{
	return g_object_new (TEPL_TYPE_INFO_BAR, NULL);
}

/**
 * tepl_info_bar_new_simple:
 * @msg_type: the message type.
 * @primary_msg: the primary message.
 * @secondary_msg: (nullable): the secondary message, or %NULL.
 *
 * Creates a new #TeplInfoBar with an icon (depending on @msg_type), a primary
 * message and a secondary message.
 *
 * Returns: a new #TeplInfoBar.
 * Since: 2.0
 */
TeplInfoBar *
tepl_info_bar_new_simple (GtkMessageType  msg_type,
			  const gchar    *primary_msg,
			  const gchar    *secondary_msg)
{
	TeplInfoBar *info_bar;

	g_return_val_if_fail (primary_msg != NULL, NULL);

	info_bar = tepl_info_bar_new ();

	gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar), msg_type);
	tepl_info_bar_add_icon (info_bar);
	tepl_info_bar_add_primary_message (info_bar, primary_msg);

	if (secondary_msg != NULL)
	{
		tepl_info_bar_add_secondary_message (info_bar, secondary_msg);
	}

	return info_bar;
}

static const gchar *
get_icon_name (TeplInfoBar *info_bar)
{
	GtkMessageType msg_type;

	msg_type = gtk_info_bar_get_message_type (GTK_INFO_BAR (info_bar));

	switch (msg_type)
	{
		case GTK_MESSAGE_INFO:
			return "dialog-information";

		case GTK_MESSAGE_WARNING:
			return "dialog-warning";

		case GTK_MESSAGE_QUESTION:
			return "dialog-question";

		case GTK_MESSAGE_ERROR:
			return "dialog-error";

		case GTK_MESSAGE_OTHER:
		default:
			/* No icon */
			break;
	}

	return NULL;
}

/**
 * tepl_info_bar_add_icon:
 * @info_bar: a #TeplInfoBar.
 *
 * Adds an icon on the left, determined by the message type. So before calling
 * this function, gtk_info_bar_set_message_type() must have been called.
 *
 * The icon is not updated when the message type changes. Another #TeplInfoBar
 * must be created in that case.
 *
 * Since: 2.0
 */
void
tepl_info_bar_add_icon (TeplInfoBar *info_bar)
{
	TeplInfoBarPrivate *priv;
	const gchar *icon_name;
	GtkWidget *image;

	g_return_if_fail (TEPL_IS_INFO_BAR (info_bar));

	priv = tepl_info_bar_get_instance_private (info_bar);

	icon_name = get_icon_name (info_bar);
	if (icon_name == NULL)
	{
		return;
	}

	image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_DIALOG);
	gtk_widget_set_valign (image, GTK_ALIGN_START);
	gtk_widget_show (image);

	gtk_grid_attach_next_to (priv->content_hgrid,
				 image,
				 GTK_WIDGET (priv->content_vgrid),
				 GTK_POS_LEFT,
				 1,
				 1);
}

/**
 * tepl_info_bar_add_primary_message:
 * @info_bar: a #TeplInfoBar.
 * @primary_msg: a primary message.
 *
 * Adds a primary message.
 * Since: 2.0
 */
void
tepl_info_bar_add_primary_message (TeplInfoBar *info_bar,
				   const gchar *primary_msg)
{
	TeplInfoBarPrivate *priv;
	gchar *primary_msg_escaped;
	gchar *primary_markup;
	GtkLabel *primary_label;

	g_return_if_fail (TEPL_IS_INFO_BAR (info_bar));
	g_return_if_fail (primary_msg != NULL);

	priv = tepl_info_bar_get_instance_private (info_bar);

	primary_msg_escaped = g_markup_escape_text (primary_msg, -1);
	primary_markup = g_strdup_printf ("<b>%s</b>", primary_msg_escaped);
	primary_label = tepl_info_bar_create_label ();
	gtk_label_set_markup (primary_label, primary_markup);
	g_free (primary_markup);
	g_free (primary_msg_escaped);

	gtk_widget_show (GTK_WIDGET (primary_label));
	gtk_container_add (GTK_CONTAINER (priv->content_vgrid),
			   GTK_WIDGET (primary_label));
}

/**
 * tepl_info_bar_add_secondary_message:
 * @info_bar: a #TeplInfoBar.
 * @secondary_msg: a secondary message.
 *
 * Adds a secondary message.
 * Since: 2.0
 */
void
tepl_info_bar_add_secondary_message (TeplInfoBar *info_bar,
				     const gchar *secondary_msg)
{
	TeplInfoBarPrivate *priv;
	gchar *secondary_msg_escaped;
	gchar *secondary_markup;
	GtkLabel *secondary_label;

	g_return_if_fail (TEPL_IS_INFO_BAR (info_bar));
	g_return_if_fail (secondary_msg != NULL);

	priv = tepl_info_bar_get_instance_private (info_bar);

	secondary_msg_escaped = g_markup_escape_text (secondary_msg, -1);
	secondary_markup = g_strdup_printf ("<small>%s</small>", secondary_msg_escaped);
	secondary_label = tepl_info_bar_create_label ();
	gtk_label_set_markup (secondary_label, secondary_markup);
	g_free (secondary_markup);
	g_free (secondary_msg_escaped);

	gtk_widget_show (GTK_WIDGET (secondary_label));
	gtk_container_add (GTK_CONTAINER (priv->content_vgrid),
			   GTK_WIDGET (secondary_label));
}

/**
 * tepl_info_bar_add_content_widget:
 * @info_bar: a #TeplInfoBar.
 * @content: a #GtkWidget.
 *
 * Adds @content to @info_bar.
 *
 * #TeplInfoBar has an internal container, to be able to add the icon and add
 * primary or secondary messages. The internal container is added to the content
 * area, as returned by gtk_info_bar_get_content_area(). So if you use a
 * #TeplInfoBar and you need to add a custom #GtkWidget, it is better to use
 * this function instead of adding the #GtkWidget directly to the content area.
 *
 * Since: 2.0
 */
void
tepl_info_bar_add_content_widget (TeplInfoBar *info_bar,
				  GtkWidget   *content)
{
	TeplInfoBarPrivate *priv;

	g_return_if_fail (TEPL_IS_INFO_BAR (info_bar));
	g_return_if_fail (GTK_IS_WIDGET (content));

	priv = tepl_info_bar_get_instance_private (info_bar);

	gtk_container_add (GTK_CONTAINER (priv->content_vgrid), content);
}

/**
 * tepl_info_bar_add_close_button:
 * @info_bar: a #TeplInfoBar.
 *
 * Calls gtk_info_bar_set_show_close_button(), and additionnally closes the
 * @info_bar when the #GtkInfoBar::response signal is received with the
 * @response_id %GTK_RESPONSE_CLOSE.
 *
 * Since: 2.0
 */
void
tepl_info_bar_add_close_button (TeplInfoBar *info_bar)
{
	TeplInfoBarPrivate *priv;

	g_return_if_fail (TEPL_IS_INFO_BAR (info_bar));

	priv = tepl_info_bar_get_instance_private (info_bar);

	gtk_info_bar_set_show_close_button (GTK_INFO_BAR (info_bar), TRUE);

	priv->close_button_added = TRUE;
}

/**
 * tepl_info_bar_create_label:
 *
 * Utility function to create a #GtkLabel suitable for a #GtkInfoBar. The
 * wrapping and alignment is configured. The label is also set as selectable,
 * for example to copy an error message and search an explanation on the web.
 *
 * Returns: (transfer floating): a new #GtkLabel suitable for a #GtkInfoBar.
 * Since: 1.0
 */
GtkLabel *
tepl_info_bar_create_label (void)
{
	GtkLabel *label;

	label = GTK_LABEL (gtk_label_new (NULL));
	gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
	gtk_label_set_xalign (label, 0.0);
	gtk_label_set_line_wrap (label, TRUE);
	gtk_label_set_line_wrap_mode (label, PANGO_WRAP_WORD_CHAR);
	gtk_label_set_selectable (label, TRUE);

	/* Since the wrapping is enabled, we need to set a minimum width.
	 *
	 * If a minimum width is not set, adding an info bar to a container
	 * (e.g. a TeplTab) can make the GtkWindow height to grow. Because
	 * without a minimum width (and without ellipsization), when the user
	 * resizes the window (e.g. reducing the width) the widgets inside the
	 * window must be able to be drawn. When the info bar must be drawn with
	 * a width of e.g. 20 pixels, it takes a huge height because of the text
	 * wrapping. So by setting a minimum width to the label, the maximum
	 * height that the info bar can take is limited, so in most cases the
	 * GtkWindow current height is sufficient to draw the info bar with its
	 * maximum height.
	 *
	 * See:
	 * https://wiki.gnome.org/HowDoI/Labels
	 *
	 * There is also a safety net in tepl_tab_add_info_bar() which calls
	 * gtk_widget_set_size_request() on the GtkInfoBar, to set a minimum
	 * width.
	 */
	gtk_label_set_width_chars (label, 30);

	return label;
}

void
_tepl_info_bar_set_size_request (GtkInfoBar *info_bar)
{
	gint min_width;
	gint min_height;

	g_return_if_fail (GTK_IS_INFO_BAR (info_bar));

	gtk_widget_get_size_request (GTK_WIDGET (info_bar), &min_width, &min_height);

	/* If min_width != -1, gtk_widget_set_size_request() has already been
	 * called, so don't change the value.
	 */
	if (min_width == -1)
	{
		/* Safety net to avoid in most cases the GtkWindow height to
		 * grow.
		 *
		 * The gtk_label_set_width_chars() call in
		 * tepl_info_bar_create_label() fixes the problem at the root,
		 * but we cannot enforce all GtkLabel of @info_bar to be created
		 * with tepl_info_bar_create_label(), so a safety net is better.
		 */
		gtk_widget_set_size_request (GTK_WIDGET (info_bar), 300, min_height);
	}
}
