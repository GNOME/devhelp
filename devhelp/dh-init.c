/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2012 Aleksander Morgado <aleksander@gnu.org>
 * SPDX-FileCopyrightText: 2017-2020 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"
#include "dh-init.h"
#include <glib/gi18n-lib.h>
#include <webkit2/webkit2.h>
#include "dh-book-list.h"
#include "dh-profile.h"
#include "dh-settings.h"

/**
 * dh_init:
 *
 * Initializes the Devhelp library (e.g. for the internationalization).
 *
 * This function can be called several times, but is meant to be called at the
 * beginning of main(), before any other Devhelp function call.
 *
 * Since version 3.38, this function enables the WebKitGTK sandbox by calling
 * webkit_web_context_set_sandbox_enabled() on the default #WebKitWebContext.
 */
void
dh_init (void)
{
        static gboolean done = FALSE;

        if (!done) {
                WebKitWebContext *webkit_context;

                bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
                bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

                webkit_context = webkit_web_context_get_default ();
                webkit_web_context_set_sandbox_enabled (webkit_context, TRUE);

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
 * https://gitlab.gnome.org/GNOME/gtksourceview/commit/e761de9c2bee90c232875bbc41e6e73e1f63e145
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
                _dh_book_list_unref_default ();
                _dh_profile_unref_default ();
                _dh_settings_unref_default ();
                done = TRUE;
        }
}
