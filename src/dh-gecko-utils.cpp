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

#include "dh-gecko-utils.h"

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
dh_gecko_utils_set_font (gint         type,
			 const gchar *fontname,
			 gint         fontsize)
{
	switch (type) {
	case DH_GECKO_PREF_FONT_VARIABLE:
		gecko_prefs_set_string ("font.name.variable.x-western", 
					fontname);
		gecko_prefs_set_int ("font.size.variable.x-western", 
				     fontsize);
		break;
	case DH_GECKO_PREF_FONT_FIXED:
		gecko_prefs_set_string ("font.name.fixed.x-western", 
					fontname);
		gecko_prefs_set_int ("font.size.fixed.x-western", 
				     fontsize);
		break;
	}
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
