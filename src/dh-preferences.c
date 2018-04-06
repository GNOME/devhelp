/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
 * Copyright (C) 2012 Thomas Bechtold <toabctl@gnome.org>
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
 *
 * Devhelp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Devhelp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Devhelp.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dh-preferences.h"
#include <glib/gi18n.h>
#include <devhelp/devhelp.h>
#include "dh-settings-app.h"

enum {
        COLUMN_BOOK = 0,
        COLUMN_TITLE,
        N_COLUMNS
};

typedef struct {
        /* Book Shelf tab */
        GtkCheckButton *bookshelf_group_by_language_checkbutton;
        GtkTreeView *bookshelf_view;
        GtkListStore *bookshelf_store;

        /* Fonts tab */
        GtkCheckButton *use_system_fonts_checkbutton;
        GtkGrid *custom_fonts_grid;
        GtkFontButton *variable_font_button;
        GtkFontButton *fixed_font_button;
} DhPreferencesPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhPreferences, dh_preferences, GTK_TYPE_DIALOG)

static void
dh_preferences_dispose (GObject *object)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (DH_PREFERENCES (object));

        g_clear_object (&priv->bookshelf_store);

        G_OBJECT_CLASS (dh_preferences_parent_class)->dispose (object);
}

static void
dh_preferences_response (GtkDialog *dialog,
                         gint       response_id)
{
        gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
dh_preferences_class_init (DhPreferencesClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
        GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

        object_class->dispose = dh_preferences_dispose;

        dialog_class->response = dh_preferences_response;

        /* Bind class to template */
        gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/devhelp/dh-preferences.ui");

        // Book Shelf tab
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, bookshelf_group_by_language_checkbutton);
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, bookshelf_view);

        // Fonts tab
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, use_system_fonts_checkbutton);
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, custom_fonts_grid);
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, variable_font_button);
        gtk_widget_class_bind_template_child_private (widget_class, DhPreferences, fixed_font_button);
}

static gboolean
is_language_group_active (const gchar *language)
{
        DhSettings *settings;
        DhBookManager *book_manager;
        GList *books;
        GList *l;

        g_return_val_if_fail (language != NULL, FALSE);

        settings = dh_settings_get_default ();

        book_manager = dh_book_manager_get_singleton ();
        books = dh_book_manager_get_books (book_manager);

        for (l = books; l != NULL; l = l->next) {
                DhBook *cur_book = DH_BOOK (l->data);

                if (g_strcmp0 (language, dh_book_get_language (cur_book)) != 0)
                        continue;

                if (dh_settings_is_book_enabled (settings, cur_book))
                        return TRUE;
        }

        return FALSE;
}

static gboolean
is_language_group_inconsistent (const gchar *language)
{
        DhSettings *settings;
        DhBookManager *book_manager;
        GList *books;
        GList *l;
        gboolean is_first_book;
        gboolean is_first_book_enabled;

        g_return_val_if_fail (language != NULL, FALSE);

        settings = dh_settings_get_default ();

        book_manager = dh_book_manager_get_singleton ();
        books = dh_book_manager_get_books (book_manager);

        is_first_book = TRUE;

        for (l = books; l != NULL; l = l->next) {
                DhBook *cur_book = DH_BOOK (l->data);
                gboolean is_cur_book_enabled;

                if (g_strcmp0 (language, dh_book_get_language (cur_book)) != 0)
                        continue;

                is_cur_book_enabled = dh_settings_is_book_enabled (settings, cur_book);

                if (is_first_book) {
                        is_first_book_enabled = is_cur_book_enabled;
                        is_first_book = FALSE;
                } else if (is_cur_book_enabled != is_first_book_enabled) {
                        /* Inconsistent */
                        return TRUE;
                }
        }

        /* Consistent */
        return FALSE;
}

static void
set_language_group_enabled (const gchar *language,
                            gboolean     enabled)
{
        DhBookManager *book_manager;
        GList *books;
        GList *l;

        book_manager = dh_book_manager_get_singleton ();
        books = dh_book_manager_get_books (book_manager);

        for (l = books; l != NULL; l = l->next) {
                DhBook *cur_book = DH_BOOK (l->data);

                if (g_strcmp0 (language, dh_book_get_language (cur_book)) == 0)
                        dh_book_set_enabled (cur_book, enabled);
        }
}

