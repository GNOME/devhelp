/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
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

#include "devhelp/dh-search-context.h"

static gboolean
strv_equal (GStrv strv1,
            GStrv strv2)
{
        gint i1;
        gint i2;

        if (strv1 == NULL || strv2 == NULL)
                return strv1 == NULL && strv2 == NULL;

        for (i1 = 0, i2 = 0;
             strv1[i1] != NULL && strv2[i2] != NULL;
             i1++, i2++) {
                const gchar *cur_str1 = strv1[i1];
                const gchar *cur_str2 = strv2[i2];

                if (!g_str_equal (cur_str1, cur_str2))
                        return FALSE;
        }

        return strv1[i1] == NULL && strv2[i2] == NULL;
}

static void
check_process_search_string (const gchar *search_string,
                             gboolean     expected_valid,
                             const gchar *expected_book_id,
                             const gchar *expected_page_id,
                             GStrv        expected_keywords)
{
        DhSearchContext *search_context;

        search_context = _dh_search_context_new (search_string);
        g_assert_cmpint (expected_valid, ==, (search_context != NULL));

        if (search_context == NULL) {
                g_assert (expected_book_id == NULL);
                g_assert (expected_page_id == NULL);
                g_assert (expected_keywords == NULL);
                return;
        }

        g_assert_cmpstr (_dh_search_context_get_book_id (search_context), ==, expected_book_id);
        g_assert_cmpstr (_dh_search_context_get_page_id (search_context), ==, expected_page_id);
        g_assert (strv_equal (_dh_search_context_get_keywords (search_context), expected_keywords));

        _dh_search_context_free (search_context);
}

static void
test_process_search_string (void)
{
        GPtrArray *array;
        GStrv keywords1;
        GStrv keywords2;

        /* Empty or only whitespace. */
        check_process_search_string ("", FALSE, NULL, NULL, NULL);
        check_process_search_string (" ", FALSE, NULL, NULL, NULL);
        check_process_search_string (" \t ", FALSE, NULL, NULL, NULL);
        check_process_search_string (" \t \n", FALSE, NULL, NULL, NULL);

        /* book_id and page_id without keywords. */
        check_process_search_string ("book:devhelp", TRUE, "devhelp", NULL, NULL);
        check_process_search_string ("page:DhBook", TRUE, NULL, "DhBook", NULL);
        check_process_search_string ("book:devhelp page:DhBook", TRUE, "devhelp", "DhBook", NULL);
        check_process_search_string ("page:DhBook book:devhelp", TRUE, "devhelp", "DhBook", NULL);
        check_process_search_string ("book:devhelp page:", TRUE, "devhelp", NULL, NULL);
        check_process_search_string ("book: page:DhBook", TRUE, NULL, "DhBook", NULL);
        check_process_search_string ("book:devhelp book:gtk3", FALSE, NULL, NULL, NULL);
        check_process_search_string ("page:DhBook page:DhCompletion", FALSE, NULL, NULL, NULL);

        /* Normal keywords before book_id or page_id. */
        check_process_search_string ("dh_link_ book:devhelp", FALSE, NULL, NULL, NULL);
        check_process_search_string ("dh_link_ get page:DhLink", FALSE, NULL, NULL, NULL);
        check_process_search_string ("dh_link_ get book:devhelp page:DhLink", FALSE, NULL, NULL, NULL);

        /* Only normal keywords. */

        array = g_ptr_array_new ();
        g_ptr_array_add (array, (gpointer) "dh_link_");
        g_ptr_array_add (array, NULL);
        keywords1 = (GStrv) g_ptr_array_free (array, FALSE);

        array = g_ptr_array_new ();
        g_ptr_array_add (array, (gpointer) "dh_link_");
        g_ptr_array_add (array, (gpointer) "get");
        g_ptr_array_add (array, NULL);
        keywords2 = (GStrv) g_ptr_array_free (array, FALSE);

        check_process_search_string ("dh_link_", TRUE, NULL, NULL, keywords1);
        check_process_search_string (" dh_link_ ", TRUE, NULL, NULL, keywords1);
        check_process_search_string ("dh_link_ get", TRUE, NULL, NULL, keywords2);
        check_process_search_string ("dh_link_  get  ", TRUE, NULL, NULL, keywords2);

        /* book_id, page_id and keywords. */
        check_process_search_string ("book:devhelp dh_link_", TRUE, "devhelp", NULL, keywords1);
        check_process_search_string ("book:devhelp  dh_link_  get ", TRUE, "devhelp", NULL, keywords2);
        check_process_search_string ("page:DhLink dh_link_", TRUE, NULL, "DhLink", keywords1);
        check_process_search_string ("page:DhLink dh_link_ get", TRUE, NULL, "DhLink", keywords2);
        check_process_search_string ("book:devhelp page:DhLink dh_link_", TRUE, "devhelp", "DhLink", keywords1);
        check_process_search_string ("book:devhelp  page:DhLink  \t dh_link_ ", TRUE, "devhelp", "DhLink", keywords1);
        check_process_search_string ("page:DhLink book:devhelp dh_link_", TRUE, "devhelp", "DhLink", keywords1);
        check_process_search_string ("book:devhelp page:DhLink dh_link_ get", TRUE, "devhelp", "DhLink", keywords2);
        check_process_search_string ("book:devhelp page:DhLink dh_link_  \t get\n", TRUE, "devhelp", "DhLink", keywords2);

        g_free (keywords1);
        g_free (keywords2);
}

