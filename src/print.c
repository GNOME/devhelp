/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Richard Hult <rhult@codefactory.se>
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
 * Author:  Richard Hult <rhult@codefactory.se>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <glib.h>
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

#include "print.h"

void
print (GtkHTML *html)
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
	
	gtk_html_print (html, ctx);
	
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
