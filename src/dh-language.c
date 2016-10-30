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
#include "dh-language.h"
#include <string.h>

struct _DhLanguage {
        /* Name of the language */
        gchar *name;
        /* Number of books enabled in the language */
        gint   n_books_enabled;
};

/**
 * dh_language_free:
 * @language: a #DhLanguage object
 *
 * Free memory associated with the language.
 */
void
dh_language_free (DhLanguage *language)
{
        g_free (language->name);
        g_slice_free (DhLanguage, language);
}

/**
 * dh_language_new:
 * @name: the name of the language
 *
 * Create a new #DhLanguage object.
 *
 * Returns: a new #DhLanguage object
 */
DhLanguage *
dh_language_new (const gchar *name)
{
        DhLanguage *language;

        g_return_val_if_fail (name != NULL, NULL);

        language = g_slice_new0 (DhLanguage);
        language->name = g_strdup (name);

        return language;
}

/**
 * dh_language_compare:
 * @language_a: a #DhLanguage object
 * @language_b: the #DhLanguage object to compare with
 *
 * Compares the name of @language_a with the name @language_b.
 *
 * Returns: an integer less than, equal to, or greater than zero, if the name
 * of @language_a is <, == or > than the name of @language_b
 */
gint
dh_language_compare (const DhLanguage *language_a,
                     const DhLanguage *language_b)
{
        g_return_val_if_fail (language_a != NULL, -1);
        g_return_val_if_fail (language_b != NULL, -1);

        return strcmp (language_a->name, language_b->name);
}

/**
 * dh_language_compare_by_name:
 * @language_a: a #DhLanguage object
 * @language_name_b: the language name to compare with
 *
 * Compares the name of @language_a with @language_name_b.
 *
 * Returns: an integer less than, equal to, or greater than zero, if the name
 * of @language_a is <, == or > than @language_name_b
 */
gint
dh_language_compare_by_name (const DhLanguage *language_a,
                             const gchar      *language_name_b)
{
        g_return_val_if_fail (language_a != NULL, -1);
        g_return_val_if_fail (language_name_b != NULL, -1);

        return strcmp (language_a->name, language_name_b);
}

/**
 * dh_language_get_name:
 * @language: a #DhLanguage object
 *
 * Get the language name.
 *
 * Returns: The name of the language
 */
const gchar *
dh_language_get_name (DhLanguage *language)
{
        g_return_val_if_fail (language != NULL, NULL);

        return language->name;
}

/**
 * dh_language_get_n_books_enabled:
 * @language: a #DhLanguage object
 *
 * Get the number of enabled books
 *
 * Returns: The number of enabled books
 */
gint
dh_language_get_n_books_enabled (DhLanguage *language)
{
        g_return_val_if_fail (language != NULL, -1);

        return language->n_books_enabled;
}

/**
 * dh_language_inc_n_books_enabled:
 * @language: a #DhLanguage object
 *
 * Increase the number of enabled books for this language.
 */
void
dh_language_inc_n_books_enabled (DhLanguage *language)
{
        g_return_if_fail (language != NULL);

        language->n_books_enabled++;
}

/**
 * dh_language_dec_n_books_enabled:
 * @language: a #DhLanguage object
 *
 * Decrease the number of enabled books for this language.
 *
 * Returns: %TRUE if the counter is decreased to zero, %FALSE otherwise.
 */
gboolean
dh_language_dec_n_books_enabled (DhLanguage *language)
{
        g_return_val_if_fail (language != NULL, FALSE);

        language->n_books_enabled--;
        return (language->n_books_enabled <= 0) ? TRUE : FALSE;
}
