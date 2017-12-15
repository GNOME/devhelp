/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2010 Lanedo GmbH
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DH_LANGUAGE_H
#define DH_LANGUAGE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DH_TYPE_LANGUAGE                (dh_language_get_type ())
#define DH_LANGUAGE(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), DH_TYPE_LANGUAGE, DhLanguage))
#define DH_LANGUAGE_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), DH_TYPE_LANGUAGE, DhLanguageClass))
#define DH_IS_LANGUAGE(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DH_TYPE_LANGUAGE))
#define DH_IS_LANGUAGE_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), DH_TYPE_LANGUAGE))
#define DH_LANGUAGE_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), DH_TYPE_LANGUAGE, DhLanguageClass))

typedef struct _DhLanguage DhLanguage;
typedef struct _DhLanguageClass DhLanguageClass;

struct _DhLanguage {
        GObject parent_instance;
};

struct _DhLanguageClass {
        GObjectClass parent_class;
};

GType        dh_language_get_type            (void) G_GNUC_CONST;
DhLanguage  *dh_language_new                 (const gchar *name);
const gchar *dh_language_get_name            (DhLanguage *language);
gint         dh_language_compare             (DhLanguage *language_a,
                                              DhLanguage *language_b);
gint         dh_language_compare_by_name     (DhLanguage  *language_a,
                                              const gchar *language_name_b);
gint         dh_language_get_n_books_enabled (DhLanguage *language);
void         dh_language_inc_n_books_enabled (DhLanguage *language);
gboolean     dh_language_dec_n_books_enabled (DhLanguage *language);

G_END_DECLS

#endif /* DH_LANGUAGE_H */
