DEFINES = 						\
	-DLOCALEDIR=\""$(datadir)/locale"\" 	  	\
	-DDATA_DIR=\"$(datadir)\"			\
	-DPREFIX="\"$(prefix)"\"		 	\
	-DSYSCONFDIR=\""$(sysconfdir)"\"                \
	-DDATADIR=\""$(datadir)"\"                      \
	-DLIBDIR=\""$(libdir)"\"			\
	-DDTD_DIR="\"$(datadir)/devhelp/dtd"\"		\
	-DGLADEDIR=\"$(datadir)/devhelp\"		\
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

devhelp_LDFLAGS = -R$(MOZILLA_HOME) $(AM_LDFLAGS)

# Build the C++ code in a separate library so we can use our warning
# flags for the C code.
noinst_LTLIBRARIES = libcppwrapper.la

libcppwrapper_la_SOURCES =				\
	dh-gecko-utils.cpp				\
	dh-gecko-utils.h

mozilla_include_subdirs =	\
	.			\
	commandhandler		\
	content			\
	dom			\
	find			\
	gfx			\
	locale			\
	prefix			\
	webbrwsr		\
	string

libcppwrapper_la_CPPFLAGS = 				\
	$(DEFINES)					\
	-DALLOW_PRIVATE_API				\
	-DMOZILLA_HOME=\""$(MOZILLA_HOME)\""		\
	-include $(MOZILLA_INCLUDE_ROOT)/mozilla-config.h			\
	$(addprefix -I$(MOZILLA_INCLUDE_ROOT)/,$(mozilla_include_subdirs))	\
	$(LIBDEVHELP_CFLAGS)				\
	$(AM_CPPFLAGS)

libcppwrapper_la_CXXFLAGS =				\
	$(MOZILLA_COMPONENT_CFLAGS)			\
	$(AM_CXXFLAGS)

libcppwrapper_la_LIBADD =				\
	$(MOZILLA_COMPONENT_LIBS)

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
	dh-window.h

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
	dh-window.c

libdevhelp_1_la_CPPFLAGS =				\
	$(DEFINES)					\
	$(AM_CPPFLAGS)

libdevhelp_1_la_CFLAGS = 				\
	$(WARN_CFLAGS)					\
	$(LIBDEVHELP_CFLAGS)				\
	$(MOZILLA_COMPONENT_CFLAGS)			\
	$(AM_CFLAGS)

libdevhelp_1_la_LIBADD =				\
	libcppwrapper.la				\
	$(LIBDEVHELP_LIBS)				\
	$(MOZILLA_COMPONENT_LIBS)

libdevhelp_1_la_LDFLAGS = -R$(MOZILLA_HOME) $(AM_LDFLAGS)

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

BUILT_SOURCES =						\
	dh-marshal.h					\
	dh-marshal.c

EXTRA_DIST =						\
	dh-marshal.list					\
	$(devhelpinclude_HEADERS)

CLEANFILES = $(BUILT_SOURCES)
