SUBDIRS = ui dtd

@INTLTOOL_DESKTOP_RULE@

DESKTOP_IN_FILES = devhelp.desktop.in
DESKTOP_FILES =$(DESKTOP_IN_FILES:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)

schemasdir       = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = devhelp.schemas.in
schemas_DATA     = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

EXTRA_DIST =			\
	$(DESKTOP_IN_FILES)	\
	$(DESKTOP_FILES)        \
	$(schemas_in_files)     \
	$(schemas_DATA)

install-data-local: $(schema_DATA)
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
	for p in $^ ; do \
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p >&1 > /dev/null; \
	done \
	fi
endif

CLEANFILES = $(DESKTOP_FILES)

DISTCLEANFILES = $(DESKTOP_FILES)  \
		 $(schemas_DATA)
