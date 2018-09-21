/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2008 Imendio AB
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
#include <locale.h>
#include <glib/gi18n.h>
#include <devhelp/devhelp.h>
#include <amtk/amtk.h>
#include "dh-app.h"
#include "dh-settings-app.h"

int
main (int argc, char **argv)
{
        DhApp *application;
        gint status;

        setlocale (LC_ALL, "");
        textdomain (GETTEXT_PACKAGE);

        dh_init ();
        amtk_init ();

        application = dh_app_new ();
        status = g_application_run (G_APPLICATION (application), argc, argv);
        g_object_unref (application);

        amtk_finalize ();
        dh_finalize ();
        dh_settings_app_unref_singleton ();

        return status;
}
