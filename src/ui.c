/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
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
 * Author: Johan Dahlin <zilch.am@home.se>
 *
 * Create the ui and all callbacks.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-about.h>
#include <libgnomeui/gnome-appbar.h>
#include <glade/glade.h>
#include <string.h>

#include "ui.h"
#include "book.h"
#include "bookmark-manager.h"
#include "history.h"
#include "html-widget.h"
#include "main.h"
#include "preferences.h"
#include "preferences-dialog.h"
#include "books-dialog.h"

typedef enum {
	DEVHELP_PIXMAP_NONE,
	DEVHELP_PIXMAP_BOOK,
	DEVHELP_PIXMAP_DOC
} DevhelpPixmap;

static void            devhelp_destroy_pixmaps (DevHelpPixmaps   *pixmaps);
static GtkWidget      *get_menu_item_from_index (DevHelp *devhelp, gint index);

DevHelpPixmaps *
devhelp_create_pixmaps (DevHelp *devhelp)
{
	DevHelpPixmaps   *pixmaps;
	GtkStyle         *style;
	GdkPixbuf        *pixbuf;
	
	pixmaps = g_new0 (DevHelpPixmaps, 1);
	style   = devhelp->window->style;
	
	pixbuf = gdk_pixbuf_new_from_file (DATA_DIR "/images/devhelp/book_red.xpm");
	gdk_pixbuf_render_pixmap_and_mask (pixbuf,
					   &pixmaps->pixmap_closed,
					   &pixmaps->mask_closed,
					   127);
					   
	pixbuf = gdk_pixbuf_new_from_file (DATA_DIR "/images/devhelp/book_open.xpm");
	gdk_pixbuf_render_pixmap_and_mask (pixbuf,
					   &pixmaps->pixmap_opened,
					   &pixmaps->mask_opened,
					   127);
	
	pixbuf = gdk_pixbuf_new_from_file (DATA_DIR "/images/devhelp/helpdoc.xpm");
	gdk_pixbuf_render_pixmap_and_mask (pixbuf,
					   &pixmaps->pixmap_helpdoc,
					   &pixmaps->mask_helpdoc,
					   127);

	return pixmaps;
}

void  
devhelp_insert_book_node (DevHelp          *devhelp, 
			  GtkCTreeNode     *parent, 
			  BookNode         *node,
			  DevHelpPixmaps   *pixmaps)
{
	GSList         *chapters;
	gchar          *text;
	GtkCTreeNode   *ctree_node = NULL;
	GnomeVFSURI    *uri;
	Book           *book;
	Document       *document;
	
	g_return_if_fail (devhelp != NULL);
	g_return_if_fail (node != NULL);

	text = (gchar *) book_node_get_title (node);

	if (book_node_is_chapter (node)) {
		/* Is the book hidden, then skip */
		document = book_node_get_document (node);
		book = document_get_book (document);

		if (book_is_visible (book) == FALSE) {
			return;
		}
	
		ctree_node = gtk_ctree_insert_node (GTK_CTREE (devhelp->ctree),
						    parent,
						    NULL,
						    &text,
						    5,
						    pixmaps->pixmap_closed, 
						    pixmaps->mask_closed,
						    pixmaps->pixmap_opened, 
						    pixmaps->mask_opened, 
						    FALSE,
						    FALSE);
	} else {
		ctree_node = gtk_ctree_insert_node (GTK_CTREE (devhelp->ctree),
						    parent,
						    NULL,
						    &text,
						    5,
						    pixmaps->pixmap_helpdoc, 
						    pixmaps->mask_helpdoc,
						    pixmaps->pixmap_helpdoc, 
						    pixmaps->mask_helpdoc,
						    FALSE,
						    FALSE);
	}
	
	gtk_ctree_node_set_row_data (GTK_CTREE (devhelp->ctree), 
				     ctree_node, node);
	
	chapters = book_node_get_contents (node);
	
	for (; chapters; chapters = chapters->next) {
		devhelp_insert_book_node (devhelp, 
					  ctree_node,
					  (BookNode *) chapters->data,
					  pixmaps);
	}
}

void 
devhelp_create_book_tree (DevHelp *devhelp) 
{
	GSList           *books;
	Book             *book;
	GSList           *chapters;
	DevHelpPixmaps   *pixmaps;
	gchar            *text[1];
	GtkCTreeNode     *ctree_node;
	
	g_return_if_fail (devhelp != NULL);

	pixmaps = devhelp_create_pixmaps (devhelp);
	
	books = bookshelf_get_books (devhelp->bookshelf);
	
	gtk_clist_freeze (GTK_CLIST (devhelp->ctree));
	
	for (; books; books = books->next) {
		book = BOOK (books->data);

		devhelp_insert_book_node (devhelp, NULL, 
					  book_get_root (book), pixmaps);
	}

	gtk_clist_thaw (GTK_CLIST (devhelp->ctree));
	
	g_free (pixmaps);
}

