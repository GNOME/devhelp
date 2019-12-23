/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DH_APPLICATION_WINDOW_H
#define DH_APPLICATION_WINDOW_H

#include <glib.h>
#include <devhelp/dh-notebook.h>
#include <devhelp/dh-sidebar.h>

G_BEGIN_DECLS

void    dh_application_window_bind_sidebar_and_notebook         (DhSidebar  *sidebar,
                                                                 DhNotebook *notebook);

G_END_DECLS

#endif /* DH_APPLICATION_WINDOW_H */
