/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
 * Copyright (C) 2017 Sébastien Wilmet <swilmet@gnome.org>
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

#include "config.h"
#include "dh-init.h"
#include <glib/gi18n-lib.h>
#include "dh-book-manager.h"
#include "dh-profile.h"
#include "dh-settings.h"

/**
 * dh_init:
 *
 * Initializes the Devhelp library (e.g. for the internationalization).
 *
 * This function can be called several times, but is meant to be called at the
 * beginning of main(), before any other Devhelp function call.
 */
void
dh_init (void)
{
        static gboolean done = FALSE;

        if (!done) {
                bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
                bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
                done = TRUE;
        }
}

/**
 * dh_finalize:
 *
 * Free the resources allocated by Devhelp. For example it unrefs the singleton
 * objects.
 *
 * It is not mandatory to call this function, it's just to be friendlier to
 * memory debugging tools. This function is meant to be called at the end of
 * main(). It can be called several times.
 *
 * Since: 3.26
 */

/* Another way is to use a DSO destructor, see gconstructor.h in GLib.
 *
 * The advantage of calling dh_finalize() at the end of main() is that
 * gobject-list [1] correctly reports that all Dh* objects have been finalized
 * when quitting the application. On the other hand a DSO destructor runs after
 * the gobject-list's last output, so it's much less convenient, see:
 * https://git.gnome.org/browse/gtksourceview/commit/?id=e761de9c2bee90c232875bbc41e6e73e1f63e145
 *
 * [1] A tool for debugging the lifetime of GObjects:
 * https://github.com/danni/gobject-list
 */
void
dh_finalize (void)
{
        static gboolean done = FALSE;

        /* Unref the singletons only once, even if this function is called
         * multiple times, to see if a reference is not released correctly.
         * Normally the singletons have a ref count of 1. If for some reason the
         * ref count is increased somewhere, it needs to be decreased
         * accordingly, at the right place.
         */
        if (!done) {
                _dh_book_manager_unref_singleton ();
                _dh_profile_unref_default ();
                _dh_settings_unref_default ();
                done = TRUE;
        }
}
