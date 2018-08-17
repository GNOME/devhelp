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

#include "dh-completion.h"
#include <string.h>

/**
 * SECTION:dh-completion
 * @Title: DhCompletion
 * @Short_description: Support for automatic string completion
 *
 * #DhCompletion is a basic replacement for #GCompletion. #GCompletion (part of
 * GLib) is deprecated.
 *
 * #GCompletion is implemented with a simple #GList, while #DhCompletion uses a
 * (sorted) #GSequence instead (or a #GList of #DhCompletion objects with
 * dh_completion_aggregate_complete()). So #DhCompletion should scale better
 * with more data, and #DhCompletion should be more appropriate if the same data
 * is used several times (on the other hand if the data is used only once,
 * #GCompletion should be faster).
 *
 * #DhCompletion works only with UTF-8 strings, and copies the strings.
 *
 * #DhCompletion is add-only, strings cannot be removed. But with
 * dh_completion_aggregate_complete(), a #DhCompletion object can be removed
 * from the #GList.
 */

typedef struct {
        /* Element types: gchar*, owned. */
        GSequence *sequence;
} DhCompletionPrivate;

typedef struct {
        const gchar *prefix;
        gsize prefix_bytes_length;
        gchar *longest_prefix;
} CompletionData;

G_DEFINE_TYPE_WITH_PRIVATE (DhCompletion, dh_completion, G_TYPE_OBJECT)

static gint
compare_func (gconstpointer a,
              gconstpointer b,
              gpointer      user_data)
{
        const gchar *str_a = a;
        const gchar *str_b = b;

        /* We rely on the fact that if str_a is not equal to str_b but one is
         * the prefix of the other, the shorter string is sorted before the
         * longer one (i.e. the shorter string is "less than" the longer
         * string). See do_complete().
         */

        return g_strcmp0 (str_a, str_b);
}

static void
completion_data_init (CompletionData *data,
                      const gchar    *prefix)
{
        data->prefix = prefix;
        data->prefix_bytes_length = strlen (prefix);
        data->longest_prefix = NULL;
}

static void
dh_completion_finalize (GObject *object)
{
        DhCompletion *completion = DH_COMPLETION (object);
        DhCompletionPrivate *priv = dh_completion_get_instance_private (completion);

        g_sequence_free (priv->sequence);

        G_OBJECT_CLASS (dh_completion_parent_class)->finalize (object);
}

static void
dh_completion_class_init (DhCompletionClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = dh_completion_finalize;
}

static void
dh_completion_init (DhCompletion *completion)
{
        DhCompletionPrivate *priv = dh_completion_get_instance_private (completion);
        priv->sequence = g_sequence_new (g_free);
}

/**
 * dh_completion_new:
 *
 * Returns: a new #DhCompletion object.
 * Since: 3.28
 */
DhCompletion *
dh_completion_new (void)
{
        return g_object_new (DH_TYPE_COMPLETION, NULL);
}

/**
 * dh_completion_add_string:
 * @completion: a #DhCompletion.
 * @str: a string.
 *
 * Adds a string to the @completion object.
 *
 * After adding all the strings you need to call dh_completion_sort().
 *
 * Since: 3.28
 */
void
dh_completion_add_string (DhCompletion *completion,
                          const gchar  *str)
{
        DhCompletionPrivate *priv = dh_completion_get_instance_private (completion);

        g_return_if_fail (DH_IS_COMPLETION (completion));
        g_return_if_fail (str != NULL);

        g_sequence_append (priv->sequence, g_strdup (str));
}

/**
 * dh_completion_sort:
 * @completion: a #DhCompletion.
 *
 * Sorts all the strings. It is required to call this function after adding
 * strings with dh_completion_add_string().
 *
 * Since: 3.28
 */
void
dh_completion_sort (DhCompletion *completion)
{
        DhCompletionPrivate *priv = dh_completion_get_instance_private (completion);

        g_return_if_fail (DH_IS_COMPLETION (completion));

        g_sequence_sort (priv->sequence,
                         compare_func,
                         NULL);
}

static gboolean
bytes_equal (const gchar *str1_start_pos,
             const gchar *str1_end_pos,
             const gchar *str2_start_pos,
             const gchar *str2_end_pos)
{
        const gchar *str1_pos;
        const gchar *str2_pos;

        for (str1_pos = str1_start_pos, str2_pos = str2_start_pos;
             str1_pos < str1_end_pos && str2_pos < str2_end_pos;
             str1_pos++, str2_pos++) {
                if (*str1_pos != *str2_pos)
                        return FALSE;
        }

        return str1_pos == str1_end_pos && str2_pos == str2_end_pos;
}

/* cur_str must have data->prefix as prefix. */
static void
adjust_longest_prefix (CompletionData *data,
                       const gchar    *cur_str)
{
        const gchar *cur_str_pos;
        gchar *longest_prefix_pos;

        /* Skip the first bytes, they are equal. */
        cur_str_pos = cur_str + data->prefix_bytes_length;
        longest_prefix_pos = data->longest_prefix + data->prefix_bytes_length;

        while (*cur_str_pos != '\0' && *longest_prefix_pos != '\0') {
                const gchar *cur_str_next_pos;
                gchar *longest_prefix_next_pos;

                cur_str_next_pos = g_utf8_find_next_char (cur_str_pos, NULL);
                longest_prefix_next_pos = g_utf8_find_next_char (longest_prefix_pos, NULL);

                if (!bytes_equal (cur_str_pos, cur_str_next_pos,
                                  longest_prefix_pos, longest_prefix_next_pos)) {
                        break;
                }

                cur_str_pos = cur_str_next_pos;
                longest_prefix_pos = longest_prefix_next_pos;
        }

        if (*longest_prefix_pos != '\0') {
                /* Shrink data->longest_prefix. */
                *longest_prefix_pos = '\0';
        }
}

