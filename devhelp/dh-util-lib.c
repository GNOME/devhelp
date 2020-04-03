/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2001 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2004, 2008 Imendio AB
 * SPDX-FileCopyrightText: 2015, 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
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

static void
sidebar_link_selected_cb (DhSidebar  *sidebar,
                          DhLink     *link,
                          DhNotebook *notebook)
{
        gchar *uri;
        DhWebView *web_view;

        uri = dh_link_get_uri (link);
        if (uri == NULL)
                return;

        web_view = dh_notebook_get_active_web_view (notebook);
        if (web_view != NULL)
                webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), uri);

        g_free (uri);
}

static void
sync_active_web_view_uri_to_sidebar (DhNotebook *notebook,
                                     DhSidebar  *sidebar)
{
        DhWebView *web_view;
        const gchar *uri = NULL;

        g_signal_handlers_block_by_func (sidebar,
                                         sidebar_link_selected_cb,
                                         notebook);

        web_view = dh_notebook_get_active_web_view (notebook);
        if (web_view != NULL)
                uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW (web_view));
        if (uri != NULL)
                dh_sidebar_select_uri (sidebar, uri);

        g_signal_handlers_unblock_by_func (sidebar,
                                           sidebar_link_selected_cb,
                                           notebook);
}

static DhNotebook *
get_notebook_containing_web_view (DhWebView *web_view)
{
        GtkWidget *widget;

        widget = GTK_WIDGET (web_view);

        while (widget != NULL) {
                widget = gtk_widget_get_parent (widget);

                if (DH_IS_NOTEBOOK (widget))
                        return DH_NOTEBOOK (widget);
        }

        g_return_val_if_reached (NULL);
}

static void
web_view_load_changed_cb (DhWebView       *web_view,
                          WebKitLoadEvent  load_event,
                          DhSidebar       *sidebar)
{
        DhNotebook *notebook;

        notebook = get_notebook_containing_web_view (web_view);

        if (load_event == WEBKIT_LOAD_COMMITTED &&
            web_view == dh_notebook_get_active_web_view (notebook)) {
                sync_active_web_view_uri_to_sidebar (notebook, sidebar);
        }
}

static void
notebook_page_added_after_cb (GtkNotebook *notebook,
                              GtkWidget   *child,
                              guint        page_num,
                              DhSidebar   *sidebar)
{
        DhTab *tab;
        DhWebView *web_view;

        g_return_if_fail (DH_IS_TAB (child));

        tab = DH_TAB (child);
        web_view = dh_tab_get_web_view (tab);

        g_signal_connect_object (web_view,
                                 "load-changed",
                                 G_CALLBACK (web_view_load_changed_cb),
                                 sidebar,
                                 0);
}

static void
notebook_switch_page_after_cb (DhNotebook *notebook,
                               GtkWidget  *new_page,
                               guint       new_page_num,
                               DhSidebar  *sidebar)
{
        sync_active_web_view_uri_to_sidebar (notebook, sidebar);
}

void
_dh_util_bind_sidebar_and_notebook (DhSidebar  *sidebar,
                                    DhNotebook *notebook)
{
        g_return_if_fail (DH_IS_SIDEBAR (sidebar));
        g_return_if_fail (DH_IS_NOTEBOOK (notebook));
        g_return_if_fail (dh_notebook_get_active_tab (notebook) == NULL);

        g_signal_connect_object (sidebar,
                                 "link-selected",
                                 G_CALLBACK (sidebar_link_selected_cb),
                                 notebook,
                                 0);

        g_signal_connect_object (notebook,
                                 "page-added",
                                 G_CALLBACK (notebook_page_added_after_cb),
                                 sidebar,
                                 G_CONNECT_AFTER);

        g_signal_connect_object (notebook,
                                 "switch-page",
                                 G_CALLBACK (notebook_switch_page_after_cb),
                                 sidebar,
                                 G_CONNECT_AFTER);
}