void
gtk_ctree_goto (GtkCTree *ctree,
		BookNode *book_node)
{
	GtkCTreeNode   *node;
	GtkCTreeRow    *row;
	GtkCTreeNode   *parent;
	
	g_return_if_fail (ctree != NULL);
	g_return_if_fail (GTK_IS_CTREE (ctree));
	
	node = gtk_ctree_find_by_row_data (ctree, NULL, book_node);
	
	if (node) {
		row = GTK_CTREE_ROW (node);

		while (parent = row->parent) {
			gtk_ctree_expand (ctree, row->parent);
			row = GTK_CTREE_ROW (parent);
		}

		gtk_ctree_select (ctree, node);
		gtk_ctree_node_moveto (ctree, node, 0, 0.5, 0.5);
	}
}

gboolean
gtk_clist_if_exact_go_there (GtkCList    *clist,
			     const gchar *string)
{
	gint        i;
	gboolean    full_hit;
	gchar      *text;

	g_return_if_fail (clist != NULL);
	g_return_if_fail (GTK_IS_CLIST (clist));
	
	i = 0;
	full_hit = FALSE;

	while (gtk_clist_get_text (clist, i++, 0, &text)) {
		if (strcmp (text, string) == 0) {
			full_hit = TRUE;
			gtk_clist_moveto (clist, i - 1, 0, 0, 0);
			gtk_clist_select_row (clist, i - 1, 0);
			break;
		}
	}
	
	return full_hit;
}

#define MAX_HITS 250

/* Prepends contents of a GSList to a GtkCList
 *
 */
void
gtk_clist_set_contents (GtkCList *clist,
			GSList   *list)
{
	Function   *function;
	GSList     *node;
	gchar      *tmp[1];
	gint        row;
	gchar      *data;
	gint        hits;
	
	g_return_if_fail (clist != NULL);
	g_return_if_fail (GTK_IS_CLIST (clist));
	g_return_if_fail (list != NULL);

	gtk_clist_clear (clist);
	gtk_clist_freeze (clist);

	hits = 0;

	for (node = list; node; node = node->next) {
		if (hits++ >= MAX_HITS)
			break;
		
		function = (Function *) node->data;
		
		tmp[0] = function->name;

		row = gtk_clist_append (clist, tmp);
		gtk_clist_set_row_data (clist, row, function);
	}

	gtk_clist_thaw (clist);
}

static void
main_window_realize_cb (GtkWidget *widget,
			DevHelp   *devhelp)
{
	GtkWidget *hpaned;
	gint       width;
    
	g_return_if_fail (widget != NULL);
	g_return_if_fail (devhelp != NULL);	

	/* Does not work yet. */
	width = preferences_get_sidebar_position (devhelp->preferences);
	width = 230;
	gtk_paned_set_position (GTK_PANED (devhelp->hpaned), width);
}

static gboolean
main_window_delete_event_cb (GtkWidget*   widget,
			     GdkEventAny *event,
			     DevHelp     *devhelp)
{
	gtk_main_quit ();
	return FALSE;
}

static void
main_window_key_press_event_cb (GtkWidget*   widget,
				GdkEventKey *event,
				DevHelp     *devhelp)
{
	GtkCList     *clist;
	GtkCTreeNode *node;
	guint         pos;
	
	g_return_if_fail (devhelp != NULL);
	
	if ((event->state & (GDK_SHIFT_MASK)) == (GDK_SHIFT_MASK)) {
		switch (event->keyval) {
		case GDK_Left:
		case GDK_Right:
			/* If it's not the CTree page, ignore */
			if (gtk_notebook_get_current_page (devhelp->notebook) != 0)
				break;
			
			/* Expand/Collapse the CTree */
			node = gtk_ctree_node_nth (devhelp->ctree, GTK_CLIST (devhelp->ctree)->focus_row);
			if (event->keyval == GDK_Left)
				gtk_ctree_collapse (devhelp->ctree, node);
			else if (event->keyval == GDK_Right)
				gtk_ctree_expand (devhelp->ctree, node);				
				
		case GDK_Up:
		case GDK_Down:
			/* Up/Down moves up down */

			/* Check which page that is focused, and get the correct clist */
			if (gtk_notebook_get_current_page (devhelp->notebook) == 0)
				clist = GTK_CLIST (devhelp->ctree);
			else
				clist = devhelp->clist;

			/* Move up or down? */
			pos = clist->focus_row;
			if (event->keyval == GDK_Up)
				pos--;
			else if (event->keyval == GDK_Down)
				pos++;

			if (pos == -1 || pos == clist->rows) {
				break;
			}

			clist->focus_row = pos;
			
			gtk_widget_draw_focus (GTK_WIDGET (devhelp->ctree));
			
			gtk_clist_select_row (clist, pos, 0);
			/* Is the current row visible, if not, scroll (with gtk_clist_moveto) */
			if (gtk_clist_row_is_visible (clist, pos) != GTK_VISIBILITY_FULL)
				gtk_clist_moveto (clist, pos, 0, 0, 0);
			
			break;
		}
	}
	else if ((event->state & (GDK_CONTROL_MASK)) == (GDK_CONTROL_MASK)) {
		switch (event->keyval) {
		case GDK_l:
			/* Select the search text and focus it, it the search tab is shown. */
			if (gtk_notebook_get_current_page (devhelp->notebook) == 1) {
				gtk_editable_select_region (GTK_EDITABLE (devhelp->entry), 0, -1);
				gtk_widget_grab_focus (devhelp->entry);
			}
		}
	}
}

