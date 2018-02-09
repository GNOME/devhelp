/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DH_COMPLETION_H
#define DH_COMPLETION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DH_TYPE_COMPLETION             (dh_completion_get_type ())
#define DH_COMPLETION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_COMPLETION, DhCompletion))
#define DH_COMPLETION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_COMPLETION, DhCompletionClass))
#define DH_IS_COMPLETION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_COMPLETION))
#define DH_IS_COMPLETION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_COMPLETION))
#define DH_COMPLETION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_COMPLETION, DhCompletionClass))

typedef struct _DhCompletion         DhCompletion;
typedef struct _DhCompletionClass    DhCompletionClass;
typedef struct _DhCompletionPrivate  DhCompletionPrivate;

struct _DhCompletion {
        GObject parent;

        DhCompletionPrivate *priv;
};

struct _DhCompletionClass {
        GObjectClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_completion_get_type                  (void);

DhCompletion *  dh_completion_new                       (void);

void            dh_completion_add_string                (DhCompletion *completion,
                                                         const gchar  *str);

void            dh_completion_sort                      (DhCompletion *completion);

gchar *         dh_completion_complete                  (DhCompletion *completion,
                                                         const gchar  *prefix);

gchar *         dh_completion_aggregate_complete        (GList       *completion_objects,
                                                         const gchar *prefix);

G_END_DECLS

#endif /* DH_COMPLETION_H */
