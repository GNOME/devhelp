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
        libdevhelp.la

devhelpdir=$(libdir)/devhelp

devhelp_LTLIBRARIES =					\
	libdevhelp.la	

libdevhelp_la_SOURCES =					\
	dh-marshal-main.c				\
	dh-base.c					\
	dh-base.h					\
	dh-book-tree.c					\
	dh-book-tree.h					\
	dh-error.c					\
	dh-error.h					\
	dh-history.c                            	\
	dh-history.h    	                        \
	$(html_widget_impl) 				\
	dh-html.h					\
	dh-keyword-model.c				\
	dh-keyword-model.h				\
	dh-link.c					\
	dh-link.h					\
	dh-search.c					\
	dh-search.h					\
	dh-parser.c					\
	dh-parser.h					\
	dh-util.c					\
	dh-util.h					\
	dh-window.c					\
	dh-window.h			

libdevhelp_la_LIBADD =					\
	@LIBDEVHELP_LIBS@				\
	@HTML_WIDGET_LIBS@

libdevhelp_la_LDFLAGS = -avoid-version -module

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
	$(NULL)
