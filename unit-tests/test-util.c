/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2017 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "devhelp/dh-util-lib.h"

static void
check_get_possible_index_files (const gchar *book_directory_path,
                                const gchar *book_basename)
{
        GFile *book_directory;
        GSList *list;
        GSList *l;
        gint i;

        book_directory = g_file_new_for_path (book_directory_path);
        list = _dh_util_get_possible_index_files (book_directory);

        g_assert_cmpint (g_slist_length (list), ==, 4);

        for (l = list, i = 0; l != NULL; l = l->next, i++) {
                GFile *index_file = G_FILE (l->data);
                gchar *expected_basename;
                gchar *basename;
                gchar *expected_path;
                const gchar *path;

                switch (i) {
                        case 0:
                                expected_basename = g_strconcat (book_basename, ".devhelp2", NULL);
                                break;

                        case 1:
                                expected_basename = g_strconcat (book_basename, ".devhelp2.gz", NULL);
                                break;

                        case 2:
                                expected_basename = g_strconcat (book_basename, ".devhelp", NULL);
                                break;

                        case 3:
                                expected_basename = g_strconcat (book_basename, ".devhelp.gz", NULL);
                                break;

                        default:
                                g_assert_not_reached ();
                }

                basename = g_file_get_basename (index_file);
                g_assert_cmpstr (basename, ==, expected_basename);

                expected_path = g_build_filename (book_directory_path, expected_basename, NULL);
                path = g_file_peek_path (index_file);
                g_assert_cmpstr (path, ==, expected_path);

                g_free (expected_basename);
                g_free (basename);
                g_free (expected_path);
        }

        g_object_unref (book_directory);
        g_slist_free_full (list, g_object_unref);
}

static void
test_get_possible_index_files (void)
{
        check_get_possible_index_files ("/usr/share/gtk-doc/html/glib", "glib");
        check_get_possible_index_files ("/usr/share/gtk-doc/html/glib/", "glib");
        check_get_possible_index_files ("/usr/share/gtk-doc/html/gtk3", "gtk3");
        check_get_possible_index_files ("/usr/share/gtk-doc/html/gtk3/", "gtk3");
}

int
main (int    argc,
      char **argv)
{
        g_test_init (&argc, &argv, NULL);

        g_test_add_func ("/util/get_possible_index_files", test_get_possible_index_files);

        return g_test_run ();
}
