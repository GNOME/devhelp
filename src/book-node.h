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

#ifndef __BOOK_NODE_H__
#define __BOOK_NODE_H__

#include <libgnomevfs/gnome-vfs.h>

typedef struct _Document    Document;
typedef struct _BookNode    BookNode;

const gchar *       book_node_get_title        (const BookNode       *node);
BookNode *          book_node_get_parent       (const BookNode       *node);
GSList *            book_node_get_contents     (const BookNode       *node);

gboolean            book_node_is_chapter       (const BookNode       *node);
 
GnomeVFSURI *       book_node_get_uri          (const BookNode       *node);

Document *          book_node_get_document     (const BookNode       *node);

const gchar *       book_node_get_anchor       (const BookNode       *node);

GnomeVFSURI *       document_get_uri           (const Document       *document,
                                                const gchar          *anchor);

#endif /* __BOOK_NODE_H__ */

