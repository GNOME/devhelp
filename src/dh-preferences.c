/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
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
#include <gtk/gtk.h>
#include <string.h>
#include "dh-util.h"
#include "dh-preferences.h"
#include "ige-conf.h"
#include "dh-base.h"

typedef struct {
	GtkWidget *dialog;

        /* Fonts tab */
	GtkWidget *system_fonts_button;
	GtkWidget *fonts_table;
	GtkWidget *variable_font_button;
	GtkWidget *fixed_font_button;
	guint      use_system_fonts_id;
	guint      system_var_id;
	guint      system_fixed_id;
	guint      var_id;
	guint      fixed_id;

        /* Book Shelf tab */
        DhBookManager *book_manager;
        GtkTreeView   *booklist_treeview;
        GtkListStore  *booklist_store;
        GtkWidget     *group_by_language_button;
} DhPreferences;

/* Fonts-tab related */
static void     preferences_fonts_font_set_cb               (GtkFontButton    *button,
                                                             gpointer          user_data);
static void     preferences_fonts_system_fonts_toggled_cb   (GtkToggleButton  *button,
                                                             gpointer          user_data);
#if 0
static void     preferences_fonts_var_font_notify_cb        (IgeConf          *client,
                                                             const gchar      *path,
                                                             gpointer          user_data);
static void     preferences_fonts_fixed_font_notify_cb      (IgeConf          *client,
                                                             const gchar      *path,
                                                             gpointer          user_data);
static void     preferences_fonts_use_system_font_notify_cb (IgeConf          *client,
                                                             const gchar      *path,
                                                             gpointer          user_data);
static void     preferences_connect_conf_listeners          (void);
#endif
static void     preferences_fonts_get_font_names            (gboolean          use_system_fonts,
                                                             gchar           **variable,
                                                             gchar           **fixed);

/* Bookshelf-tab related */
static void     preferences_bookshelf_tree_selection_toggled_cb    (GtkCellRendererToggle *cell_renderer,
                                                                    gchar                 *path,
                                                                    gpointer               user_data);
static void     preferences_bookshelf_populate_store               (void);
static void     preferences_bookshelf_clean_store                  (void);
static void     preferences_bookshelf_add_book_to_store            (DhBook                *book,
                                                                    gboolean               group_by_language);
static void     preferences_bookshelf_book_created_cb              (DhBookManager         *book_manager,
                                                                    GObject               *book_object,
                                                                    gpointer               user_data);
static void     preferences_bookshelf_book_deleted_cb              (DhBookManager         *book_manager,
                                                                    GObject               *book_object,
                                                                    gpointer               user_data);
static void     preferences_bookshelf_find_language_group          (const gchar           *language,
                                                                    GtkTreeIter           *exact_iter,
                                                                    gboolean              *exact_found,
                                                                    GtkTreeIter           *next_iter,
                                                                    gboolean              *next_found);
static void     preferences_bookshelf_find_book                    (DhBook                *book,
                                                                    const GtkTreeIter     *first,
                                                                    GtkTreeIter           *exact_iter,
                                                                    gboolean              *exact_found,
                                                                    GtkTreeIter           *next_iter,
                                                                    gboolean              *next_found);
static void     preferences_bookshelf_group_by_language_cb         (GObject               *object,
                                                                    GParamSpec            *pspec,
                                                                    gpointer               user_data);
static void     preferences_bookshelf_group_by_language_toggled_cb (GtkToggleButton       *button,
                                                                    gpointer               user_data);

#define DH_CONF_PATH                  "/apps/devhelp"
#define DH_CONF_USE_SYSTEM_FONTS      DH_CONF_PATH "/ui/use_system_fonts"
#define DH_CONF_VARIABLE_FONT         DH_CONF_PATH "/ui/variable_font"
#define DH_CONF_FIXED_FONT            DH_CONF_PATH "/ui/fixed_font"
#define DH_CONF_SYSTEM_VARIABLE_FONT  "/desktop/gnome/interface/font_name"
#define DH_CONF_SYSTEM_FIXED_FONT     "/desktop/gnome/interface/monospace_font_name"
#define DH_CONF_GROUP_BY_LANGUAGE     DH_CONF_PATH "/ui/use_system_fonts"

