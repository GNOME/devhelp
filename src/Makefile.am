INCLUDES = 						\
	-DLOCALEDIR=\""$(datadir)/locale"\" 	  	\
	-DDATA_DIR=\"$(datadir)\"			\
	-DPREFIX="\"$(prefix)"\"		 	\
	-DSYSCONFDIR=\""$(sysconfdir)"\"                \
	-DDATADIR=\""$(datadir)"\"                      \
	-DLIBDIR=\""$(libdir)"\"			\
	-DDTD_DIR="\"$(datadir)/devhelp/dtd"\"		\
	-DGLADEDIR=\"$(datadir)/devhelp\"		\
	-DG_LOG_DOMAIN=\"Devhelp\"

MOZILLA_INCLUDES =					\
	$(MOZILLA_COMPONENT_CFLAGS)			\
	-I$(MOZILLA_INCLUDE_ROOT)			\
	-I$(MOZILLA_INCLUDE_ROOT)/content		\
	-I$(MOZILLA_INCLUDE_ROOT)/dom			\
	-I$(MOZILLA_INCLUDE_ROOT)/find			\
	-I$(MOZILLA_INCLUDE_ROOT)/locale		\
	-I$(MOZILLA_INCLUDE_ROOT)/webbrwsr		\
	-I$(MOZILLA_INCLUDE_ROOT)/string		\
	-I$(MOZILLA_INCLUDE_ROOT)/pref			\
	-I$(MOZILLA_INCLUDE_ROOT)/gfx			\
	-I$(MOZILLA_INCLUDE_ROOT)/commandhandler	\
	-DALLOW_PRIVATE_API				\
	-DMOZILLA_HOME=\""$(MOZILLA_HOME)\""		\
	-include $(MOZILLA_INCLUDE_ROOT)/mozilla-config.h

bin_PROGRAMS = devhelp

devhelp_SOURCES =					\
	dh-main.c

devhelp_CFLAGS = 					\
	$(WARN_CFLAGS)					\
	$(DEVHELP_CFLAGS)

devhelp_LDADD = 					\
        $(DEVHELP_LIBS)					\
        libdevhelp-1.la

# Build the C++ code in a separate library so we can use our warning
# flags for the C code.
noinst_LTLIBRARIES = libcppwrapper.la

libcppwrapper_la_CPPFLAGS = 				\
	$(MOZILLA_INCLUDES)				\
	$(LIBDEVHELP_CFLAGS)

libcppwrapper_la_SOURCES =				\
	dh-gecko-utils.cpp				\
	dh-gecko-utils.h

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

libdevhelp_1_la_CFLAGS = 				\
	$(WARN_CFLAGS)					\
	$(LIBDEVHELP_CFLAGS)				\
	$(MOZILLA_COMPONENT_CFLAGS)

libdevhelp_1_la_LIBADD =				\
	libcppwrapper.la				\
	$(LIBDEVHELP_LIBS)				\
	$(MOZILLA_COMPONENT_LIBS)

libdevhelp_1_la_LDFLAGS = -R$(MOZILLA_HOME)

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
