/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2018 Sébastien Wilmet <swilmet@gnome.org>
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

#include "dh-search-context.h"
#include <string.h>

/* DhSearchContext is a helper class for a search instance, with the search
 * string as data.
 */

struct _DhSearchContext {
        /* The content of the search string: */

        gchar *book_id;
        gchar *page_id;

        // If non-NULL, contains at least one non-empty string.
        GStrv keywords;

        /* Derived data: */

        // Element-type: KeywordData*.
        GSList *keywords_data;

        gchar *joined_keywords;

        guint case_sensitive : 1;
};

typedef struct _KeywordData {
        gchar *keyword;

        /* Created only if has_glob and is_first. */
        GPatternSpec *pattern_spec_prefix;

        /* Created only if has_glob. */
        GPatternSpec *pattern_spec_anywhere;

        guint is_first : 1;
        guint has_glob : 1;
} KeywordData;

/* Process the input search string and extract:
 * - If "book:" prefix given, a book_id;
 * - If "page:" prefix given, a page_id;
 * - All remaining keywords.
 *
 * "book:" and "page:" must be before the other keywords.
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
                        /* Must be before normal keywords. */
                        if (keyword_num > 0) {
                                ret = FALSE;
                                goto out;
                        }

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
                        /* Must be before normal keywords. */
                        if (keyword_num > 0) {
                                ret = FALSE;
                                goto out;
                        }

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

static KeywordData *
keyword_data_new (const gchar *keyword,
                  gboolean     is_first)
{
        KeywordData *data;

        g_assert (keyword != NULL);

        data = g_new0 (KeywordData, 1);

        data->keyword = g_strdup (keyword);
        data->is_first = is_first != FALSE;
        data->has_glob = (strchr (keyword, '*') != NULL ||
                          strchr (keyword, '?') != NULL);

        if (data->has_glob) {
                gchar *pattern;

                if (is_first) {
                        pattern = g_strdup_printf ("%s*", keyword);
                        data->pattern_spec_prefix = g_pattern_spec_new (pattern);
                        g_free (pattern);
                }

                pattern = g_strdup_printf ("*%s*", keyword);
                data->pattern_spec_anywhere = g_pattern_spec_new (pattern);
                g_free (pattern);
        }

        return data;
}

static void
keyword_data_free (gpointer _data)
{
        KeywordData *data = _data;

        if (data == NULL)
                return;

        g_free (data->keyword);

        if (data->pattern_spec_prefix != NULL)
                g_pattern_spec_free (data->pattern_spec_prefix);

        if (data->pattern_spec_anywhere != NULL)
                g_pattern_spec_free (data->pattern_spec_anywhere);

        g_free (data);
}

static void
create_keywords_data (DhSearchContext *search)
{
        gint keyword_num;

        g_assert (search->keywords_data == NULL);

        if (search->keywords == NULL)
                return;

        for (keyword_num = 0; search->keywords[keyword_num] != NULL; keyword_num++) {
                const gchar *cur_keyword = search->keywords[keyword_num];
                KeywordData *data;

                data = keyword_data_new (cur_keyword, keyword_num == 0);
                search->keywords_data = g_slist_prepend (search->keywords_data, data);
        }

        search->keywords_data = g_slist_reverse (search->keywords_data);
}

