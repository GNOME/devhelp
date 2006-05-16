imagedir = $(datadir)/devhelp/images
image_DATA = 		\
	book_open.png   \
	book_closed.png \
	helpdoc.png

appicondir = $(datadir)/icons/hicolor/48x48/apps
appicon_DATA = devhelp.png
svgicondir = $(datadir)/icons/hicolor/scalable/apps
svgicon_DATA = devhelp.svg

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

uidir = $(datadir)/devhelp/ui
ui_DATA = 				\
	window.ui

gladedir = $(datadir)/devhelp
glade_DATA =                            \
	devhelp.glade

EXTRA_DIST = $(image_DATA) 		\
	     $(appicon_DATA)		\
	     $(ui_DATA)                 \
	     $(glade_DATA)		\
	     $(svgicon_DATA)