static gboolean
bookshelf_store_changed_foreach_func (GtkTreeModel *model,
                                      GtkTreePath  *path,
                                      GtkTreeIter  *iter,
                                      gpointer      data)
{
        /* Emit ::row-changed for every row. */
        gtk_tree_model_row_changed (model, path, iter);
        return FALSE;
}

/* Have a dumb implementation, normally the performance is not a problem with a
 * small GtkListStore.
 */
static void
bookshelf_store_changed (DhPreferences *prefs)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);

        gtk_tree_model_foreach (GTK_TREE_MODEL (priv->bookshelf_store),
                                bookshelf_store_changed_foreach_func,
                                NULL);
}

static void
bookshelf_populate_store (DhPreferences *prefs)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);
        DhSettings *settings;
        gboolean group_by_language;
        DhBookManager *book_manager;
        GList *books;
        GList *l;
        GSList *inserted_languages = NULL;

        gtk_list_store_clear (priv->bookshelf_store);

        settings = dh_settings_get_default ();
        group_by_language = dh_settings_get_group_books_by_language (settings);

        book_manager = dh_book_manager_get_singleton ();
        books = dh_book_manager_get_books (book_manager);

        for (l = books; l != NULL; l = l->next) {
                DhBook *book = DH_BOOK (l->data);
                gchar *indented_title = NULL;
                const gchar *title;
                const gchar *language;

                /* Insert book */

                if (group_by_language) {
                        indented_title = g_strdup_printf ("     %s", dh_book_get_title (book));
                        title = indented_title;
                } else {
                        title = dh_book_get_title (book);
                }

                gtk_list_store_insert_with_values (priv->bookshelf_store, NULL, -1,
                                                   COLUMN_BOOK, book,
                                                   COLUMN_TITLE, title,
                                                   -1);

                g_free (indented_title);

                /* Insert language if needed */

                if (!group_by_language)
                        continue;

                language = dh_book_get_language (book);
                if (g_slist_find_custom (inserted_languages, language, (GCompareFunc)g_strcmp0) != NULL)
                        /* Already inserted. */
                        continue;

                gtk_list_store_insert_with_values (priv->bookshelf_store, NULL, -1,
                                                   COLUMN_BOOK, NULL,
                                                   COLUMN_TITLE, language,
                                                   -1);

                inserted_languages = g_slist_prepend (inserted_languages, g_strdup (language));
        }

        g_slist_free_full (inserted_languages, g_free);
}

static void
bookshelf_group_books_by_language_notify_cb (DhSettings    *settings,
                                             GParamSpec    *pspec,
                                             DhPreferences *prefs)
{
        bookshelf_populate_store (prefs);
}

static void
bookshelf_book_created_cb (DhBookManager *book_manager,
                           DhBook        *book,
                           DhPreferences *prefs)
{
        bookshelf_populate_store (prefs);
}

static void
bookshelf_book_deleted_cb (DhBookManager *book_manager,
                           DhBook        *book,
                           DhPreferences *prefs)
{
        bookshelf_populate_store (prefs);
}

static void
bookshelf_row_toggled_cb (GtkCellRendererToggle *cell_renderer,
                          gchar                 *path,
                          DhPreferences         *prefs)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);
        GtkTreeIter iter;
        DhBook *book = NULL;
        gchar *title = NULL;

        if (!gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (priv->bookshelf_store),
                                                  &iter,
                                                  path)) {
                return;
        }

        gtk_tree_model_get (GTK_TREE_MODEL (priv->bookshelf_store),
                            &iter,
                            COLUMN_BOOK, &book,
                            COLUMN_TITLE, &title,
                            -1);

        if (book != NULL) {
                dh_book_set_enabled (book, !dh_book_get_enabled (book));
                bookshelf_store_changed (prefs);
        } else {
                const gchar *language = title;
                gboolean enable;

                if (is_language_group_inconsistent (language))
                        enable = TRUE;
                else
                        enable = !is_language_group_active (language);

                set_language_group_enabled (language, enable);
                bookshelf_store_changed (prefs);
        }

        g_clear_object (&book);
        g_free (title);
}

