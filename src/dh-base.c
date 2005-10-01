/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004-2005 Imendio AB
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

#include <config.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtkmain.h>
#include <gdk/gdkx.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include "dh-gecko-utils.h"
#include "dh-window.h"
#include "dh-link.h"
#include "dh-parser.h"
#include "dh-preferences.h"
#include "dh-base.h"

#define d(x)

struct _DhBasePriv {
	GSList     *windows;
	GNode      *book_tree;
	GList      *keywords;
	GHashTable *books;
};

static void base_init                (DhBase      *base);
static void base_class_init          (DhBaseClass *klass);
static void base_window_finalized_cb (DhBase      *base,
				      DhWindow    *window);
static void base_init_books          (DhBase      *base);
static void base_add_books           (DhBase      *base,
				      const gchar *directory);


static GObjectClass *parent_class;

GType
dh_base_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (DhBaseClass),
			NULL,
			NULL,
			(GClassInitFunc) base_class_init,
			NULL,
			NULL,
			sizeof (DhBase),
			0,
			(GInstanceInitFunc) base_init,
		};

		type = g_type_register_static (G_TYPE_OBJECT, "DhBase",
					       &info, 0);
	}

	return type;
}

static void
base_init (DhBase *base)
{
        DhBasePriv *priv;
	int         n_screens, i;

        priv = g_new0 (DhBasePriv, 1);
        base->priv = priv;

	priv->windows   = NULL;
	priv->book_tree = g_node_new (NULL);
	priv->keywords  = NULL;
	priv->books     = g_hash_table_new_full (g_str_hash, g_str_equal,
						 g_free, g_free);

	/* For some reason, libwnck doesn't seem to update its list of
	 * workspaces etc if we don't do this.
	 */
	n_screens = gdk_display_get_n_screens (gdk_display_get_default());
	for (i = 0; i < n_screens; i++) {
		WnckScreen *screen;

		screen = wnck_screen_get (i);
	}
}

static void
base_class_init (DhBaseClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);
}

static void
base_window_finalized_cb (DhBase *base, DhWindow *window)
{
	DhBasePriv *priv;

	priv = base->priv;

	priv->windows = g_slist_remove (priv->windows, window);

	if (g_slist_length (priv->windows) == 0) {
		gtk_main_quit ();
	}
}

static gint
book_sort_func (gconstpointer a,
		gconstpointer b)
{
	DhLink      *link_a, *link_b;
	const gchar *name_a, *name_b;

	link_a = ((GNode *) a)->data;
	link_b = ((GNode *) b)->data;

	name_a = link_a->name;
	if (!name_a) {
		name_a = "";
	}

	name_b = link_b->name;
	if (!name_b) {
		name_b = "";
	}

	if (g_ascii_strncasecmp (name_a, "the ", 4) == 0) {
		name_a += 4;
	}
	if (g_ascii_strncasecmp (name_b, "the ", 4) == 0) {
		name_b += 4;
	}

	return g_utf8_collate (name_a, name_b);
}

static void
base_sort_books (DhBase *base)
{
	DhBasePriv *priv;
	GNode      *n;
	DhLink     *link;
	GList      *list = NULL, *l;

	priv = base->priv;

	if (base->priv->book_tree) {
		n = base->priv->book_tree->children;

		while (n) {
			list = g_list_prepend (list, n);
			n = n->next;
		}

		list = g_list_sort (list, book_sort_func);
	}

	for (l = list; l; l = l->next) {
		n = l->data;
		link = n->data;
		g_node_unlink (n);
	}

	for (l = list; l; l = l->next) {
		n = l->data;

		g_node_append (base->priv->book_tree, n);
	}

	g_list_free (list);
}

static void
base_init_books (DhBase *base)
{
	const gchar *env;
	gchar       *dir;

	dir = g_build_filename (g_get_home_dir (), ".devhelp", "books", NULL);
	base_add_books (base, dir);
	g_free (dir);

	env = g_getenv ("DEVHELP_SEARCH_PATH");
	if (env) {
		gchar **paths, **p;

		paths = g_strsplit (env, ":", -1);
		for (p = paths; *p != NULL; p++) {
			base_add_books (base, *p);
		}
		g_strfreev (paths);
	}

	env = g_getenv ("GNOME2_PATH");
	if (env) {
		gchar **paths, **p;

		paths = g_strsplit (env, ":", -1);
		for (p = paths; *p != NULL; p++) {
			base_add_books (base, *p);
		}
		g_strfreev (paths);
	}

	/* Insert the books from default gtk-doc install path. */
	base_add_books (base, DATADIR "/gtk-doc/html");
	base_add_books (base, "/usr/share/gtk-doc/html");
	base_add_books (base, DATADIR "/devhelp/books");

	base_sort_books (base);
}

