/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
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
 *
 * Author: Mikael Hallendal <micke@codefactory.se>
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

#define URL_DELIM "../"
#define URL_DELIM_LENGTH 3

gchar *        util_url_split             (const gchar          *url,
					   gchar               **anchor);
gchar *        util_url_get_book_name     (const gchar          *url);

gint           util_url_get_un_depth      (const gchar          *url);

gchar *        util_url_get_anchor        (const gchar          *url);

gchar *        util_uri_get_anchor        (const GnomeVFSURI    *uri);

/* Taken from gnome-vfs CVS. */
GnomeVFSURI *  util_uri_relative_new      (const gchar          *text_uri,
					   const GnomeVFSURI    *base);

gboolean       util_uri_is_relative       (const gchar          *uri);


#endif /* GNOME_VFS_URI_H */
