/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@codefactory.se>
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
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Mikael Hallendal <micke@codefactory.se>
 */

#ifndef __DH_PROFILE_H__
#define __DH_PROFILE_H__

#include <glib-object.h>

typedef struct _DhProfile      DhProfile;
typedef struct _DhProfileClass DhProfileClass;
typedef struct _DhProfilePriv  DhProfilePriv;

#define DH_TYPE_PROFILE         (dh_profile_get_type ())
#define DH_PROFILE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DH_TYPE_PROFILE, DhProfile))
#define DH_PROFILE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), DH_TYPE_PROFILE, DhProfileClass))
#define DH_IS_PROFILE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DH_TYPE_PROFILE))
#define DH_IS_PROFILE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), DH_TYPE_PROFILE))
#define DH_PROFILE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DH_TYPE_PROFILE, DhProfileClass))


struct _DhProfile {
        GObject        parent;
        
        DhProfilePriv *priv;
};

struct _DhProfileClass {
        GObjectClass parent_class;
};

GType        dh_profile_get_type       (void);
DhProfile *  dh_profile_new            (void);
GNode *      dh_profile_open           (DhProfile     *profile,
					GList        **keywords,
					GError       **error);
GSList *     dh_profiles_init          (void);

#endif /* __DH_PROFILE_H__ */
