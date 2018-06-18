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