static void
addbook_dialog_clicked_cb (GtkButton        *button,
			   GtkFileSelection *selector)
{
	DevHelp     *devhelp;
	gchar       *dirname;
	gchar       *filename;
	gchar       *root;
	struct stat  sb;
	
	devhelp = gtk_object_get_data (GTK_OBJECT (selector), "devhelp");
		
	filename = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (selector)));

	/* Change into directory if that's what user selected. */
	if ((stat (filename, &sb) == 0) && S_ISDIR (sb.st_mode)) {
		gchar *last_slash = strrchr (filename, '/');
		
		/* The file selector needs a '/' at the end of a
		 * directory name.
		 */
		if (!last_slash || *(last_slash + 1) != '\0') {
			dirname = g_strconcat (filename, "/", NULL);
		} else {
			dirname = g_strdup (filename);
		}
		gtk_file_selection_set_filename (selector, dirname);
		g_free (dirname);
	}
	else {
		gint pos = strlen (filename)-1;
		
		gtk_widget_destroy (GTK_WIDGET (selector));
		
		while (filename[pos--] != '/');
		dirname = g_strndup (filename, pos+1);

		chdir (dirname);
		g_free (dirname);
		
		/* Install the book in the users home directory */
		root = g_strdup_printf ("%s/.devhelp", getenv ("HOME"));
		install_book (devhelp, filename, root);
		
		g_free (root);
	}
}

static gboolean 
addbook_key_event (GtkFileSelection *file_sel, GdkEventKey *event)
{
	if (event->keyval == GDK_Escape) {
		gtk_button_clicked (GTK_BUTTON (file_sel->cancel_button));
		return TRUE;
	} else
		return FALSE;
}

static void
menu_file_addbook_activate_cb (GtkMenuItem *menu_item,
                               DevHelp     *devhelp)
{
	GtkWidget   *selector;
	
	selector = gtk_file_selection_new (_("Choose a book..."));
	gtk_window_set_wmclass (GTK_WINDOW (selector), "FileSel", "DevHelp"); 
	
	gtk_object_set_data (GTK_OBJECT (selector), "devhelp", devhelp);
	
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (selector)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (addbook_dialog_clicked_cb), selector);
	
	gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (selector)->cancel_button),
				   "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
				   (gpointer) selector);
	
	gtk_signal_connect (GTK_OBJECT (selector), "key_press_event",
			    GTK_SIGNAL_FUNC (addbook_key_event), NULL);
	
	gtk_widget_show (selector);
}

static void
menu_file_print_activate_cb (GtkMenuItem *menu_item,
			     DevHelp     *devhelp)
{
	g_return_if_fail (devhelp != NULL);
	
	print (devhelp->html_widget);
}

static void
menu_file_exit_activate_cb (GtkMenuItem *menu_item,
			    DevHelp     *devhelp)
{
	gtk_object_destroy (GTK_OBJECT (devhelp->window));
	gtk_main_quit ();
}

/* A bit of hackery to show/hide the sidebar. Don't look directly at it,
 * it will hurt your eyes.
 */
void
devhelp_ui_show_sidebar (DevHelp *devhelp, gboolean show)
{
	if (show && !gtk_widget_is_ancestor (devhelp->scrolled, devhelp->right_frame)) {
		gtk_object_ref (GTK_OBJECT (devhelp->scrolled));
		gtk_container_remove (GTK_CONTAINER (devhelp->hbox), devhelp->scrolled);
		gtk_container_add (GTK_CONTAINER (devhelp->right_frame), devhelp->scrolled);
		gtk_object_unref (GTK_OBJECT (devhelp->scrolled));
	        gtk_widget_show (GTK_WIDGET (devhelp->hpaned));
	} else if (!show && gtk_widget_is_ancestor (devhelp->scrolled, devhelp->right_frame)) {
		gtk_object_ref (GTK_OBJECT (devhelp->scrolled));
		gtk_container_remove (GTK_CONTAINER (devhelp->right_frame), devhelp->scrolled);
		gtk_box_pack_start (GTK_BOX (devhelp->hbox), devhelp->scrolled, TRUE, TRUE, 0);
		gtk_object_unref (GTK_OBJECT (devhelp->scrolled));
		gtk_widget_hide (GTK_WIDGET (devhelp->hpaned));
	}
}

static void
menu_view_show_sidebar_activate_cb (GtkMenuItem *menu_item,
				    DevHelp     *devhelp)
{
	gboolean       active;
		
	g_return_if_fail (devhelp != NULL);

	if (GTK_CHECK_MENU_ITEM (menu_item)->active) {
		devhelp_ui_show_sidebar (devhelp, TRUE);
		preferences_set_sidebar_visible (devhelp->preferences, TRUE);
	} else {
		devhelp_ui_show_sidebar (devhelp, FALSE);
		preferences_set_sidebar_visible (devhelp->preferences, FALSE);
	}
}

void
devhelp_ui_set_zoom_level (DevHelp *devhelp, gint index)
{
	gdouble   magnification;
	GtkWidget *item;

	magnification = zoom_levels[index].data / 100.0;
	magnification = CLAMP (magnification, 0.05, 20.0);
	
	gtk_html_set_magnification (GTK_HTML (devhelp->html_widget), magnification);

	/* Update the menu state to reflect the new setting. */
	item = get_menu_item_from_index (devhelp, index);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
}

