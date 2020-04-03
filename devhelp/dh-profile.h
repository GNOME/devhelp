/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_PROFILE_H
#define DH_PROFILE_H

#include <glib-object.h>
#include <devhelp/dh-book-list.h>
#include <devhelp/dh-settings.h>

G_BEGIN_DECLS

#define DH_TYPE_PROFILE             (dh_profile_get_type ())
#define DH_PROFILE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_PROFILE, DhProfile))
#define DH_PROFILE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_PROFILE, DhProfileClass))
#define DH_IS_PROFILE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_PROFILE))
#define DH_IS_PROFILE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_PROFILE))
#define DH_PROFILE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_PROFILE, DhProfileClass))

typedef struct _DhProfile         DhProfile;
typedef struct _DhProfileClass    DhProfileClass;
typedef struct _DhProfilePrivate  DhProfilePrivate;

struct _DhProfile {
        GObject parent;

        DhProfilePrivate *priv;
};

struct _DhProfileClass {
        GObjectClass parent_class;

        /* Padding for future expansion */
        gpointer padding[12];
};

GType           dh_profile_get_type             (void);

G_GNUC_INTERNAL
DhProfile *     _dh_profile_new                 (DhSettings *settings,
                                                 DhBookList *book_list);

DhProfile *     dh_profile_get_default          (void);

G_GNUC_INTERNAL
void            _dh_profile_unref_default       (void);

DhSettings *    dh_profile_get_settings         (DhProfile *profile);

DhBookList *    dh_profile_get_book_list        (DhProfile *profile);

G_END_DECLS

#endif /* DH_PROFILE_H */
