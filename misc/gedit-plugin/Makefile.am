DESKTOP_IN_FILES = devhelp.plugin.desktop.in
DESKTOP_FILES = devhelp.plugin.desktop

$(DESKTOP_FILES): $(DESKTOP_IN_FILES)
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

plugindir = $(libdir)/gedit/plugins
plugin_PYTHON = devhelp.py
plugin_DATA = devhelp.plugin

devhelp.plugin: devhelp.plugin.desktop
	cp devhelp.plugin.desktop devhelp.plugin

CLEANFILES = $(DESKTOP_FILES) $(plugin_DATA)
EXTRA_DIST = $(DESKTOP_IN_FILES)
