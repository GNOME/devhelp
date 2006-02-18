imagedir = $(datadir)/devhelp/images
image_DATA = 		\
	book_open.png   \
	book_closed.png \
	helpdoc.png

appicondir = $(datadir)/icons/hicolor/48x48/apps
appicon_DATA = devhelp.png

svgicondir = $(datadir)/icons/hicolor/scalable/48x48/apps
svgicon_DATA = devhelp.svg

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
	     COPYING

install-data-hook:
	gtk-update-icon-cache || :
