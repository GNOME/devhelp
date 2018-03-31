/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2017 Sébastien Wilmet <swilmet@gnome.org>
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

#include <devhelp/devhelp.h>

#define DEVHELP_BOOK_BASE_PATH "/usr/share/gtk-doc/html/devhelp-3"

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