void
devhelp_ui_set_autocompletion (DevHelp *devhelp, gboolean value)
{
	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (devhelp->autocomp_checkbutton), value);
}

static void
autocompletion_toggled_cb (GtkToggleButton *button,
			   DevHelp         *devhelp)
{
	gboolean value;

	value = gtk_toggle_button_get_active (button);

	devhelp_ui_set_autocompletion (devhelp, value);
	preferences_set_autocompletion (devhelp->preferences, value);

	/* Add this when we actually have non-autocompletion working! */
	/*gtk_widget_set_sensitive (devhelp->search_button, !value);*/
	gtk_widget_set_sensitive (devhelp->search_button, FALSE);
}

static void
menu_view_zoom_toggled_cb (GtkCheckMenuItem *menu_item,
			   DevHelp          *devhelp)
{
	gint level;

	/* We get this callback when the button is toggled off
	 * as well, but we're only interested when it's toggled
	 * on.
	 */
	if (!menu_item->active) {
		return;
	}
	
	level = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (menu_item),
						      "level"));
	preferences_set_zoom_level (devhelp->preferences, level);
}

static void
menu_help_about_activate_cb (GtkMenuItem *menu_item)
{
	GtkWidget    *about;
	const gchar  *authors[] = {
		"Johan Dahlin <zilch.am@home.se>",
		"Mikael Hallendal <micke@codefactory.se>",
		"Rickard Hult <rhult@codefactory.se>",
		NULL
	};
	 
	about = gnome_about_new ("DevHelp", VERSION,
				 _("(C) Copyright 2001, Johan Dahlin"), authors,
				 _("A developer's help browser"), NULL);
	gtk_widget_show (GTK_WIDGET (about));
}

static void
toolbar_button_forward_clicked_cb (GtkWidget *button,
				   DevHelp   *devhelp)
{
	const Document   *document;
	gchar            *anchor = NULL;
	GnomeVFSURI      *uri;
	
	g_return_if_fail (devhelp != NULL);

	document = history_go_forward (devhelp->history, &anchor);

	if (document) {
		uri = document_get_uri (document, anchor);
		
		if (uri) {
			bookshelf_open_document (devhelp->bookshelf, document);
			html_widget_open_uri (devhelp->html_widget, uri);
			gnome_vfs_uri_unref (uri);
		}
		
		if (anchor) {
			g_free (anchor);
		}
	}
}

static void
toolbar_button_back_clicked_cb (GtkWidget   *button,
				DevHelp     *devhelp)
{
	const Document   *document;
	gchar            *anchor = NULL;
	GnomeVFSURI      *uri;
	
	g_return_if_fail (devhelp != NULL);

	document = history_go_back (devhelp->history, &anchor);

	if (document) {
		uri = document_get_uri (document, anchor);
		
		if (uri) {
			bookshelf_open_document (devhelp->bookshelf, 
						 document);
			html_widget_open_uri (devhelp->html_widget, uri);
			gnome_vfs_uri_unref (uri);
		}
		
		if (anchor) {
			g_free (anchor);
		}
	}
}

static void
toolbar_buttons_update_cb (History   *history,
			   gboolean   exist,
			   GtkWidget *widget) 
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_BUTTON (widget));
	
	gtk_widget_set_sensitive (widget, exist);
}

static void
scrolled_size_allocate_cb (GtkWidget     *widget,
			   GtkAllocation *allocation,
			   DevHelp       *devhelp)
{
	g_return_if_fail (devhelp != NULL);

	/*g_print ("width is: %d\n", allocation->width);*/
	/*preferences_set_sidebar_position (devhelp->preferences, allocation->width);*/
}

static void
ctree_select_row_cb (GtkCTree       *ctree,
		     GtkCTreeNode   *node,
		     gint            column,
		     DevHelp        *devhelp)
{
	BookNode      *book_node;
	GnomeVFSURI   *uri;
	gchar         *str_uri;
	
	g_return_if_fail (devhelp != NULL);
	
	book_node = (BookNode *) gtk_ctree_node_get_row_data (GTK_CTREE (devhelp->ctree), node);

	if (book_node) {
		bookshelf_open_document (devhelp->bookshelf, 
					 book_node_get_document (book_node));
		history_goto (devhelp->history, 
			      book_node_get_document (book_node), 
			      book_node_get_anchor (book_node));

		uri = book_node_get_uri (book_node);
 		html_widget_open_uri (devhelp->html_widget, uri);

		gnome_vfs_uri_unref (uri);
	}
}

static void
clist_select_row_cb (GtkCList   *clist,
		     gint        row,
		     gint        column,
		     GdkEvent   *event,
		     DevHelp    *devhelp)
{
	Function      *function;
	BookNode      *book_node;
	GnomeVFSURI   *uri;
	

	function = (Function *) gtk_clist_get_row_data (GTK_CLIST (devhelp->clist), row);
	
	if (!function) {
		return;
	}
	
	bookshelf_open_document (devhelp->bookshelf, function->document);
	
	history_goto (devhelp->history, function->document, function->anchor);

	uri = document_get_uri (function->document, function->anchor);
	
	if (uri) {
		html_widget_open_uri (devhelp->html_widget, uri);
		gnome_vfs_uri_unref (uri);
	} else {
		g_print (_("Couldn't find book_node for function '%s'\n"), 
			 function->name);
	}
}

