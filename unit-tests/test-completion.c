/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
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

#include <devhelp/devhelp.h>

static void
test_empty (void)
{
        DhCompletion *completion;
        gchar *result;

        completion = dh_completion_new ();
        dh_completion_sort (completion);

        result = dh_completion_complete (completion, "daft");
        g_assert (result == NULL);

        g_object_unref (completion);
}

static void
test_empty_string (void)
{
        DhCompletion *completion;
        gchar *result;

        completion = dh_completion_new ();
        dh_completion_add_string (completion, "daft");
        dh_completion_sort (completion);

        /* Complete empty string. */
        result = dh_completion_complete (completion, "");
        g_assert_cmpstr (result, ==, "daft");
        g_free (result);

        g_object_unref (completion);

        /* Empty string in DhCompletion. */
        completion = dh_completion_new ();
        dh_completion_add_string (completion, "");
        dh_completion_sort (completion);

        // String not found.
        result = dh_completion_complete (completion, "a");
        g_assert (result == NULL);
        // String found.
        result = dh_completion_complete (completion, "");
        g_assert (result == NULL);

        // String found, cannot complete.
        dh_completion_add_string (completion, "daft");
        dh_completion_sort (completion);
        result = dh_completion_complete (completion, "");
        g_assert (result == NULL);

        // Empty string doesn't have prefix, can complete.
        result = dh_completion_complete (completion, "d");
        g_assert_cmpstr (result, ==, "daft");
        g_free (result);

        g_object_unref (completion);
}

static void
test_complete_simple (void)
{
        DhCompletion *completion;
        gchar *result;

        completion = dh_completion_new ();

        dh_completion_add_string (completion, "a");
        dh_completion_add_string (completion, "ba");
        dh_completion_add_string (completion, "baa");
        dh_completion_add_string (completion, "bab");
        dh_completion_add_string (completion, "c");
        dh_completion_sort (completion);

        /* Completion possible. */
        result = dh_completion_complete (completion, "b");
        g_assert_cmpstr (result, ==, "ba");
        g_free (result);

        /* Exact match found (among other strings with prefix). */
        result = dh_completion_complete (completion, "ba");
        g_assert (result == NULL);

        /* Exact match found (only string with prefix). */
        result = dh_completion_complete (completion, "bab");
        g_assert (result == NULL);

        /* No strings with prefix found. */
        result = dh_completion_complete (completion, "d");
        g_assert (result == NULL);

        /* Strings with prefix found, but cannot complete. */
        dh_completion_add_string (completion, "bb");
        dh_completion_sort (completion);
        result = dh_completion_complete (completion, "b");
        g_assert (result == NULL);

        /* Only one string with prefix found. */
        dh_completion_add_string (completion, "dh_book_new");
        dh_completion_sort (completion);
        result = dh_completion_complete (completion, "dh");
        g_assert_cmpstr (result, ==, "dh_book_new");
        g_free (result);

        g_object_unref (completion);
}

static void
test_utf8 (void)
{
        DhCompletion *completion;
        gchar *result;

        completion = dh_completion_new ();

        /* \300 in octal is the first byte of a 2-bytes UTF-8 char. */
        dh_completion_add_string (completion, "aa\300\200aa");
        dh_completion_add_string (completion, "aa\300\200ab");
        dh_completion_sort (completion);

        result = dh_completion_complete (completion, "a");
        g_assert_cmpstr (result, ==, "aa\300\200a");
        g_free (result);

        dh_completion_add_string (completion, "aa\300\201aa");
        dh_completion_sort (completion);

        result = dh_completion_complete (completion, "a");
        g_assert_cmpstr (result, ==, "aa"); /* not "aa\300" */
        g_free (result);

        result = dh_completion_complete (completion, "aa\300\200");
        g_assert_cmpstr (result, ==, "aa\300\200a");
        g_free (result);

        result = dh_completion_complete (completion, "aa\300\200a");
        g_assert (result == NULL);

        result = dh_completion_complete (completion, "aa\300\200aa");
        g_assert (result == NULL);

        result = dh_completion_complete (completion, "b");
        g_assert (result == NULL);

        /* 2-bytes char + 3-bytes char. */
        dh_completion_add_string (completion, "zz\300\200zz");
        dh_completion_add_string (completion, "zz\340\200\200zz");
        dh_completion_sort (completion);

        result = dh_completion_complete (completion, "z");
        g_assert_cmpstr (result, ==, "zz");
        g_free (result);

        result = dh_completion_complete (completion, "zz");
        g_assert (result == NULL);

        result = dh_completion_complete (completion, "zz\300\200");
        g_assert_cmpstr (result, ==, "zz\300\200zz");
        g_free (result);

        g_object_unref (completion);
}

static void
test_aggregate_complete (void)
{
        DhCompletion *completion1;
        DhCompletion *completion2;
        DhCompletion *completion3;
        GList *list = NULL;
        gchar *result;

        completion1 = dh_completion_new ();
        dh_completion_add_string (completion1, "a");
        dh_completion_add_string (completion1, "baa");
        dh_completion_sort (completion1);

        completion2 = dh_completion_new ();
        dh_completion_add_string (completion2, "ba");
        dh_completion_sort (completion2);

        completion3 = dh_completion_new ();
        dh_completion_add_string (completion3, "bb");
        dh_completion_sort (completion3);

        /* With only completion1. */
        list = g_list_append (list, completion1);
        result = dh_completion_aggregate_complete (list, "b");
        g_assert_cmpstr (result, ==, "baa");
        g_free (result);

        /* With completion1 and completion2. */
        list = g_list_append (list, completion2);
        result = dh_completion_aggregate_complete (list, "b");
        g_assert_cmpstr (result, ==, "ba");
        g_free (result);

        /* With the 3 objects. */
        list = g_list_append (list, completion3);
        result = dh_completion_aggregate_complete (list, "b");
        g_assert (result == NULL);

        result = dh_completion_aggregate_complete (list, "ba");
        g_assert (result == NULL);

        g_object_unref (completion1);
        g_object_unref (completion2);
        g_object_unref (completion3);
        g_list_free (list);
}

int
main (int    argc,
      char **argv)
{
        g_test_init (&argc, &argv, NULL);

        g_test_add_func ("/completion/empty", test_empty);
        g_test_add_func ("/completion/empty_string", test_empty_string);
        g_test_add_func ("/completion/complete_simple", test_complete_simple);
        g_test_add_func ("/completion/utf-8", test_utf8);
        g_test_add_func ("/completion/aggregate_complete", test_aggregate_complete);

        return g_test_run ();
}
