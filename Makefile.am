SUBDIRS = intl po src ui

SCHEMAS_DIR = $(sysconfdir)/gconf/schemas
SCHEMAS_FILE = devhelp.schemas

if INSTALL_SCHEMAS
schemasdir = $(SCHEMAS_DIR)
schemas_DATA = $(SCHEMAS_FILE)
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
	idl/GNOME_DevHelp.idl

Develdir = $(datadir)/gnome/apps/Development
Devel_in_files = devhelp.desktop.in
Devel_DATA = $(Devel_in_files:.desktop.in=.desktop)

@XML_I18N_MERGE_DESKTOP_RULE@

if INSTALL_SCHEMAS
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) gconftool --shutdown
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) gconftool --makefile-install-rule $(top_srcdir)/$(SCHEMAS_FILE)
endif

