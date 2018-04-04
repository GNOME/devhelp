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

#include "dh-profile.h"
#include "dh-profile-builder.h"

/**
 * SECTION:dh-profile
 * @Title: DhProfile
 * @Short_description: libdevhelp profile
 */

struct _DhProfilePrivate {
        DhSettings *settings;
};

static DhProfile *default_instance = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (DhProfile, dh_profile, G_TYPE_OBJECT)

static void
dh_profile_dispose (GObject *object)
{
        DhProfile *profile = DH_PROFILE (object);

        g_clear_object (&profile->priv->settings);

        G_OBJECT_CLASS (dh_profile_parent_class)->dispose (object);
}

static void
dh_profile_finalize (GObject *object)
{
        if (default_instance == DH_PROFILE (object))
                default_instance = NULL;

        G_OBJECT_CLASS (dh_profile_parent_class)->finalize (object);
}

static void
dh_profile_class_init (DhProfileClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = dh_profile_dispose;
        object_class->finalize = dh_profile_finalize;
}

static void
dh_profile_init (DhProfile *profile)
{
        profile->priv = dh_profile_get_instance_private (profile);
}

DhProfile *
_dh_profile_new (DhSettings *settings)
{
        DhProfile *profile;

        g_return_val_if_fail (DH_IS_SETTINGS (settings), NULL);

        profile = g_object_new (DH_TYPE_PROFILE, NULL);
        profile->priv->settings = g_object_ref (settings);

        return profile;
}

/**
 * dh_profile_get_default:
 *
 * Gets the default #DhProfile object. It has the default #DhSettings object as
 * returned by dh_settings_get_default().
 *
 * Returns: (transfer none): the default #DhProfile object.
 * Since: 3.30
 */
DhProfile *
dh_profile_get_default (void)
{
        if (default_instance == NULL) {
                DhProfileBuilder *builder;

                builder = dh_profile_builder_new ();
                default_instance = dh_profile_builder_create_object (builder);
                g_object_unref (builder);
        }

        return default_instance;
}

void
_dh_profile_unref_default (void)
{
        if (default_instance != NULL)
                g_object_unref (default_instance);

        /* default_instance is not set to NULL here, it is set to NULL in
         * dh_profile_finalize() (i.e. when we are sure that the ref count
         * reaches 0).
         */
}

/**
 * dh_profile_get_settings:
 * @profile: a #DhProfile.
 *
 * Returns: (transfer none): the #DhSettings of @profile.
 * Since: 3.30
 */
DhSettings *
dh_profile_get_settings (DhProfile *profile)
{
        g_return_val_if_fail (DH_IS_PROFILE (profile), NULL);

        return profile->priv->settings;
}