static void
check_case_sensitive (const gchar *search_string,
                      gboolean     expected_case_sensitive)
{
        DhSearchContext *search_context;

        search_context = _dh_search_context_new (search_string);
        g_assert (search_context != NULL);
        g_assert_cmpint (_dh_search_context_get_case_sensitive (search_context), ==, expected_case_sensitive);
        _dh_search_context_free (search_context);
}

static void
test_case_sensitive (void)
{
        /* Only keywords. */
        check_case_sensitive ("dh_link_", FALSE);
        check_case_sensitive ("dh_link_ get", FALSE);
        check_case_sensitive ("DhLink", TRUE);
        check_case_sensitive ("a DhLink", TRUE);

        /* book_id and page_id only. */
        check_case_sensitive ("book:devhelp", FALSE);
        check_case_sensitive ("page:DhLink", FALSE);
        check_case_sensitive ("book:devhelp page:DhLink", FALSE);

        /* book_id, page_id and keywords. */

        // Only normal keywords must be taken into account for case sensitivity.
        check_case_sensitive ("book:devhelp page:DhLink dh_link_ get", FALSE);
        check_case_sensitive ("page:DhLink dh_link_ get", FALSE);

        check_case_sensitive ("book:devhelp dh_link_ get", FALSE);
        check_case_sensitive ("book:devhelp page:DhLink DhLink", TRUE);
        check_case_sensitive ("book:devhelp DhLink", TRUE);
}

static void
check_link_simple (const gchar *search_string,
                   const gchar *link_name,
                   gboolean     prefix,
                   gboolean     expected_match,
                   gboolean     expected_exact)
{
        DhSearchContext *search_context;
        DhLink *book_link;
        DhLink *link;
        gboolean match;
        gboolean exact;

        search_context = _dh_search_context_new (search_string);
        g_assert (search_context != NULL);

        book_link = dh_link_new_book ("/usr/share/gtk-doc/html/devhelp-3",
                                      "devhelp",
                                      "Devhelp Reference Manual",
                                      "index.html");

        link = dh_link_new (DH_LINK_TYPE_FUNCTION,
                            book_link,
                            link_name,
                            "ClassName.html#function-name");

        match = _dh_search_context_match_link (search_context, link, prefix);
        g_assert_cmpint (match, ==, expected_match);

        if (match && prefix) {
                exact = _dh_search_context_is_exact_link (search_context, link);
                g_assert_cmpint (exact, ==, expected_exact);
        } else {
                g_assert (!expected_exact);
        }

        _dh_search_context_free (search_context);
        dh_link_unref (book_link);
        dh_link_unref (link);
}