static void
search_entry_changed_cb (GtkEditable *editable,
			 DevHelp     *devhelp)
{
	g_return_if_fail (devhelp != NULL);
	
	function_database_idle_search (devhelp->function_database);
}

static gint
complete_idle (gpointer user_data) 
{
	DevHelp      *devhelp;
	gchar        *text, *completed;
	gint          text_length;

	g_return_if_fail (user_data != NULL);
	devhelp = (DevHelp*)user_data;
		
	text = gtk_entry_get_text (GTK_ENTRY (devhelp->entry));
	
	completed = function_database_get_completion (devhelp->function_database, text);

	if (completed) {
		text_length = strlen (text);
		
		gtk_entry_set_text (GTK_ENTRY (devhelp->entry), completed);

		gtk_editable_select_region (GTK_EDITABLE (devhelp->entry), text_length, -1);
		gtk_editable_set_position (GTK_EDITABLE (devhelp->entry), text_length);
	}
	
	devhelp->complete = 0;
	return 0;
}

static void
search_entry_insert_text_cb (GtkEditable   *editable,
                             gchar         *new_text,
                             gint           new_text_length,
                             gint          *position,
                             DevHelp       *devhelp)
{
	g_return_if_fail (devhelp != NULL);
	
	if (!devhelp->complete) {
		devhelp->complete = gtk_idle_add (complete_idle, devhelp);
	}
}

gboolean
search_entry_key_press_cb (GtkWidget     *widget, 
			   GdkEventKey   *event)
{
	gchar *txt;
	
	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_EDITABLE (widget), FALSE);

	d(puts(__FUNCTION__));
	
	switch (event->keyval) {
	case GDK_Tab:
		gtk_editable_select_region (GTK_EDITABLE (widget), 0, 0);
		gtk_editable_set_position (GTK_EDITABLE (widget), -1);
		return TRUE;
		break;
        default:
		break;
	}
	
	return FALSE;
}

static void
search_entry_activate_cb (GtkEditable *editable,
			  DevHelp     *devhelp)
{
	g_return_if_fail (devhelp != NULL);
	
	devhelp_search (devhelp, gtk_entry_get_text (GTK_ENTRY (editable)));
}

static void
search_button_clicked_cb (GtkButton *button,
			  DevHelp   *devhelp)
{
	g_return_if_fail (devhelp != NULL);
	
	devhelp_search (devhelp, gtk_entry_get_text (devhelp->entry));
}

static void
html_on_url_cb (GtkHTML     *html,
		gchar       *url,
		GnomeAppBar *bar)
{
	gchar     *tmp;

	g_return_if_fail (bar != NULL);
	g_return_if_fail (GNOME_IS_APPBAR (bar));

	if (url != NULL) {
		tmp = g_strdup_printf (_("Link to %s"), url);
		gnome_appbar_push (bar, tmp);
		g_free (tmp);
	} else {
		gnome_appbar_pop (bar);
	}
}

static void
html_enter_notify_event_cb (GtkWidget        *widget,
			    GdkEventCrossing *event,
			    GnomeAppBar      *bar)
{
	g_return_if_fail (bar != NULL);
    
	gnome_appbar_clear_stack (bar);
}

static void
html_link_clicked_cb (GtkWidget   *widget,
		      gchar       *url,
		      DevHelp     *devhelp)
{
	BookNode       *book_node;
	GnomeVFSURI    *uri;
	Document       *document;
	gchar          *anchor;
		
	g_return_if_fail (devhelp != NULL);
	
	document = bookshelf_find_document (devhelp->bookshelf, url, &anchor);

	if (document) {
		bookshelf_open_document (devhelp->bookshelf, document);
		
		history_goto (devhelp->history, document, anchor);
		
		if (gtk_notebook_get_current_page (devhelp->notebook) == 0) {
			gtk_signal_handler_block_by_func (GTK_OBJECT (devhelp->ctree),
							  ctree_select_row_cb,
							  devhelp);

			book_node = bookshelf_find_node (devhelp->bookshelf,
							 document,
							 anchor);
			
			gtk_ctree_goto (devhelp->ctree, book_node); 
			
			gtk_signal_handler_unblock_by_func (GTK_OBJECT (devhelp->ctree),
							    ctree_select_row_cb,
							    devhelp);
		}
		
		uri = document_get_uri (document, anchor);
		html_widget_open_uri (devhelp->html_widget, uri);
		gnome_vfs_uri_unref (uri);
	} else {
		g_warning (_("Cannot find clicked link (%s) in bookshelf\n"), 
			   url);
	}
}

static void
bookmark_selected_cb (GtkMenuItem *menu_item,
		      DevHelp     *devhelp)
{
	Bookmark       *bookmark;

	g_return_if_fail (devhelp != NULL);
	
	d(puts(__FUNCTION__));

	bookmark = (Bookmark *) gtk_object_get_data (GTK_OBJECT (menu_item),
						     "bookmark");
	
	if (bookmark) {
/* 		gtk_ctree_goto (devhelp->ctree, bookmark->url); */
	}
}

