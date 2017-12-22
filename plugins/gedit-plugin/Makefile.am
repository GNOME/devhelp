DESKTOP_IN_FILES = devhelp.plugin.desktop.in
PLUGIN_FILE = devhelp.plugin

$(PLUGIN_FILE): $(DESKTOP_IN_FILES)
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

plugindir = $(libdir)/gedit/plugins
plugin_PYTHON = devhelp.py
plugin_DATA = $(PLUGIN_FILE)

CLEANFILES = $(PLUGIN_FILE)
EXTRA_DIST = $(DESKTOP_IN_FILES)

-include $(top_srcdir)/git.mk