/* Returns TRUE to continue the iteration.
 * cur_str must have data->prefix as prefix.
 */
static gboolean
next_completion_iteration (CompletionData *data,
                           const gchar    *cur_str)
{
        if (cur_str == NULL)
                return TRUE;

        if (data->longest_prefix == NULL) {
                data->longest_prefix = g_strdup (cur_str);
                /* After this point, data->longest_prefix can only shrink. */
        } else {
                adjust_longest_prefix (data, cur_str);
        }

        /* Back to data->prefix, stop the iteration, the longest_prefix can no
         * longer shrink.
         */
        if (g_str_equal (data->longest_prefix, data->prefix)) {
                g_free (data->longest_prefix);
                data->longest_prefix = NULL;
                return FALSE;
        }

        return TRUE;
}

/* Like dh_completion_complete() but with @found_string_with_prefix in
 * addition, to differentiate two different cases when %NULL is returned.
 *
 * Another implementation solution: instead of returning (NULL +
 * found_string_with_prefix=TRUE), return a string equal to @prefix. But it
 * would be harder to document (because it's less explicit) and less convenient
 * to use as a public API (I think for a public API we don't want as a return
 * value a string equal to @prefix).
 */
static gchar *
do_complete (DhCompletion *completion,
             const gchar  *prefix,
             gboolean     *found_string_with_prefix)
{
        DhCompletionPrivate *priv = dh_completion_get_instance_private (completion);
        GSequenceIter *iter;
        CompletionData data;

        if (found_string_with_prefix != NULL)
                *found_string_with_prefix = FALSE;

        g_return_val_if_fail (DH_IS_COMPLETION (completion), NULL);
        g_return_val_if_fail (prefix != NULL, NULL);

        iter = g_sequence_search (priv->sequence,
                                  (gpointer) prefix,
                                  compare_func,
                                  NULL);

        /* There can be an exact match just *before* iter, since compare_func
         * returns 0 in that case.
         */
        if (!g_sequence_iter_is_begin (iter)) {
                GSequenceIter *prev_iter;
                const gchar *prev_str;

                prev_iter = g_sequence_iter_prev (iter);
                prev_str = g_sequence_get (prev_iter);

                /* If there is an exact match, the prefix can not be completed. */
                if (g_str_equal (prev_str, prefix)) {
                        if (found_string_with_prefix != NULL)
                                *found_string_with_prefix = TRUE;
                        return NULL;
                }
        }

        completion_data_init (&data, prefix);

        /* All the other strings in the GSequence that have @prefix as prefix
         * will be *after* iter, see the comment in compare_func().
         */
        while (!g_sequence_iter_is_end (iter)) {
                const gchar *cur_str = g_sequence_get (iter);

                if (!g_str_has_prefix (cur_str, prefix))
                        break;

                if (found_string_with_prefix != NULL)
                        *found_string_with_prefix = TRUE;

                if (!next_completion_iteration (&data, cur_str))
                        break;

                iter = g_sequence_iter_next (iter);
        }

        return data.longest_prefix;
}

/**
 * dh_completion_complete:
 * @completion: a #DhCompletion.
 * @prefix: the string to complete.
 *
 * This function does the equivalent of:
 * 1. Searches the data structure of @completion to find all strings that have
 *    @prefix as prefix.
 * 2. From the list found at step 1, find the longest prefix that still matches
 *    all the strings in the list.
 *
 * This function assumes that @prefix and the strings contained in @completion
 * are in UTF-8. If all the strings are valid UTF-8, then the return value will
 * also be valid UTF-8 (it won't return a partial multi-byte character).
 *
 * Returns: (transfer full) (nullable): the completed prefix, or %NULL if a
 * longer prefix has not been found. Free with g_free() when no longer needed.
 * Since: 3.28
 */
gchar *
dh_completion_complete (DhCompletion *completion,
                        const gchar  *prefix)
{
        return do_complete (completion, prefix, NULL);
}

/**
 * dh_completion_aggregate_complete:
 * @completion_objects: (element-type DhCompletion) (nullable): a #GList of
 *   #DhCompletion objects.
 * @prefix: the string to complete.
 *
 * The same as dh_completion_complete(), but aggregated for several
 * #DhCompletion objects.
 *
 * Returns: (transfer full) (nullable): the completed prefix, or %NULL if a
 * longer prefix has not been found. Free with g_free() when no longer needed.
 * Since: 3.28
 */
gchar *
dh_completion_aggregate_complete (GList       *completion_objects,
                                  const gchar *prefix)
{
        CompletionData data;
        GList *l;

        g_return_val_if_fail (prefix != NULL, NULL);

        completion_data_init (&data, prefix);

        for (l = completion_objects; l != NULL; l = l->next) {
                DhCompletion *cur_completion = DH_COMPLETION (l->data);
                gchar *cur_longest_prefix;
                gboolean found_string_with_prefix;

                cur_longest_prefix = do_complete (cur_completion,
                                                  prefix,
                                                  &found_string_with_prefix);

                if (cur_longest_prefix == NULL && found_string_with_prefix) {
                        /* Stop the completion, it is not possible to complete
                         * @prefix.
                         */
                        g_free (data.longest_prefix);
                        return NULL;
                }

                if (!next_completion_iteration (&data, cur_longest_prefix)) {
                        g_free (cur_longest_prefix);
                        break;
                }

                g_free (cur_longest_prefix);
        }

        return data.longest_prefix;
}
