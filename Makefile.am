SUBDIRS = dtd libegg po src ui misc

@INTLTOOL_DESKTOP_RULE@

DESKTOP_IN_FILES = devhelp.desktop.in
DESKTOP_FILES =$(DESKTOP_IN_FILES:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)

@INTLTOOL_KEYS_RULE@

KEYS_IN_FILES=devhelp.keys.in
KEYS_FILES = $(KEYS_IN_FILES:.keys.in=.keys)

mimedir = $(datadir)/mime-info
mime_DATA = $(KEYS_FILES) devhelp.mime

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdevhelp-1.0.pc

EXTRA_DIST =			\
	TODO 			\
	$(DESKTOP_IN_FILES)	\
	$(KEYS_IN_FILES)	\
	$(mime_DATA)            \
	devhelp.desktop.in 	\
	devhelp.spec.in 	\
	libdevhelp-1.0.pc.in    \
	intltool-merge.in 	\
	intltool-update.in 	\
	intltool-extract.in	\
	misc/devhelp.el	\
	misc/html2funcs.py	\
	misc/html2xml.py	\
	misc/devhelp.vim

dist-hook: devhelp.spec
	cp devhelp.spec $(distdir)

CLEANFILES = $(KEYS_FILES) $(DESKTOP_FILES)

DISTCLEANFILES = intltool-extract intltool-merge intltool-update