/* Book list store columns... */
#define LTCOLUMN_ENABLED      0
#define LTCOLUMN_TITLE        1
#define LTCOLUMN_BOOK         2
#define LTCOLUMN_WEIGHT       3
#define LTCOLUMN_INCONSISTENT 4

static DhPreferences *prefs;

static void
preferences_init (void)
{
        if (!prefs) {
                prefs = g_new0 (DhPreferences, 1);
                prefs->book_manager = dh_base_get_book_manager (dh_base_get ());
                g_signal_connect (prefs->book_manager,
                                  "book-created",
                                  G_CALLBACK (preferences_bookshelf_book_created_cb),
                                  NULL);
                g_signal_connect (prefs->book_manager,
                                  "book-deleted",
                                  G_CALLBACK (preferences_bookshelf_book_deleted_cb),
                                  NULL);
                g_signal_connect (prefs->book_manager,
                                  "notify::group-by-language",
                                  G_CALLBACK (preferences_bookshelf_group_by_language_cb),
                                  NULL);
        }
}

static void
preferences_shutdown (void)
{
        if (!prefs) {
                return;
        }

        gtk_list_store_clear (prefs->booklist_store);
        gtk_widget_destroy (GTK_WIDGET (prefs->dialog));

        g_free (prefs);
        prefs = NULL;
}

static void
preferences_fonts_font_set_cb (GtkFontButton *button,
                               gpointer       user_data)
{
	DhPreferences *prefs = user_data;
	const gchar   *font_name;
	const gchar   *key;

	font_name = gtk_font_button_get_font_name (button);

	if (GTK_WIDGET (button) == prefs->variable_font_button) {
		key = DH_CONF_VARIABLE_FONT;
	} else {
		key = DH_CONF_FIXED_FONT;
	}

	ige_conf_set_string (ige_conf_get (), key, font_name);
}

static void
preferences_fonts_system_fonts_toggled_cb (GtkToggleButton *button,
                                           gpointer         user_data)
{
	DhPreferences *prefs = user_data;
	gboolean       active;

	active = gtk_toggle_button_get_active (button);

	ige_conf_set_bool (ige_conf_get (),
                           DH_CONF_USE_SYSTEM_FONTS,
                           active);

	gtk_widget_set_sensitive (prefs->fonts_table, !active);
}

#if 0
static void
preferences_fonts_var_font_notify_cb (IgeConf     *client,
                                      const gchar *path,
                                      gpointer     user_data)
{
	DhPreferences *prefs = user_data;
	gboolean       use_system_fonts;
	gchar         *font_name;

        ige_conf_get_bool (ige_conf_get (),
                           DH_CONF_USE_SYSTEM_FONTS,
                           &use_system_fonts);

	if (prefs->variable_font_button) {
		ige_conf_get_string (ige_conf_get (), path, &font_name);
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->variable_font_button),
					       font_name);
                g_free (font_name);
	}
}

static void
preferences_fonts_fixed_font_notify_cb (IgeConf     *client,
                                        const gchar *path,
                                        gpointer     user_data)
{
	DhPreferences *prefs = user_data;
	gboolean       use_system_fonts;
	gchar         *font_name;

	ige_conf_get_bool (ige_conf_get (),
                           DH_CONF_USE_SYSTEM_FONTS,
                           &use_system_fonts);

	if (prefs->fixed_font_button) {
                ige_conf_get_string (ige_conf_get (), path, &font_name);
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->fixed_font_button),
					       font_name);
                g_free (font_name);
	}
}

static void
preferences_fonts_use_system_font_notify_cb (IgeConf     *client,
                                             const gchar *path,
                                             gpointer     user_data)
{
	DhPreferences *prefs = user_data;
	gboolean       use_system_fonts;

	ige_conf_get_bool (ige_conf_get (), path, &use_system_fonts);

	if (prefs->system_fonts_button) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->system_fonts_button),
					      use_system_fonts);
	}

	if (prefs->fonts_table) {
		gtk_widget_set_sensitive (prefs->fonts_table, !use_system_fonts);
	}
}

