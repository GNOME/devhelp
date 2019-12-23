/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2002 CodeFactory AB
 * SPDX-FileCopyrightText: 2002 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_KEYWORD_MODEL_H
#define DH_KEYWORD_MODEL_H

#include <glib-object.h>
#include <devhelp/dh-link.h>
#include <devhelp/dh-profile.h>

G_BEGIN_DECLS

#define DH_TYPE_KEYWORD_MODEL            (dh_keyword_model_get_type ())
#define DH_KEYWORD_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_KEYWORD_MODEL, DhKeywordModel))
#define DH_KEYWORD_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_KEYWORD_MODEL, DhKeywordModelClass))
#define DH_IS_KEYWORD_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_KEYWORD_MODEL))
#define DH_IS_KEYWORD_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_KEYWORD_MODEL))
#define DH_KEYWORD_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_KEYWORD_MODEL, DhKeywordModelClass))

typedef struct _DhKeywordModel      DhKeywordModel;
typedef struct _DhKeywordModelClass DhKeywordModelClass;

struct _DhKeywordModel {
        GObject parent_instance;
};

struct _DhKeywordModelClass {
        GObjectClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

enum {
        DH_KEYWORD_MODEL_COL_NAME,
        DH_KEYWORD_MODEL_COL_LINK,
        DH_KEYWORD_MODEL_COL_CURRENT_BOOK_FLAG,
        DH_KEYWORD_MODEL_NUM_COLS
};

GType           dh_keyword_model_get_type  (void);

DhKeywordModel *dh_keyword_model_new       (void);

DhLink *        dh_keyword_model_filter    (DhKeywordModel *model,
                                            const gchar    *search_string,
                                            const gchar    *current_book_id,
                                            DhProfile      *profile);

G_END_DECLS

#endif /* DH_KEYWORD_MODEL_H */
