/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
 * Copyright (C) 2012 Thomas Bechtold <toabctl@gnome.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "dh-preferences.h"
#include <string.h>
#include <devhelp/devhelp.h>
#include "devhelp/dh-settings.h"
#include "devhelp/dh-util.h"

static GtkWidget *prefs_dialog = NULL;

enum {
        COLUMN_ENABLED = 0,
        COLUMN_TITLE,
        COLUMN_BOOK,
        COLUMN_WEIGHT,
        COLUMN_INCONSISTENT,
        N_COLUMNS
};

typedef struct {
        /* Fonts tab */
        GtkCheckButton *system_fonts_button;
        GtkGrid *fonts_grid;
        GtkFontButton *variable_font_button;
        GtkFontButton *fixed_font_button;
        guint      use_system_fonts_id;
        guint      system_var_id;
        guint      system_fixed_id;
        guint      var_id;
        guint      fixed_id;

        /* Book Shelf tab */
        GtkCellRendererToggle *bookshelf_enabled_toggle;
        GtkListStore *bookshelf_store;
        GtkCheckButton *bookshelf_group_by_language_button;
} DhPreferencesPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhPreferences, dh_preferences, GTK_TYPE_DIALOG)

static void
dh_preferences_response (GtkDialog *dlg,
                         gint       response_id)
{
        gtk_widget_destroy (GTK_WIDGET (dlg));
}

static void
dh_preferences_class_init (DhPreferencesClass *klass)
{
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
        GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

        dialog_class->response = dh_preferences_response;

        /* Bind class to template */
        gtk_widget_class_set_template_from_resource (widget_class,
                                                     "/org/gnome/devhelp/dh-preferences.ui");
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, system_fonts_button);
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, fonts_grid);
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, variable_font_button);
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, fixed_font_button);
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, bookshelf_store);
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, bookshelf_group_by_language_button);
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, bookshelf_enabled_toggle);
}

static void
preferences_bookshelf_clean_store (DhPreferences *prefs)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);

        gtk_list_store_clear (priv->bookshelf_store);
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
preferences_bookshelf_find_book (DhPreferences     *prefs,
                                 DhBook            *book,
                                 const GtkTreeIter *first,
                                 GtkTreeIter       *exact_iter,
                                 gboolean          *exact_found,
                                 GtkTreeIter       *next_iter,
                                 gboolean          *next_found)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);
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
                if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->bookshelf_store), &loop_iter)) {
                        /* Store is empty, not found */
                        return;
                }
        } else {
                loop_iter = *first;
        }

        do {
                DhBook *in_list_book = NULL;

                gtk_tree_model_get (GTK_TREE_MODEL (priv->bookshelf_store),
                                    &loop_iter,
                                    COLUMN_BOOK, &in_list_book,
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
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->bookshelf_store),
                                           &loop_iter));
}

/* Tries to find:
 *  - An exact match of the language group
 *  - The language group which should be just after our given language group.
 *  - Both.
 */
static void
preferences_bookshelf_find_language_group (DhPreferences *prefs,
                                           const gchar   *language,
                                           GtkTreeIter   *exact_iter,
                                           gboolean      *exact_found,
                                           GtkTreeIter   *next_iter,
                                           gboolean      *next_found)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);
        GtkTreeIter loop_iter;

        g_assert ((exact_iter && exact_found) || (next_iter && next_found));

        /* Reset all flags to not found */
        if (exact_found)
                *exact_found = FALSE;
        if (next_found)
                *next_found = FALSE;

        if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->bookshelf_store),
                                            &loop_iter)) {
                /* Store is empty, not found */
                return;
        }

        do {
                DhBook *book = NULL;
                gchar  *title = NULL;

                /* Look for language titles, which are those where there
                 * is no book object associated in the row */
                gtk_tree_model_get (GTK_TREE_MODEL (priv->bookshelf_store),
                                    &loop_iter,
                                    COLUMN_TITLE, &title,
                                    COLUMN_BOOK,  &book,
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
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->bookshelf_store),
                                           &loop_iter));
}

