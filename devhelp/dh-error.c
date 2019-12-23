/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2002 CodeFactory AB
 * SPDX-FileCopyrightText: 2002 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dh-error.h"

GQuark
_dh_error_quark (void)
{
        static GQuark quark = 0;

        if (quark == 0)
                quark = g_quark_from_static_string ("libdevhelp-error-quark");

        return quark;
}