/* FIXME: This is not hooked up yet (to update the dialog if the values are
 * changed outside of devhelp).
 */
static void
preferences_connect_conf_listeners (void)
{
	IgeConf *conf;

	conf = ige_conf_get ();

	prefs->use_system_fonts_id =
		ige_conf_notify_add (conf,
                                     DH_CONF_USE_SYSTEM_FONTS,
                                     preferences_use_system_font_notify_cb,
                                     prefs);
	prefs->system_var_id =
		ige_conf_notify_add (conf,
                                     DH_CONF_SYSTEM_VARIABLE_FONT,
                                     preferences_var_font_notify_cb,
                                     prefs);
	prefs->system_fixed_id =
		ige_conf_notify_add (conf,
                                     DH_CONF_SYSTEM_FIXED_FONT,
                                     preferences_fixed_font_notify_cb,
                                     prefs);
	prefs->var_id =
		ige_conf_notify_add (conf,
                                     DH_CONF_VARIABLE_FONT,
                                     preferences_var_font_notify_cb,
                                     prefs);
	prefs->fixed_id =
		ige_conf_notify_add (conf,
                                     DH_CONF_FIXED_FONT,
                                     preferences_fixed_font_notify_cb,
                                     prefs);
}
#endif

/* FIXME: Use the functions in dh-util.c for this. */
static void
preferences_fonts_get_font_names (gboolean   use_system_fonts,
                                  gchar    **variable,
                                  gchar    **fixed)
{
	gchar   *var_font_name;
        gchar   *fixed_font_name;
	IgeConf *conf;

	conf = ige_conf_get ();

	if (use_system_fonts) {
#ifdef GDK_WINDOWING_QUARTZ
                var_font_name = g_strdup ("Lucida Grande 14");
                fixed_font_name = g_strdup ("Monaco 14");
#else
		ige_conf_get_string (conf,
                                     DH_CONF_SYSTEM_VARIABLE_FONT,
                                     &var_font_name);
		ige_conf_get_string (conf,
                                     DH_CONF_SYSTEM_FIXED_FONT,
                                     &fixed_font_name);
#endif
	} else {
		ige_conf_get_string (conf,
                                     DH_CONF_VARIABLE_FONT,
                                     &var_font_name);
                ige_conf_get_string (conf,
                                     DH_CONF_FIXED_FONT,
                                     &fixed_font_name);
	}

	*variable = var_font_name;
	*fixed = fixed_font_name;
}

static void
preferences_bookshelf_set_language_inconsistent (const gchar *language)
{
        GtkTreeIter loop_iter;
        GtkTreeIter language_iter;
        gboolean    language_iter_found;
        gboolean    one_book_enabled = FALSE;
        gboolean    one_book_disabled = FALSE;

        preferences_bookshelf_find_language_group (language,
                                                   &language_iter,
                                                   &language_iter_found,
                                                   NULL,
                                                   NULL);
        if (!language_iter_found) {
                return;
        }

        loop_iter = language_iter;
        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (prefs->booklist_store),
                                         &loop_iter)) {
                DhBook   *book;
                gboolean  enabled;

                gtk_tree_model_get (GTK_TREE_MODEL (prefs->booklist_store),
                                    &loop_iter,
                                    LTCOLUMN_BOOK,       &book,
                                    LTCOLUMN_ENABLED,    &enabled,
                                    -1);
                if (!book) {
                        /* Reached next language group */
                        break;
                }
                g_object_unref (book);

                if (enabled)
                        one_book_enabled = TRUE;
                else
                        one_book_disabled = TRUE;

                if (one_book_enabled == one_book_disabled)
                        break;
        }

        /* If at least one book is enabled AND another book is disabled,
         * we need to set inconsistent state */
        if (one_book_enabled == one_book_disabled) {
                gtk_list_store_set (prefs->booklist_store, &language_iter,
                                    LTCOLUMN_INCONSISTENT, TRUE,
                                    -1);
                return;
        }

        gtk_list_store_set (prefs->booklist_store, &language_iter,
                            LTCOLUMN_ENABLED, one_book_enabled,
                            LTCOLUMN_INCONSISTENT, FALSE,
                            -1);
}

