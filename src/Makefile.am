NULL =
INCLUDES = 						\
	-I$(top_srcdir) 				\
	-I$(top_builddir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DDATA_DIR=\"$(datadir)\"			\
	-DPREFIX="\"$(prefix)"\"			\
	-DSYSCONFDIR=\""$(sysconfdir)"\"                \
	-DDATADIR=\""$(datadir)"\"                      \
	-DLIBDIR=\""$(libdir)"\"			\
	-DDTD_DIR="\"$(datadir)/devhelp/dtd"\"		\
	@DEVHELP_CFLAGS@ 				\
	@LIBDEVHELP_CFLAGS@ 				\
	$(NULL)

if HTML_WIDGET_MOZILLA
html_widget_impl=dh-html-mozilla.c
else
html_widget_impl=dh-html-gtkhtml2.c
endif

bin_PROGRAMS = devhelp

devhelp_SOURCES = 					\
	dh-main.c

devhelp_LDADD = 					\
        @DEVHELP_LIBS@ 					\
        libdevhelp-1.la					\
	$(top_builddir)/libegg/menu/libegg-menu.la

devhelpdir=$(libdir)

devhelphdir = $(includedir)/devhelp-1.0/devhelp
devhelph_HEADERS = 					\
	dh-base.h					\
	dh-book-tree.h					\
	dh-error.h					\
	dh-history.h    	                        \
	dh-html.h					\
	dh-keyword-model.h				\
	dh-link.h					\
	dh-search.h					\
	dh-parser.h					\
	dh-util.h					\
	dh-window.h

lib_LTLIBRARIES =					\
	libdevhelp-1.la	

libdevhelp_1_la_SOURCES =				\
	dh-marshal-main.c				\
	dh-base.c					\
	dh-book-tree.c					\
	dh-error.c					\
	dh-history.c                            	\
	$(html_widget_impl) 				\
	dh-keyword-model.c				\
	dh-link.c					\
	dh-search.c					\
	dh-parser.c					\
	dh-util.c					\
	dh-window.c

libdevhelp_1_la_LIBADD =				\
	@LIBDEVHELP_LIBS@				\
	@HTML_WIDGET_LIBS@				\
	$(top_builddir)/libegg/menu/libegg-menu.la

dh-marshal-main.c: dh-marshal.c dh-marshal.h

dh-marshal.h: dh-marshal.list
	cd $(srcdir) && \
	( @GLIB_GENMARSHAL@ --prefix=dh_marshal dh-marshal.list --header > dh-marshal1.tmp \
	&& mv dh-marshal1.tmp dh-marshal.h ) \
	|| ( rm -f dh-marshal1.tmp && exit 1 )

dh-marshal.c: dh-marshal.h
	cd $(srcdir) && \
	( @GLIB_GENMARSHAL@ --prefix=dh_marshal dh-marshal.list --body > dh-marshal2.tmp \
	&& mv dh-marshal2.tmp dh-marshal.c ) \
	|| ( rm -f dh-marshal2.tmp && exit 1 )

EXTRA_DIST =                            	\
	dh-marshal.list				\
	dh-html-mozilla.c			\
	dh-html-gtkhtml2.c			\
	$(devhelph_HEADERS)			\
	$(NULL)
