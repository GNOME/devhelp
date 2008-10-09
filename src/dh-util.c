/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001      Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004,2008 Imendio AB
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
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "dh-util.h"

static GladeXML *
get_glade_file (const gchar *filename,
                const gchar *root,
                const gchar *domain,
                const gchar *first_required_widget,
                va_list args)
{
        GladeXML   *gui;
        const char *name;
        GtkWidget **widget_ptr;

        gui = glade_xml_new (filename, root, domain);
        if (!gui) {
                g_warning ("Couldn't find necessary glade file '%s'", filename);
                return NULL;
        }

        for (name = first_required_widget; name; name = va_arg (args, char *)) {
                widget_ptr = va_arg (args, void *);
                *widget_ptr = glade_xml_get_widget (gui, name);

                if (!*widget_ptr) {
                        g_warning ("Glade file '%s' is missing widget '%s'.",
                                   filename, name);
                        continue;
                }
        }

        return gui;
}

GladeXML *
dh_util_glade_get_file (const gchar *filename,
                        const gchar *root,
                        const gchar *domain,
                        const gchar *first_required_widget,
                        ...)
{
        va_list   args;
        GladeXML *gui;

        va_start (args, first_required_widget);
        gui = get_glade_file (filename,
                              root,
                              domain,
                              first_required_widget,
                              args);
        va_end (args);

        return gui;
}

void
dh_util_glade_connect (GladeXML *gui,
                       gpointer  user_data,
                       gchar    *first_widget,
                       ...)
{
        va_list      args;
        const gchar *name;
        const gchar *signal;
        GtkWidget   *widget;
        gpointer    *callback;

        va_start (args, first_widget);

        for (name = first_widget; name; name = va_arg (args, char *)) {
                signal = va_arg (args, void *);
                callback = va_arg (args, void *);

                widget = glade_xml_get_widget (gui, name);
                if (!widget) {
                        g_warning ("Glade file is missing widget '%s', aborting",
                                   name);
                        continue;
                }

                g_signal_connect (widget,
                                  signal,
                                  G_CALLBACK (callback),
                                  user_data);
        }

        va_end (args);
}

gchar *
dh_util_build_data_filename (const gchar *first_part,
                             ...)
{
        const gchar  *datadir = NULL;
        va_list       args;
        const gchar  *part;
        gchar       **strv;
        gint          i;
        gchar        *ret;

        va_start (args, first_part);

#ifdef GDK_WINDOWING_QUARTZ
        datadir = g_getenv ("DEVHELP_DATADIR");
#endif

        if (datadir == NULL) {
                datadir = DATADIR;
        }

        /* 2 = 1 initial component + terminating NULL element. */
        strv = g_malloc (sizeof (gchar *) * 2);
        strv[0] = (gchar *) datadir;

        i = 1;
        for (part = first_part; part; part = va_arg (args, char *), i++) {
                /* +2 = 1 new element + terminating NULL element. */
                strv = g_realloc (strv, sizeof (gchar*) * (i + 2));
                strv[i] = (gchar *) part;
        }

        strv[i] = NULL;
        ret = g_build_filenamev (strv);
        g_free (strv);

        va_end (args);

        return ret;
}
