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

#include "devhelp.h"

static void
test_belongs_to_page (void)
{
}

int
main (int    argc,
      char **argv)
{
        g_test_init (&argc, &argv, NULL);

        g_test_add_func ("/link/belongs_to_page", test_belongs_to_page);

        return g_test_run ();
}
