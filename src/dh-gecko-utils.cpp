/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio AB
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

#include <config.h>

#include <gtkmozembed.h>

#include <nsCOMPtr.h>
#include <nsMemory.h>
#include <nsEmbedString.h>
#include <nsIPrefService.h>
#define MOZILLA_INTERNAL_API
#include <nsIServiceManager.h>
#undef MOZILLA_INTERNAL_API
#include <nsIWindowWatcher.h>
#include <nsIIOService.h>
#include <nsISupportsPrimitives.h>
#include <nsILocalFile.h>
#include <nsIURI.h>

#include <stdlib.h>

#if defined (HAVE_CHROME_NSICHROMEREGISTRYSEA_H)
#include <chrome/nsIChromeRegistrySea.h>
#elif defined(MOZ_NSIXULCHROMEREGISTRY_SELECTSKIN)
#include <nsIChromeRegistry.h>
#endif

#ifdef ALLOW_PRIVATE_API
// FIXME: For setting the locale. hopefully gtkmozembed will do itself soon
#include <nsILocaleService.h>
#endif

#include "dh-util.h"
#include "dh-gecko-utils.h"

static gboolean
dh_util_split_font_string (const gchar *font_name, gchar **name, gint *size)
{
	PangoFontDescription *desc;
	PangoFontMask mask = (PangoFontMask) (PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_SIZE);
	gboolean retval = FALSE;

	desc = pango_font_description_from_string (font_name);
	if (!desc) return FALSE;

	if ((pango_font_description_get_set_fields (desc) & mask) == mask) {
		*size = PANGO_PIXELS (pango_font_description_get_size (desc));
		*name = g_strdup (pango_font_description_get_family (desc));
		retval = TRUE;
	}

	pango_font_description_free (desc);

	return retval;
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
dh_gecko_utils_set_font (gint type, const gchar *fontname)
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

static nsresult
getUILang (nsAString& aUILang)
{
	nsresult rv;

	nsCOMPtr<nsILocaleService> localeService = do_GetService (NS_LOCALESERVICE_CONTRACTID);
	if (!localeService)
	{
		g_warning ("Could not get locale service!\n");
		return NS_ERROR_FAILURE;
	}

	rv = localeService->GetLocaleComponentForUserAgent (aUILang);

	if (NS_FAILED (rv))
	{
		g_warning ("Could not determine locale!\n");
		return NS_ERROR_FAILURE;
	}

	return NS_OK;
}

static nsresult 
gecko_utils_init_chrome (void)
{
/* FIXME: can we just omit this on new-toolkit ? */
#if defined(MOZ_NSIXULCHROMEREGISTRY_SELECTSKIN) || defined(HAVE_CHROME_NSICHROMEREGISTRYSEA_H)
        nsresult rv;
        nsEmbedString uiLang;

#ifdef HAVE_CHROME_NSICHROMEREGISTRYSEA_H
        nsCOMPtr<nsIChromeRegistrySea> chromeRegistry = do_GetService (NS_CHROMEREGISTRY_CONTRACTID);
#else
        nsCOMPtr<nsIXULChromeRegistry> chromeRegistry = do_GetService (NS_CHROMEREGISTRY_CONTRACTID);
#endif
        NS_ENSURE_TRUE (chromeRegistry, NS_ERROR_FAILURE);

        // Set skin to 'classic' so we get native scrollbars.
        rv = chromeRegistry->SelectSkin (nsEmbedCString("classic/1.0"), PR_FALSE);
        NS_ENSURE_SUCCESS (rv, NS_ERROR_FAILURE);

        // set locale
        rv = chromeRegistry->SetRuntimeProvider(PR_TRUE);
        NS_ENSURE_SUCCESS (rv, NS_ERROR_FAILURE);

        rv = getUILang(uiLang);
        NS_ENSURE_SUCCESS (rv, NS_ERROR_FAILURE);

        nsEmbedCString cUILang;
        NS_UTF16ToCString (uiLang, NS_CSTRING_ENCODING_UTF8, cUILang);

        return chromeRegistry->SelectLocale (cUILang, PR_FALSE);
#else
        return NS_OK;
#endif
}

extern "C" void
dh_gecko_utils_init_services (void)
{
	gchar *profile_dir;
	
	gtk_moz_embed_set_comp_path (MOZILLA_HOME);
	
	profile_dir = g_build_filename (g_getenv ("HOME"), 
					".gnome2",
					"devhelp",
					"mozilla", NULL);

	gtk_moz_embed_set_profile_path (profile_dir, "Devhelp");
	g_free (profile_dir);

	gtk_moz_embed_push_startup ();
        
	gecko_prefs_set_string ("font.size.unit", "pt");
	gecko_utils_init_chrome ();
}

