/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2008 Sven Herzberg
 *
 * Devhelp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Devhelp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Devhelp.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DH_ASSISTANT_VIEW_H
#define DH_ASSISTANT_VIEW_H

#include <webkit2/webkit2.h>
#include <devhelp/dh-link.h>

G_BEGIN_DECLS

#define DH_TYPE_ASSISTANT_VIEW         (dh_assistant_view_get_type ())
#define DH_ASSISTANT_VIEW(i)           (G_TYPE_CHECK_INSTANCE_CAST ((i), DH_TYPE_ASSISTANT_VIEW, DhAssistantView))
#define DH_ASSISTANT_VIEW_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), DH_TYPE_ASSISTANT_VIEW, DhAssistantViewClass))
#define DH_IS_ASSISTANT_VIEW(i)        (G_TYPE_CHECK_INSTANCE_TYPE ((i), DH_TYPE_ASSISTANT_VIEW))
#define DH_IS_ASSISTANT_VIEW_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), DH_ASSISTANT_VIEW))
#define DH_ASSISTANT_VIEW_GET_CLASS(i) (G_TYPE_INSTANCE_GET_CLASS ((i), DH_TYPE_ASSISTANT_VIEW, DhAssistantView))

typedef struct _DhAssistantView      DhAssistantView;
typedef struct _DhAssistantViewClass DhAssistantViewClass;

struct _DhAssistantView {
        WebKitWebView parent_instance;
};

struct _DhAssistantViewClass {
        WebKitWebViewClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_assistant_view_get_type              (void) G_GNUC_CONST;

GtkWidget *     dh_assistant_view_new                   (void);

gboolean        dh_assistant_view_set_link              (DhAssistantView *view,
                                                         DhLink          *link);

gboolean        dh_assistant_view_search                (DhAssistantView *view,
                                                         const gchar     *str);

G_END_DECLS

#endif /* DH_ASSISTANT_VIEW_H */
