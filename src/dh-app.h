/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
 * Copyright (C) 2017 Sébastien Wilmet <swilmet@gnome.org>
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

#ifndef DH_APP_H
#define DH_APP_H

#include <gtk/gtk.h>
#include "dh-window.h"

G_BEGIN_DECLS

#define DH_TYPE_APP         (dh_app_get_type ())
#define DH_APP(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DH_TYPE_APP, DhApp))
#define DH_APP_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), DH_TYPE_APP, DhAppClass))
#define DH_IS_APP(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DH_TYPE_APP))
#define DH_IS_APP_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), DH_TYPE_APP))
#define DH_APP_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DH_TYPE_APP, DhAppClass))

typedef struct _DhApp        DhApp;
typedef struct _DhAppClass   DhAppClass;
typedef struct _DhAppPrivate DhAppPrivate;

struct _DhApp {
        GtkApplication parent_instance;
        DhAppPrivate *priv;
};

struct _DhAppClass {
        GtkApplicationClass parent_class;
};

GType           dh_app_get_type                 (void) G_GNUC_CONST;

DhApp *         dh_app_new                      (void);

DhWindow *      dh_app_get_active_main_window   (DhApp    *app,
                                                 gboolean  create_if_none);

G_END_DECLS

#endif /* DH_APP_H */
