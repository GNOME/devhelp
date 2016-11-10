
resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/dh.gresource.xml)
dh-resources.c: dh.gresource.xml $(resource_files)
	glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name dh $(srcdir)/dh.gresource.xml
dh-resources.h: dh.gresource.xml $(resource_files)
	glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name dh $(srcdir)/dh.gresource.xml

BUILT_SOURCES = 		\
	dh-enum-types.c		\
	dh-enum-types.h		\
	dh-resources.c	 	\
	dh-resources.h 		\
	$(NULL)

EXTRA_DIST =				\
	dh-enum-types.c.template	\
	dh-enum-types.h.template	\
	dh.gresource.xml		\
	$(resource_files)		\
	$(NULL)

AM_CPPFLAGS = 						\
	-I$(top_srcdir)					\
	-DLOCALEDIR=\""$(localedir)"\" 	  		\
	-DDATADIR=\""$(datadir)"\"                      \
	-DG_LOG_DOMAIN=\"Devhelp\"			\
	-DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES		\
	$(WARN_CFLAGS)

AM_LDFLAGS =						\
	$(WARN_LDFLAGS)

bin_PROGRAMS = devhelp

devhelp_SOURCES =					\
	dh-main.c

devhelp_CPPFLAGS =					\
	$(AM_CPPFLAGS)

devhelp_CFLAGS =					\
	$(DEVHELP_CFLAGS)

devhelp_LDADD = 					\
	$(DEVHELP_LIBS)					\
        libdevhelp-3.la

devhelp_LDFLAGS =					\
	$(AM_LDFLAGS)

lib_LTLIBRARIES = libdevhelp-3.la

libdevhelp_public_headers =		\
	devhelp.h			\
	dh-app.h			\
	dh-assistant.h			\
	dh-assistant-view.h		\
	dh-book.h			\
	dh-book-manager.h		\
	dh-book-tree.h			\
	dh-error.h			\
	dh-keyword-model.h		\
	dh-language.h			\
	dh-link.h			\
	dh-sidebar.h			\
	dh-window.h

libdevhelp_public_c_files =		\
	dh-app.c			\
	dh-assistant.c			\
	dh-assistant-view.c		\
	dh-book.c			\
	dh-book-manager.c		\
	dh-book-tree.c			\
	dh-common.c			\
	dh-error.c			\
	dh-keyword-model.c		\
	dh-language.c			\
	dh-link.c			\
	dh-sidebar.c			\
	dh-window.c

libdevhelp_private_headers =		\
	dh-parser.h			\
	dh-preferences.h		\
	dh-settings.h			\
	dh-util.h

libdevhelp_private_c_files =		\
	dh-parser.c			\
	dh-preferences.c		\
	dh-util.c			\
	dh-settings.c

devhelpincludedir = $(includedir)/devhelp-3.0/devhelp
devhelpinclude_HEADERS = $(libdevhelp_public_headers)

libdevhelp_3_la_SOURCES =			\
	$(libdevhelp_public_headers)		\
	$(libdevhelp_public_c_files)		\
	$(libdevhelp_private_headers)		\
	$(libdevhelp_private_c_files)

# do not distribute generated files
nodist_libdevhelp_3_la_SOURCES =			\
	$(BUILT_SOURCES)

libdevhelp_3_la_CPPFLAGS =				\
	$(AM_CPPFLAGS)					\
	$(ZLIB_CPPFLAGS)

libdevhelp_3_la_CFLAGS =				\
	$(DEVHELP_CFLAGS)

libdevhelp_3_la_LIBADD =				\
	$(DEVHELP_LIBS)					\
	$(ZLIB_LIBS)					\
	$(LIBM)

libdevhelp_3_la_LDFLAGS =				\
	$(AM_LDFLAGS)					\
	-no-undefined					\
	-version-info ${LIBDEVHELP_CURRENT}:${LIBDEVHELP_REVISION}:${LIBDEVHELP_AGE} \
	-export-symbols-regex ^dh_ \
	$(ZLIB_LDFLAGS)

dh-enum-types.h: dh-enum-types.h.template $(libdevhelp_public_headers) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template dh-enum-types.h.template $(libdevhelp_public_headers)) > $@

dh-enum-types.c: dh-enum-types.c.template $(libdevhelp_public_headers) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template dh-enum-types.c.template $(libdevhelp_public_headers)) > $@

DISTCLEANFILES = $(BUILT_SOURCES)

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS =			\
	--add-include-path=$(srcdir)		\
	--warn-all				\
	--identifier-prefix Dh			\
	--identifier-prefix dh			\
	--pkg-export libdevhelp-3.0		\
	--c-include="devhelp/devhelp.h"

INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
introspection_sources = 			\
	$(libdevhelp_public_c_files)		\
	$(libdevhelp_public_headers)

Devhelp-3.0.gir: libdevhelp-3.la
Devhelp_3_0_gir_INCLUDES = Gtk-3.0 WebKit2-4.0
Devhelp_3_0_gir_CFLAGS = $(INCLUDES)
Devhelp_3_0_gir_LIBS = libdevhelp-3.la
Devhelp_3_0_gir_FILES = $(introspection_sources)
Devhelp_3_0_gir_SCANNERFLAGS = $(WARN_SCANNERFLAGS)
INTROSPECTION_GIRS += Devhelp-3.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

DISTCLEANFILES += $(gir_DATA) $(typelib_DATA)
endif