static void
preferences_bookshelf_add_book_to_store (DhPreferences *prefs,
                                         DhBook        *book,
                                         gboolean       group_by_language)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);
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
                preferences_bookshelf_find_language_group (prefs,
                                                           language_title,
                                                           &language_iter,
                                                           &language_iter_found,
                                                           &next_language_iter,
                                                           &next_language_iter_found);
                /* New language group needs to be created? */
                if (!language_iter_found) {
                        if (!next_language_iter_found) {
                                gtk_list_store_append (priv->bookshelf_store,
                                                       &language_iter);
                        } else {
                                gtk_list_store_insert_before (priv->bookshelf_store,
                                                              &language_iter,
                                                              &next_language_iter);
                        }

                        gtk_list_store_set (priv->bookshelf_store,
                                            &language_iter,
                                            COLUMN_ENABLED,      dh_book_get_enabled (book),
                                            COLUMN_TITLE,        language_title,
                                            COLUMN_BOOK,         NULL,
                                            COLUMN_WEIGHT,       PANGO_WEIGHT_BOLD,
                                            COLUMN_INCONSISTENT, FALSE,
                                            -1);

                        first_in_language = TRUE;
                }

                /* If we got to add first book in a given language group, just append it. */
                if (first_in_language) {
                        gtk_list_store_insert_after (priv->bookshelf_store,
                                                     &book_iter,
                                                     &language_iter);
                } else {
                        GtkTreeIter first_book_iter;
                        GtkTreeIter next_book_iter;
                        gboolean    next_book_iter_found;
                        gboolean    language_inconsistent = FALSE;
                        gboolean    language_enabled = FALSE;

                        /* We may need to reset the inconsistent status of the language item */
                        gtk_tree_model_get (GTK_TREE_MODEL (priv->bookshelf_store),
                                            &language_iter,
                                            COLUMN_ENABLED, &language_enabled,
                                            COLUMN_INCONSISTENT, &language_inconsistent,
                                            -1);
                        /* If inconsistent already, do nothing */
                        if (!language_inconsistent) {
                                if (language_enabled != dh_book_get_enabled (book)) {
                                        gtk_list_store_set (priv->bookshelf_store,
                                                            &language_iter,
                                                            COLUMN_INCONSISTENT, TRUE,
                                                            -1);
                                }
                        }

                        /* The language will have at least one book, so we move iter to it */
                        first_book_iter = language_iter;
                        gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->bookshelf_store), &first_book_iter);

                        /* Find next possible book in language group */
                        preferences_bookshelf_find_book (prefs,
                                                         book,
                                                         &first_book_iter,
                                                         NULL,
                                                         NULL,
                                                         &next_book_iter,
                                                         &next_book_iter_found);
                        if (!next_book_iter_found) {
                                gtk_list_store_append (priv->bookshelf_store,
                                                       &book_iter);
                        } else {
                                gtk_list_store_insert_before (priv->bookshelf_store,
                                                              &book_iter,
                                                              &next_book_iter);
                        }
                }

                /* Add new item with indented title */
                indented_title = g_strdup_printf ("     %s", dh_book_get_title (book));
                gtk_list_store_set (priv->bookshelf_store,
                                    &book_iter,
                                    COLUMN_ENABLED,      dh_book_get_enabled (book),
                                    COLUMN_TITLE,        indented_title,
                                    COLUMN_BOOK,         book,
                                    COLUMN_WEIGHT,       PANGO_WEIGHT_NORMAL,
                                    COLUMN_INCONSISTENT, FALSE,
                                    -1);
                g_free (indented_title);
        } else {
                /* No language grouping, just order by book title */
                GtkTreeIter next_book_iter;
                gboolean    next_book_iter_found;

                preferences_bookshelf_find_book (prefs,
                                                 book,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 &next_book_iter,
                                                 &next_book_iter_found);
                if (!next_book_iter_found) {
                        gtk_list_store_append (priv->bookshelf_store,
                                               &book_iter);
                } else {
                        gtk_list_store_insert_before (priv->bookshelf_store,
                                                      &book_iter,
                                                      &next_book_iter);
                }

                gtk_list_store_set (priv->bookshelf_store,
                                    &book_iter,
                                    COLUMN_ENABLED,  dh_book_get_enabled (book),
                                    COLUMN_TITLE,    dh_book_get_title (book),
                                    COLUMN_BOOK,     book,
                                    COLUMN_WEIGHT,   PANGO_WEIGHT_NORMAL,
                                    -1);
        }
}