static void
test_link_simple (void)
{
        /* Prefix match but not exact. */
        check_link_simple ("dh_link_", "dh_link_new", TRUE, TRUE, FALSE);
        check_link_simple ("dh_link_", "dh_link_new", FALSE, FALSE, FALSE);

        /* Prefix match and exact. */
        check_link_simple ("dh_link_new", "dh_link_new", TRUE, TRUE, TRUE);
        check_link_simple ("dh_link_new", "dh_link_new", FALSE, FALSE, FALSE);

        /* Nonprefix match. */
        check_link_simple ("link", "dh_link_new", TRUE, FALSE, FALSE);
        check_link_simple ("link", "dh_link_new", FALSE, TRUE, FALSE);

        /* Case insensitive. */
        check_link_simple ("link", "DhLink", TRUE, FALSE, FALSE);
        check_link_simple ("link", "DhLink", FALSE, TRUE, FALSE);

        /* Case sensitive. */
        check_link_simple ("Link", "DhLink", TRUE, FALSE, FALSE);
        check_link_simple ("Link", "DhLink", FALSE, TRUE, FALSE);
        check_link_simple ("Link", "dh_link_new", TRUE, FALSE, FALSE);
        check_link_simple ("Link", "dh_link_new", FALSE, FALSE, FALSE);

        /* Several keywords. */
        check_link_simple ("dh_link_ book", "dh_link_new_book", TRUE, TRUE, FALSE);
        check_link_simple ("dh_link_ book", "dh_link_new_book", FALSE, FALSE, FALSE);
        check_link_simple ("dh_link_ book", "dh_link_new", TRUE, FALSE, FALSE);
        check_link_simple ("dh_link_ book", "dh_link_new", FALSE, FALSE, FALSE);

        /* Globs */
        check_link_simple ("dh_link_*book", "dh_link_new_book", TRUE, TRUE, FALSE);
        check_link_simple ("dh_link_*book", "dh_link_new_book", FALSE, FALSE, FALSE);
        check_link_simple ("dh_link_*book", "dh_link_new", TRUE, FALSE, FALSE);
        check_link_simple ("dh_link_*book", "dh_link_new", FALSE, FALSE, FALSE);

        check_link_simple ("??_link_new", "dh_link_new", TRUE, TRUE, FALSE);
        check_link_simple ("??_link_new", "dh_link_new", FALSE, FALSE, FALSE);
        check_link_simple ("??_link_new", "dh_link_compare", TRUE, FALSE, FALSE);
        check_link_simple ("??_link_new", "dh_link_compare", FALSE, FALSE, FALSE);

        /* Several keywords, not necessarily in the same order. */
        check_link_simple ("gtk window application", "gtk_window_get_application", TRUE, TRUE, FALSE);
        check_link_simple ("gtk window application", "gtk_window_get_application", FALSE, FALSE, FALSE);
        check_link_simple ("gtk window application", "GtkApplicationWindow", TRUE, TRUE, FALSE);
        check_link_simple ("gtk window application", "GtkApplicationWindow", FALSE, FALSE, FALSE);

        check_link_simple ("gtk*window*application", "gtk_window_get_application", TRUE, TRUE, FALSE);
        check_link_simple ("gtk*window*application", "gtk_window_get_application", FALSE, FALSE, FALSE);
        check_link_simple ("gtk*window*application", "GtkApplicationWindow", TRUE, FALSE, FALSE);
        check_link_simple ("gtk*window*application", "GtkApplicationWindow", FALSE, FALSE, FALSE);

        /* Prefix appearing several times.
         * The DhLink must not appear two times in the search results.
         */
        check_link_simple ("GTK CELL_RENDERER_ACCEL_MODE_GTK", "GTK_CELL_RENDERER_ACCEL_MODE_GTK",
                           TRUE, TRUE, FALSE);
        check_link_simple ("GTK CELL_RENDERER_ACCEL_MODE_GTK", "GTK_CELL_RENDERER_ACCEL_MODE_GTK",
                           FALSE, FALSE, FALSE);
        check_link_simple ("GTK* CELL_RENDERER_ACCEL_MODE_GTK", "GTK_CELL_RENDERER_ACCEL_MODE_GTK",
                           TRUE, TRUE, FALSE);
        check_link_simple ("GTK* CELL_RENDERER_ACCEL_MODE_GTK", "GTK_CELL_RENDERER_ACCEL_MODE_GTK",
                           FALSE, FALSE, FALSE);
}

int
main (int    argc,
      char **argv)
{
        g_test_init (&argc, &argv, NULL);

        g_test_add_func ("/search_context/process_search_string", test_process_search_string);
        g_test_add_func ("/search_context/case_sensitive", test_case_sensitive);
        g_test_add_func ("/search_context/link_simple", test_link_simple);

        return g_test_run ();
}