/* The implementation is simpler with a sort function. Performance is normally
 * not a problem with a small GtkListStore. A previous implementation didn't use
 * a sort function and inserted the books and language groups at the right place
 * directly by walking through the GtkListStore, but it takes a lot of code to
 * do that.
 */
static gint
bookshelf_sort_func (GtkTreeModel *model,
                     GtkTreeIter  *iter_a,
                     GtkTreeIter  *iter_b,
                     gpointer      user_data)
{
        DhBook *book_a;
        DhBook *book_b;
        gchar *title_a;
        gchar *title_b;
        const gchar *language_a;
        const gchar *language_b;
        gint ret;

        gtk_tree_model_get (model,
                            iter_a,
                            COLUMN_BOOK, &book_a,
                            COLUMN_TITLE, &title_a,
                            -1);

        gtk_tree_model_get (model,
                            iter_b,
                            COLUMN_BOOK, &book_b,
                            COLUMN_TITLE, &title_b,
                            -1);

        if (book_a != NULL)
                language_a = dh_book_get_language (book_a);
        else
                language_a = title_a;

        if (book_b != NULL)
                language_b = dh_book_get_language (book_b);
        else
                language_b = title_b;

        ret = g_strcmp0 (language_a, language_b);
        if (ret != 0) {
                /* Different language. */
                goto out;
        }

        /* Same language. */

        if (book_a == NULL) {
                if (book_b == NULL) {
                        /* Duplicated language group, should not happen. */
                        g_warn_if_reached ();
                        ret = 0;
                } else {
                        /* @iter_a is the language group and @iter_b is a book
                         * inside that language group.
                         */
                        ret = -1;
                }
        } else if (book_b == NULL) {
                /* @iter_b is the language group and @iter_a is a book inside
                 * that language group.
                 */
                ret = 1;
        } else {
                ret = dh_book_cmp_by_title (book_a, book_b);
        }

out:
        g_clear_object (&book_a);
        g_clear_object (&book_b);
        g_free (title_a);
        g_free (title_b);
        return ret;
}

static void
init_bookshelf_store (DhPreferences *prefs)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);

        g_assert (priv->bookshelf_store == NULL);
        priv->bookshelf_store = gtk_list_store_new (N_COLUMNS,
                                                    DH_TYPE_BOOK,
                                                    G_TYPE_STRING); /* Title */

        gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (priv->bookshelf_store),
                                                 bookshelf_sort_func,
                                                 NULL, NULL);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->bookshelf_store),
                                              GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                              GTK_SORT_ASCENDING);

        bookshelf_populate_store (prefs);
}

static void
bookshelf_cell_data_func_toggle (GtkTreeViewColumn *column,
                                 GtkCellRenderer   *cell,
                                 GtkTreeModel      *model,
                                 GtkTreeIter       *iter,
                                 gpointer           data)
{
        DhBook *book = NULL;
        gchar *title = NULL;
        gboolean active;
        gboolean inconsistent;

        gtk_tree_model_get (model,
                            iter,
                            COLUMN_BOOK, &book,
                            COLUMN_TITLE, &title,
                            -1);

        if (book != NULL) {
                DhSettings *settings = dh_settings_get_default ();

                active = dh_settings_is_book_enabled (settings, book);
                inconsistent = FALSE;
        } else {
                active = is_language_group_active (title);
                inconsistent = is_language_group_inconsistent (title);
        }

        g_object_set (cell,
                      "active", active,
                      "inconsistent", inconsistent,
                      NULL);

        g_clear_object (&book);
        g_free (title);
}

static void
bookshelf_cell_data_func_text (GtkTreeViewColumn *column,
                               GtkCellRenderer   *cell,
                               GtkTreeModel      *model,
                               GtkTreeIter       *iter,
                               gpointer           data)
{
        DhBook *book = NULL;
        gchar *title = NULL;
        PangoWeight weight;

        gtk_tree_model_get (model,
                            iter,
                            COLUMN_BOOK, &book,
                            COLUMN_TITLE, &title,
                            -1);

        if (book != NULL)
                weight = PANGO_WEIGHT_NORMAL;
        else
                weight = PANGO_WEIGHT_BOLD; /* For the language group. */

        g_object_set (cell,
                      "text", title,
                      "weight", weight,
                      NULL);

        g_clear_object (&book);
        g_free (title);
}