static void
preferences_bookshelf_populate_store (DhPreferences *prefs)
{
        DhBookManager *book_manager;
        GList *l;
        gboolean group_by_language;

        book_manager = dh_book_manager_get_singleton ();
        group_by_language = dh_book_manager_get_group_by_language (book_manager);

        /* This list already comes ordered, but we don't care */
        for (l = dh_book_manager_get_books (book_manager);
             l;
             l = g_list_next (l)) {
                preferences_bookshelf_add_book_to_store (prefs,
                                                         DH_BOOK (l->data),
                                                         group_by_language);
        }
}

static void
preferences_bookshelf_group_by_language_cb (GObject       *object,
                                            GParamSpec    *pspec,
                                            DhPreferences *prefs)
{
        preferences_bookshelf_clean_store (prefs);
        preferences_bookshelf_populate_store (prefs);
}

static void
preferences_bookshelf_set_language_inconsistent (DhPreferences *prefs,
                                                 const gchar *language)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);
        GtkTreeIter loop_iter;
        GtkTreeIter language_iter;
        gboolean    language_iter_found;
        gboolean    one_book_enabled = FALSE;
        gboolean    one_book_disabled = FALSE;

        preferences_bookshelf_find_language_group (prefs,
                                                   language,
                                                   &language_iter,
                                                   &language_iter_found,
                                                   NULL,
                                                   NULL);
        if (!language_iter_found) {
                return;
        }

        loop_iter = language_iter;
        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->bookshelf_store),
                                         &loop_iter)) {
                DhBook   *book;
                gboolean  enabled;

                gtk_tree_model_get (GTK_TREE_MODEL (priv->bookshelf_store),
                                    &loop_iter,
                                    COLUMN_BOOK,       &book,
                                    COLUMN_ENABLED,    &enabled,
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
                gtk_list_store_set (priv->bookshelf_store, &language_iter,
                                    COLUMN_INCONSISTENT, TRUE,
                                    -1);
                return;
        }

        gtk_list_store_set (priv->bookshelf_store, &language_iter,
                            COLUMN_ENABLED, one_book_enabled,
                            COLUMN_INCONSISTENT, FALSE,
                            -1);
}

static void
preferences_bookshelf_book_deleted_cb (DhBookManager *book_manager,
                                       DhBook        *book,
                                       DhPreferences *prefs)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);
        GtkTreeIter  exact_iter;
        gboolean     exact_iter_found;

        preferences_bookshelf_find_book (prefs,
                                         book,
                                         NULL,
                                         &exact_iter,
                                         &exact_iter_found,
                                         NULL,
                                         NULL);
        if (exact_iter_found) {
                gtk_list_store_remove (priv->bookshelf_store, &exact_iter);
                preferences_bookshelf_set_language_inconsistent (prefs, dh_book_get_language (book));
        }
}

static void
preferences_bookshelf_book_created_cb (DhBookManager *book_manager,
                                       DhBook        *book,
                                       DhPreferences *prefs)
{
        gboolean group_by_language;

        group_by_language = dh_book_manager_get_group_by_language (book_manager);
        preferences_bookshelf_add_book_to_store (prefs, book, group_by_language);
}

static void
preferences_bookshelf_tree_selection_toggled_cb (GtkCellRendererToggle *cell_renderer,
                                                 gchar                 *path,
                                                 DhPreferences         *prefs)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);
        DhBookManager *book_manager;
        GtkTreeIter iter;

        book_manager = dh_book_manager_get_singleton ();

        if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (priv->bookshelf_store),
                                                 &iter,
                                                 path)) {
                gpointer book = NULL;
                gboolean enabled;

                gtk_tree_model_get (GTK_TREE_MODEL (priv->bookshelf_store),
                                    &iter,
                                    COLUMN_BOOK,       &book,
                                    COLUMN_ENABLED,    &enabled,
                                    -1);

                if (book) {
                        /* Update book conf */
                        dh_book_set_enabled (book, !enabled);

                        gtk_list_store_set (priv->bookshelf_store, &iter,
                                            COLUMN_ENABLED, !enabled,
                                            -1);
                        /* Now we need to look for the language group of this item,
                         * in order to set the inconsistent state if applies */
                        if (dh_book_manager_get_group_by_language (book_manager)) {
                                preferences_bookshelf_set_language_inconsistent (prefs, dh_book_get_language (book));
                        }

                } else {
                        GtkTreeIter loop_iter;

                        /* We should only reach this if we are grouping by language */
                        g_assert (dh_book_manager_get_group_by_language (book_manager) == TRUE);

                        /* Set new status in the language group item */
                        gtk_list_store_set (priv->bookshelf_store, &iter,
                                            COLUMN_ENABLED,      !enabled,
                                            COLUMN_INCONSISTENT, FALSE,
                                            -1);

                        /* And set new status in all books of the same language */
                        loop_iter = iter;
                        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->bookshelf_store),
                                                         &loop_iter)) {
                                gtk_tree_model_get (GTK_TREE_MODEL (priv->bookshelf_store),
                                                    &loop_iter,
                                                    COLUMN_BOOK, &book,
                                                    -1);
                                if (!book) {
                                        /* Found next language group, finish */
                                        return;
                                }

                                /* Update book conf */
                                dh_book_set_enabled (book, !enabled);

                                gtk_list_store_set (priv->bookshelf_store,
                                                    &loop_iter,
                                                    COLUMN_ENABLED, !enabled,
                                                    -1);
                        }
                }
        }
}

