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
#include <gtkmozembed_internal.h>

#include <nsCOMPtr.h>
#include <nsMemory.h>
#include <nsEmbedString.h>
#include <nsIPrefService.h>
#include <nsICommandManager.h>
#include <nsIInterfaceRequestorUtils.h>
#define MOZILLA_INTERNAL_API
#include <nsIServiceManager.h>
#undef MOZILLA_INTERNAL_API
#include <nsISupportsPrimitives.h>
#include <nsILocalFile.h>
#include <nsIDOMMouseEvent.h>
#include <nsIWebBrowserFind.h>
#include <nsStringAPI.h>

#include <stdlib.h>

#ifndef HAVE_GECKO_1_8
#if defined (HAVE_CHROME_NSICHROMEREGISTRYSEA_H)
#include <chrome/nsIChromeRegistrySea.h>
#elif defined(MOZ_NSIXULCHROMEREGISTRY_SELECTSKIN)
#include <nsIChromeRegistry.h>
#endif

#ifdef ALLOW_PRIVATE_API
// FIXME: For setting the locale. hopefully gtkmozembed will do itself soon
#include <nsILocaleService.h>
#endif
#endif /* !HAVE_GECKO_1_8 */

#include "dh-util.h"
#include "dh-gecko-utils.h"
#include "Yelper.h"

gint
dh_gecko_utils_get_mouse_event_button (gpointer event)
{
	nsIDOMMouseEvent *aMouseEvent;
	PRUint16          button;

	aMouseEvent = (nsIDOMMouseEvent *) event;

	aMouseEvent->GetButton (&button);

	return button + 1;
}

gint
dh_gecko_utils_get_mouse_event_modifiers (gpointer event)
{
	nsIDOMMouseEvent *aMouseEvent;
	PRBool            ctrl, alt, shift, meta;
	gint              mask;
	
	aMouseEvent = (nsIDOMMouseEvent *) event;

	aMouseEvent->GetCtrlKey (&ctrl);
	aMouseEvent->GetAltKey (&alt);
	aMouseEvent->GetShiftKey (&shift);
	aMouseEvent->GetMetaKey (&meta);

	mask = 0;
	if (ctrl) {
		mask |= GDK_CONTROL_MASK;
	}
	if (alt || meta) {
		mask |= GDK_MOD1_MASK;
	}
	if (shift) {
		mask |= GDK_SHIFT_MASK;
	}

	return mask;
}

static nsresult
do_command (GtkMozEmbed *embed,
	    const char  *command)
{
	nsCOMPtr<nsIWebBrowser>     webBrowser;
	nsCOMPtr<nsICommandManager> cmdManager;

	gtk_moz_embed_get_nsIWebBrowser (embed, getter_AddRefs (webBrowser));
	
	cmdManager = do_GetInterface (webBrowser);
	
	return cmdManager->DoCommand (command, nsnull, nsnull);
}

void
dh_gecko_utils_copy_selection (GtkMozEmbed *embed)
{
	do_command (embed, "cmd_copy");
}

static gboolean
dh_util_split_font_string (const gchar *font_name, gchar **name, gint *size)
{
	PangoFontDescription *desc;
	PangoFontMask         mask;
	gboolean              retval = FALSE;

	if (font_name == NULL) {
		return FALSE;
	}

	mask = (PangoFontMask) (PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_SIZE);
	
	desc = pango_font_description_from_string (font_name);
	if (!desc) {
		return FALSE;
	}
	
	if ((pango_font_description_get_set_fields (desc) & mask) == mask) {
		*size = PANGO_PIXELS (pango_font_description_get_size (desc));
		*name = g_strdup (pango_font_description_get_family (desc));
		retval = TRUE;
	}

	pango_font_description_free (desc);

	return retval;
}

static gboolean
gecko_prefs_set_bool (const gchar *key, gboolean value)
{
	nsresult rv;
	nsCOMPtr<nsIPrefService> prefService (do_GetService (NS_PREFSERVICE_CONTRACTID, &rv));
	NS_ENSURE_SUCCESS (rv, FALSE);

	nsCOMPtr<nsIPrefBranch> pref;
	rv = prefService->GetBranch ("", getter_AddRefs (pref));
	NS_ENSURE_SUCCESS (rv, FALSE);

	rv = pref->SetBoolPref (key, value);

	return NS_SUCCEEDED (rv) != PR_FALSE;
}