static void
preferences_bookshelf_tree_selection_toggled_cb (GtkCellRendererToggle *cell_renderer,
                                                 gchar                 *path,
                                                 gpointer               user_data)
{
        GtkTreeIter iter;

        if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (prefs->booklist_store),
                                                 &iter,
                                                 path))
        {
                gpointer book = NULL;
                gboolean enabled;

                gtk_tree_model_get (GTK_TREE_MODEL (prefs->booklist_store),
                                    &iter,
                                    LTCOLUMN_BOOK,       &book,
                                    LTCOLUMN_ENABLED,    &enabled,
                                    -1);

                if (book) {
                        /* Update book conf */
                        dh_book_set_enabled (book, !enabled);

                        gtk_list_store_set (prefs->booklist_store, &iter,
                                            LTCOLUMN_ENABLED, !enabled,
                                            -1);
                        /* Now we need to look for the language group of this item,
                         * in order to set the inconsistent state if applies */
                        if (dh_book_manager_get_group_by_language (prefs->book_manager)) {
                                preferences_bookshelf_set_language_inconsistent (dh_book_get_language (book));
                        }

                } else {
                        GtkTreeIter loop_iter;

                        /* We should only reach this if we are grouping by language */
                        g_assert (dh_book_manager_get_group_by_language (prefs->book_manager) == TRUE);

                        /* Set new status in the language group item */
                        gtk_list_store_set (prefs->booklist_store, &iter,
                                            LTCOLUMN_ENABLED,      !enabled,
                                            LTCOLUMN_INCONSISTENT, FALSE,
                                            -1);

                        /* And set new status in all books of the same language */
                        loop_iter = iter;
                        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (prefs->booklist_store),
                                                         &loop_iter)) {
                                gtk_tree_model_get (GTK_TREE_MODEL (prefs->booklist_store),
                                                    &loop_iter,
                                                    LTCOLUMN_BOOK, &book,
                                                    -1);
                                if (!book) {
                                        /* Found next language group, finish */
                                        return;
                                }

                                /* Update book conf */
                                dh_book_set_enabled (book, !enabled);

                                gtk_list_store_set (prefs->booklist_store,
                                                    &loop_iter,
                                                    LTCOLUMN_ENABLED, !enabled,
                                                    -1);
                        }
                }
        }
}

static void
preferences_bookshelf_group_by_language_cb (GObject    *object,
                                            GParamSpec *pspec,
                                            gpointer    user_data)
{
        preferences_bookshelf_clean_store ();
        preferences_bookshelf_populate_store ();
}

static void
preferences_bookshelf_book_created_cb (DhBookManager *book_manager,
                                       GObject       *book_object,
                                       gpointer       user_data)
{
        preferences_bookshelf_add_book_to_store (DH_BOOK (book_object),
                                                 dh_book_manager_get_group_by_language (prefs->book_manager));
}

static void
preferences_bookshelf_book_deleted_cb (DhBookManager *book_manager,
                                       GObject       *book_object,
                                       gpointer       user_data)
{
        DhBook      *book = DH_BOOK (book_object);
        GtkTreeIter  exact_iter;
        gboolean     exact_iter_found;

        preferences_bookshelf_find_book (book,
                                         NULL,
                                         &exact_iter,
                                         &exact_iter_found,
                                         NULL,
                                         NULL);
        if (exact_iter_found) {
                gtk_list_store_remove (prefs->booklist_store, &exact_iter);
                preferences_bookshelf_set_language_inconsistent (dh_book_get_language (book));
        }
}

/* Tries to find:
 *  - An exact match of the language group
 *  - The language group which should be just after our given language group.
 *  - Both.
 */
