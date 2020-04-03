/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2001-2003 CodeFactory AB
 * SPDX-FileCopyrightText: 2001-2008 Imendio AB
 * SPDX-License-Identifier: GPL-3.0-or-later
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
