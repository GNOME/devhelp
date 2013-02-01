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

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = devhelp.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

EXTRA_DIST =			\
	libdevhelp-3.0.pc.in    \
	$(desktop_in_files)	\
	$(gsettings_SCHEMAS)    \
	devhelp.convert \
	$(assistant_DATA)

CLEANFILES =			\
	$(DESKTOP_FILES)	\
	$(defaults_DATA)

DISTCLEANFILES =		\
	$(desktop_DATA)
