NULL =
INCLUDES = 						  \
	-I$(top_srcdir) 				  \
	-I$(top_builddir) 				  \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	  \
	-DDATA_DIR=\"$(datadir)\"			  \
	-DPREFIX="\"$(prefix)"\"		 	  \
	-DSYSCONFDIR=\""$(sysconfdir)"\"                  \
	-DDATADIR=\""$(datadir)"\"                        \
	-DLIBDIR=\""$(libdir)"\"			  \
	-DDTD_DIR="\"$(datadir)/devhelp/dtd"\"		  \
	-DGLADEDIR=\"$(datadir)/devhelp\"                 \
	@DEVHELP_CFLAGS@ 				  \
	@LIBDEVHELP_CFLAGS@ 		 	  	  \
	$(MOZILLA_COMPONENT_CFLAGS)                       \
	-I$(MOZILLA_INCLUDE_ROOT)/dom                     \
	-I$(MOZILLA_INCLUDE_ROOT)/find                    \
	-I$(MOZILLA_INCLUDE_ROOT)/webbrwsr		  \
	-I$(MOZILLA_INCLUDE_ROOT)/string		  \
	-I$(MOZILLA_INCLUDE_ROOT)/pref                    \
	-I$(MOZILLA_INCLUDE_ROOT)/gfx                     \
	-include $(MOZILLA_INCLUDE_ROOT)/mozilla-config.h \
	$(NULL)

bin_PROGRAMS = devhelp-bin

devhelp_bin_SOURCES = 					\
	dh-main.c

devhelp_bin_LDADD = 					\
        @DEVHELP_LIBS@ 					\
        libdevhelp-1.la

devhelpdir=$(libdir)

devhelphdir = $(includedir)/devhelp-1.0/devhelp
devhelph_HEADERS = 					\
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

lib_LTLIBRARIES =					\
	libdevhelp-1.la	

libdevhelp_1_la_SOURCES =				\
	dh-marshal-main.c				\
	dh-base.c					\
	dh-book-tree.c					\
	dh-error.c					\
	dh-html.c                                       \
	dh-gecko-utils.cpp                              \
	dh-gecko-utils.h                                \
	dh-keyword-model.c				\
	dh-link.c					\
	dh-search.c					\
	dh-parser.c					\
	dh-preferences.c                                \
	dh-util.c					\
	dh-window.c

libdevhelp_1_la_LIBADD =				\
	@LIBDEVHELP_LIBS@				\
	$(MOZILLA_COMPONENT_LIBS)

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
	$(devhelph_HEADERS)			\
	$(NULL)
