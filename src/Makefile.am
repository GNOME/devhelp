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

devhelp_LDFLAGS = $(AM_LDFLAGS)

lib_LTLIBRARIES = libdevhelp-1.la

devhelpincludedir = $(includedir)/devhelp-1.0/devhelp
devhelpinclude_HEADERS = 				\
	dh-base.h					\
	dh-book-tree.h					\
	dh-error.h					\
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
	$(AM_CPPFLAGS)

libdevhelp_1_la_CFLAGS = 				\
	$(WARN_CFLAGS)					\
	$(LIBDEVHELP_CFLAGS)				\
	$(WEBKIT_CFLAGS)					\
	$(AM_CFLAGS)

libdevhelp_1_la_LIBADD =				\
	$(LIBDEVHELP_LIBS)				\
	$(WEBKIT_LIBS)					\
	$(WEBKIT_EXTRA_LIBS)

libdevhelp_1_la_LDFLAGS = $(AM_LDFLAGS)

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
