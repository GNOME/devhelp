/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dh-profile.h"
#include "dh-profile-builder.h"

/**
 * SECTION:dh-profile
 * @Title: DhProfile
 * @Short_description: libdevhelp profile
 *
 * #DhProfile permits to configure other libdevhelp objects. For example
 * #DhSidebar has the #DhSidebar:profile construct-only property. A #DhProfile
 * contains a #DhSettings object and a #DhBookList object. As a convention for
 * other libdevhelp classes that use #DhProfile, if the #DhProfile is not
 * provided (i.e. it is set to %NULL), then the default profile is used, see
 * dh_profile_get_default().
 *
 * There is the possibility to run in parallel multiple profiles in the same
 * process, for example:
 * - In an IDE for different projects or different programming languages.
 * - In different #GtkWindow's of the API browser application.
 *
 * With #DhSettings it's possible to share some #GSettings keys between
 * different profiles.
 *
 * A possible use-case is to have one "generic" profile, which corresponds to
 * the default profile as returned by dh_profile_get_default(). And another
 * profile tailored to a specific development platform (for example GNOME),
 * providing additional features useful for that development platform (for
 * example to download the latest API documentation, have a start page, etc).
 */

struct _DhProfilePrivate {
        DhSettings *settings;
        DhBookList *book_list;
};

static DhProfile *default_instance = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (DhProfile, dh_profile, G_TYPE_OBJECT)

static void
dh_profile_dispose (GObject *object)
{
        DhProfile *profile = DH_PROFILE (object);

        g_clear_object (&profile->priv->settings);
        g_clear_object (&profile->priv->book_list);

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
_dh_profile_new (DhSettings *settings,
                 DhBookList *book_list)
{
        DhProfile *profile;

        g_return_val_if_fail (DH_IS_SETTINGS (settings), NULL);
        g_return_val_if_fail (DH_IS_BOOK_LIST (book_list), NULL);

        profile = g_object_new (DH_TYPE_PROFILE, NULL);
        profile->priv->settings = g_object_ref (settings);
        profile->priv->book_list = g_object_ref (book_list);

        return profile;
}

/**
 * dh_profile_get_default:
 *
 * Gets the default #DhProfile object. It has the default #DhSettings object as
 * returned by dh_settings_get_default(), and the default #DhBookList object as
 * returned by dh_book_list_get_default().
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
 * Gets the #DhSettings object of @profile. The returned object is guaranteed to
 * be the same for the lifetime of @profile.
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

/**
 * dh_profile_get_book_list:
 * @profile: a #DhProfile.
 *
 * Gets the #DhBookList object of @profile. The returned object is guaranteed to
 * be the same for the lifetime of @profile.
 *
 * Returns: (transfer none): the #DhBookList of @profile.
 * Since: 3.30
 */
DhBookList *
dh_profile_get_book_list (DhProfile *profile)
{
        g_return_val_if_fail (DH_IS_PROFILE (profile), NULL);

        return profile->priv->book_list;
}
