/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
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

#ifndef DH_PROFILE_BUILDER_H
#define DH_PROFILE_BUILDER_H

#include <glib-object.h>
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

DhProfile *             dh_profile_builder_create_object        (DhProfileBuilder *builder);

G_END_DECLS

#endif /* DH_PROFILE_BUILDER_H */
