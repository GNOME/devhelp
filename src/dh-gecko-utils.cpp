/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio HB
 * Copyright (C) 2004 Marco Pesenti Gritti
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
 */

#include <gtkmozembed.h>
#include <gtkmozembed_internal.h>
#include <nsIWebBrowser.h>
#include <nsIWebBrowserFind.h>
#include <nsCOMPtr.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsReadableUtils.h>
#include <nsString.h>
#include <nsIPrefService.h>
#include <nsIServiceManager.h>
#include <stdlib.h>

#include "dh-util.h"
#include "dh-gecko-utils.h"

static gboolean
dh_util_split_font_string (const gchar *font_name, gchar **name, gint *size)
{
	gchar *tmp_name, *ch;
	
	tmp_name = g_strdup (font_name);

	ch = g_utf8_strrchr (tmp_name, -1, ' ');
	if (!ch || ch == tmp_name) {
		return FALSE;
	}

	*ch = '\0';

	*name = g_strdup (tmp_name);
	*size = strtol (ch + 1, (char **) NULL, 10);
	
	return TRUE;
}

static gboolean
gecko_prefs_set_string (const gchar *key, const gchar *value)
{
	nsCOMPtr<nsIPrefService> prefService =
		do_GetService (NS_PREFSERVICE_CONTRACTID);
	nsCOMPtr<nsIPrefBranch> pref;
	prefService->GetBranch ("", getter_AddRefs (pref));

	if (pref) {
		nsresult rv = pref->SetCharPref (key, value);
		return NS_SUCCEEDED (rv) ? TRUE : FALSE;
	}
	
	return FALSE;

}

static gboolean
gecko_prefs_set_int (const gchar *key, gint value)
{
	nsCOMPtr<nsIPrefService> prefService =
		do_GetService (NS_PREFSERVICE_CONTRACTID);
	nsCOMPtr<nsIPrefBranch> pref;
	prefService->GetBranch ("", getter_AddRefs (pref));

	if (pref) {
		nsresult rv = pref->SetIntPref (key, value);
		return NS_SUCCEEDED (rv) ? TRUE : FALSE;
	}
	
	return FALSE;
}
extern "C" void
dh_gecko_utils_set_font_unit (const gchar *unit)
{
        gecko_prefs_set_string ("font.size.unit", unit);
}

extern "C" void 
dh_gecko_utils_set_font (gint         type, const gchar *fontname)
{
	gchar *name;
	gint   size;

	name = NULL;
	if (!dh_util_split_font_string (fontname, &name, &size)) {
		g_free (name);
		return;
	}
	
	switch (type) {
	case DH_GECKO_PREF_FONT_VARIABLE:
		gecko_prefs_set_string ("font.name.variable.x-western", 
					name);
		gecko_prefs_set_int ("font.size.variable.x-western", 
				     size);
		break;
	case DH_GECKO_PREF_FONT_FIXED:
		gecko_prefs_set_string ("font.name.fixed.x-western", 
					name);
		gecko_prefs_set_int ("font.size.fixed.x-western", 
				     size);
		break;
	}

	g_free (name);
}		   

#if 0
extern "C" gboolean
dh_gecko_find (GtkMozEmbed  *embed,
	       const gchar  *str,
	       gboolean      match_case,
	       gboolean      wrap,
	       gboolean      forward)
{
    PRBool didFind;
    nsCString matchString;

    matchString.Assign (str);

    nsCOMPtr<nsIWebBrowser> webBrowser;
    gtk_moz_embed_get_nsIWebBrowser (embed, getter_AddRefs(webBrowser));

    nsCOMPtr<nsIWebBrowserFind> finder (do_GetInterface(webBrowser));
    NS_ENSURE_TRUE (finder, NS_ERROR_FAILURE);

    finder->SetFindBackwards (!forward);
    finder->SetSearchString (ToNewUnicode (matchString));
    finder->SetMatchCase (match_case);
    finder->SetWrapFind (wrap);

    finder->FindNext (&didFind);

    return didFind;
}
#endif
