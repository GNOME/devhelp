SUBDIRS = intl po src ui

SCHEMAS_DIR = $(sysconfdir)/gconf/schemas
SCHEMAS_FILE = devhelp.schemas

schemasdir = $(SCHEMAS_DIR)
schemas_DATA = $(SCHEMAS_FILE)

install-schemas: devhelp.schemas
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) gconftool-1 --shutdown
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) gconftool-1 --makefile-install-rule $(top_srcdir)/$(SCHEMAS_FILE)

if INSTALL_SCHEMAS
install-data-local: install-schemas
endif

EXTRA_DIST =			\
	TODO 			\
	devhelp.desktop.in 	\
	devhelp.spec 		\
	devhelp.spec.in 	\
	$(SCHEMAS_FILE)		\
	xml-i18n-merge.in 	\
	xml-i18n-update.in 	\
	xml-i18n-extract.in	\
	misc/devhelp.el		\
	misc/html2funcs.py	\
	misc/html2xml.py	\
	misc/devhelp.vim	\
	idl/GNOME_DevHelp.idl

Develdir = $(datadir)/gnome/apps/Development
Devel_in_files = devhelp.desktop.in
Devel_DATA = $(Devel_in_files:.desktop.in=.desktop)

@XML_I18N_MERGE_DESKTOP_RULE@

