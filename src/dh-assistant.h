/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2008 Imendio AB
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_ASSISTANT_H
#define DH_ASSISTANT_H

#include <gtk/gtk.h>
#include "dh-app.h"

G_BEGIN_DECLS

#define DH_TYPE_ASSISTANT         (dh_assistant_get_type ())
#define DH_ASSISTANT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DH_TYPE_ASSISTANT, DhAssistant))
#define DH_ASSISTANT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), DH_TYPE_ASSISTANT, DhAssistantClass))
#define DH_IS_ASSISTANT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DH_TYPE_ASSISTANT))
#define DH_IS_ASSISTANT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), DH_TYPE_ASSISTANT))
#define DH_ASSISTANT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DH_TYPE_ASSISTANT, DhAssistantClass))

typedef struct _DhAssistant      DhAssistant;
typedef struct _DhAssistantClass DhAssistantClass;

struct _DhAssistant {
        GtkApplicationWindow parent_instance;
};

struct _DhAssistantClass {
        GtkApplicationWindowClass parent_class;
};

GType           dh_assistant_get_type   (void) G_GNUC_CONST;

DhAssistant *   dh_assistant_new        (DhApp *application);

gboolean        dh_assistant_search     (DhAssistant *assistant,
                                         const gchar *str);

G_END_DECLS

#endif /* DH_ASSISTANT_H */