static void
bookmark_added_cb (BookmarkManager *bm,
		   Bookmark *bookmark,
		   DevHelp *devhelp)
{
	GtkWidget     *new_bookmark_item;

	g_return_if_fail (bookmark != NULL);
	g_return_if_fail (devhelp != NULL);

	d(puts(__FUNCTION__));

	new_bookmark_item = gtk_menu_item_new_with_label (bookmark->name);
	gtk_widget_show (new_bookmark_item);
	
	gtk_object_set_data (GTK_OBJECT (new_bookmark_item), "bookmark", bookmark);
	
	gtk_signal_connect (GTK_OBJECT (new_bookmark_item), "activate",
			    GTK_SIGNAL_FUNC (bookmark_selected_cb), devhelp);

	gtk_menu_append (GTK_MENU (devhelp->bookmark_menu), new_bookmark_item);
}

static void
bookmark_add_cb (GtkMenuItem *menuitem, DevHelp *devhelp)
{
	d(puts(__FUNCTION__));

	g_return_if_fail (devhelp != NULL);
	
/* 	bookmark_manager_add (devhelp->bookmark_manager, */
/* 			      gtk_html_get_title (GTK_HTML (devhelp->html_widget)), */
/* 			      history_get_current (devhelp->history)); */
}

gchar *
get_search_string_cb (FunctionDatabase *fd,
		      DevHelp *devhelp) 
{
	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));
	g_return_if_fail (devhelp != NULL);
	
	g_return_val_if_fail (devhelp != NULL, NULL);

	return gtk_entry_get_text (devhelp->entry);
}

void
exact_hit_found_cb (FunctionDatabase   *fd, 
		    Function           *function, 
		    DevHelp            *devhelp)
{
	
	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));
	g_return_if_fail (devhelp != NULL);

	gtk_clist_if_exact_go_there (devhelp->clist, function->name);
}

/* devhelp_search: Highlevel search function
 *
 */
void
devhelp_search (DevHelp *devhelp,
		const gchar *string)
{
	GSList   *list;

	g_return_if_fail (string != NULL);

	function_database_search (devhelp->function_database, string);
}

void 
hits_found_cb (FunctionDatabase *fd,
	       GSList *hits,
	       gpointer user_data)
{
	DevHelp   *devhelp;
	
	d(puts(__FUNCTION__));

	g_return_if_fail (fd != NULL);
	g_return_if_fail (IS_FUNCTION_DATABASE (fd));
	g_return_if_fail (hits != NULL);
	g_return_if_fail (user_data != NULL);
	
	devhelp = (DevHelp *) user_data;

	gtk_clist_set_contents (devhelp->clist, hits);
}

/* create_ui: create ui components, or load them with glade
 *
 */
