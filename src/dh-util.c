/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include "dh-util.h"

#define d(x)

gchar *
dh_util_url_split (const gchar *url, gchar **anchor)
{
	gchar   *ch;
	gchar   *base;
	guint    len;
	gchar   *local_anchor = NULL;

	g_return_val_if_fail (url != NULL, NULL);
	
	if ((ch = strchr (url, '#'))) {
		len = ch - url;
		base = g_strndup (url, len);
		
		local_anchor = g_strdup (ch);
	} else {
		base = g_strdup (url);
	}
	
	if (anchor) {
		*anchor = local_anchor;
		d(g_print ("Anchor %s\n", *anchor));
	}
		
	return base;
}

gchar *
dh_util_url_get_book_name (const gchar *url)
{
	gchar   **dirs;
	gchar   **dir;
	gchar    *name = NULL;
	
	dirs = g_strsplit (url, "/", 4);
	
	dir = dirs;

	while (*dir && !strcmp (*dir, "..") || !strcmp (*dir, ".")) {
		++dir;
	};

	if (*dir) {
		name = g_strdup (*dir);
	} 
	
	g_strfreev (dirs);

	return name;
}

gint
dh_util_url_get_un_depth (const gchar *url)
{
	const gchar   *ch;
	gint           un_depth;

	g_return_val_if_fail (url != NULL, 0);

	ch       = url;
	un_depth = 0;
	
	while (ch = strstr (ch, URL_DELIM)) {
		un_depth++;
		ch += URL_DELIM_LENGTH;
	}

	return un_depth;
}

gchar *
dh_util_url_get_anchor (const gchar *url) 
{
	gchar   *ch;
	
	if ((ch = strchr (url, '#'))) {
		return g_strdup (ch);
	}

	return NULL;
}

gchar *
dh_util_uri_get_anchor (const GnomeVFSURI *uri)
{
	gchar   *str_uri;
	gchar   *anchor;
	
	str_uri = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	anchor = dh_util_url_get_anchor (str_uri);

	g_free (str_uri);
	
	return anchor;
}

/* ----------------------------------------------------------------- */
/*                          From GNOME VFS                           */
/* ----------------------------------------------------------------- */
static void
remove_internal_relative_components (char *uri_current)
{
	char *segment_prev, *segment_cur;
	size_t len_prev, len_cur;

	len_prev = len_cur = 0;
	segment_prev = NULL;

	segment_cur = uri_current;

	while (*segment_cur) {
		len_cur = strcspn (segment_cur, "/");

		if (len_cur == 1 && segment_cur[0] == '.') {
			/* Remove "." 's */
			if (segment_cur[1] == '\0') {
				segment_cur[0] = '\0';
				break;
			} else {
				memmove (segment_cur, segment_cur + 2, strlen (segment_cur + 2) + 1);
				continue;
			}
		} else if (len_cur == 2 && segment_cur[0] == '.' && segment_cur[1] == '.' ) {
			/* Remove ".."'s (and the component to the left of it) that aren't at the
			 * beginning or to the right of other ..'s
			 */
			if (segment_prev) {
				if (! (len_prev == 2
				       && segment_prev[0] == '.'
				       && segment_prev[1] == '.')) {
				       	if (segment_cur[2] == '\0') {
						segment_prev[0] = '\0';
						break;
				       	} else {
						memmove (segment_prev, segment_cur + 3, strlen (segment_cur + 3) + 1);

						segment_cur = segment_prev;
						len_cur = len_prev;

						/* now we find the previous segment_prev */
						if (segment_prev == uri_current) {
							segment_prev = NULL;
						} else if (segment_prev - uri_current >= 2) {
							segment_prev -= 2;
							for ( ; segment_prev > uri_current && segment_prev[0] != '/' 
							      ; segment_prev-- );
							if (segment_prev[0] == '/') {
								segment_prev++;
							}
						}
						continue;
					}
				}
			}
		}

		/*Forward to next segment */

		if (segment_cur [len_cur] == '\0') {
			break;
		}
		 
		segment_prev = segment_cur;
		len_prev = len_cur;
		segment_cur += len_cur + 1;	
	}
	
}

gboolean
dh_util_uri_is_relative (const char *uri)
{
	const char *current;

	/* RFC 2396 section 3.1 */
	for (current = uri ; 
		*current
		&& 	((*current >= 'a' && *current <= 'z')
			 || (*current >= 'A' && *current <= 'Z')
			 || (*current >= '0' && *current <= '9')
			 || ('-' == *current)
			 || ('+' == *current)
			 || ('.' == *current)) ;
	     current++);

	return  !(':' == *current);
}

