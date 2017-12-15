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

#include "config.h"
#include "dh-language.h"
#include <string.h>

typedef struct {
        /* Name of the language */
        gchar *name;

        /* Number of books enabled in the language */
        gint   n_books_enabled;
} DhLanguagePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DhLanguage, dh_language, G_TYPE_OBJECT)

static void
dh_language_finalize (GObject *object)
{
        DhLanguagePrivate *priv = dh_language_get_instance_private (DH_LANGUAGE (object));

        g_free (priv->name);

        G_OBJECT_CLASS (dh_language_parent_class)->finalize (object);
}

static void
dh_language_class_init (DhLanguageClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = dh_language_finalize;
}

static void
dh_language_init (DhLanguage *language)
{
}

DhLanguage *
dh_language_new (const gchar *name)
{
        DhLanguage *language;
        DhLanguagePrivate *priv;

        g_return_val_if_fail (name != NULL, NULL);

        language = g_object_new (DH_TYPE_LANGUAGE, NULL);

        priv = dh_language_get_instance_private (language);
        priv->name = g_strdup (name);

        return language;
}

gint
dh_language_compare (DhLanguage *language_a,
                     DhLanguage *language_b)
{
        g_return_val_if_fail (language_a != NULL, -1);
        g_return_val_if_fail (language_b != NULL, -1);

        return strcmp (dh_language_get_name (language_a),
                       dh_language_get_name (language_b));
}

gint
dh_language_compare_by_name (DhLanguage  *language_a,
                             const gchar *language_name_b)
{
        g_return_val_if_fail (language_a != NULL, -1);
        g_return_val_if_fail (language_name_b != NULL, -1);

        return strcmp (dh_language_get_name (language_a),
                       language_name_b);
}

const gchar *
dh_language_get_name (DhLanguage *language)
{
        DhLanguagePrivate *priv;

        g_return_val_if_fail (language != NULL, NULL);

        priv = dh_language_get_instance_private (language);
        return priv->name;
}

gint
dh_language_get_n_books_enabled (DhLanguage *language)
{
        DhLanguagePrivate *priv;

        g_return_val_if_fail (language != NULL, -1);

        priv = dh_language_get_instance_private (language);
        return priv->n_books_enabled;
}

void
dh_language_inc_n_books_enabled (DhLanguage *language)
{
        DhLanguagePrivate *priv;

        g_return_if_fail (language != NULL);

        priv = dh_language_get_instance_private (language);
        priv->n_books_enabled++;
}

gboolean
dh_language_dec_n_books_enabled (DhLanguage *language)
{
        DhLanguagePrivate *priv;

        g_return_val_if_fail (language != NULL, FALSE);

        priv = dh_language_get_instance_private (language);
        priv->n_books_enabled--;
        return (priv->n_books_enabled <= 0) ? TRUE : FALSE;
}