static void
preferences_bookshelf_find_language_group (const gchar *language,
                                           GtkTreeIter *exact_iter,
                                           gboolean    *exact_found,
                                           GtkTreeIter *next_iter,
                                           gboolean    *next_found)
{
        GtkTreeIter loop_iter;

        g_assert ((exact_iter && exact_found) || (next_iter && next_found));

        /* Reset all flags to not found */
        if (exact_found)
                *exact_found = FALSE;
        if (next_found)
                *next_found = FALSE;

        if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (prefs->booklist_store),
                                            &loop_iter)) {
                /* Store is empty, not found */
                return;
        }

        do {
                DhBook *book = NULL;
                gchar  *title = NULL;

                /* Look for language titles, which are those where there
                 * is no book object associated in the row */
                gtk_tree_model_get (GTK_TREE_MODEL (prefs->booklist_store),
                                    &loop_iter,
                                    LTCOLUMN_TITLE, &title,
                                    LTCOLUMN_BOOK,  &book,
                                    -1);

                /* If we got a book, it's not a language row */
                if (book) {
                        g_free (title);
                        g_object_unref (book);
                        continue;
                }

                if (exact_iter &&
                    g_ascii_strcasecmp (title, language) == 0) {
                        /* Exact match found! */
                        *exact_iter = loop_iter;
                        *exact_found = TRUE;
                        if (!next_iter) {
                                /* If we were not requested to look for the next one, end here */
                                g_free (title);
                                return;
                        }
                } else if (next_iter &&
                           g_ascii_strcasecmp (title, language) > 0) {
                        *next_iter = loop_iter;
                        *next_found = TRUE;
                        /* There's no way to have an exact match after the next, so end here */
                        g_free (title);
                        return;
                }

                g_free (title);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (prefs->booklist_store),
                                           &loop_iter));
}

/* Tries to find, starting at 'first' (if given):
 *  - An exact match of the book
 *  - The book which should be just after our given book:
 *      - If first is set, the next book must be in the same language group
 *         as the given book.
 *      - If first is NOT set, we don't care about language groups as we're
 *         iterating from the beginning of the list.
 *  - Both.
 */
static void
preferences_bookshelf_find_book (DhBook            *book,
                                 const GtkTreeIter *first,
                                 GtkTreeIter       *exact_iter,
                                 gboolean          *exact_found,
                                 GtkTreeIter       *next_iter,
                                 gboolean          *next_found)
{
        GtkTreeIter loop_iter;

        g_assert ((exact_iter && exact_found) || (next_iter && next_found));

        /* Reset all flags to not found */
        if (exact_found)
                *exact_found = FALSE;
        if (next_found)
                *next_found = FALSE;

        /* Setup iteration start */
        if (!first) {
                /* If no first given, start iterating from the start of the model */
                if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (prefs->booklist_store), &loop_iter)) {
                        /* Store is empty, not found */
                        return;
                }
        } else {
                loop_iter = *first;
        }

        do {
                DhBook *in_list_book = NULL;

                gtk_tree_model_get (GTK_TREE_MODEL (prefs->booklist_store),
                                    &loop_iter,
                                    LTCOLUMN_BOOK, &in_list_book,
                                    -1);

                /* We may have reached the start of the next language group here */
                if (first && !in_list_book) {
                        *next_iter = loop_iter;
                        *next_found = TRUE;
                        return;
                }

                /* We can compare pointers directly as we're playing with references
                 * of the same object */
                if (exact_iter &&
                    in_list_book == book) {
                        *exact_iter = loop_iter;
                        *exact_found = TRUE;
                        if (!next_iter) {
                                /* If we were not requested to look for the next one, end here */
                                g_object_unref (in_list_book);
                                return;
                        }
                } else if (next_iter &&
                           dh_book_cmp_by_title (in_list_book, book) > 0) {
                        *next_iter = loop_iter;
                        *next_found = TRUE;
                        g_object_unref (in_list_book);
                        return;
                }

                if (in_list_book)
                        g_object_unref (in_list_book);
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (prefs->booklist_store),
                                           &loop_iter));
}

