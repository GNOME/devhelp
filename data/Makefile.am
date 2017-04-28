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

appstream_in_files = org.gnome.Devhelp.appdata.xml.in
appstream_XML = $(appstream_in_files:.xml.in=.xml)
@APPSTREAM_XML_RULES@

$(appstream_XML): $(appstream_in_files)
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

desktopdir = $(datadir)/applications
desktop_in_files = org.gnome.Devhelp.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

$(desktop_DATA): $(desktop_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

dbusservicedir = $(datadir)/dbus-1/services
dbusservice_DATA = org.gnome.Devhelp.service

org.gnome.Devhelp.service: Makefile
	$(AM_V_GEN) (echo '[D-BUS Service]'; \
		     echo 'Name=org.gnome.Devhelp'; \
		     echo 'Exec=${bindir}/devhelp --gapplication-service') > $@.tmp && \
		     mv $@.tmp $@

man_MANS = devhelp.1

EXTRA_DIST =			\
	libdevhelp-3.0.pc.in    \
	$(desktop_in_files)	\
	$(gsettings_SCHEMAS)    \
	devhelp.convert \
	$(assistant_DATA) \
	$(man_MANS) \
	$(appstream_in_files)

CLEANFILES =			\
	$(DESKTOP_FILES)	\
	$(defaults_DATA)	\
	$(desktop_DATA) 	\
	$(dbusservice_DATA)	\
	$(appstream_XML)

-include $(top_srcdir)/git.mk
