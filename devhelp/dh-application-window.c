/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dh-application-window.h"
#include "dh-util-lib.h"

/**
 * SECTION:dh-application-window
 * @Title: DhApplicationWindow
 * @Short_description: For the main application window
 *
 * Functions for the main application window.
 */

/**
 * dh_application_window_bind_sidebar_and_notebook:
 * @sidebar: a #DhSidebar.
 * @notebook: an empty #DhNotebook.
 *
 * Binds @sidebar and @notebook:
 * - When the #DhSidebar::link-selected signal is emitted, open the URI in the
 *   active #DhWebView.
 * - On #GtkNotebook::switch-page or when the user clicks on a link, calls
 *   dh_sidebar_select_uri() with the new active URI.
 *
 * You need to call this function when the #DhNotebook is empty, i.e. before
 * adding the first #DhTab.
 *
 * Note that this function doesn't take a “self” window parameter, to be more
 * flexible: it is possible to have several pairs of #DhSidebar/#DhNotebook per
 * window, to show different #DhProfile's.
 *
 * Since: 3.30
 */
void
dh_application_window_bind_sidebar_and_notebook (DhSidebar  *sidebar,
                                                 DhNotebook *notebook)
{
        g_return_if_fail (DH_IS_SIDEBAR (sidebar));
        g_return_if_fail (DH_IS_NOTEBOOK (notebook));
        g_return_if_fail (dh_notebook_get_active_tab (notebook) == NULL);

        /* Have the implementation separate from dh-application-window, because
         * it is planned to have a real DhApplicationWindow class in the
         * libdevhelp that will have similar signal handlers, it would be
         * confusing to have two times the same signal handlers.
         *
         * API design:
         * But the public function belongs to the window, since the window is
         * the container containing the two widgets. Another container
         * containing the two widgets, in the Devhelp app, is the horizontal
         * GtkPaned, but it would have been less flexible to create a GtkPaned
         * subclass with the bind() function, because it would have forced an
         * IDE to use GtkPaned to be able to bind the two widgets (an IDE may
         * want to use something else than a GtkPaned).
         */
        _dh_util_bind_sidebar_and_notebook (sidebar, notebook);
}
