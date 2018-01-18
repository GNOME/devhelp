/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
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

#include "dh-search-context.h"
#include <string.h>

struct _DhSearchContext {
        gchar *book_id;
        gchar *page_id;
        GStrv keywords;
        guint case_sensitive : 1;
};

/* Process the input search string and extract:
 *  - If "book:" prefix given, a book_id;
 *  - If "page:" prefix given, a page_id;
 *  - All remaining keywords.
 *
 * Returns TRUE if the extraction is successfull, FALSE if the @search_string is
 * invalid.
 */
static gboolean
process_search_string (DhSearchContext *search,
                       const gchar     *search_string)
{
        gchar *processed = NULL;
        GStrv tokens = NULL;
        gint token_num;
        gint keyword_num;
        gboolean ret = TRUE;

        g_assert (search->book_id == NULL);
        g_assert (search->page_id == NULL);
        g_assert (search->keywords == NULL);

        /* First, remove all leading and trailing whitespaces in the search
         * string.
         */
        processed = g_strdup (search_string);
        g_strstrip (processed);

        /* Also avoid words being separated by more than one whitespace, or
         * g_strsplit() will give us empty strings.
         */
        {
                gchar *aux;

                aux = processed;
                while ((aux = strchr (aux, ' ')) != NULL) {
                        g_strchug (++aux);
                }
        }

        /* If after all this we get an empty string, nothing else to do. */
        if (processed[0] == '\0') {
                ret = FALSE;
                goto out;
        }

        /* Split the input string into tokens */
        tokens = g_strsplit (processed, " ", 0);

        /* Allocate output keywords */
        search->keywords = g_new0 (gchar *, g_strv_length (tokens) + 1);
        keyword_num = 0;

        for (token_num = 0; tokens[token_num] != NULL; token_num++) {
                const gchar *cur_token = tokens[token_num];
                const gchar *prefix;
                gint prefix_len;

                /* Book prefix? */
                prefix = "book:";
                if (g_str_has_prefix (cur_token, prefix)) {
                        prefix_len = strlen (prefix);

                        /* If keyword given but no content, skip it. */
                        if (cur_token[prefix_len] == '\0') {
                                continue;
                        }

                        /* We got a second request of book, don't allow this. */
                        if (search->book_id != NULL) {
                                ret = FALSE;
                                goto out;
                        }

                        search->book_id = g_strdup (cur_token + prefix_len);
                        continue;
                }

                /* Page prefix? */
                prefix = "page:";
                if (g_str_has_prefix (cur_token, prefix)) {
                        prefix_len = strlen (prefix);

                        /* If keyword given but no content, skip it. */
                        if (cur_token[prefix_len] == '\0') {
                                continue;
                        }

                        /* We got a second request of page, don't allow this. */
                        if (search->page_id != NULL) {
                                ret = FALSE;
                                goto out;
                        }

                        search->page_id = g_strdup (cur_token + prefix_len);
                        continue;
                }

                /* Then, a new keyword to look for. */
                search->keywords[keyword_num] = g_strdup (cur_token);
                keyword_num++;
        }

        if (keyword_num == 0) {
                g_free (search->keywords);
                search->keywords = NULL;
        }

out:
        g_free (processed);
        g_strfreev (tokens);
        return ret;
}

static gboolean
contains_uppercase_letter (const gchar *str)
{
        const gchar *p;

        for (p = str; *p != '\0'; p++) {
                if (g_ascii_isupper (*p))
                        return TRUE;
        }

        return FALSE;
}

static void
set_case_sensitive (DhSearchContext *search)
{
        gint i;

        search->case_sensitive = FALSE;

        if (search->keywords == NULL)
                return;

        /* Searches are case sensitive when any uppercase letter is used in the
         * search terms, matching Vim smartcase behaviour.
         */
        for (i = 0; search->keywords[i] != NULL; i++) {
                const gchar *cur_keyword = search->keywords[i];

                if (contains_uppercase_letter (cur_keyword)) {
                        search->case_sensitive = TRUE;
                        break;
                }
        }
}

/* Returns: (transfer full) (nullable): a new #DhSearchContext, or %NULL if
 * @search_string is invalid.
 */
DhSearchContext *
_dh_search_context_new (const gchar *search_string)
{
        DhSearchContext *search;

        g_return_val_if_fail (search_string != NULL, NULL);

        search = g_new0 (DhSearchContext, 1);

        if (!process_search_string (search, search_string)) {
                _dh_search_context_free (search);
                return NULL;
        }

        set_case_sensitive (search);

        return search;
}

void
_dh_search_context_free (DhSearchContext *search)
{
        if (search == NULL)
                return;

        g_free (search->book_id);
        g_free (search->page_id);
        g_strfreev (search->keywords);
        g_free (search);
}

const gchar *
_dh_search_context_get_book_id (DhSearchContext *search)
{
        g_return_val_if_fail (search != NULL, NULL);

        return search->book_id;
}

const gchar *
_dh_search_context_get_page_id (DhSearchContext *search)
{
        g_return_val_if_fail (search != NULL, NULL);

        return search->page_id;
}

GStrv
_dh_search_context_get_keywords (DhSearchContext *search)
{
        g_return_val_if_fail (search != NULL, NULL);

        return search->keywords;
}

gboolean
_dh_search_context_get_case_sensitive (DhSearchContext *search)
{
        g_return_val_if_fail (search != NULL, FALSE);

        return search->case_sensitive;
}
