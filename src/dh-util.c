/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1999 Free Software Foundation
 * Copyright (C) 2000, 2001 Eazel, Inc.
 * Copyright (C) 2001 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004 Imendio AB
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
 */

#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <libgnome/gnome-init.h>
#include <gtk/gtklabel.h>
#include "dh-util.h"

#define d(x)

static gchar *dot_dir = NULL;

static void        tagify_bold_labels (GladeXML    *xml);
static GladeXML *  get_glade_file     (const gchar *filename,
				       const gchar *root,
				       const gchar *domain,
				       const gchar *first_required_widget,
				       va_list      args);

static void
tagify_bold_labels (GladeXML *xml)
{
        const gchar *str;
        gchar       *s;
        GtkWidget   *label;
        GList       *labels, *l;
 
        labels = glade_xml_get_widget_prefix (xml, "boldlabel");
 
        for (l = labels; l; l = l->next) {
                label = l->data;
 
                if (!GTK_IS_LABEL (label)) {
                        g_warning ("Not a label, check your glade file.");
                        continue;
                }
  
                str = gtk_label_get_text (GTK_LABEL (label));
 
                s = g_strdup_printf ("<b>%s</b>", str);
                gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
                gtk_label_set_label (GTK_LABEL (label), s);
                g_free (s);
        }
 
        g_list_free (labels);
}

static GladeXML *
get_glade_file (const gchar *filename,
                const gchar *root,
                const gchar *domain,
                const gchar *first_required_widget,
		va_list args)
{
        GladeXML   *gui;
        const char *name;
        GtkWidget **widget_ptr;
 
        gui = glade_xml_new (filename, root, domain);
        if (!gui) {
                g_warning ("Couldn't find necessary glade file '%s'", filename);                return NULL;
        }
 
        for (name = first_required_widget; name; name = va_arg (args, char *)) {                widget_ptr = va_arg (args, void *);
                 
                *widget_ptr = glade_xml_get_widget (gui, name);
                 
                if (!*widget_ptr) {
                        g_warning ("Glade file '%s' is missing widget '%s'.",
                                   filename, name);
                        continue;
                }
        }
 
        tagify_bold_labels (gui);
         
        return gui;
}

const gchar *
dh_dot_dir (void)
{
	if (!dot_dir) {
		dot_dir = g_build_filename (g_get_home_dir (),
					    GNOME_DOT_GNOME,
					    "devhelp",
					    NULL);
	}

	return dot_dir;
}


GladeXML *
dh_glade_get_file (const gchar *filename,
		   const gchar *root,
		   const gchar *domain,
		   const gchar *first_required_widget,
		   ...)
{
        va_list   args;
        GladeXML *gui;
 
        va_start (args, first_required_widget);
 
        gui = get_glade_file (filename,
                              root,
                              domain,
                              first_required_widget,
                              args);
                                                                                
        va_end (args);
                                                                                
        if (!gui) {
                return NULL;
        }
                                                                                
        return gui;
}

void
dh_glade_connect (GladeXML *gui,
		  gpointer  user_data,
		  gchar    *first_widget,
		  ...)
{
        va_list      args;
        const gchar *name;
        const gchar *signal;
        GtkWidget   *widget;
        gpointer    *callback;
                                                                                
        va_start (args, first_widget);
                                                                                
        for (name = first_widget; name; name = va_arg (args, char *)) {
                signal = va_arg (args, void *);
                callback = va_arg (args, void *);
                                                                                
                widget = glade_xml_get_widget (gui, name);
                if (!widget) {
                        g_warning ("Glade file is missing widget '%s', aborting",
                                   name);
                        continue;
                }
                                                                                
                g_signal_connect (widget,
                                  signal,
                                  G_CALLBACK (callback),
                                  user_data);
        }
                                                                                
        va_end (args);
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

gchar *
dh_util_uri_relative_new (const gchar *uri, const gchar *base_uri)
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


