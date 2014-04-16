SUBDIRS = dtd icons

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdevhelp-3.0.pc

assistantdir = $(datadir)/devhelp/assistant
assistant_DATA = 		\
	assistant.js		\
	assistant.css

gsettings_SCHEMAS = org.gnome.devhelp.gschema.xml
@GSETTINGS_RULES@

gsettingsconvertdir = $(datadir)/GConf/gsettings
gsettingsconvert_DATA = devhelp.convert

app_data_in_files = devhelp.appdata.xml.in
app_data_files = $(app_data_in_files:.xml.in=.xml)

appdatadir = $(datadir)/appdata
appdata_DATA = $(app_data_files)

@INTLTOOL_XML_RULE@


@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = org.gnome.Devhelp.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

dbusservicedir = $(datadir)/dbus-1/services
dbusservice_DATA = org.gnome.Devhelp.service

org.gnome.Devhelp.service: Makefile
	$(AM_V_GEN) (echo '[D-BUS Service]'; \
		     echo 'Name=org.gnome.Devhelp'; \
		     echo 'Exec=${bindir}/devhelp --gapplication-service') > $@.tmp && \
		     mv $@.tmp $@

EXTRA_DIST =			\
	libdevhelp-3.0.pc.in    \
	$(desktop_in_files)	\
	$(gsettings_SCHEMAS)    \
	devhelp.convert \
	$(assistant_DATA) \
	$(app_data_in_files)

CLEANFILES =			\
	$(DESKTOP_FILES)	\
	$(defaults_DATA)

DISTCLEANFILES =		\
	$(desktop_DATA) \
	$(dbusservice_DATA)	\
	$(app_data_files)