static void
preferences_bookshelf_add_book_to_store (DhBook   *book,
                                         gboolean  group_by_language)
{
        GtkTreeIter  book_iter;

        /* If grouping by language we need to add the language categories */
        if (group_by_language) {
                gchar       *indented_title;
                GtkTreeIter  language_iter;
                gboolean     language_iter_found;
                GtkTreeIter  next_language_iter;
                gboolean     next_language_iter_found;
                const gchar *language_title;
                gboolean     first_in_language = FALSE;

                language_title = dh_book_get_language (book);

                /* Look for the proper language group */
                preferences_bookshelf_find_language_group (language_title,
                                                           &language_iter,
                                                           &language_iter_found,
                                                           &next_language_iter,
                                                           &next_language_iter_found);
                /* New language group needs to be created? */
                if (!language_iter_found) {
                        if (!next_language_iter_found) {
                                gtk_list_store_append (prefs->booklist_store,
                                                       &language_iter);
                        } else {
                                gtk_list_store_insert_before (prefs->booklist_store,
                                                              &language_iter,
                                                              &next_language_iter);
                        }

                        gtk_list_store_set (prefs->booklist_store,
                                            &language_iter,
                                            LTCOLUMN_ENABLED,      dh_book_get_enabled (book),
                                            LTCOLUMN_TITLE,        language_title,
                                            LTCOLUMN_BOOK,         NULL,
                                            LTCOLUMN_WEIGHT,       PANGO_WEIGHT_BOLD,
                                            LTCOLUMN_INCONSISTENT, FALSE,
                                            -1);

                        first_in_language = TRUE;
                }

                /* If we got to add first book in a given language group, just append it. */
                if (first_in_language) {
                        gtk_list_store_insert_after (prefs->booklist_store,
                                                     &book_iter,
                                                     &language_iter);
                } else {
                        GtkTreeIter first_book_iter;
                        GtkTreeIter next_book_iter;
                        gboolean    next_book_iter_found;
                        gboolean    language_inconsistent = FALSE;
                        gboolean    language_enabled = FALSE;

                        /* We may need to reset the inconsistent status of the language item */
                        gtk_tree_model_get (GTK_TREE_MODEL (prefs->booklist_store),
                                            &language_iter,
                                            LTCOLUMN_ENABLED, &language_enabled,
                                            LTCOLUMN_INCONSISTENT, &language_inconsistent,
                                            -1);
                        /* If inconsistent already, do nothing */
                        if (!language_inconsistent) {
                                if (language_enabled != dh_book_get_enabled (book)) {
                                        gtk_list_store_set (prefs->booklist_store,
                                                            &language_iter,
                                                            LTCOLUMN_INCONSISTENT, TRUE,
                                                            -1);
                                }
                        }

                        /* The language will have at least one book, so we move iter to it */
                        first_book_iter = language_iter;
                        gtk_tree_model_iter_next (GTK_TREE_MODEL (prefs->booklist_store), &first_book_iter);

                        /* Find next possible book in language group */
                        preferences_bookshelf_find_book (book,
                                                         &first_book_iter,
                                                         NULL,
                                                         NULL,
                                                         &next_book_iter,
                                                         &next_book_iter_found);
                        if (!next_book_iter_found) {
                                gtk_list_store_append (prefs->booklist_store,
                                                       &book_iter);
                        } else {
                                gtk_list_store_insert_before (prefs->booklist_store,
                                                              &book_iter,
                                                              &next_book_iter);
                        }
                }

                /* Add new item with indented title */
                indented_title = g_strdup_printf ("     %s", dh_book_get_title (book));
                gtk_list_store_set (prefs->booklist_store,
                                    &book_iter,
                                    LTCOLUMN_ENABLED,      dh_book_get_enabled (book),
                                    LTCOLUMN_TITLE,        indented_title,
                                    LTCOLUMN_BOOK,         book,
                                    LTCOLUMN_WEIGHT,       PANGO_WEIGHT_NORMAL,
                                    LTCOLUMN_INCONSISTENT, FALSE,
                                    -1);
                g_free (indented_title);
        } else {
                /* No language grouping, just order by book title */
                GtkTreeIter next_book_iter;
                gboolean    next_book_iter_found;

                preferences_bookshelf_find_book (book,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 &next_book_iter,
                                                 &next_book_iter_found);
                if (!next_book_iter_found) {
                        gtk_list_store_append (prefs->booklist_store,
                                               &book_iter);
                } else {
                        gtk_list_store_insert_before (prefs->booklist_store,
                                                      &book_iter,
                                                      &next_book_iter);
                }

                gtk_list_store_set (prefs->booklist_store,
                                    &book_iter,
                                    LTCOLUMN_ENABLED,  dh_book_get_enabled (book),
                                    LTCOLUMN_TITLE,    dh_book_get_title (book),
                                    LTCOLUMN_BOOK,     book,
                                    LTCOLUMN_WEIGHT,   PANGO_WEIGHT_NORMAL,
                                    -1);
        }
}

