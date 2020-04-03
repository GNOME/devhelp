/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
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
