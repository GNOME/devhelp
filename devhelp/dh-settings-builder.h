/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
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
