/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * This file is part of Devhelp.
 *
 * Copyright (C) 2001 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004, 2008 Imendio AB
 * Copyright (C) 2015, 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
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

#include "config.h"
#include "dh-util-lib.h"
#include "dh-link.h"

gchar *
_dh_util_build_data_filename (const gchar *first_part,
                              ...)
{
        gchar        *datadir = NULL;
        va_list       args;
        const gchar  *part;
        gchar       **strv;
        gint          i;
        gchar        *ret;

        va_start (args, first_part);

        if (datadir == NULL) {
                datadir = g_strdup (DATADIR);
        }

        /* 2 = 1 initial component + terminating NULL element. */
        strv = g_malloc (sizeof (gchar *) * 2);
        strv[0] = (gchar *) datadir;

        i = 1;
        for (part = first_part; part; part = va_arg (args, char *), i++) {
                /* +2 = 1 new element + terminating NULL element. */
                strv = g_realloc (strv, sizeof (gchar*) * (i + 2));
                strv[i] = (gchar *) part;
        }

        strv[i] = NULL;
        ret = g_build_filenamev (strv);
        g_free (strv);

        g_free (datadir);

        va_end (args);

        return ret;
}

/* We're only going to expect ASCII strings here, so there's no point in
 * playing with g_unichar_totitle() and such.
 * Note that we modify the string in place.
 */
void
_dh_util_ascii_strtitle (gchar *str)
{
        gboolean word_start;

        if (str == NULL)
                return;

        word_start = TRUE;
        while (*str != '\0') {
                if (g_ascii_isalpha (*str)) {
                        *str = (word_start ?
                                g_ascii_toupper (*str) :
                                g_ascii_tolower (*str));
                        word_start = FALSE;
                } else {
                        word_start = TRUE;
                }
                str++;
        }
}

gchar *
_dh_util_create_data_uri_for_filename (const gchar *filename,
                                       const gchar *mime_type)
{
        gchar *data;
        gsize  data_length;
        gchar *base64;
        gchar *uri;

        if (!g_file_get_contents (filename, &data, &data_length, NULL))
                return NULL;

        base64 = g_base64_encode ((const guchar *)data, data_length);
        g_free (data);

        uri = g_strdup_printf ("data:%s;charset=utf8;base64,%s", mime_type, base64);
        g_free(base64);

        return uri;
}

/* Adds q2 onto the end of q1, and frees q2. */
void
_dh_util_queue_concat (GQueue *q1,
                       GQueue *q2)
{
        g_return_if_fail (q1 != NULL);

        if (q2 == NULL)
                return;

        if (q1->head == NULL) {
                g_assert_cmpint (q1->length, ==, 0);
                g_assert (q1->tail == NULL);

                q1->head = q2->head;
                q1->tail = q2->tail;
                q1->length = q2->length;
        } else if (q2->head != NULL) {
                g_assert_cmpint (q1->length, >, 0);
                g_assert_cmpint (q2->length, >, 0);
                g_assert (q1->tail != NULL);
                g_assert (q2->tail != NULL);

                q1->tail->next = q2->head;
                q2->head->prev = q1->tail;

                q1->tail = q2->tail;
                q1->length += q2->length;
        } else {
                g_assert_cmpint (q2->length, ==, 0);
                g_assert (q2->tail == NULL);
        }

        q2->head = NULL;
        q2->tail = NULL;
        q2->length = 0;
        g_queue_free (q2);
}

static gboolean
unref_node_link (GNode    *node,
                 gpointer  data)
{
        dh_link_unref (node->data);
        return FALSE;
}

void
_dh_util_free_book_tree (GNode *book_tree)
{
        if (book_tree == NULL)
                return;

        g_node_traverse (book_tree,
                         G_IN_ORDER,
                         G_TRAVERSE_ALL,
                         -1,
                         unref_node_link,
                         NULL);

        g_node_destroy (book_tree);
}

/* Returns: (transfer full) (element-type GFile): the list of possible Devhelp
 * index files in @book_directory, in order of preference.
 */
GSList *
_dh_util_get_possible_index_files (GFile *book_directory)
{
        const gchar *extensions[] = {
                ".devhelp2",
                ".devhelp2.gz",
                ".devhelp",
                ".devhelp.gz",
                NULL
        };
        gchar *directory_name;
        GSList *list = NULL;
        gint i;

        g_return_val_if_fail (G_IS_FILE (book_directory), NULL);

        directory_name = g_file_get_basename (book_directory);
        g_return_val_if_fail (directory_name != NULL, NULL);

        for (i = 0; extensions[i] != NULL; i++) {
                const gchar *cur_extension = extensions[i];
                gchar *index_file_name;
                GFile *index_file;

                /* The name of the directory the index file is in and the name
                 * of the index file (minus the extension) must match.
                 */
                index_file_name = g_strconcat (directory_name, cur_extension, NULL);

                index_file = g_file_get_child (book_directory, index_file_name);
                list = g_slist_prepend (list, index_file);

                g_free (index_file_name);
        }

        list = g_slist_reverse (list);

        g_free (directory_name);
        return list;
}
