/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <rhult@codefactory.se>
 * Copyright (C) 2002 Mikael Hallendal <micke@codefactory.se>
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

#ifndef __DH_INDEX_MODEL_H__
#define __DH_INDEX_MODEL_H__

#include <glib-object.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreesortable.h>

#define DH_TYPE_INDEX_MODEL	          (dh_index_model_get_type ())
#define DH_INDEX_MODEL(obj)	          (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_INDEX_MODEL, DhIndexModel))
#define DH_INDEX_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_INDEX_MODEL, DhIndexModelClass))
#define DH_IS_INDEX_MODEL(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_INDEX_MODEL))
#define DH_IS_INDEX_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_INDEX_MODEL))
#define DH_INDEX_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_INDEX_MODEL, DhIndexModelClass))

typedef struct _DhIndexModel       DhIndexModel;
typedef struct _DhIndexModelClass  DhIndexModelClass;
typedef struct _DhIndexModelPriv   DhIndexModelPriv;

struct _DhIndexModel
{
        GObject           parent;

        DhIndexModelPriv *priv;

};

struct _DhIndexModelClass
{
	GObjectClass parent_class;
};

enum {
	DH_INDEX_MODEL_COL_NAME,
	DH_INDEX_MODEL_COL_SECTION,
	DH_INDEX_MODEL_NR_OF_COLS
};

GtkType          dh_index_model_get_type     (void);

DhIndexModel *   dh_index_model_new          (void);
void             dh_index_model_set_words    (DhIndexModel   *model,
					      GList          *index_words);

void             dh_index_model_filter       (DhIndexModel   *model,
					      const gchar    *string);

#endif /* __DH_INDEX_MODEL_H__ */
