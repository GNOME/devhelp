INCLUDES = 						  \
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
	-I$(MOZILLA_INCLUDE_ROOT)	                  \
	-I$(MOZILLA_INCLUDE_ROOT)/content		  \
	-I$(MOZILLA_INCLUDE_ROOT)/dom                     \
	-I$(MOZILLA_INCLUDE_ROOT)/find                    \
	-I$(MOZILLA_INCLUDE_ROOT)/locale		\
	-I$(MOZILLA_INCLUDE_ROOT)/webbrwsr		  \
	-I$(MOZILLA_INCLUDE_ROOT)/string		  \
	-I$(MOZILLA_INCLUDE_ROOT)/pref                    \
	-I$(MOZILLA_INCLUDE_ROOT)/gfx                     \
	-DALLOW_PRIVATE_API                               \
	-DMOZILLA_HOME=\""$(MOZILLA_HOME)\""              \
	-include $(MOZILLA_INCLUDE_ROOT)/mozilla-config.h 

bin_PROGRAMS = devhelp

devhelp_SOURCES = 					\
	dh-main.c

devhelp_LDADD = 					\
        @DEVHELP_LIBS@ 					\
        libdevhelp-1.la

devhelp_LDFLAGS =                                       \
	-R$(MOZILLA_HOME)

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
	bacon-message-connection.c                      \
	bacon-message-connection.h                      \
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

BUILT_SOURCES =                                 \
	dh-marshal.h                            \
	dh-marshal.c

EXTRA_DIST =                            	\
	dh-marshal.list				\
	$(devhelph_HEADERS)

CLEANFILES = $(BUILT_SOURCES)
