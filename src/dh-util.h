/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004 Imendio hB
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

#ifndef __DH_UTIL_H__
#define __DH_UTIL_H__

#include <glib.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs.h>

const gchar *  dh_dot_dir                    (void);
GladeXML *     dh_glade_get_file             (const gchar *filename,
					      const gchar *root,
					      const gchar *domain,
					      const gchar *first_required_widget,
					      ...);
void           dh_glade_connect              (GladeXML    *gui,
					      gpointer     user_data,
					      gchar       *first_widget,
					      ...);
/* Taken from gnome-vfs CVS. */
gchar *        dh_util_uri_relative_new      (const gchar          *text_uri,
					      const gchar          *base_uri);

gboolean       dh_util_uri_is_relative       (const gchar          *uri);

#endif /* __DH_UTIL_H__ */
