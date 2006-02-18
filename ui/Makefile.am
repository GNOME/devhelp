imagedir = $(datadir)/devhelp/images
image_DATA = 		\
	book_open.png   \
	book_closed.png \
	helpdoc.png

appicondir = $(datadir)/icons/hicolor/48x48/apps
appicon_DATA = devhelp.png
svgicondir = $(datadir)/icons/hicolor/scalable/apps
svgicon_DATA = devhelp.svg

install-data-hook:
	gtk-update-icon-cache -f -t $(datadir)/icons/hicolor || :

uidir = $(datadir)/devhelp/ui
ui_DATA = 				\
	window.ui

gladedir = $(datadir)/devhelp
glade_DATA =                            \
	devhelp.glade

EXTRA_DIST = $(image_DATA) 		\
	     $(appicon_DATA)		\
	     $(ui_DATA)                 \
	     $(glade_DATA)