static void
init_bookshelf_view (DhPreferences *prefs)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);
        GtkCellRenderer *cell_renderer_toggle;
        GtkCellRenderer *cell_renderer_text;

        gtk_tree_view_set_model (priv->bookshelf_view,
                                 GTK_TREE_MODEL (priv->bookshelf_store));

        /* Enabled column */
        cell_renderer_toggle = gtk_cell_renderer_toggle_new ();
        gtk_tree_view_insert_column_with_data_func (priv->bookshelf_view,
                                                    -1,
                                                    _("Enabled"),
                                                    cell_renderer_toggle,
                                                    bookshelf_cell_data_func_toggle,
                                                    NULL, NULL);

        g_signal_connect_object (cell_renderer_toggle,
                                 "toggled",
                                 G_CALLBACK (bookshelf_row_toggled_cb),
                                 prefs,
                                 0);

        /* Title column */
        cell_renderer_text = gtk_cell_renderer_text_new ();
        gtk_tree_view_insert_column_with_data_func (priv->bookshelf_view,
                                                    -1,
                                                    _("Title"),
                                                    cell_renderer_text,
                                                    bookshelf_cell_data_func_text,
                                                    NULL, NULL);
}

static void
init_bookshelf_tab (DhPreferences *prefs)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);
        DhSettings *settings;
        DhBookManager *book_manager;

        init_bookshelf_store (prefs);
        init_bookshelf_view (prefs);

        settings = dh_settings_get_default ();

        g_object_bind_property (settings, "group-books-by-language",
                                priv->bookshelf_group_by_language_checkbutton, "active",
                                G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

        g_signal_connect_object (settings,
                                 "notify::group-books-by-language",
                                 G_CALLBACK (bookshelf_group_books_by_language_notify_cb),
                                 prefs,
                                 0);

        book_manager = dh_book_manager_get_singleton ();

        g_signal_connect_object (book_manager,
                                 "book-created",
                                 G_CALLBACK (bookshelf_book_created_cb),
                                 prefs,
                                 0);

        g_signal_connect_object (book_manager,
                                 "book-deleted",
                                 G_CALLBACK (bookshelf_book_deleted_cb),
                                 prefs,
                                 0);
}

static void
init_fonts_tab (DhPreferences *prefs)
{
        DhPreferencesPrivate *priv = dh_preferences_get_instance_private (prefs);
        DhSettingsApp *settings_app;
        GSettings *fonts_settings;

        settings_app = dh_settings_app_get_singleton ();
        fonts_settings = dh_settings_app_peek_fonts_settings (settings_app);

        g_settings_bind (fonts_settings, "use-system-fonts",
                         priv->use_system_fonts_checkbutton, "active",
                         G_SETTINGS_BIND_DEFAULT);

        g_object_bind_property (priv->use_system_fonts_checkbutton, "active",
                                priv->custom_fonts_grid, "sensitive",
                                G_BINDING_DEFAULT |
                                G_BINDING_SYNC_CREATE |
                                G_BINDING_INVERT_BOOLEAN);

        g_settings_bind (fonts_settings, "variable-font",
                         priv->variable_font_button, "font",
                         G_SETTINGS_BIND_DEFAULT);

        g_settings_bind (fonts_settings, "fixed-font",
                         priv->fixed_font_button, "font",
                         G_SETTINGS_BIND_DEFAULT);
}

static void
dh_preferences_init (DhPreferences *prefs)
{
        gtk_widget_init_template (GTK_WIDGET (prefs));

        gtk_window_set_destroy_with_parent (GTK_WINDOW (prefs), TRUE);

        init_bookshelf_tab (prefs);
        init_fonts_tab (prefs);
}

void
dh_preferences_show_dialog (GtkWindow *parent)
{
        static GtkWindow *prefs = NULL;

        g_return_if_fail (GTK_IS_WINDOW (parent));

        if (prefs == NULL) {
                prefs = g_object_new (DH_TYPE_PREFERENCES,
                                      "use-header-bar", TRUE,
                                      NULL);

                g_signal_connect (prefs,
                                  "destroy",
                                  G_CALLBACK (gtk_widget_destroyed),
                                  &prefs);
        }

        if (parent != gtk_window_get_transient_for (prefs))
                gtk_window_set_transient_for (prefs, parent);

        gtk_window_present (prefs);
}
