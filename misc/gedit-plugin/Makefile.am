@INTLTOOL_DESKTOP_RULE@
DESKTOP_IN_FILES = devhelp.desktop.in
DESKTOP_FILES = devhelp.desktop

plugindir = $(libdir)/gedit/plugins
plugin_PYTHON = devhelp.py
plugin_DATA = devhelp.plugin

devhelp.plugin: devhelp.desktop
	cp devhelp.desktop devhelp.plugin

CLEANFILES = $(DESKTOP_FILES) $(plugin_DATA)
EXTRA_DIST = $(DESKTOP_IN_FILES)
