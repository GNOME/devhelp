AM_CPPFLAGS = 						\
	-I$(top_srcdir)					\
	-DLOCALEDIR=\""$(localedir)"\" 	  		\
	-DDATADIR=\""$(datadir)"\"                      \
	-DG_LOG_DOMAIN=\"Devhelp\"			\
	-DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES		\
	$(WARN_CFLAGS)

AM_LDFLAGS =						\
	$(WARN_LDFLAGS)

libdevhelp_public_headers =		\
	devhelp.h			\
	dh-assistant-view.h		\
	dh-book.h			\
	dh-book-manager.h		\
	dh-book-tree.h			\
	dh-keyword-model.h		\
	dh-language.h			\
	dh-link.h			\
	dh-sidebar.h			\
	$(NULL)

libdevhelp_public_c_files =		\
	dh-assistant-view.c		\
	dh-book.c			\
	dh-book-manager.c		\
	dh-book-tree.c			\
	dh-common.c			\
	dh-keyword-model.c		\
	dh-language.c			\
	dh-link.c			\
	dh-sidebar.c			\
	$(NULL)

libdevhelp_private_headers =		\
	dh-error.h			\
	dh-parser.h			\
	dh-preferences.h		\
	dh-settings.h			\
	dh-util.h			\
	$(NULL)

libdevhelp_private_c_files =		\
	dh-error.c			\
	dh-parser.c			\
	dh-preferences.c		\
	dh-util.c			\
	dh-settings.c			\
	$(NULL)

libdevhelp_built_public_headers =	\
	dh-enum-types.h			\
	$(NULL)

libdevhelp_built_public_c_files =	\
	dh-enum-types.c			\
	$(NULL)

libdevhelp_built_sources =			\
	$(libdevhelp_built_public_headers)	\
	$(libdevhelp_built_public_c_files)	\
	dh-resources.c	 			\
	dh-resources.h 				\
	$(NULL)

app_headers =			\
	dh-app.h		\
	dh-assistant.h		\
	dh-window.h		\
	$(NULL)

app_c_files =			\
	dh-app.c		\
	dh-assistant.c		\
	dh-main.c		\
	dh-window.c		\
	$(NULL)

app_built_headers =		\
	dh-enum-types-app.h	\
	$(NULL)

app_built_c_files =		\
	dh-enum-types-app.c	\
	$(NULL)

app_built_sources =		\
	$(app_built_headers)	\
	$(app_built_c_files)	\
	$(NULL)

BUILT_SOURCES = 			\
	$(libdevhelp_built_sources)	\
	$(app_built_sources)		\
	$(NULL)

# The library
lib_LTLIBRARIES = libdevhelp-3.la

devhelpincludedir = $(includedir)/devhelp-3.0/devhelp
devhelpinclude_HEADERS = $(libdevhelp_public_headers)
nodist_devhelpinclude_HEADERS = $(libdevhelp_built_public_headers)

libdevhelp_3_la_SOURCES =		\
	$(libdevhelp_public_headers)	\
	$(libdevhelp_public_c_files)	\
	$(libdevhelp_private_headers)	\
	$(libdevhelp_private_c_files)

# Do not distribute generated files.
nodist_libdevhelp_3_la_SOURCES =	\
	$(libdevhelp_built_sources)

libdevhelp_3_la_CPPFLAGS =	\
	$(AM_CPPFLAGS)		\
	$(ZLIB_CPPFLAGS)

libdevhelp_3_la_CFLAGS =	\
	$(DEVHELP_CFLAGS)

libdevhelp_3_la_LIBADD =	\
	$(DEVHELP_LIBS)		\
	$(ZLIB_LIBS)		\
	$(LIBM)

libdevhelp_3_la_LDFLAGS =								\
	$(AM_LDFLAGS)									\
	-no-undefined									\
	-version-info ${LIBDEVHELP_CURRENT}:${LIBDEVHELP_REVISION}:${LIBDEVHELP_AGE}	\
	-export-symbols-regex ^dh_							\
	$(ZLIB_LDFLAGS)

# The application
bin_PROGRAMS = devhelp

devhelp_SOURCES =	\
	$(app_headers)	\
	$(app_c_files)	\
	$(NULL)

nodist_devhelp_SOURCES =	\
	$(app_built_sources)

devhelp_CPPFLAGS =	\
	$(AM_CPPFLAGS)

devhelp_CFLAGS =		\
	$(DEVHELP_CFLAGS)

devhelp_LDADD = 		\
	$(DEVHELP_LIBS)		\
        libdevhelp-3.la

devhelp_LDFLAGS =	\
	$(AM_LDFLAGS)

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/dh.gresource.xml)
dh-resources.c: dh.gresource.xml $(resource_files)
	glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name dh $(srcdir)/dh.gresource.xml
dh-resources.h: dh.gresource.xml $(resource_files)
	glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name dh $(srcdir)/dh.gresource.xml

dh-enum-types.h: dh-enum-types.h.template $(libdevhelp_public_headers) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template dh-enum-types.h.template $(libdevhelp_public_headers)) > $@

dh-enum-types.c: dh-enum-types.c.template $(libdevhelp_public_headers) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template dh-enum-types.c.template $(libdevhelp_public_headers)) > $@

dh-enum-types-app.h: dh-enum-types-app.h.template $(app_headers) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template dh-enum-types-app.h.template $(app_headers)) > $@

dh-enum-types-app.c: dh-enum-types-app.c.template $(app_headers) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template dh-enum-types-app.c.template $(app_headers)) > $@

EXTRA_DIST =				\
	dh-enum-types.c.template	\
	dh-enum-types.h.template	\
	dh-enum-types-app.c.template	\
	dh-enum-types-app.h.template	\
	dh.gresource.xml		\
	$(resource_files)		\
	$(NULL)

CLEANFILES = $(BUILT_SOURCES)

# GObject Introspection
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
	$(libdevhelp_public_headers)		\
	$(libdevhelp_built_public_c_files)	\
	$(libdevhelp_built_public_headers)

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

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif # HAVE_INTROSPECTION

-include $(top_srcdir)/git.mk
