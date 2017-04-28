/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEVHELP_H
#define DEVHELP_H

#include <glib.h>

/* Explicitly include all the exported headers */
#include "dh-assistant.h"
#include "dh-assistant-view.h"
#include "dh-book-manager.h"
#include "dh-language.h"
#include "dh-book.h"
#include "dh-book-tree.h"
#include "dh-error.h"
#include "dh-keyword-model.h"
#include "dh-link.h"
#include "dh-sidebar.h"
#include "dh-window.h"

G_BEGIN_DECLS

void dh_init (void);

G_END_DECLS

#endif /* DEVHELP_H */
