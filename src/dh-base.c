/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004-2008 Imendio AB
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
#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <unistd.h>
#include <gdk/gdkx.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#endif

#include "dh-window.h"
#include "dh-link.h"
#include "dh-parser.h"
#include "dh-preferences.h"
#include "dh-assistant.h"
#include "dh-util.h"
#include "ige-conf.h"
#include "dh-base.h"

typedef struct {
        GSList      *windows;
        GSList      *assistants;
        GNode       *book_tree;
        GList       *keywords;
        GHashTable  *books;
} DhBasePriv;

G_DEFINE_TYPE (DhBase, dh_base, G_TYPE_OBJECT);

#define GET_PRIVATE(instance) G_TYPE_INSTANCE_GET_PRIVATE \
  (instance, DH_TYPE_BASE, DhBasePriv);

static void dh_base_init           (DhBase      *base);
static void dh_base_class_init     (DhBaseClass *klass);
static void base_init_books        (DhBase      *base);
static void base_add_books         (DhBase      *base,
                                    const gchar *directory);

#ifdef GDK_WINDOWING_QUARTZ
static void base_add_xcode_docsets (DhBase      *base,
                                    const gchar *path);
#endif

static DhBase *base_instance;

static void
base_finalize (GObject *object)
{
        DhBasePriv *priv;

        priv = GET_PRIVATE (object);

        /* FIXME: Free things... */

        G_OBJECT_CLASS (dh_base_parent_class)->finalize (object);
}

static void
dh_base_class_init (DhBaseClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = base_finalize;

	g_type_class_add_private (klass, sizeof (DhBasePriv));
}

static void
dh_base_init (DhBase *base)
{
        DhBasePriv *priv = GET_PRIVATE (base);
        IgeConf    *conf;
        gchar      *path;

        conf = ige_conf_get ();
        path = dh_util_build_data_filename ("devhelp", "devhelp.defaults", NULL);
        ige_conf_add_defaults (conf, path);
        g_free (path);

        priv->book_tree = g_node_new (NULL);
        priv->books = g_hash_table_new_full (g_str_hash, g_str_equal,
                                             g_free, g_free);

#ifdef GDK_WINDOWING_X11
        {
                gint n_screens, i;

                /* For some reason, libwnck doesn't seem to update its list of
                 * workspaces etc if we don't do this.
                 */
                n_screens = gdk_display_get_n_screens (gdk_display_get_default ());
                for (i = 0; i < n_screens; i++) {
                        WnckScreen *screen;

                        screen = wnck_screen_get (i);
                }
        }
#endif
}

static void
base_window_or_assistant_finalized_cb (DhBase   *base,
                                       gpointer  window_or_assistant)
{
        DhBasePriv *priv = GET_PRIVATE (base);

        priv->windows = g_slist_remove (priv->windows, window_or_assistant);
        priv->assistants = g_slist_remove (priv->assistants, window_or_assistant);

        if (priv->windows == NULL && priv->assistants == NULL) {
                gtk_main_quit ();
        }
}

static gint
book_sort_func (gconstpointer a,
                gconstpointer b)
{
        DhLink      *link_a;
        DhLink      *link_b;

        link_a = ((GNode *) a)->data;
        link_b = ((GNode *) b)->data;

        return dh_util_cmp_book (link_a, link_b);
}

static void
base_sort_books (DhBase *base)
{
        DhBasePriv *priv = GET_PRIVATE (base);
        GNode      *n;
        DhLink     *link;
        GList      *list = NULL, *l;

        if (priv->book_tree) {
                n = priv->book_tree->children;

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

                g_node_append (priv->book_tree, n);
        }

        g_list_free (list);
}

static void
base_add_books_in_data_dir (DhBase      *base,
                            const gchar *data_dir)
{
        gchar *dir;

        dir = g_build_filename (data_dir, "gtk-doc", "html", NULL);
        base_add_books (base, dir);
        g_free (dir);

        dir = g_build_filename (data_dir, "devhelp", "books", NULL);
        base_add_books (base, dir);
        g_free (dir);
}