static void
dh_preferences_init (DhPreferences *prefs)
{
        DhPreferencesPrivate *priv;
        DhBookManager *book_manager;
        DhSettings *settings;
        GSettings *settings_fonts;
        GSettings *settings_contents;

        priv = dh_preferences_get_instance_private (prefs);

        gtk_widget_init_template (GTK_WIDGET (prefs));

        book_manager = dh_book_manager_get_singleton ();

        g_signal_connect_object (book_manager,
                                 "book-created",
                                 G_CALLBACK (preferences_bookshelf_book_created_cb),
                                 prefs,
                                 0);

        g_signal_connect_object (book_manager,
                                 "book-deleted",
                                 G_CALLBACK (preferences_bookshelf_book_deleted_cb),
                                 prefs,
                                 0);

        g_signal_connect_object (book_manager,
                                 "notify::group-by-language",
                                 G_CALLBACK (preferences_bookshelf_group_by_language_cb),
                                 prefs,
                                 0);

        /* setup GSettings bindings */
        settings = dh_settings_get_singleton ();
        settings_fonts = dh_settings_peek_fonts_settings (settings);
        settings_contents = dh_settings_peek_contents_settings (settings);
        g_settings_bind (settings_fonts, "use-system-fonts",
                         priv->system_fonts_button, "active",
                         G_SETTINGS_BIND_DEFAULT);
        g_settings_bind (settings_fonts, "use-system-fonts",
                         priv->fonts_grid, "sensitive",
                         G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_INVERT_BOOLEAN);
        g_settings_bind (settings_fonts, "fixed-font",
                         priv->fixed_font_button, "font-name",
                         G_SETTINGS_BIND_DEFAULT);
        g_settings_bind (settings_fonts, "variable-font",
                         priv->variable_font_button, "font-name",
                         G_SETTINGS_BIND_DEFAULT);

        g_settings_bind (settings_contents, "group-books-by-language",
                         priv->bookshelf_group_by_language_button, "active",
                         G_SETTINGS_BIND_DEFAULT);

        g_signal_connect (priv->bookshelf_enabled_toggle,
                          "toggled",
                          G_CALLBACK (preferences_bookshelf_tree_selection_toggled_cb),
                          prefs);

        preferences_bookshelf_populate_store (prefs);
}

void
dh_preferences_show_dialog (GtkWindow *parent)
{
        g_return_if_fail (GTK_IS_WINDOW (parent));

        if (prefs_dialog == NULL) {
                prefs_dialog = GTK_WIDGET (g_object_new (DH_TYPE_PREFERENCES,
                                                         "use-header-bar", 1,
                                                         NULL));
                g_signal_connect (prefs_dialog,
                                  "destroy",
                                  G_CALLBACK (gtk_widget_destroyed),
                                  &prefs_dialog);
        }

        if (parent != gtk_window_get_transient_for (GTK_WINDOW (prefs_dialog))) {
                gtk_window_set_transient_for (GTK_WINDOW (prefs_dialog),
                                              parent);
                gtk_window_set_destroy_with_parent (GTK_WINDOW (prefs_dialog), TRUE);
        }

        gtk_window_present (GTK_WINDOW (prefs_dialog));
}