static gboolean
gecko_prefs_set_string (const gchar *key, const gchar *value)
{
	nsresult rv;
	nsCOMPtr<nsIPrefService> prefService (do_GetService (NS_PREFSERVICE_CONTRACTID, &rv));
	NS_ENSURE_SUCCESS (rv, FALSE);

	nsCOMPtr<nsIPrefBranch> pref;
	rv = prefService->GetBranch ("", getter_AddRefs (pref));
	NS_ENSURE_SUCCESS (rv, FALSE);

	rv = pref->SetCharPref (key, value);

	return NS_SUCCEEDED (rv) != PR_FALSE;
}

static gboolean
gecko_prefs_set_int (const gchar *key, gint value)
{
	nsresult rv;
	nsCOMPtr<nsIPrefService> prefService (do_GetService (NS_PREFSERVICE_CONTRACTID, &rv));
	NS_ENSURE_SUCCESS (rv, FALSE);

	nsCOMPtr<nsIPrefBranch> pref;
	rv = prefService->GetBranch ("", getter_AddRefs (pref));
	NS_ENSURE_SUCCESS (rv, FALSE);

	rv = pref->SetIntPref (key, value);

	return NS_SUCCEEDED (rv) != PR_FALSE;
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

#ifndef HAVE_GECKO_1_8

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

#endif /* !HAVE_GECKO_1_8 */

static nsresult
gecko_utils_init_prefs (void)
{
	nsresult rv;
	nsCOMPtr<nsIPrefService> prefService (do_GetService (NS_PREFSERVICE_CONTRACTID, &rv));
	NS_ENSURE_SUCCESS (rv, rv);

	nsCOMPtr<nsILocalFile> file;
	rv = NS_NewNativeLocalFile (nsEmbedCString (SHAREDIR "/default-prefs.js"),
				   PR_TRUE, getter_AddRefs (file));
	NS_ENSURE_SUCCESS (rv, rv);

	rv = prefService->ReadUserPrefs (file);
	rv |= prefService->ReadUserPrefs (nsnull);
	NS_ENSURE_SUCCESS (rv, rv);

	return rv;
}

extern "C" void
dh_gecko_utils_init (void)
{
	if (!g_thread_supported ()) {
		g_thread_init (NULL);
	}

#ifdef HAVE_GECKO_1_9
	NS_LogInit ();
#endif

#ifdef HAVE_GECKO_1_9
	gtk_moz_embed_set_path (GECKO_HOME);
#else
	gtk_moz_embed_set_comp_path (GECKO_HOME);
#endif

	gchar *profile_dir = g_build_filename (g_get_home_dir (),
					       ".gnome2",
					       "devhelp",
					       "mozilla", NULL);

	gtk_moz_embed_set_profile_path (profile_dir, "Devhelp");
	g_free (profile_dir);

	gtk_moz_embed_push_startup ();

	gecko_utils_init_prefs ();

#ifndef HAVE_GECKO_1_8
	gecko_utils_init_chrome ();
#endif
}

extern "C" void
dh_gecko_utils_shutdown (void)
{
	gtk_moz_embed_pop_startup ();

#ifdef HAVE_GECKO_1_9
	NS_LogTerm ();
#endif
}


extern "C" gboolean
dh_gecko_utils_search_find (Yelper *yelper, const gchar * text)
{
	yelper->Init();
	return yelper->Find (text);
}

extern "C" gboolean
dh_gecko_utils_search_find_again (Yelper *yelper, gboolean backward)
{
	yelper->Init();
	return yelper->FindAgain(!backward);
}

extern "C" void
dh_gecko_utils_search_set_case_sensitive (Yelper * yelper, gboolean case_sensitive)
{
	yelper->Init();
	yelper->SetFindProperties (NULL, case_sensitive, FALSE);
}

extern "C" Yelper *
dh_gecko_utils_create_yelper (GtkMozEmbed *gecko)
{
	Yelper *yelper = new Yelper (gecko);
	return yelper;
}

extern "C" void
dh_gecko_utils_destroy_yelper (Yelper *yelper)
{
	delete yelper;
}
