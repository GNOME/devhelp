SUBDIRS = intl po src ui

schemadir = $(sysconfdir)/gconf/schemas
schema_DATA = devhelp.schemas

EXTRA_DIST =			\
	TODO 			\
	devhelp.desktop.in 	\
	devhelp.spec 		\
	devhelp.spec.in 	\
	$(schema_DATA) 		\
	xml-i18n-merge.in 	\
	xml-i18n-update.in 	\
	xml-i18n-extract.in	\
	misc/devhelp.el		\
	misc/html2funcs.py	\
	misc/html2xml.py	\
	idl/GNOME_Devhelp.idl

Develdir = $(datadir)/gnome/apps/Development
Devel_in_files = devhelp.desktop.in
Devel_DATA = $(Devel_in_files:.desktop.in=.desktop)

@XML_I18N_MERGE_DESKTOP_RULE@

install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_CONFIG_SOURCE) gconftool --shutdown
	GCONF_CONFIG_SOURCE=$(GCONF_CONFIG_SOURCE) gconftool --makefile-install-rule $(top_srcdir)/$(schema_DATA)