static void
base_init_books (DhBase *base)
{
        const gchar * const * system_dirs;

        base_add_books_in_data_dir (base, g_get_user_data_dir ());

        system_dirs = g_get_system_data_dirs ();
        while (*system_dirs) {
                base_add_books_in_data_dir (base, *system_dirs);
                system_dirs++;
        }

#ifdef GDK_WINDOWING_QUARTZ
        base_add_xcode_docsets (
                base,
                "/Library/Developer/Shared/Documentation/DocSets");
#endif

        base_sort_books (base);
}

static gchar *
base_get_book_path (DhBase      *base,
                    const gchar *base_path,
                    const gchar *name,
                    const gchar *suffix)
{
        gchar *tmp;
        gchar *book_path;

        tmp = g_build_filename (base_path, name, name, NULL);
        book_path = g_strconcat (tmp, ".", suffix, NULL);
        g_free (tmp);

        if (!g_file_test (book_path, G_FILE_TEST_EXISTS)) {
                g_free (book_path);
                return NULL;
        }

        return book_path;
}

#ifdef GDK_WINDOWING_QUARTZ
static void
base_add_xcode_docset (DhBase      *base,
                       const gchar *path)
{
        DhBasePriv  *priv = GET_PRIVATE (base);
        gchar       *tmp;
        gboolean     seems_like_devhelp = FALSE;
        GDir        *dir;
        const gchar *name;

        /* Do some sanity checking on the directory first so we don't have
         * to go through several hundreds of files in every docset.
         */
        tmp = g_build_filename (path, "style.css", NULL);
        if (g_file_test (tmp, G_FILE_TEST_EXISTS)) {
                gchar *tmp;

                tmp = g_build_filename (path, "index.sgml", NULL);
                if (g_file_test (tmp, G_FILE_TEST_EXISTS)) {
                        seems_like_devhelp = TRUE;
                }
                g_free (tmp);
        }
        g_free (tmp);

        if (!seems_like_devhelp) {
                return;
        }

        dir = g_dir_open (path, 0, NULL);
        if (!dir) {
                return;
        }

        while ((name = g_dir_read_name (dir)) != NULL) {
                gchar  *p;
                GError *error = NULL;

                p = strrchr (name, '.');
                if (strcmp (p, ".devhelp2") == 0) {
                        gchar *book_name;
                        gchar *book_path;

                        book_name = g_strdup (name);
                        p = strrchr (book_name, '.');
                        p[0] = '\0';

                        if (g_hash_table_lookup (priv->books, book_name)) {
                                g_free (book_name);
                                continue;
                        }

                        book_path = g_build_filename (path, name, NULL);

                        if (!dh_parser_read_file  (book_path,
                                                   priv->book_tree,
                                                   &priv->keywords,
                                                   &error)) {
                                g_warning ("Failed to read '%s': %s",
                                           book_path, error->message);
                                g_clear_error (&error);

                                g_free (book_path);
                                g_free (book_name);
                        } else {
                                g_hash_table_insert (priv->books,
                                                     book_name,
                                                     book_path);
                        }
                }
        }

        g_dir_close (dir);
}


/* This isn't really -any- Xcode docset, just gtk-doc docs converted to
 * docsets.
 *
 * Path should point to a DocSets directory.
 */
static void
base_add_xcode_docsets (DhBase      *base,
                        const gchar *path)
{
        GDir        *dir;
        const gchar *name;

        dir = g_dir_open (path, 0, NULL);
        if (!dir) {
                return;
        }

        while ((name = g_dir_read_name (dir)) != NULL) {
                gchar *docset_path;

                docset_path = g_build_filename (path,
                                                name,
                                                "Contents",
                                                "Resources",
                                                "Documents",
                                                NULL);
                base_add_xcode_docset (base, docset_path);
                g_free (docset_path);
        }

        g_dir_close (dir);
}
#endif

