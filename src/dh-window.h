/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2002 CodeFactory AB
 * SPDX-FileCopyrightText: 2001-2002 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2005 Imendio AB
 * SPDX-FileCopyrightText: 2017-2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_WINDOW_H
#define DH_WINDOW_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DH_TYPE_WINDOW         (dh_window_get_type ())
#define DH_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DH_TYPE_WINDOW, DhWindow))
#define DH_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), DH_TYPE_WINDOW, DhWindowClass))
#define DH_IS_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DH_TYPE_WINDOW))
#define DH_IS_WINDOW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), DH_TYPE_WINDOW))
#define DH_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DH_TYPE_WINDOW, DhWindowClass))

typedef struct _DhWindow       DhWindow;
typedef struct _DhWindowClass  DhWindowClass;

struct _DhWindow {
        GtkApplicationWindow parent_instance;
};

struct _DhWindowClass {
        GtkApplicationWindowClass parent_class;
};

GType           dh_window_get_type              (void) G_GNUC_CONST;

GtkWidget *     dh_window_new                   (GtkApplication *application);

void            dh_window_search                (DhWindow    *window,
                                                 const gchar *str);

void            _dh_window_display_uri          (DhWindow    *window,
                                                 const gchar *uri);

G_END_DECLS

#endif /* DH_WINDOW_H */
