/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_PROFILE_BUILDER_H
#define DH_PROFILE_BUILDER_H

#include <glib-object.h>
#include <devhelp/dh-book-list.h>
#include <devhelp/dh-profile.h>
#include <devhelp/dh-settings.h>

G_BEGIN_DECLS

#define DH_TYPE_PROFILE_BUILDER             (dh_profile_builder_get_type ())
#define DH_PROFILE_BUILDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_PROFILE_BUILDER, DhProfileBuilder))
#define DH_PROFILE_BUILDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_PROFILE_BUILDER, DhProfileBuilderClass))
#define DH_IS_PROFILE_BUILDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_PROFILE_BUILDER))
#define DH_IS_PROFILE_BUILDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_PROFILE_BUILDER))
#define DH_PROFILE_BUILDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_PROFILE_BUILDER, DhProfileBuilderClass))

typedef struct _DhProfileBuilder         DhProfileBuilder;
typedef struct _DhProfileBuilderClass    DhProfileBuilderClass;
typedef struct _DhProfileBuilderPrivate  DhProfileBuilderPrivate;

struct _DhProfileBuilder {
        GObject parent;

        DhProfileBuilderPrivate *priv;
};

struct _DhProfileBuilderClass {
        GObjectClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType                   dh_profile_builder_get_type             (void);

DhProfileBuilder *      dh_profile_builder_new                  (void);

void                    dh_profile_builder_set_settings         (DhProfileBuilder *builder,
                                                                 DhSettings       *settings);

void                    dh_profile_builder_set_book_list        (DhProfileBuilder *builder,
                                                                 DhBookList       *book_list);

DhProfile *             dh_profile_builder_create_object        (DhProfileBuilder *builder);

G_END_DECLS

#endif /* DH_PROFILE_BUILDER_H */
