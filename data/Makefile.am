SUBDIRS = ui dtd icons

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdevhelp-3.0.pc

assistantdir = $(datadir)/devhelp/assistant
assistant_DATA = 		\
	assistant.js		\
	assistant.css

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = devhelp.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = devhelp.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

defaultsdir = $(datadir)/devhelp
defaults_DATA = devhelp.defaults
devhelp.defaults: devhelp.schemas.in
	cp $(top_srcdir)/data/devhelp.schemas.in $(top_builddir)/data/devhelp.defaults

EXTRA_DIST =			\
	libdevhelp-3.0.pc.in    \
	$(desktop_in_files)	\
	$(schemas_in_files)     \
	$(schemas_DATA)		\
	$(assistant_DATA)

install-data-local: $(schema_DATA)
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
	for p in $^ ; do \
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p >&1 > /dev/null; \
	done \
	fi
endif

CLEANFILES =			\
	$(DESKTOP_FILES)	\
	$(defaults_DATA)

DISTCLEANFILES =		\
	$(desktop_DATA)		\
	$(schemas_DATA)
