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
	$(DEVHELP_CFLAGS) 				\
	$(NULL)

bin_PROGRAMS = devhelp-2

devhelp_2_SOURCES = 				\
	$(gnome_devhelp_idl_sources)		\
	dh-view.c				\
	dh-view.h				\
	dh-window.c				\
	dh-window.h				\
	help-browser.c				\
	help-browser.h				\
	main.c

devhelp_2_LDADD = 				\
	      $(DEVHELP_LIBS) 			\
              libdevhelp2.la

devhelp_2dir=$(libdir)/devhelp-2

devhelp_2_LTLIBRARIES =				\
	libdevhelp2.la

libdevhelp2_la_SOURCES =			\
	$(gnome_devhelp_idl_sources)		\
	book.c					\
	book.h					\
	book-node.h				\
	dh-book-parser.c			\
	dh-book-parser.h			\
	dh-book-tree.c				\
	dh-book-tree.h				\
	dh-bookshelf.c				\
	dh-bookshelf.h                     	\
	dh-controller.c				\
	dh-controller.h				\
	dh-marshal.c				\
	dh-marshal.h				\
	dh-profile.c				\
	dh-profile.h				\
	dh-search.c     	           	\
	dh-search.h				\
	function-database.c			\
	function-database.h			\
	dh-history.c                            \
	dh-history.h                            \
	dh-util.c				\
	dh-util.h

libdevhelp2_la_LIBADD =				\
	$(LIBDEVHELP_LIBS)	

libdevhelp2_la_LDFLAGS = -avoid-version -module

gnome_devhelp_idl_sources =          \
	GNOME_DevHelp-stubs.c        \
	GNOME_DevHelp-skels.c        \
	GNOME_DevHelp.h              \
	GNOME_DevHelp-common.c

marshal_sources = 		\
        dh-marshal.h 		\
        dh-marshal.c  	 	\
        $(NULL)

dh-marshal.h: dh-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=dh_marshal > $@
dh-marshal.c: dh-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --body --prefix=dh_marshal > $@

$(gnome_devhelp_idl_sources): gnome_devhelp_idl_stamp
gnome_devhelp_idl_stamp: $(top_srcdir)/idl/GNOME_DevHelp.idl $(ORBIT_IDL)
	$(ORBIT_IDL) $(DEVHELP_IDL_INCLUDES) -I$(top_srcdir) $<
	touch $@

$(devhelp_OBJECTS): gnome_devhelp_idl_stamp
$(libdevhelp2_la_OBJECTS): gnome_devhelp_idl_stamp $(marshal_sources)

serverdir = $(libdir)/bonobo/servers
server_in_files = GNOME_DevHelp.server.in
server_DATA = $(server_in_files:.server.in=.server)

GNOME_DevHelp.server.in: $(srcdir)/GNOME_DevHelp.server.in.in $(top_builddir)/config.status
	sed -e "s|\@COMPONENT_LIBDIR\@|$(devhelp_2dir)|" \
	$(srcdir)/GNOME_DevHelp.server.in.in > GNOME_DevHelp.server.in

@INTLTOOL_SERVER_RULE@

CLEANFILES  = 					\
	$(gnome_devhelp_idl_sources)		\
	gnome_devhelp_idl_stamp			\
	$(NULL)

EXTRA_DIST =                            	\
	$(server_in_files)			\
	$(server_DATA)				\
	dh-marshal.list			\
	$(NULL)

BUILT_SOURCES = \
	$(gnome_devhelp_idl_sources)
	$(NULL)

