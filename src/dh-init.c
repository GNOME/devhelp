/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "dh-init.h"
#include "dh-book-manager.h"
#include "dh-settings.h"

void
dh_init (void)
{
}

/**
 * dh_free_resources:
 *
 * Free the resources allocated by Devhelp. For example it unrefs the singleton
 * objects.
 *
 * It is not mandatory to call this function, it's just to be friendlier to
 * memory debugging tools. This function is meant to be called at the end of
 * main().
 *
 * Since: 3.26
 */

/* Another way is to use a DSO destructor, see gconstructor.h in GLib.
 *
 * The advantage of calling dh_free_resources() at the end of main() is that
 * gobject-list [1] correctly reports that all Dh* objects have been finalized
 * when quitting the application. On the other hand a DSO destructor runs after
 * the gobject-list's last output, so it's much less convenient, see:
 * https://git.gnome.org/browse/gtksourceview/commit/?id=e761de9c2bee90c232875bbc41e6e73e1f63e145
 *
 * [1] A tool for debugging the lifetime of GObjects:
 * https://github.com/danni/gobject-list
 */
void
dh_free_resources (void)
{
        _dh_book_manager_free_singleton ();
        _dh_settings_free_singleton ();
}
