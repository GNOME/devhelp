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

#ifndef __DH_HISTORY_H__
#define __DH_HISTORY_H__

#include <glib-object.h>
#include "book.h"

#define DH_TYPE_HISTORY        (dh_history_get_type ())
#define HISTORY(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), DH_TYPE_HISTORY, DhHistory))
#define DH_HISTORY_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), DH_TYPE_HISTORY, DhHistoryClass))
#define DH_IS_HISTORY(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), DH_TYPE_HISTORY))
#define DH_IS_HISTORY_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), DH_TYPE_HISTORY))

typedef struct _DhHistory      DhHistory;
typedef struct _DhHistoryClass DhHistoryClass;
typedef struct _DhHistoryPriv  DhHistoryPriv;

struct _DhHistory {
	GObject         parent;
	
	DhHistoryPriv      *priv;
};

struct _DhHistoryClass 
{
	GObjectClass    parent_class;

	/* Signals */
	void   (*forward_exists_changed)     (DhHistory    *history,
					      gboolean      exists);
	void   (*back_exists_changed)        (DhHistory    *history,
					      gboolean      exists);
};

GType            dh_history_get_type      (void);
DhHistory *      dh_history_new           (void);

void             dh_history_goto          (DhHistory             *history,
					   const gchar           *str);

gchar *          dh_history_go_forward    (DhHistory             *history);

gchar *          dh_history_go_back       (DhHistory             *history);

gchar *          dh_history_get_current   (DhHistory             *history);

gboolean         dh_history_exist_forward (DhHistory             *history);
gboolean         dh_history_exist_back    (DhHistory             *history);

#endif /* __DH_HISTORY_H__ */
