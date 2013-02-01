dh-marshal.h: dh-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) $< --header --prefix=_dh_marshal dh-marshal.list > $@

dh-marshal.c: dh-marshal.list
	$(AM_V_GEN) echo "#include \"dh-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=_dh_marshal dh-marshal.list >> $@

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/dh.gresource.xml)
dh-resources.c: dh.gresource.xml $(resource_files)
	glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name dh $(srcdir)/dh.gresource.xml
dh-resources.h: dh.gresource.xml $(resource_files)
	glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name dh $(srcdir)/dh.gresource.xml

BUILT_SOURCES = 		\
	dh-marshal.h		\
	dh-marshal.c		\
	dh-enum-types.h		\
	dh-enum-types.c		\
	dh-resources.c	 	\
	dh-resources.h 		\
	$(NULL)

EXTRA_DIST =			\
	dh-marshal.list		\
	dh-enum-types.c.template \
	dh-enum-types.h.template \
	$(resource_files)	\
	dh.gresource.xml	\
	$(NULL)

AM_CPPFLAGS = 						\
	-I$(top_srcdir)					\
	-DLOCALEDIR=\""$(localedir)"\" 	  		\
	-DDATADIR=\""$(datadir)"\"                      \
	-DG_LOG_DOMAIN=\"Devhelp\"			\
	-DG_DISABLE_SINGLE_INCLUDES			\
	-DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES		\
	-DGTK_DISABLE_SINGLE_INCLUDES			\
	$(WARN_CFLAGS)

if WITH_WEBKIT2
AM_CPPFLAGS += -DHAVE_WEBKIT2
endif

bin_PROGRAMS = devhelp

devhelp_SOURCES =					\
	dh-main.c

devhelp_CPPFLAGS =					\
	$(AM_CPPFLAGS)

devhelp_CFLAGS =					\
	$(DEVHELP_CFLAGS)

devhelp_LDADD = 					\
	$(DEVHELP_LIBS)					\
	$(top_builddir)/libgd/libgd.la \
        libdevhelp-3.la

lib_LTLIBRARIES = libdevhelp-3.la

INST_H_FILES = 						\
	devhelp.h					\
	dh-app.h					\
	dh-assistant.h					\
	dh-assistant-view.h				\
	dh-book-manager.h				\
	dh-language.h					\
	dh-book.h					\
	dh-book-tree.h					\
	dh-error.h					\
	dh-keyword-model.h				\
	dh-link.h					\
	dh-search.h					\
	dh-window.h

devhelpincludedir = $(includedir)/devhelp-3.0/devhelp
devhelpinclude_HEADERS = $(INST_H_FILES)

libdevhelp_3_la_SOURCES =				\
	dh-marshal.c					\
	dh-marshal.h					\
	dh-enum-types.c					\
	dh-enum-types.h					\
	dh-app.c					\
	dh-assistant.c					\
	dh-assistant-view.c				\
	dh-book-manager.c				\
	dh-common.c						\
	dh-language.c					\
	dh-book.c					\
	dh-book-tree.c					\
	dh-error.c					\
	dh-keyword-model.c				\
	dh-link.c					\
	dh-search.c					\
	dh-parser.c					\
	dh-preferences.c				\
	dh-util.c					\
	dh-window.c					\
	dh-parser.h					\
	dh-preferences.h                                \
	dh-util.h					\
	dh-settings.c					\
	dh-settings.h					\
	dh-resources.c				 	\
	dh-resources.h 					\
	eggfindbar.c					\
	eggfindbar.h

libdevhelp_3_la_CPPFLAGS =				\
	$(AM_CPPFLAGS)					\
	$(ZLIB_CPPFLAGS)

libdevhelp_3_la_CFLAGS =				\
	-I$(top_srcdir)/libgd \
	$(top_builddir)/libgd/libgd.la \
	$(LIBDEVHELP_CFLAGS)

libdevhelp_3_la_LIBADD =				\
	$(LIBDEVHELP_LIBS)				\
	$(ZLIB_LIBS)					\
	$(LIBM)

libdevhelp_3_la_LDFLAGS =				\
	-no-undefined					\
	-version-info ${LIBDEVHELP_CURRENT}:${LIBDEVHELP_REVISION}:${LIBDEVHELP_AGE} \
	-export-symbols-regex ^dh_ \
	$(ZLIB_LDFLAGS)

dh-enum-types.h: dh-enum-types.h.template $(INST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template dh-enum-types.h.template $(INST_H_FILES)) > $@

dh-enum-types.c: dh-enum-types.c.template $(INST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template dh-enum-types.c.template $(INST_H_FILES)) > $@

if HAVE_PLATFORM_OSX
libdevhelp_3_la_CPPFLAGS += -xobjective-c
endif
