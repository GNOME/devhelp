/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 CodeFactory AB
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

#ifndef __HISTORY_H__
#define __HISTORY_H__

#include <gtk/gtkobject.h>
#include <gtk/gtktypeutils.h>
#include <libgnomevfs/gnome-vfs.h>
#include "book.h"
#define TYPE_HISTORY        (history_get_type ())
#define HISTORY(o)          (GTK_CHECK_CAST ((o), TYPE_HISTORY, History))
#define HISTORY_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), TYPE_HISTORY, HistoryClass))
#define IS_HISTORY(o)       (GTK_CHECK_TYPE ((o), TYPE_HISTORY))
#define IS_HISTORY_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), TYPE_HISTORY))

typedef struct _History      History;
typedef struct _HistoryClass HistoryClass;
typedef struct _HistoryPriv  HistoryPriv;

struct _History {
	GtkObject         parent;
	
	HistoryPriv      *priv;
};

struct _HistoryClass 
{
	GtkObjectClass    parent_class;

	/* Signals */
	void   (*forward_exists_changed)     (History    *history,
					      gboolean    exists);
	void   (*back_exists_changed)        (History    *history,
					      gboolean    exists);
};

GtkType               history_get_type      (void);
History *             history_new           (void);

void                  history_goto          (History             *history,
					     const Document      *document,
					     const gchar         *anchor);

const Document *      history_go_forward    (History             *history,
					     gchar              **anchor);

const Document *      history_go_back       (History             *history,
					     gchar              **anchor);

const Document *      history_get_current   (History             *history,
					     gchar              **anchor);

gboolean              history_exist_forward (History             *history);
gboolean              history_exist_back    (History             *history);

#endif /* __HISTORY_H__ */