DevHelp *
devhelp_create_ui (void)
{
	DevHelp   *devhelp;
	GladeXML  *gui;
	GtkWidget *w;
	GtkWidget *statusbar;
	GtkWidget *toolbar_button_back, *toolbar_button_forward;
	gboolean   sidebar_show_value;
	gint       zoom_index;
	gchar     *local_dir;
	
	devhelp = g_new0 (DevHelp, 1);

	devhelp->preferences = preferences_new (devhelp);
	
	glade_gnome_init ();

	gui = glade_xml_new (DATA_DIR "/devhelp/glade/devhelp.glade", "main_app");

	/* Main window */
	devhelp->window = glade_xml_get_widget (gui, "main_app");
	gtk_signal_connect (GTK_OBJECT (devhelp->window), "key_press_event",
			    GTK_SIGNAL_FUNC (main_window_key_press_event_cb), devhelp);
	gtk_signal_connect (GTK_OBJECT (devhelp->window), "delete_event", 
			    GTK_SIGNAL_FUNC (main_window_delete_event_cb), devhelp);

	devhelp->notebook = GTK_NOTEBOOK (glade_xml_get_widget (gui, "notebook1"));
	devhelp->bookmark_menu = glade_xml_get_widget (gui, "bookmarks_menu");
	statusbar = glade_xml_get_widget (gui, "appbar1");
	
	/* GtkHTML */
	devhelp->html_widget = HTML_WIDGET (html_widget_new ());
	gtk_signal_connect (GTK_OBJECT (devhelp->html_widget), "on_url",
			    GTK_SIGNAL_FUNC (html_on_url_cb), statusbar);
	gtk_signal_connect (GTK_OBJECT (devhelp->html_widget), "enter_notify_event",
			    GTK_SIGNAL_FUNC (html_enter_notify_event_cb), statusbar);
	gtk_signal_connect (GTK_OBJECT (devhelp->html_widget), "link_clicked",
			    GTK_SIGNAL_FUNC (html_link_clicked_cb), devhelp);
	
	/* Menu items */
	w = glade_xml_get_widget (gui, "menu_file_addbook");
	gtk_signal_connect (GTK_OBJECT (w), "activate",
			    GTK_SIGNAL_FUNC (menu_file_addbook_activate_cb), devhelp);
	
	w = glade_xml_get_widget (gui, "menu_file_print");
	gtk_signal_connect (GTK_OBJECT (w), "activate",
			    GTK_SIGNAL_FUNC (menu_file_print_activate_cb), devhelp);
	
	w = glade_xml_get_widget (gui, "menu_file_exit");
	gtk_signal_connect (GTK_OBJECT (w), "activate",
			    GTK_SIGNAL_FUNC (menu_file_exit_activate_cb), devhelp);
#if 0	
	w = glade_xml_get_widget (gui, "menu_add_bookmark");
	gtk_signal_connect (GTK_OBJECT (w),  "activate",
			    GTK_SIGNAL_FUNC (bookmark_add_cb), devhelp);
#endif
	w = glade_xml_get_widget (gui, "menu_view_show_sidebar");
	sidebar_show_value = preferences_get_sidebar_visible (devhelp->preferences);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (w), sidebar_show_value);
	gtk_signal_connect (GTK_OBJECT (w), "activate",
			    GTK_SIGNAL_FUNC (menu_view_show_sidebar_activate_cb), devhelp);

	zoom_index = preferences_get_zoom_level (devhelp->preferences);
	
	w = glade_xml_get_widget (gui, "menu_view_zoom_tiny");
	devhelp->zoom_tiny = w;
	gtk_object_set_data (GTK_OBJECT (w), "level", GINT_TO_POINTER (ZOOM_TINY_INDEX));
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (menu_view_zoom_toggled_cb), devhelp);	

	w = glade_xml_get_widget (gui, "menu_view_zoom_small");
	devhelp->zoom_small = w;
	gtk_object_set_data (GTK_OBJECT (w), "level", GINT_TO_POINTER (ZOOM_SMALL_INDEX));
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (menu_view_zoom_toggled_cb), devhelp);	

	w = glade_xml_get_widget (gui, "menu_view_zoom_medium");
	devhelp->zoom_medium = w;
	gtk_object_set_data (GTK_OBJECT (w), "level", GINT_TO_POINTER (ZOOM_MEDIUM_INDEX));
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (menu_view_zoom_toggled_cb), devhelp);	
	
	w = glade_xml_get_widget (gui, "menu_view_zoom_large");
	devhelp->zoom_large = w;
	gtk_object_set_data (GTK_OBJECT (w), "level", GINT_TO_POINTER (ZOOM_LARGE_INDEX));
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (menu_view_zoom_toggled_cb), devhelp);	
	
	w = glade_xml_get_widget (gui, "menu_view_zoom_huge");
	devhelp->zoom_huge = w;
	gtk_object_set_data (GTK_OBJECT (w), "level", GINT_TO_POINTER (ZOOM_HUGE_INDEX));
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (menu_view_zoom_toggled_cb), devhelp);	

	/* Set the right zoom menu item active. */
	w = get_menu_item_from_index (devhelp, zoom_index);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (w), TRUE);
	
	w = glade_xml_get_widget (gui, "menu_options_preferences");
	gtk_signal_connect (GTK_OBJECT (w), "activate",
			    GTK_SIGNAL_FUNC (menu_preferences_activate_cb), devhelp);

	w = glade_xml_get_widget (gui, "menu_options_books");
	gtk_signal_connect (GTK_OBJECT (w), "activate",
			    GTK_SIGNAL_FUNC (menu_options_books_activate_cb), devhelp);

	w = glade_xml_get_widget (gui, "menu_help_about");
	gtk_signal_connect (GTK_OBJECT (w), "activate",
			    GTK_SIGNAL_FUNC (menu_help_about_activate_cb), NULL);

	w = glade_xml_get_widget (gui, "autocompletion_checkbutton");
	devhelp->autocomp_checkbutton = w;
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (autocompletion_toggled_cb), devhelp);

	/* Toolbar */
	toolbar_button_back = glade_xml_get_widget (gui, "back_button");
	gtk_signal_connect (GTK_OBJECT (toolbar_button_back), "clicked",
			    GTK_SIGNAL_FUNC (toolbar_button_back_clicked_cb), devhelp);
	
	toolbar_button_forward = glade_xml_get_widget (gui, "forward_button");
	gtk_signal_connect (GTK_OBJECT (toolbar_button_forward), "clicked",
			    GTK_SIGNAL_FUNC (toolbar_button_forward_clicked_cb), devhelp);

	/* CTree */
	devhelp->ctree = GTK_CTREE (gtk_ctree_new (1, 0));
	gtk_clist_set_column_width (GTK_CLIST (devhelp->ctree), 0, 80);
	gtk_clist_set_selection_mode (GTK_CLIST (devhelp->ctree), GTK_SELECTION_BROWSE);
	
	gtk_signal_connect (GTK_OBJECT (devhelp->ctree), "tree_select_row",
			    GTK_SIGNAL_FUNC (ctree_select_row_cb), devhelp);
	w = glade_xml_get_widget (gui, "browse_scrolledwindow");
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (w), GTK_WIDGET (devhelp->ctree));
	
	/* CList */
	devhelp->clist = GTK_CLIST (gtk_clist_new (1)); 
	gtk_signal_connect (GTK_OBJECT (devhelp->clist), "select_row", 
			    GTK_SIGNAL_FUNC (clist_select_row_cb), devhelp);

	w = glade_xml_get_widget (gui, "search_scrolledwindow");
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (w), GTK_WIDGET (devhelp->clist));

	/* Search entry */
	devhelp->entry = GTK_ENTRY (glade_xml_get_widget (GLADE_XML (gui), "search_entry"));
	gtk_signal_connect (GTK_OBJECT (devhelp->entry), "activate",
			    GTK_SIGNAL_FUNC (search_entry_activate_cb), devhelp);
	gtk_signal_connect (GTK_OBJECT (devhelp->entry), "changed",
			    GTK_SIGNAL_FUNC (search_entry_changed_cb), devhelp);
	gtk_signal_connect (GTK_OBJECT (devhelp->entry), "insert-text",
			    GTK_SIGNAL_FUNC (search_entry_insert_text_cb), devhelp);
	gtk_signal_connect_after (GTK_OBJECT (devhelp->entry), "key-press-event",
				  GTK_SIGNAL_FUNC (search_entry_key_press_cb), NULL);

	/* Search button */
	devhelp->search_button = glade_xml_get_widget (gui, "search_button");
	gtk_signal_connect (GTK_OBJECT (devhelp->search_button), "clicked",
			    GTK_SIGNAL_FUNC (search_button_clicked_cb), devhelp);

	/* Add this when we actually have non-autocompletion working! */
	/*gtk_widget_set_sensitive (devhelp->search_button,
	  !preferences_get_autocompletion (devhelp->preferences));*/
	gtk_widget_set_sensitive (devhelp->search_button, FALSE);
	
	/* Box */
	devhelp->hbox = glade_xml_get_widget (gui, "hbox");

	/* Paned */
	devhelp->hpaned = glade_xml_get_widget (gui, "hpaned1");
	gtk_signal_connect (GTK_OBJECT (devhelp->window), "realize",
			    GTK_SIGNAL_FUNC (main_window_realize_cb), devhelp);

	/* Bookmark manager */
	devhelp->bookmark_manager = bookmark_manager_new ();

	gtk_signal_connect (GTK_OBJECT (devhelp->bookmark_manager), "bookmark_added",
			    GTK_SIGNAL_FUNC (bookmark_added_cb), devhelp);

	/* History */
	devhelp->history = history_new ();
	
	gtk_signal_connect (GTK_OBJECT (devhelp->history), "forward_exists_changed",
			    GTK_SIGNAL_FUNC (toolbar_buttons_update_cb), toolbar_button_forward);
	
	gtk_signal_connect (GTK_OBJECT (devhelp->history), "back_exists_changed",
			    GTK_SIGNAL_FUNC (toolbar_buttons_update_cb), toolbar_button_back);
	
	/* Function database */
	devhelp->function_database = function_database_new ();
	gtk_signal_connect (GTK_OBJECT (devhelp->function_database), "get_search_string",
			    GTK_SIGNAL_FUNC (get_search_string_cb), devhelp);

	gtk_signal_connect (GTK_OBJECT (devhelp->function_database), "exact_hit_found",
			    GTK_SIGNAL_FUNC (exact_hit_found_cb), devhelp);

	gtk_signal_connect (GTK_OBJECT (devhelp->function_database), "hits_found",
			    GTK_SIGNAL_FUNC (hits_found_cb), devhelp);

	/* ScrolledWindow */
	devhelp->scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (devhelp->scrolled),
					GTK_POLICY_AUTOMATIC, 
					GTK_POLICY_AUTOMATIC);

	devhelp->right_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (devhelp->right_frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (devhelp->right_frame), devhelp->scrolled);
	
	gtk_container_add (GTK_CONTAINER (devhelp->scrolled),
			   GTK_WIDGET (devhelp->html_widget));
	
	gtk_paned_pack2 (GTK_PANED (devhelp->hpaned),
			 devhelp->right_frame,
			 FALSE, FALSE);
	
	function_database_freeze (devhelp->function_database);
	local_dir = g_strdup_printf ("%s/.devhelp", getenv ("HOME"));
	devhelp->bookshelf = bookshelf_new (DATA_DIR"/devhelp/specs", devhelp->function_database);
	bookshelf_add_directory (devhelp->bookshelf, local_dir);
	g_free (local_dir);
	function_database_thaw (devhelp->function_database);

	devhelp_create_book_tree (devhelp);
	
	gtk_widget_show_all (GTK_WIDGET (devhelp->window));

	/* Connect settings after everything is shown so that we don't get lots of
	 * set calls while things are requestion/allocating sizes.
	 */
	gtk_signal_connect (GTK_OBJECT (devhelp->scrolled), "size_allocate",
			    GTK_SIGNAL_FUNC (scrolled_size_allocate_cb), devhelp);

	/* Settings. */
	devhelp_ui_show_sidebar (devhelp, sidebar_show_value);
	devhelp_ui_set_zoom_level (devhelp, zoom_index);
	devhelp_ui_set_autocompletion (devhelp,
				       preferences_get_autocompletion (devhelp->preferences));
	
	/* ... more settings here: */

	gtk_object_unref (GTK_OBJECT (gui));
	
	return devhelp;
}

static GtkWidget *
get_menu_item_from_index (DevHelp *devhelp, gint index)
{
	GtkWidget *item;
		
	switch (index) {
	case ZOOM_TINY_INDEX:
		item = devhelp->zoom_tiny;
		break;
	case ZOOM_SMALL_INDEX:
		item = devhelp->zoom_small;
		break;
	case ZOOM_MEDIUM_INDEX:
		item = devhelp->zoom_medium;
		break;
	case ZOOM_LARGE_INDEX:
		item = devhelp->zoom_large;
		break;
	case ZOOM_HUGE_INDEX:
		item = devhelp->zoom_huge;
		break;
	default:
		g_warning (_("Zoom index out of range."));
		return devhelp->zoom_medium;
	}		

	return item;
}
