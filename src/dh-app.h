/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
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
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __DH_APP_H__
#define __DH_APP_H__

#include <gtk/gtk.h>

#include "dh-book-manager.h"

G_BEGIN_DECLS

#define DH_TYPE_APP         (dh_app_get_type ())
#define DH_APP(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DH_TYPE_APP, DhApp))
#define DH_APP_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), DH_TYPE_APP, DhAppClass))
#define DH_IS_APP(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DH_TYPE_APP))
#define DH_IS_APP_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), DH_TYPE_APP))
#define DH_APP_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DH_TYPE_APP, DhAppClass))

typedef struct _DhApp        DhApp;
typedef struct _DhAppClass   DhAppClass;

struct _DhApp {
        GtkApplication parent_instance;
};

struct _DhAppClass {
        GtkApplicationClass parent_class;
};

GType dh_app_get_type (void) G_GNUC_CONST;

DhApp         *dh_app_new               (void);

DhBookManager *dh_app_peek_book_manager (DhApp *self);
GtkWindow     *dh_app_peek_first_window (DhApp *self);
GtkWindow     *dh_app_peek_assistant    (DhApp *self);

void           dh_app_new_window        (DhApp *self);
void           dh_app_quit              (DhApp *self);
void           dh_app_search            (DhApp *self,
                                         const gchar *keyword);
void           dh_app_search_assistant  (DhApp *self,
                                         const gchar *keyword);
void           dh_app_raise             (DhApp *self);

gboolean      _dh_app_has_app_menu      (DhApp *app);

G_END_DECLS

#endif /* __DH_APP_H__ */