static void
base_add_books (DhBase *base, const gchar *path)
{
	DhBasePriv  *priv;
	GDir        *dir;
	const gchar *name;

	priv = base->priv;

	d(g_print ("Adding books from %s\n", path));

	dir = g_dir_open (path, 0, NULL);
	if (!dir) {
		return;
	}

	while ((name = g_dir_read_name (dir)) != NULL) {
		gchar  *tmp;
		gchar  *book_path;
		GError *error = NULL;

		if (g_hash_table_lookup (priv->books, name)) {
			continue;
		}

		tmp = g_build_filename (path, name, name, NULL);
		book_path = g_strconcat (tmp, ".devhelp", NULL);
		if (!g_file_test (book_path, G_FILE_TEST_EXISTS)) {
			g_free (book_path);
			book_path = g_strconcat (tmp, ".devhelp.gz", NULL);
		}

		g_free (tmp);

		if (!g_file_test (book_path, G_FILE_TEST_EXISTS)) {
			g_free (book_path);
			continue;
		}

		if (!dh_parser_read_file  (book_path,
					   priv->book_tree,
					   &priv->keywords,
					   &error)) {
			g_warning ("Failed to read '%s': %s",
				   book_path, error->message);
			g_clear_error (&error);
		} else {
			g_hash_table_insert (priv->books,
					     g_strdup (name),
					     book_path);
			d(g_print ("Found book: '%s'\n", book_path));
		}

		g_free (book_path);
	}

	g_dir_close (dir);
}

DhBase *
dh_base_new (void)
{
        DhBase          *base;
	DhBasePriv      *priv;
	static gboolean  initialized = FALSE;

	if (!initialized) {
		dh_gecko_utils_init_services ();
		initialized = TRUE;
	}

        base = g_object_new (DH_TYPE_BASE, NULL);
	priv = base->priv;

	base_init_books (base);

	dh_preferences_init ();

	return base;
}

GtkWidget *
dh_base_new_window (DhBase *base)
{
	DhBasePriv *priv;
	GtkWidget  *window;

        g_return_val_if_fail (DH_IS_BASE (base), NULL);

	priv = base->priv;

        window = dh_window_new (base);

	priv->windows = g_slist_prepend (priv->windows, window);

	g_object_weak_ref (G_OBJECT (window),
			   (GWeakNotify) base_window_finalized_cb,
			   base);

	dh_window_show (DH_WINDOW (window));

	return window;
}

GNode *
dh_base_get_book_tree (DhBase *base)
{
	g_return_val_if_fail (DH_IS_BASE (base), NULL);

	return base->priv->book_tree;
}

GList *
dh_base_get_keywords (DhBase *base)
{
	g_return_val_if_fail (DH_IS_BASE (base), NULL);

	return base->priv->keywords;
}

GSList *
dh_base_get_windows (DhBase *base)
{
	DhBasePriv *priv;

	g_return_val_if_fail (DH_IS_BASE (base), NULL);

	priv = base->priv;

	return priv->windows;
}

GtkWidget *
dh_base_get_window_on_current_workspace (DhBase *base)
{
	DhBasePriv    *priv;
	WnckWorkspace *workspace;
	WnckScreen    *screen;
	GtkWidget     *window;
	GList         *windows, *w;
	GSList        *l;
	gulong         xid;
	pid_t          pid;

	g_return_val_if_fail (DH_IS_BASE (base), NULL);

	priv = base->priv;

	if (!priv->windows) {
		return NULL;
	}

	screen = wnck_screen_get (0);
	if (!screen) {
		return NULL;
	}

	workspace = wnck_screen_get_active_workspace (screen);
	if (!workspace) {
		return NULL;
	}

	xid = 0;
	pid = getpid ();

	/* Use _stacked so we can use the one on top. */
	windows = wnck_screen_get_windows_stacked (screen);
	windows = g_list_last (windows);

	for (w = windows; w; w = w->prev) {
		if (wnck_window_is_on_workspace (w->data, workspace) &&
		    wnck_window_get_pid (w->data) == pid) {
			xid = wnck_window_get_xid (w->data);
			break;
		}
	}

	if (!xid) {
		return NULL;
	}

	/* Return the first matching window we have. */
	for (l = priv->windows; l; l = l->next) {
		window = l->data;

		if (GDK_WINDOW_XID (window->window) == xid) {
			return window;
		}
	}

	return NULL;
}

