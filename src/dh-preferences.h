/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2004-2008 Imendio AB
 * Copyright (C) 2010 Lanedo GmbH
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

#ifndef DH_PREFERENCES_H
#define DH_PREFERENCES_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DH_TYPE_PREFERENCES                (dh_preferences_get_type ())
#define DH_PREFERENCES(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_PREFERENCES, DhPreferences))
#define DH_PREFERENCES_CONST(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_PREFERENCES, DhPreferences const))
#define DH_PREFERENCES_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_PREFERENCES, DhPreferencesClass))
#define DH_IS_PREFERENCES(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_PREFERENCES))
#define DH_IS_PREFERENCES_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_PREFERENCES))
#define DH_PREFERENCES_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_PREFERENCES, DhPreferencesClass))

typedef struct _DhPreferences             DhPreferences;
typedef struct _DhPreferencesClass        DhPreferencesClass;

struct _DhPreferences
{
        GtkDialog parent;
};

struct _DhPreferencesClass
{
        GtkDialogClass parent_class;
};

GType dh_preferences_get_type (void) G_GNUC_CONST;

void dh_preferences_show_dialog (GtkWindow *parent);

G_END_DECLS

#endif /* DH_PREFERENCES_H */
