/*
 *  Copyright (C) 2000-2004 Marco Pesenti Gritti
 *  Copyright (C) 2003-2005 Christian Persch
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  $Id: Yelper.h,v 1.4 2006/09/17 17:24:32 chpe Exp $
 */

#ifndef __YELPER_H__
#define __YELPER_H__

#include <gtkmozembed.h>
#include <nsCOMPtr.h>
#include <nsIContentViewer.h>

#if 0
#include "yelp-print.h"
#endif

class nsIDOMWindow;
class nsITypeAheadFind;
class nsIWebBrowser;

class Yelper
{
public:
	Yelper (GtkMozEmbed *aEmbed);
	~Yelper ();

	nsresult Init ();
	void Destroy ();

	void DoCommand (const char *aCommand);
	
	nsresult SetZoom (float aTextZoom);
	nsresult GetZoom (float *aTextZoom);

	void SetFindProperties (const char *aSearchString,
				PRBool aCaseSensitive,
				PRBool aWrap);
	PRBool Find (const char *aSearchString);
	PRBool FindAgain (PRBool aForward);
	void SetSelectionAttention (PRBool aSelectionAttention);

	void ProcessMouseEvent (void *aEvent);

#if 0 // Disable those methods for compiling errors
	nsresult Print (YelpPrintInfo *print_info, PRBool preview,
			int *prev_pages);	
	nsresult PrintPreviewNavigate (int page_no);
	nsresult PrintPreviewEnd ();
#endif

private:
	PRPackedBool mInitialised;
	PRPackedBool mSelectionAttention;
	PRPackedBool mHasFocus;

	GtkMozEmbed *mEmbed;
	nsCOMPtr<nsIWebBrowser> mWebBrowser;
	nsCOMPtr<nsIDOMWindow> mDOMWindow;
	nsCOMPtr<nsITypeAheadFind> mFinder;
	
	nsresult GetContentViewer (nsIContentViewer **aViewer);
};

#endif /* !__YELPER_H__ */
