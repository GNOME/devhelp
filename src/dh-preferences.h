/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * SPDX-FileCopyrightText: 2004-2008 Imendio AB
 * SPDX-FileCopyrightText: 2010 Lanedo GmbH
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_PREFERENCES_H
#define DH_PREFERENCES_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DH_TYPE_PREFERENCES                (dh_preferences_get_type ())
#define DH_PREFERENCES(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_PREFERENCES, DhPreferences))
#define DH_PREFERENCES_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_PREFERENCES, DhPreferencesClass))
#define DH_IS_PREFERENCES(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_PREFERENCES))
#define DH_IS_PREFERENCES_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_PREFERENCES))
#define DH_PREFERENCES_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_PREFERENCES, DhPreferencesClass))

typedef struct _DhPreferences             DhPreferences;
typedef struct _DhPreferencesClass        DhPreferencesClass;

struct _DhPreferences {
        GtkDialog parent;
};

struct _DhPreferencesClass {
        GtkDialogClass parent_class;
};

GType   dh_preferences_get_type         (void);

void    dh_preferences_show_dialog      (GtkWindow *parent);

G_END_DECLS

#endif /* DH_PREFERENCES_H */
