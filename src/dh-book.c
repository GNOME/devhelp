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
 */

#include <glib-object.h>
#include <libgnome/gnome-i18n.h>
#include <gsf/gsf-infile.h>
#include <gsf/gsf-infile-zip.h>
#include <gsf/gsf-input-memory.h>
#include <gsf/gsf-input-stdio.h>

#include "dh-error.h"
#include "dh-book-old.h"
#include "dh-book.h"

typedef enum {
        BOOK_TYPE_ZIP,
        BOOK_TYPE_SPEC,
        BOOK_TYPE_OLD,
        BOOK_TYPE_INVALID
} BookType;

static BookType  book_check_type      (const gchar     *uri);


static gboolean  book_read_zip        (GsfInput        *input, 
                                       DhBook          *book, 
                                       GNode           *contents, 
                                       GList          **keywords, 
                                       GError         **error);

static gboolean  book_read_metadata   (GsfInfile       *infile,
                                       DhBook          *book,
                                       GError         **error);

static gboolean  book_read_contents   (GsfInfile       *infile, 
                                       GNode           *contents,
                                       GError         **error);

static gboolean  book_read_keywords   (GsfInfile       *infile, 
                                       GNode           *contents,
                                       GError         **error);

static gboolean  book_read_spec       (GsfInput        *input, 
                                       DhBook          *book, 
                                       GNode           *contents, 
                                       GList          **keywords, 
                                       GError         **error);

static gboolean  book_read_old        (GsfInput        *input, 
                                       DhBook          *book, 
                                       GNode           *contents, 
                                       GList          **keywords, 
                                       GError         **error);

static BookType
book_check_type (const gchar *uri)
{
        gchar *ext;
        
        ext = strrchr (uri, '.');

        if (g_ascii_strcasecmp (ext, ".dhb") == 0) { /* Devhelp zip */
                return BOOK_TYPE_ZIP;
        }
        
        if (g_ascii_strcasecmp (ext, ".dhs") == 0) { /* Devhelp spec */
                return BOOK_TYPE_SPEC;
        }
        
        if (g_ascii_strcasecmp (ext, ".devhelp") == 0) { /* Old book format */
                return BOOK_TYPE_OLD;
        }
        
        return BOOK_TYPE_INVALID;
}

static gboolean
book_read_zip (GsfInput  *input, 
               DhBook    *book, 
               GNode     *contents, 
               GList    **keywords, 
               GError   **error) 
{
        GsfInfile *infile;
        
        infile = gsf_infile_zip_new (input, NULL);
                
        if (!infile) {
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_MALFORMED_BOOK,
                             _("'%s' is not a valid book"), 
                             gsf_input_name (input));
                g_object_unref (input);
                return FALSE;
        }
                
        if (book) {
                if (!book_read_metadata (infile, book, error)) {
                        g_set_error (error,
                                     DH_ERROR,
                                     DH_ERROR_MALFORMED_BOOK,
                                     _("Book '%s' is malformed."),
                                     gsf_input_name (input));
                        
                        return FALSE;
                }
        }

        if (contents) {
                if (!book_read_contents (infile, contents, error)) {
                        g_set_error (error,
                                     DH_ERROR,
                                     DH_ERROR_MALFORMED_BOOK,
                                     _("Book '%s' is malformed."),
                                     gsf_input_name (input));
                        return FALSE;
                }
        }
        
        if (keywords) {
                if (!book_read_keywords (infile, contents, error)) {
                        g_set_error (error,
                                     DH_ERROR,
                                     DH_ERROR_MALFORMED_BOOK,
                                     _("Book '%s' is malformed."),
                                     gsf_input_name (input));
                        return FALSE;
                }
        }

        return TRUE;
}
       
static gboolean
book_read_metadata (GsfInfile *infile, DhBook *book, GError **error)
{
        GsfInput *input;
        
        input = gsf_infile_child_by_name (infile, "metadata.xml");
        
        if (!input) {
                return FALSE;
        }
        
        return TRUE;
}

static gboolean
book_read_contents (GsfInfile *infile, GNode *contents, GError **error)
{
        return TRUE;
}

static gboolean
book_read_keywords (GsfInfile *infile, GNode *contents, GError **error)
{
        return TRUE;
}

static gboolean
book_read_spec (GsfInput  *input, 
                DhBook    *book, 
                GNode     *contents, 
                GList    **keywords, 
                GError   **error)
{
}

static gboolean
book_read_old (GsfInput  *input, 
               DhBook    *book, 
               GNode     *contents, 
               GList    **keywords, 
               GError   **error)
{
        return dh_book_old_read (input, book, contents, keywords, error);
}

gboolean 
dh_book_read (const gchar  *uri, 
              DhBook       *book,
              GNode        *contents,
              GList       **keywords,
              GError      **error)
{
        GsfInput  *input;
        GsfInfile *infile;
        GsfInput  *read_input;
        BookType   type;

        type = book_check_type (uri);

        if (type == BOOK_TYPE_INVALID) {
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_INVALID_BOOK_TYPE,
                             _("'%s' is not a valid book"), uri);
                return FALSE;
        }
        
        if (!g_file_test (uri, G_FILE_TEST_EXISTS)) {
                g_set_error (error,
                             DH_ERROR,
                             DH_ERROR_FILE_NOT_FOUND,
                             _("Couldn't find file '%s'"), uri);
                return FALSE;
        }

        input = gsf_input_mmap_new (uri, NULL);
        
        if (!input) {
                input = gsf_input_stdio_new (uri, error);
        }
        
        if (!input) {
                return FALSE;
        }

        switch (type) {
        case BOOK_TYPE_ZIP:
                return book_read_zip (input, book, contents, keywords, error);
                break;
        case BOOK_TYPE_SPEC:
                return book_read_spec (input, book, contents, keywords, error);
                break;
        case BOOK_TYPE_OLD:
                return book_read_old (input, book, contents, keywords, error);
                break;
        default:
                g_assert_not_reached ();
        }

        return TRUE;
}


       