static void
base_add_books (DhBase      *base,
                const gchar *path)
{
        DhBasePriv  *priv = GET_PRIVATE (base);
        GDir        *dir;
        const gchar *name;

        dir = g_dir_open (path, 0, NULL);
        if (!dir) {
                return;
        }

        while ((name = g_dir_read_name (dir)) != NULL) {
                gchar  *book_path;
                GError *error = NULL;

                if (g_hash_table_lookup (priv->books, name)) {
                        continue;
                }

                book_path = base_get_book_path (base, path, name, "devhelp2");
                if (!book_path) {
                        book_path = base_get_book_path (base, path, name, "devhelp2.gz");
                }
                if (!book_path) {
                        book_path = base_get_book_path (base, path, name, "devhelp");
                }
                if (!book_path) {
                        book_path = base_get_book_path (base, path, name, "devhelp.gz");
                }

                if (!book_path) {
                        continue;
                }

                if (!dh_parser_read_file  (book_path,
                                           priv->book_tree,
                                           &priv->keywords,
                                           &error)) {
                        g_warning ("Failed to read '%s': %s",
                                   book_path, error->message);
                        g_clear_error (&error);

                        g_free (book_path);
                } else {
                        g_hash_table_insert (priv->books,
                                             g_strdup (name),
                                             book_path);
                }
        }

        g_dir_close (dir);
}

DhBase *
dh_base_get (void)
{
        if (!base_instance) {
                base_instance = g_object_new (DH_TYPE_BASE, NULL);

                base_init_books (base_instance);
        }

        return base_instance;
}

DhBase *
dh_base_new (void)
{
        if (base_instance) {
                g_error ("You can only have one DhBase instance.");
        }

        return dh_base_get ();
}

GtkWidget *
dh_base_new_window (DhBase *base)
{
        DhBasePriv *priv;
        GtkWidget  *window;

        g_return_val_if_fail (DH_IS_BASE (base), NULL);

        priv = GET_PRIVATE (base);

        window = dh_window_new (base);

        priv->windows = g_slist_prepend (priv->windows, window);

        g_object_weak_ref (G_OBJECT (window),
                           (GWeakNotify) base_window_or_assistant_finalized_cb,
                           base);

        return window;
}

GtkWidget *
dh_base_new_assistant (DhBase *base)
{
        DhBasePriv *priv;
        GtkWidget  *assistant;

        g_return_val_if_fail (DH_IS_BASE (base), NULL);

        priv = GET_PRIVATE (base);

        assistant = dh_assistant_new (base);

        priv->assistants = g_slist_prepend (priv->assistants, assistant);

        g_object_weak_ref (G_OBJECT (assistant),
                           (GWeakNotify) base_window_or_assistant_finalized_cb,
                           base);

        return assistant;
}

GNode *
dh_base_get_book_tree (DhBase *base)
{
        DhBasePriv *priv;

        g_return_val_if_fail (DH_IS_BASE (base), NULL);

        priv = GET_PRIVATE (base);

        return priv->book_tree;
}

GList *
dh_base_get_keywords (DhBase *base)
{
        DhBasePriv *priv;

        g_return_val_if_fail (DH_IS_BASE (base), NULL);

        priv = GET_PRIVATE (base);

        return priv->keywords;
}

GtkWidget *
dh_base_get_window_on_current_workspace (DhBase *base)
{
        DhBasePriv *priv;

        g_return_val_if_fail (DH_IS_BASE (base), NULL);

        priv = GET_PRIVATE (base);

        if (!priv->windows) {
                return NULL;
        }

#ifdef GDK_WINDOWING_X11
        {
                WnckWorkspace *workspace;
                WnckScreen    *screen;
                GtkWidget     *window;
                GList         *windows, *w;
                GSList        *l;
                gulong         xid;
                pid_t          pid;

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

#if GTK_CHECK_VERSION (2,14,0)
                        if (GDK_WINDOW_XID (gtk_widget_get_window (window)) == xid) {
#else
                        if (GDK_WINDOW_XID (window->window) == xid) {
#endif
                                return window;
                        }
                }
        }

        return NULL;
#else
        return priv->windows->data;
#endif
}

GtkWidget *
dh_base_get_window (DhBase *base)
{
        GtkWidget *window;

        g_return_val_if_fail (DH_IS_BASE (base), NULL);

        window = dh_base_get_window_on_current_workspace (base);
        if (!window) {
                window = dh_base_new_window (base);
                gtk_window_present (GTK_WINDOW (window));
        }

        return window;
}
