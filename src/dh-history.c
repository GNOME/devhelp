/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Mikael Hallendal <micke@codefactory.se>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "dh-marshal.h"
#include "dh-history.h"

#define d(x)

#define HISTORY_LENGTH 20

static void          history_init              (DhHistory       *history);
static void          history_class_init        (GObjectClass    *klass);
static void          history_destroy           (GObject         *object);
static void          history_free_history_list (GList           *history_list);

static void          history_maybe_emit        (DhHistory       *history);
					
enum { 
	FORWARD_EXISTS_CHANGED,
	BACK_EXISTS_CHANGED, 
	LAST_SIGNAL 
}; 

static gint signals[LAST_SIGNAL] = { 0 };

struct _DhHistoryPriv {
	GList      *history_list;
	GList      *current;

	gboolean    last_emit_forward;
	gboolean    last_emit_back;
};

GType
dh_history_get_type (void)
{
	static GType history_type = 0;
        
	if (!history_type) {
		static const GTypeInfo history_info = {
			sizeof (DhHistoryClass),
			NULL,
			NULL,
			(GClassInitFunc) history_class_init,
			NULL,
			NULL,
			sizeof (DhHistory),
			0,
			(GInstanceInitFunc) history_init,
		};
                
		history_type = g_type_register_static (G_TYPE_OBJECT,
						       "DhHistory",
						       &history_info, 0);
	}

	return history_type;
}

static void
history_init (DhHistory *history)
{
	DhHistoryPriv   *priv;

	d(puts(__FUNCTION__));
        
	priv = g_new0 (DhHistoryPriv, 1);
        
	priv->history_list      = NULL;
	priv->current           = NULL;
	priv->last_emit_forward = FALSE;
	priv->last_emit_back    = FALSE;
	history->priv           = priv;
}

static void
history_class_init (GObjectClass *klass)
{
	d(puts(__FUNCTION__));

	klass->finalize = history_destroy;

	signals[FORWARD_EXISTS_CHANGED] =
		g_signal_new ("forward_exists_changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (DhHistoryClass, 
					       forward_exists_changed),
			      NULL, NULL,
			      dh_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
			      1, G_TYPE_BOOLEAN);

	signals[BACK_EXISTS_CHANGED] =
		g_signal_new ("back_exists_changed",
			      G_TYPE_FROM_CLASS (klass),				
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (DhHistoryClass, 
					       back_exists_changed),
			      NULL, NULL,
			      dh_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
			      1, G_TYPE_BOOLEAN);
}

static void
history_destroy (GObject *object)
{
	DhHistory       *history;
	DhHistoryPriv   *priv;
	GList         *node;
        
	g_return_if_fail (object != NULL);
	g_return_if_fail (DH_IS_HISTORY (object));
        
	d(puts(__FUNCTION__));
        
	history = HISTORY (object);
	priv    = history->priv;
        
	for (node = priv->history_list; node; node = node->next) {
		g_free (node->data);
	}

	g_list_free (priv->history_list);

	g_free (priv);

	history->priv = NULL;
}

static void
history_free_history_list (GList *history_list)
{
	GList   *node;
        
	d(puts(__FUNCTION__));
        
	for (node = history_list; node; node = node->next) {
		g_free (node->data);
	}

	g_list_free (history_list);
}

static void
history_maybe_emit (DhHistory *history)
{
	DhHistoryPriv   *priv;
		
	g_return_if_fail (history != NULL);
	g_return_if_fail (DH_IS_HISTORY (history));
	
	priv = history->priv;
	
	if (priv->last_emit_forward != dh_history_exist_forward (history)) {
		priv->last_emit_forward = dh_history_exist_forward (history);
		
		g_signal_emit (G_OBJECT (history),
			       signals[FORWARD_EXISTS_CHANGED],
			       0,
			       priv->last_emit_forward);
	}

	if (priv->last_emit_back != dh_history_exist_back (history)) {
		priv->last_emit_back = dh_history_exist_back (history);
		
		g_signal_emit (G_OBJECT (history),
			       signals[BACK_EXISTS_CHANGED],
			       0,
			       priv->last_emit_back);
	}
}

void
dh_history_goto (DhHistory *history, const gchar *str)
{
	DhHistoryPriv   *priv;
	GList         *forward_list;
	
	g_return_if_fail (history != NULL);
	g_return_if_fail (DH_IS_HISTORY (history));

	d(puts(__FUNCTION__));

	priv = history->priv;
	
	if (dh_history_exist_forward (history)) {
		forward_list = priv->current->next;
		priv->current->next = NULL;
			
		history_free_history_list (forward_list);
	}

 	priv->history_list = g_list_append (priv->history_list, 
					    g_strdup (str));
	
	priv->current      = g_list_last (priv->history_list);

	if (g_list_length (priv->history_list) > HISTORY_LENGTH) {
		GList *l = g_list_first (priv->history_list);
		g_free (l->data);
		priv->history_list = g_list_remove_link (priv->history_list, 
							 l);
		
		g_list_free (l);
	}	
	
	history_maybe_emit (history);
}

gchar *
dh_history_go_forward (DhHistory *history)
{
	DhHistoryPriv   *priv;
        
	g_return_val_if_fail (history != NULL, NULL);
	g_return_val_if_fail (DH_IS_HISTORY (history), NULL);

	d(puts(__FUNCTION__));
        
	priv = history->priv;
        
	if (priv->current->next) {
		priv->current = priv->current->next;

		history_maybe_emit (history);
		
		return g_strdup ((gchar *)priv->current->data);
	}

	return NULL;
}

gchar *
dh_history_go_back (DhHistory *history)
{
	DhHistoryPriv   *priv;
	
	g_return_val_if_fail (history != NULL, NULL);
	g_return_val_if_fail (DH_IS_HISTORY (history), NULL);

	d(puts(__FUNCTION__));
        
	priv = history->priv;
        
	if (priv->current->prev) {
		priv->current = priv->current->prev;

		history_maybe_emit (history);

		return g_strdup ((gchar *) priv->current->data);
	}
        
	return NULL;
}

gchar *
dh_history_get_current (DhHistory *history)
{
	DhHistoryPriv   *priv;
	
	g_return_val_if_fail (history != NULL, NULL);
	g_return_val_if_fail (DH_IS_HISTORY (history), NULL);

	priv = history->priv;
	
	if (!priv->current) {
		return NULL;
	}

	return g_strdup ((gchar *) priv->current->data);
}

gboolean
dh_history_exist_forward (DhHistory *history)
{
	DhHistoryPriv   *priv;
        
	g_return_val_if_fail (history != NULL, FALSE);
	g_return_val_if_fail (DH_IS_HISTORY (history), FALSE);
        
	d(puts(__FUNCTION__));
        
	priv = history->priv;
        
	if (!priv->current) {
		return FALSE;
	}

	if (priv->current->next) {
		return TRUE;
	}

	return FALSE;
}

gboolean
dh_history_exist_back (DhHistory *history)
{
	DhHistoryPriv   *priv;
        
	g_return_val_if_fail (history != NULL, FALSE);
	g_return_val_if_fail (DH_IS_HISTORY (history), FALSE);

	d(puts(__FUNCTION__));
        
	priv = history->priv;
        
	if (!priv->current) {
		return FALSE;
	}
        
	if (priv->current->prev) {
		return TRUE;
	}
        
	return FALSE;
}

DhHistory *
dh_history_new ()
{
	d(puts(__FUNCTION__));
        
	return g_object_new (DH_TYPE_HISTORY, NULL);
}

