/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
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
#include "devhelp.h"

#include "ige-conf.h"
#include "dh-util.h"

static void
load_config_defaults (void)
{
        IgeConf    *conf;
        gchar      *path;

        conf = ige_conf_get ();
        path = dh_util_build_data_filename ("devhelp", "devhelp.defaults", NULL);
        ige_conf_add_defaults (conf, path);
        g_free (path);
}

void
dh_init (void)
{
        load_config_defaults ();
}