static void
preferences_bookshelf_populate_store (void)
{
        GList *l;
        gboolean group_by_language;

        group_by_language = dh_book_manager_get_group_by_language (prefs->book_manager);

        /* This list already comes ordered, but we don't care */
        for (l = dh_book_manager_get_books (prefs->book_manager);
             l;
             l = g_list_next (l)) {
                preferences_bookshelf_add_book_to_store (DH_BOOK (l->data),
                                                         group_by_language);
        }
}

static void
preferences_dialog_response (GtkDialog *dialog,
                             gint       response_id,
                             gpointer   user_data)
{
        preferences_shutdown ();
}

static void
preferences_bookshelf_clean_store (void)
{
        gtk_list_store_clear (prefs->booklist_store);
}

static void
preferences_bookshelf_group_by_language_toggled_cb (GtkToggleButton *button,
                                                    gpointer         user_data)
{
	DhPreferences *prefs = user_data;
	gboolean       active;

	active = gtk_toggle_button_get_active (button);

        if (dh_book_manager_get_group_by_language (prefs->book_manager) != active) {
                dh_book_manager_set_group_by_language (prefs->book_manager,
                                                       active);
        }
}

void
dh_preferences_show_dialog (GtkWindow *parent)
{
        gchar      *path;
	GtkBuilder *builder;
	gboolean    use_system_fonts;
	gchar      *var_font_name, *fixed_font_name;

        preferences_init ();

	if (prefs->dialog != NULL) {
		gtk_window_present (GTK_WINDOW (prefs->dialog));
		return;
	}

        path = dh_util_build_data_filename ("devhelp", "ui",
                                            "devhelp.builder",
                                            NULL);
	builder = dh_util_builder_get_file (
                path,
                "preferences_dialog",
                NULL,
                "preferences_dialog", &prefs->dialog,
                "fonts_table", &prefs->fonts_table,
                "system_fonts_button", &prefs->system_fonts_button,
                "variable_font_button", &prefs->variable_font_button,
                "fixed_font_button", &prefs->fixed_font_button,
                "book_manager_store", &prefs->booklist_store,
                "book_manager_treeview", &prefs->booklist_treeview,
                "group_by_language_button", &prefs->group_by_language_button,
                NULL);
        g_free (path);

	dh_util_builder_connect (
                builder,
                prefs,
                "variable_font_button", "font_set", preferences_fonts_font_set_cb,
                "fixed_font_button", "font_set", preferences_fonts_font_set_cb,
                "system_fonts_button", "toggled", preferences_fonts_system_fonts_toggled_cb,
                "book_manager_toggle", "toggled", preferences_bookshelf_tree_selection_toggled_cb,
                "group_by_language_button", "toggled", preferences_bookshelf_group_by_language_toggled_cb,
                NULL);

	ige_conf_get_bool (ige_conf_get (),
                           DH_CONF_USE_SYSTEM_FONTS,
                           &use_system_fonts);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->system_fonts_button),
				      use_system_fonts);
	gtk_widget_set_sensitive (prefs->fonts_table, !use_system_fonts);

	preferences_fonts_get_font_names (FALSE, &var_font_name, &fixed_font_name);

	if (var_font_name) {
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->variable_font_button),
					       var_font_name);
		g_free (var_font_name);
	}

	if (fixed_font_name) {
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (prefs->fixed_font_button),
					       fixed_font_name);
		g_free (fixed_font_name);
	}

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->group_by_language_button),
                                      dh_book_manager_get_group_by_language (prefs->book_manager));
        preferences_bookshelf_populate_store ();

	g_object_unref (builder);

        /* Connect to the response signal to get notified when the dialog is
         * closed or deleted */
        g_signal_connect (prefs->dialog,
                          "response",
                          G_CALLBACK (preferences_dialog_response),
                          NULL);

	gtk_window_set_transient_for (GTK_WINDOW (prefs->dialog), parent);
	gtk_widget_show_all (prefs->dialog);
}
