INCLUDES = 						\
	  -I$(top_srdir) 				\
	  -I$(top_builddir) 				\
          -DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	  -DDATA_DIR=\"$(datadir)\"			\
	  $(GNOMEUI_CFLAGS) 				\
	  $(GCONF_CFLAGS) 				\
	  $(BONOBO_CFLAGS) 				\
	  $(LIBXML_CFLAGS) 				\
	  $(GTKHTML_CFLAGS) 				\
	  $(VFS_CFLAGS)					\
	  $(LIBGLADE_CFLAGS)


bin_PROGRAMS = devhelp

devhelp_SOURCES = 			\
	$(GENERATED)                    \
	bookmark-manager.c		\
	bookmark-manager.h		\
	devhelp-window.c		\
	devhelp-window.h		\
	help-browser.c			\
	help-browser.h			\
	history.c			\
	history.h			\
	html-widget.c			\
	html-widget.h			\
	install.c			\
	install.h			\
	main.c				\
	preferences.c			\
	preferences.h			\
	preferences-dialog.c		\
	preferences-dialog.h		\
	books-dialog.c			\
	books-dialog.h

devhelp_LDADD = 			\
	      $(GNOMEUI_LIBS) 		\
	      $(GCONF_LIBS) 		\
	      $(BONOBO_LIBS)            \
	      $(VFS_LIBS)		\
	      $(GTKHTML_LIBS) 		\
	      -lgtkhtml 		\
              libdevhelp.la

devhelpdir=$(libdir)

devhelp_LTLIBRARIES =			\
	libdevhelp.la

libdevhelp_la_SOURCES =			\
	$(GENERATED)                    \
	book.c				\
	book.h				\
	book-index.c			\
	book-index.h			\
	book-node.h			\
	bookshelf.c			\
	bookshelf.h                     \
	devhelp-controller.c		\
	devhelp-controller.h		\
	devhelp-search.c                \
	devhelp-search.h		\
	function-database.c		\
	function-database.h		\
	util.c				\
	util.h

libdevhelp_la_LIBADD =			\
	$(BONOBO_LIBS)	

GENERATED =				\
	GNOME_DevHelp-skels.c		\
	GNOME_DevHelp-stubs.c		\
	GNOME_DevHelp-common.c		\
	GNOME_DevHelp.h

$(GENERATED): $(top_srcdir)/idl/GNOME_DevHelp.idl stamp-idl

stamp-idl: $(top_srcdir)/idl/GNOME_DevHelp.idl
	orbit-idl $(IDL_FLAGS) -I$(srcdir) $(IDL_FLAGS) $(top_srcdir)/idl/GNOME_DevHelp.idl
	touch stamp-idl

# Work around automake brokenness.
$(srcdir)/GNOME_DevHelp.h: stamp-idl

oafdir = $(datadir)/oaf
oaf_in_files = GNOME_DevHelp.oaf.in

oaf_DATA = $(oaf_in_files:.oaf.in=.oaf)

GNOME_DevHelp.oaf.in: $(srcdir)/GNOME_DevHelp.oaf.in.in $(top_builddir)/config.status
	sed -e "s|\@COMPONENT_LIBDIR\@|$(devhelpdir)|" \
	 $(srcdir)/GNOME_DevHelp.oaf.in.in > GNOME_DevHelp.oaf.in

CLEANFILES  += $(GENERATED) + stamp-idl GNOME_DevHelp.oaf GNOME_DevHelp.oaf.in

@XML_I18N_MERGE_OAF_RULE@

EXTRA_DIST =                            \
        $(oaf_in_files)			\
	GNOME_DevHelp.oaf.in.in
