NULL = 

@INTLTOOL_DESKTOP_RULE@

DESKTOP_IN_FILES = devhelp.desktop.in
DESKTOP_FILES =$(DESKTOP_IN_FILES:.desktop.in=.desktop)

SUBDIRS = po src ui

#SCHEMAS_DIR = $(sysconfdir)/gconf/schemas
#SCHEMAS_FILE = devhelp.schemas

#schemasdir = $(SCHEMAS_DIR)
#schemas_DATA = $(SCHEMAS_FILE)

#install-schemas: devhelp.schemas
#	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) gconftool-1 --shutdown
#	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) gconftool-1 --makefile-install-rule $(top_srcdir)/$(SCHEMAS_FILE)

#if INSTALL_SCHEMAS
#install-data-local: install-schemas
#endif

EXTRA_DIST =			\
	TODO 			\
	$(DESKTOP_IN_FILES)     \
	$(DESKTOP_FILES)        \
	devhelp.desktop.in 	\
	devhelp.spec.in 	\
	intltool-merge.in 	\
	intltool-update.in 	\
	intltool-extract.in	\
	misc/devhelp.el		\
	misc/html2funcs.py	\
	misc/html2xml.py	\
	misc/devhelp.vim	\
	idl/GNOME_DevHelp.idl	\
	$(NULL)

Develdir = $(datadir)/gnome/apps/Development
Devel_DATA = $(DESKTOP_FILES)

dist-hook: devhelp.spec
	cp devhelp.spec $(distdir)

