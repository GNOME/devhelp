/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@codefactory.se>
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
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Mikael Hallendal <micke@codefactory.se>
 */

#include <config.h>

#include "dh-profile.h"

struct _DhProfile {
        gchar  *name;
        
        GSList *books;
};

/* Need to read $(home)/.devhelp-2/profiles.xml */


/* For now, return a profile containing the old hardcoded list */
DhProfile *
dh_profile_new (void)
{
	DhProfile *profile;
	
	profile = g_new0 (DhProfile, 1);

	
	
	return profile;
}

GNode *   
dh_profile_open (DhProfile *profile, GList *keyword, GError **error)
{
	GNode *root;
	GList *keywords;
	
	if (!dh_book_parser_read_books (profile->books,
					root,
					&keywords,
					error)) {
		return NULL;
	}
}