/* If I had known this relative uri code would have ended up this long, I would
 * have done it a different way
 */
static char *
make_full_uri_from_relative (const char *base_uri, const char *uri)
{
	char *result = NULL;

	g_return_val_if_fail (base_uri != NULL, g_strdup (uri));
	g_return_val_if_fail (uri != NULL, NULL);

	/* See section 5.2 in RFC 2396 */

	/* FIXME bugzilla.eazel.com 4413: This function does not take
	 * into account a BASE tag in an HTML document, so its
	 * functionality differs from what Mozilla itself would do.
	 */

	if (dh_util_uri_is_relative (uri)) {
		char *mutable_base_uri;
		char *mutable_uri;

		char *uri_current;
		size_t base_uri_length;
		char *separator;

		/* We may need one extra character
		 * to append a "/" to uri's that have no "/"
		 * (such as help:)
		 */

		mutable_base_uri = g_malloc(strlen(base_uri)+2);
		strcpy (mutable_base_uri, base_uri);
		
		uri_current = mutable_uri = g_strdup (uri);

		/* Chew off Fragment and Query from the base_url */

		separator = strrchr (mutable_base_uri, '#'); 

		if (separator) {
			*separator = '\0';
		}

		separator = strrchr (mutable_base_uri, '?');

		if (separator) {
			*separator = '\0';
		}

		if ('/' == uri_current[0] && '/' == uri_current [1]) {
			/* Relative URI's beginning with the authority
			 * component inherit only the scheme from their parents
			 */

			separator = strchr (mutable_base_uri, ':');

			if (separator) {
				separator[1] = '\0';
			}			  
		} else if ('/' == uri_current[0]) {
			/* Relative URI's beginning with '/' absolute-path based
			 * at the root of the base uri
			 */

			separator = strchr (mutable_base_uri, ':');

			/* g_assert (separator), really */
			if (separator) {
				/* If we start with //, skip past the authority section */
				if ('/' == separator[1] && '/' == separator[2]) {
					separator = strchr (separator + 3, '/');
					if (separator) {
						separator[0] = '\0';
					}
				} else {
				/* If there's no //, just assume the scheme is the root */
					separator[1] = '\0';
				}
			}
		} else if ('#' != uri_current[0]) {
			/* Handle the ".." convention for relative uri's */

			/* If there's a trailing '/' on base_url, treat base_url
			 * as a directory path.
			 * Otherwise, treat it as a file path, and chop off the filename
			 */

			base_uri_length = strlen (mutable_base_uri);
			if ('/' == mutable_base_uri[base_uri_length-1]) {
				/* Trim off '/' for the operation below */
				mutable_base_uri[base_uri_length-1] = 0;
			} else {
				separator = strrchr (mutable_base_uri, '/');
				if (separator) {
					*separator = '\0';
				}
			}

			remove_internal_relative_components (uri_current);

			/* handle the "../"'s at the beginning of the relative URI */
			while (0 == strncmp ("../", uri_current, 3)) {
				uri_current += 3;
				separator = strrchr (mutable_base_uri, '/');
				if (separator) {
					*separator = '\0';
				} else {
					/* <shrug> */
					break;
				}
			}

			/* handle a ".." at the end */
			if (uri_current[0] == '.' && uri_current[1] == '.' 
			    && uri_current[2] == '\0') {

			    	uri_current += 2;
				separator = strrchr (mutable_base_uri, '/');
				if (separator) {
					*separator = '\0';
				}
			}

			/* Re-append the '/' */
			mutable_base_uri [strlen(mutable_base_uri)+1] = '\0';
			mutable_base_uri [strlen(mutable_base_uri)] = '/';
		}

		result = g_strconcat (mutable_base_uri, uri_current, NULL);
		g_free (mutable_base_uri); 
		g_free (mutable_uri); 

	} else {
		result = g_strdup (uri);
	}
	
	return result;
}

/**
 * gnome_vfs_uri_relative_new:
 * @text_uri: A string representing a URI.
 * @base: The base URI.
 * 
 * Create a new URI from @text_uri relative to @base.
 *
 * Return value: The new URI.
 **/
GnomeVFSURI *
dh_util_uri_relative_new (const gchar         *text_uri,
		       const GnomeVFSURI   *base)
{
	char *text_base;
	char *text_new;
	GnomeVFSURI *uri;

	text_base = gnome_vfs_uri_to_string (base, 0);
	text_new  = make_full_uri_from_relative (text_base, text_uri);

	uri = gnome_vfs_uri_new (text_new);

	g_free (text_base);
	g_free (text_new);

	return uri;
}