static void
join_keywords (DhSearchContext *search)
{
        g_assert (search->joined_keywords == NULL);

        if (search->keywords == NULL)
                return;

        search->joined_keywords = g_strjoinv (" ", search->keywords);
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
        create_keywords_data (search);
        join_keywords (search);

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
        g_slist_free_full (search->keywords_data, keyword_data_free);
        g_free (search->joined_keywords);

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

gboolean
_dh_search_context_match_book (DhSearchContext *search,
                               DhBook          *book)
{
        g_return_val_if_fail (search != NULL, FALSE);
        g_return_val_if_fail (DH_IS_BOOK (book), FALSE);

        if (!dh_book_get_enabled (book))
                return FALSE;

        if (search->book_id == NULL)
                return TRUE;

        return g_strcmp0 (search->book_id, dh_book_get_id (book)) == 0;
}

/* This function assumes that _dh_search_context_match_book() returns TRUE for
 * the DhBook containing @link (to avoid checking the book_id for each DhLink).
 */
gboolean
_dh_search_context_match_link (DhSearchContext *search,
                               DhLink          *link,
                               gboolean         prefix)
{
        gchar *str_to_free = NULL;
        const gchar *link_name;
        gboolean match = FALSE;
        GSList *l;

        g_return_val_if_fail (search != NULL, FALSE);
        g_return_val_if_fail (link != NULL, FALSE);

        /* Filter by page? */
        if (search->page_id != NULL) {
                if (!dh_link_belongs_to_page (link, search->page_id))
                        return FALSE;

                if (search->keywords == NULL)
                        /* Show all in the page, but only if prefix=TRUE, to not
                         * match two times the same link.
                         */
                        return prefix;
        }

        if (search->keywords == NULL)
                return FALSE;

        if (search->case_sensitive) {
                link_name = dh_link_get_name (link);
        } else {
                str_to_free = g_ascii_strdown (dh_link_get_name (link), -1);
                link_name = str_to_free;
        }

        g_return_val_if_fail (link_name != NULL, FALSE);

        /* Why isn't there only one GPatternSpec (or two variants:
         * prefix/anywhere) for all the keywords? For example searching
         * "dh_link_ book" (two keywords) would create the GPatternSpec
         * "dh_link_*book*". Although the implementation would be simpler, doing
         * so would be a regression in functionality. It is explained in details
         * in the user documentation of the Devhelp app.
         */

        /* Why matching by prefix only for the first keyword and not the others?
         * For several reasons:
         * - When prefix=TRUE, if data->pattern_spec_prefix was used for all
         *   keywords, it would be impossible to match the DhLink name (except
         *   if all the keywords are equal for example, but it doesn't make
         *   sense to do such a search).
         * - At least with the GTK+/GNOME APIs, normally all the symbols start
         *   with the namespace of the library. So when we search symbols, if we
         *   know in which library the symbol(s) is located, we can type the
         *   namespace as first keyword. With prefix=TRUE, this will match the
         *   namespace.
         */

        /* Use simple string functions when the keyword doesn't contain globs,
         * to improve performances (this function can be called on *every*
         * DhLink).
         */

        for (l = search->keywords_data; l != NULL; l = l->next) {
                KeywordData *data = l->data;

                if (data->is_first) {
                        if (data->has_glob) {
                                if (prefix) {
                                        match = g_pattern_match_string (data->pattern_spec_prefix, link_name);
                                } else {
                                        match = (!g_pattern_match_string (data->pattern_spec_prefix, link_name) &&
                                                 g_pattern_match_string (data->pattern_spec_anywhere, link_name));
                                }
                        } else {
                                if (prefix) {
                                        match = g_str_has_prefix (link_name, data->keyword);
                                } else {
                                        match = (!g_str_has_prefix (link_name, data->keyword) &&
                                                 strstr (link_name, data->keyword) != NULL);
                                }
                        }
                } else {
                        if (data->has_glob) {
                                match = g_pattern_match_string (data->pattern_spec_anywhere, link_name);
                        } else {
                                match = strstr (link_name, data->keyword) != NULL;
                        }
                }

                if (!match)
                        break;
        }

        g_free (str_to_free);
        return match;
}

/* This function assumes:
 * - That _dh_search_context_match_book() returns TRUE for the DhBook containing
 *   @link (to avoid checking the book_id for each DhLink).
 * - That _dh_search_context_match_link(prefix=TRUE) returns TRUE for @link.
 */
gboolean
_dh_search_context_is_exact_link (DhSearchContext *search,
                                  DhLink          *link)
{
        const gchar *name;

        g_return_val_if_fail (search != NULL, FALSE);
        g_return_val_if_fail (link != NULL, FALSE);

        if (search->page_id != NULL && search->keywords == NULL) {
                DhLinkType link_type;

                link_type = dh_link_get_link_type (link);

                /* Can be DH_LINK_TYPE_BOOK for page_id "index". */
                return (link_type == DH_LINK_TYPE_BOOK ||
                        link_type == DH_LINK_TYPE_PAGE);
        }

        if (search->keywords == NULL)
                return FALSE;

        name = dh_link_get_name (link);
        return g_strcmp0 (name, search->joined_keywords) == 0;
}
