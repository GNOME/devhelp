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
	bookmark-manager.c			\
	bookmark-manager.h			\
	devhelp-view.c				\
	devhelp-view.h				\
	devhelp-window.c			\
	devhelp-window.h			\
	help-browser.c				\
	help-browser.h				\
	install.c				\
	install.h				\
	main.c					\
	preferences.c				\
	preferences.h				\
	preferences-dialog.c			\
	preferences-dialog.h

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
	book-index.c				\
	book-index.h				\
	book-node.h				\
	books-dialog.c				\
	books-dialog.h                          \
	bookshelf.c				\
	bookshelf.h                     	\
	devhelp-controller.c			\
	devhelp-controller.h			\
	devhelp-marshal.c			\
	devhelp-marshal.h			\
	devhelp-search.c                	\
	devhelp-search.h			\
	function-database.c			\
	function-database.h			\
	history.c                               \
	history.h                               \
	util.c					\
	util.h

libdevhelp2_la_LIBADD =				\
	$(LIBDEVHELP_LIBS)	

libdevhelp2_la_LDFLAGS = -avoid-version -module

gnome_devhelp_idl_sources =          \
	GNOME_DevHelp-stubs.c        \
	GNOME_DevHelp-skels.c        \
	GNOME_DevHelp.h              \
	GNOME_DevHelp-common.c

marshal_sources = \
        devhelp-marshal.h \
        devhelp-marshal-guts.c \
        $(NULL)

devhelp-marshal.h: devhelp-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=devhelp_marshal > $@
devhelp-marshal-guts.c: devhelp-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --body --prefix=devhelp_marshal > $@

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
	devhelp-marshal.list			\
	$(NULL)

BUILT_SOURCES = \
	$(gnome_devhelp_idl_sources)
	$(NULL)

