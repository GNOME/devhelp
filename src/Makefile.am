NULL =
INCLUDES = 						\
	-I$(top_srdir) 					\
	-I$(top_builddir) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DDATA_DIR=\"$(datadir)\"			\
	-DPREFIX="\"$(prefix)"\"			\
        -DSYSCONFDIR=\""$(sysconfdir)"\"                \
        -DDATADIR=\""$(datadir)"\"                      \
        -DLIBDIR=\""$(libdir)"\"			\
	@DEVHELP_CFLAGS@ 				\
	$(NULL)

bin_PROGRAMS = devhelp-2 

devhelp_2_SOURCES = 				\
	dh-main.c

devhelp_2_LDADD = 				\
        @DEVHELP_LIBS@ 				\
        libdevhelp2.la				\
	$(top_builddir)/libegg/menu/libegg-menu.la

devhelp_2dir=$(libdir)/devhelp-2

devhelp_2_LTLIBRARIES =				\
	libdevhelp2.la

libdevhelp2_la_SOURCES =			\
	dh-base.c				\
	dh-base.h				\
	dh-book-parser.c			\
	dh-book-parser.h			\
	dh-book-tree.c				\
	dh-book-tree.h				\
	dh-history.c                            \
	dh-history.h                            \
	dh-html.c				\
	dh-html.h				\
	dh-keyword-model.c			\
	dh-keyword-model.h			\
	dh-link.c				\
	dh-link.h				\
	dh-marshal.c				\
	dh-marshal.h				\
	dh-profile.c				\
	dh-profile.h				\
	dh-search.c				\
	dh-search.h				\
	dh-util.c				\
	dh-util.h				\
	dh-window.c				\
	dh-window.h			

libdevhelp2_la_LIBADD =				\
	@LIBDEVHELP_LIBS@			\
	$(top_builddir)/libegg/menu/libegg-menu.la

libdevhelp2_la_LDFLAGS = -avoid-version -module

noinst_PROGRAMS = test-profile

test_profile_SOURCES = 				\
	test-profile.c

test_profile_LDADD =  				\
	@DEVHELP_LIBS@   			\
	libdevhelp2.la

marshal_sources = 		\
        dh-marshal.h 		\
        dh-marshal.c  	 	\
        $(NULL)

dh-marshal.h: dh-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=dh_marshal > $@
dh-marshal.c: dh-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --body --prefix=dh_marshal > $@

EXTRA_DIST =                            	\
	dh-marshal.list				\
	$(NULL)
