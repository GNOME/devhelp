SUBDIRS = devhelp

@INTLTOOL_DESKTOP_RULE@
DESKTOP_IN_FILES = devhelp.desktop.in
DESKTOP_FILES =$(DESKTOP_IN_FILES:.desktop.in=.desktop)

plugindir = $(libdir)/gedit-2/plugins
plugin_DATA = $(DESKTOP_FILES:.desktop=.gedit-plugin)

.desktop.gedit-plugin:
	cp $< $*.gedit-plugin

CLEANFILES = $(DESKTOP_FILES) $(plugin_DATA)
DISTCLEANFILES = $(DESKTOP_FILES) $(plugin_DATA)
EXTRA_DIST = $(DESKTOP_IN_FILES) $(DESKTOP_FILES) $(plugin_DATA)
