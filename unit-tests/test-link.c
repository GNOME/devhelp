/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2017 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <devhelp/devhelp.h>

#define DEVHELP_BOOK_BASE_PATH "/usr/share/gtk-doc/html/devhelp"

static void
check_belongs_to_page_book_link (DhLink *book_link)
{
        g_assert (dh_link_belongs_to_page (book_link, "index"));
        g_assert (!dh_link_belongs_to_page (book_link, "Index"));
        g_assert (!dh_link_belongs_to_page (book_link, ""));
        g_assert (!dh_link_belongs_to_page (book_link, "kiwi"));
}

static void
test_belongs_to_page (void)
{
        DhLink *book_link;
        DhLink *link;

        /* index.html */
        book_link = dh_link_new_book (DEVHELP_BOOK_BASE_PATH,
                                      "devhelp",
                                      "Devhelp Reference Manual",
                                      "index.html");
        check_belongs_to_page_book_link (book_link);
        dh_link_unref (book_link);

        /* Empty relative_url */
        book_link = dh_link_new_book (DEVHELP_BOOK_BASE_PATH,
                                      "devhelp",
                                      "Devhelp Reference Manual",
                                      "");
        check_belongs_to_page_book_link (book_link);

        /* A function */
        link = dh_link_new (DH_LINK_TYPE_FUNCTION,
                            book_link,
                            "dh_link_ref",
                            "DhLink.html#dh-link-ref");

        g_assert (dh_link_belongs_to_page (link, "DhLink"));
        g_assert (!dh_link_belongs_to_page (link, "dhlink"));
        g_assert (!dh_link_belongs_to_page (link, ""));
        g_assert (!dh_link_belongs_to_page (link, "kiwi"));

        dh_link_unref (book_link);
        dh_link_unref (link);
}

int
main (int    argc,
      char **argv)
{
        g_test_init (&argc, &argv, NULL);

        g_test_add_func ("/link/belongs_to_page", test_belongs_to_page);

        return g_test_run ();
}
