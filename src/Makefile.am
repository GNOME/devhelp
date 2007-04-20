DEFINES = 						\
	-DLOCALEDIR=\""$(datadir)/locale"\" 	  	\
	-DPREFIX="\"$(prefix)"\"		 	\
	-DSYSCONFDIR=\""$(sysconfdir)"\"                \
	-DDATADIR=\""$(datadir)"\"                      \
	-DLIBDIR=\""$(libdir)"\"			\
	-DSHAREDIR=\""$(pkgdatadir)"\"			\
	-DG_LOG_DOMAIN=\"Devhelp\"

bin_PROGRAMS = devhelp

devhelp_SOURCES =					\
	dh-main.c

devhelp_CPPFLAGS =					\
	$(DEFINES)					\
	$(AM_CPPFLAGS)

devhelp_CFLAGS = 					\
	$(WARN_CFLAGS)					\
	$(DEVHELP_CFLAGS)				\
	$(AM_CFLAGS)

devhelp_LDADD = 					\
        $(DEVHELP_LIBS)					\
        libdevhelp-1.la

devhelp_LDFLAGS = -R$(GECKO_HOME) $(AM_LDFLAGS)

# Build the C++ code in a separate library so we can use our warning
# flags for the C code.
noinst_LTLIBRARIES = libcppwrapper.la

libcppwrapper_la_SOURCES =				\
	Yelper.cpp					\
	Yelper.h					\
	dh-gecko-utils.cpp				\
	dh-gecko-utils.h

gecko_include_subdirs =	\
	.			\
	commandhandler		\
	content			\
	dom			\
	find			\
	fastfind		\
	docshell		\
	gfx			\
	gtkembedmoz		\
	locale			\
	pref			\
	webbrwsr		\
	string			\
	xpcom

libcppwrapper_la_CPPFLAGS = 				\
	$(DEFINES)					\
	-DALLOW_PRIVATE_API				\
	-DGECKO_HOME=\""$(GECKO_HOME)\""		\
	-include $(GECKO_INCLUDE_ROOT)/mozilla-config.h			\
	$(addprefix -I$(GECKO_INCLUDE_ROOT)/,$(gecko_include_subdirs))	\
	$(LIBDEVHELP_CFLAGS)				\
	$(AM_CPPFLAGS)

libcppwrapper_la_CXXFLAGS =				\
	$(GECKO_CFLAGS)					\
	$(AM_CXXFLAGS)

libcppwrapper_la_LIBADD =				\
	$(GECKO_LIBS)					\
	$(GECKO_EXTRA_LIBS)

lib_LTLIBRARIES = libdevhelp-1.la

devhelpincludedir = $(includedir)/devhelp-1.0/devhelp
devhelpinclude_HEADERS = 				\
	dh-base.h					\
	dh-book-tree.h					\
	dh-error.h					\
	dh-html.h					\
	dh-keyword-model.h				\
	dh-link.h					\
	dh-search.h					\
	dh-parser.h					\
	dh-preferences.h                                \
	dh-util.h					\
	dh-window.h					\
	eggfindbar.h

libdevhelp_1_la_SOURCES =				\
	bacon-message-connection.c			\
	bacon-message-connection.h			\
	dh-marshal-main.c				\
	dh-base.c					\
	dh-book-tree.c					\
	dh-error.c					\
	dh-html.c					\
	dh-keyword-model.c				\
	dh-link.c					\
	dh-search.c					\
	dh-parser.c					\
	dh-preferences.c				\
	dh-util.c					\
	dh-window.c					\
	eggfindbar.c

libdevhelp_1_la_CPPFLAGS =				\
	$(DEFINES)					\
	$(addprefix -I$(GECKO_INCLUDE_ROOT)/,$(gecko_include_subdirs))	\
	$(AM_CPPFLAGS)

libdevhelp_1_la_CFLAGS = 				\
	$(WARN_CFLAGS)					\
	$(LIBDEVHELP_CFLAGS)				\
	$(GECKO_CFLAGS)					\
	$(AM_CFLAGS)

libdevhelp_1_la_LIBADD =				\
	libcppwrapper.la				\
	$(LIBDEVHELP_LIBS)				\
	$(GECKO_LIBS)					\
	$(GECKO_EXTRA_LIBS)

libdevhelp_1_la_LDFLAGS = -R$(GECKO_HOME) $(AM_LDFLAGS)

dh-marshal.h: dh-marshal.list
	(cd $(srcdir) && \
	@GLIB_GENMARSHAL@ --prefix=dh_marshal dh-marshal.list --header) > xgen-gmh \
	&& (cmp -s xgen-gmh dh-marshal.h || cp xgen-gmh dh-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~

dh-marshal.c: dh-marshal.list
	(cd $(srcdir) && \
	@GLIB_GENMARSHAL@ --prefix=dh_marshal dh-marshal.list --body) > xgen-gmc \
	&& cp xgen-gmc dh-marshal.c \
	&& rm -f xgen-gmc xgen-gmc~

dh-marshal-main.c: dh-marshal.c dh-marshal.h

default_prefs_in_FILES = \
	default-prefs-common.js			\
	default-prefs-mozilla.js		\
	default-prefs-toolkit.js		\
	default-prefs-gecko-1-8.js		\
	default-prefs-gecko-1-9.js

default_prefs_files = \
	default-prefs-common.js                 \
	default-prefs-$(GECKO_FLAVOUR).js

if HAVE_GECKO_1_8
default_prefs_files += default-prefs-gecko-1-8.js
endif

if HAVE_GECKO_1_9
default_prefs_files += default-prefs-gecko-1-9.js
endif

default-prefs.js: $(default_prefs_files)
	cat $^ > $@

default_prefsdir = $(pkgdatadir)
default_prefs_DATA = default-prefs.js

BUILT_SOURCES =						\
	dh-marshal.h					\
	dh-marshal.c

EXTRA_DIST =						\
	dh-marshal.list					\
	$(devhelpinclude_HEADERS)			\
	$(default_prefs_in_FILES)

CLEANFILES = $(BUILT_SOURCES) $(default_prefs_DATA)
