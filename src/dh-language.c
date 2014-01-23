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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <string.h>

#include "dh-language.h"

struct _DhLanguage {
        /* Name of the language */
        gchar *name;
        /* Number of books enabled in the language */
        gint   n_books_enabled;
};

void
dh_language_free (DhLanguage *language)
{
        g_free (language->name);
        g_slice_free (DhLanguage, language);
}

DhLanguage *
dh_language_new (const gchar *name)
{
        DhLanguage *language;

        g_return_val_if_fail (name != NULL, NULL);

        language = g_slice_new0 (DhLanguage);
        language->name = g_strdup (name);

        return language;
}

gint
dh_language_compare (const DhLanguage *language_a,
                     const DhLanguage *language_b)
{
        g_return_val_if_fail (language_a != NULL, -1);
        g_return_val_if_fail (language_b != NULL, -1);

        return strcmp (language_a->name, language_b->name);
}

gint
dh_language_compare_by_name (const DhLanguage *language_a,
                             const gchar      *language_name_b)
{
        g_return_val_if_fail (language_a != NULL, -1);
        g_return_val_if_fail (language_name_b != NULL, -1);

        return strcmp (language_a->name, language_name_b);
}

const gchar *
dh_language_get_name (DhLanguage *language)
{
        g_return_val_if_fail (language != NULL, NULL);

        return language->name;
}


gint
dh_language_get_n_books_enabled (DhLanguage *language)
{
        g_return_val_if_fail (language != NULL, -1);

        return language->n_books_enabled;
}

void
dh_language_inc_n_books_enabled (DhLanguage *language)
{
        g_return_if_fail (language != NULL);

        language->n_books_enabled++;
}

gboolean
dh_language_dec_n_books_enabled (DhLanguage *language)
{
        g_return_val_if_fail (language != NULL, FALSE);

        language->n_books_enabled--;
        return (language->n_books_enabled <= 0) ? TRUE : FALSE;
}
