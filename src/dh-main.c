/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2008 Imendio AB
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

#include <locale.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "dh-app.h"
#include "dh-book-manager.h"
#include "dh-settings.h"

int
main (int argc, char **argv)
{
        DhApp   *application;
        gint     status;

        setlocale (LC_ALL, "");
        bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        /* Create new DhApp */
        application = dh_app_new ();

        /* And run the GtkApplication */
        status = g_application_run (G_APPLICATION (application), argc, argv);

        g_object_unref (application);

        dh_book_manager_free_singleton ();
        dh_settings_free_singleton ();

        return status;
}
