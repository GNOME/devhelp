SUBDIRS = dtd po src ui misc

@INTLTOOL_DESKTOP_RULE@

DESKTOP_IN_FILES = devhelp.desktop.in
DESKTOP_FILES =$(DESKTOP_IN_FILES:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)

@INTLTOOL_KEYS_RULE@

KEYS_IN_FILES=devhelp.keys.in
KEYS_FILES = $(KEYS_IN_FILES:.keys.in=.keys)


schemasdir       = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = devhelp.schemas.in
schemas_DATA     = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

mimedir = $(datadir)/mime-info
mime_DATA = $(KEYS_FILES) devhelp.mime

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdevhelp-1.0.pc

bin_SCRIPTS = devhelp

INTLTOOL =                      \
	intltool-extract.in     \
	intltool-merge.in       \
	intltool-update.in

EXTRA_DIST =			\
	TODO 			\
	$(DESKTOP_IN_FILES)	\
	$(DESKTOP_FILES)        \
	$(KEYS_IN_FILES)	\
	$(KEYS_FILES)           \
	$(schemas_in_files)     \
	$(schemas_DATA)         \
	$(mime_DATA)            \
	devhelp.in              \
	libdevhelp-1.0.pc.in    \ 
	$(INTLTOOL)             \
	misc/devhelp.el	        \
	misc/html2funcs.py	\
	misc/html2xml.py	\
	misc/devhelp.vim

dist-hook: devhelp.spec
	cp devhelp.spec $(distdir)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
          for p in $(schemas_DATA) ; do \
            GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$$p ; \
          done \
        fi
else
install-data-local:
endif


CLEANFILES = $(KEYS_FILES) $(DESKTOP_FILES)

DISTCLEANFILES = intltool-extract  \
		 intltool-merge    \
		 intltool-update   \
		 $(DESKTOP_FILES)  \
		 $(KEYS_FILES)     \
		 $(schemas_DATA)
