/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_SEARCH_CONTEXT_H
#define DH_SEARCH_CONTEXT_H

#include <glib.h>
#include "dh-book.h"
#include "dh-link.h"

G_BEGIN_DECLS

typedef struct _DhSearchContext DhSearchContext;

G_GNUC_INTERNAL
DhSearchContext *       _dh_search_context_new                  (const gchar *search_string);

G_GNUC_INTERNAL
void                    _dh_search_context_free                 (DhSearchContext *search);

G_GNUC_INTERNAL
const gchar *           _dh_search_context_get_book_id          (DhSearchContext *search);

G_GNUC_INTERNAL
const gchar *           _dh_search_context_get_page_id          (DhSearchContext *search);

G_GNUC_INTERNAL
GStrv                   _dh_search_context_get_keywords         (DhSearchContext *search);

G_GNUC_INTERNAL
gboolean                _dh_search_context_get_case_sensitive   (DhSearchContext *search);

G_GNUC_INTERNAL
gboolean                _dh_search_context_match_book           (DhSearchContext *search,
                                                                 DhBook          *book);

G_GNUC_INTERNAL
gboolean                _dh_search_context_match_link           (DhSearchContext *search,
                                                                 DhLink          *link,
                                                                 gboolean         prefix);

G_GNUC_INTERNAL
gboolean                _dh_search_context_is_exact_link        (DhSearchContext *search,
                                                                 DhLink          *link);

G_END_DECLS

#endif /* DH_SEARCH_CONTEXT_H */
