AM_CPPFLAGS = 				\
	-I$(top_srcdir)			\
	-DG_LOG_DOMAIN=\"Devhelp\"	\
	$(WARN_CFLAGS)

AM_LDFLAGS =		\
	$(WARN_LDFLAGS)

app_headers =			\
	dh-app.h		\
	dh-assistant.h		\
	dh-preferences.h	\
	dh-tab.h		\
	dh-tab-label.h		\
	dh-web-view.h		\
	dh-window.h		\
	tepl-info-bar.h		\
	$(NULL)

app_c_files =			\
	dh-app.c		\
	dh-assistant.c		\
	dh-main.c		\
	dh-preferences.c	\
	dh-tab.c		\
	dh-tab-label.c		\
	dh-web-view.c		\
	dh-window.c		\
	tepl-info-bar.c		\
	$(NULL)

app_built_sources =	\
	dh-resources.c	\
	$(NULL)

BUILT_SOURCES = 		\
	$(app_built_sources)	\
	$(NULL)

# The application
bin_PROGRAMS = devhelp

devhelp_SOURCES =	\
	$(app_headers)	\
	$(app_c_files)	\
	$(NULL)

# Do not distribute generated files.
nodist_devhelp_SOURCES =	\
	$(app_built_sources)

devhelp_CPPFLAGS =	\
	$(AM_CPPFLAGS)

devhelp_CFLAGS =		\
	$(DEVHELP_CFLAGS)

devhelp_LDADD = 				\
	$(DEVHELP_LIBS)				\
	$(LIBM)					\
        $(top_builddir)/devhelp/libdevhelp-3.la

devhelp_LDFLAGS =	\
	$(AM_LDFLAGS)

resource_files =		\
	dh-assistant.ui		\
	dh-preferences.ui	\
	dh-window.ui		\
	help-overlay.ui		\
	menus.ui		\
	$(NULL)

dh-resources.c: dh.gresource.xml $(resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source --c-name dh $(srcdir)/dh.gresource.xml

EXTRA_DIST =			\
	dh.gresource.xml	\
	$(resource_files)	\
	$(NULL)

CLEANFILES = $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
