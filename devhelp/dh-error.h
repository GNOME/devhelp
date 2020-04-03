/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2002 CodeFactory AB
 * SPDX-FileCopyrightText: 2002 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_ERROR_H
#define DH_ERROR_H

#include <glib.h>

G_BEGIN_DECLS

#define DH_ERROR _dh_error_quark ()

typedef enum {
        DH_ERROR_MALFORMED_BOOK
} DhError;

G_GNUC_INTERNAL
GQuark _dh_error_quark (void);

G_END_DECLS

#endif /* DH_ERROR_H */
