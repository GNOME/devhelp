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

#ifndef DH_SETTINGS_BUILDER_H
#define DH_SETTINGS_BUILDER_H

#include <glib-object.h>
#include <devhelp/dh-settings.h>

G_BEGIN_DECLS

#define DH_TYPE_SETTINGS_BUILDER             (dh_settings_builder_get_type ())
#define DH_SETTINGS_BUILDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_SETTINGS_BUILDER, DhSettingsBuilder))
#define DH_SETTINGS_BUILDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_SETTINGS_BUILDER, DhSettingsBuilderClass))
#define DH_IS_SETTINGS_BUILDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_SETTINGS_BUILDER))
#define DH_IS_SETTINGS_BUILDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_SETTINGS_BUILDER))
#define DH_SETTINGS_BUILDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_SETTINGS_BUILDER, DhSettingsBuilderClass))

typedef struct _DhSettingsBuilder         DhSettingsBuilder;
typedef struct _DhSettingsBuilderClass    DhSettingsBuilderClass;
typedef struct _DhSettingsBuilderPrivate  DhSettingsBuilderPrivate;

struct _DhSettingsBuilder {
        GObject parent;

        DhSettingsBuilderPrivate *priv;
};

struct _DhSettingsBuilderClass {
        GObjectClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_settings_builder_get_type            (void);

DhSettingsBuilder *
                dh_settings_builder_new                 (void);

void            dh_settings_builder_set_contents_path   (DhSettingsBuilder *builder,
                                                         const gchar       *contents_path);

void            dh_settings_builder_set_fonts_path      (DhSettingsBuilder *builder,
                                                         const gchar       *fonts_path);

DhSettings *    dh_settings_builder_create_object       (DhSettingsBuilder *builder);

G_END_DECLS

#endif /* DH_SETTINGS_BUILDER_H */
