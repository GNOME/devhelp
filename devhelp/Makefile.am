AM_CPPFLAGS = 				\
	-I$(top_srcdir)			\
	-DLOCALEDIR=\""$(localedir)"\" 	\
	-DDATADIR=\""$(datadir)"\"      \
	-DG_LOG_DOMAIN=\"Devhelp\"	\
	$(WARN_CFLAGS)

AM_LDFLAGS =		\
	$(WARN_LDFLAGS)

libdevhelp_public_headers =		\
	devhelp.h			\
	dh-assistant-view.h		\
	dh-book.h			\
	dh-book-manager.h		\
	dh-book-tree.h			\
	dh-completion.h			\
	dh-init.h			\
	dh-keyword-model.h		\
	dh-link.h			\
	dh-sidebar.h			\
	$(NULL)

libdevhelp_public_c_files =		\
	dh-assistant-view.c		\
	dh-book.c			\
	dh-book-manager.c		\
	dh-book-tree.c			\
	dh-completion.c			\
	dh-init.c			\
	dh-keyword-model.c		\
	dh-link.c			\
	dh-sidebar.c			\
	$(NULL)

libdevhelp_private_headers =		\
	dh-error.h			\
	dh-parser.h			\
	dh-search-context.h		\
	dh-settings.h			\
	dh-util-lib.h			\
	$(NULL)

libdevhelp_private_c_files =		\
	dh-error.c			\
	dh-parser.c			\
	dh-search-context.c		\
	dh-settings.c			\
	dh-util-lib.c			\
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
	$(NULL)

BUILT_SOURCES = 			\
	$(libdevhelp_built_sources)	\
	$(NULL)

# Helper Libtool library, so that the private functions can be used in unit
# tests.
noinst_LTLIBRARIES = libdevhelp-core.la

libdevhelp_core_la_SOURCES =		\
	$(libdevhelp_public_headers)	\
	$(libdevhelp_public_c_files)	\
	$(libdevhelp_private_headers)	\
	$(libdevhelp_private_c_files)

# Do not distribute generated files.
nodist_libdevhelp_core_la_SOURCES =	\
	$(libdevhelp_built_sources)

libdevhelp_core_la_CPPFLAGS =	\
	$(AM_CPPFLAGS)

libdevhelp_core_la_CFLAGS =	\
	$(DEVHELP_CFLAGS)

libdevhelp_core_la_LDFLAGS =	\
	$(AM_LDFLAGS)		\
	-no-undefined

# The library
lib_LTLIBRARIES = libdevhelp-3.la

devhelpincludedir = $(includedir)/devhelp-3.0/devhelp
devhelpinclude_HEADERS = $(libdevhelp_public_headers)
nodist_devhelpinclude_HEADERS = $(libdevhelp_built_public_headers)

libdevhelp_3_la_SOURCES =

libdevhelp_3_la_LIBADD =	\
	libdevhelp-core.la	\
	$(DEVHELP_LIBS)

libdevhelp_3_la_LDFLAGS =			\
	$(AM_LDFLAGS)				\
	-no-undefined				\
	-version-info $(LIBDEVHELP_LT_VERSION)	\
	-export-symbols-regex ^dh_

dh-enum-types.h: dh-enum-types.h.template $(libdevhelp_public_headers) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template dh-enum-types.h.template $(libdevhelp_public_headers)) > $@

dh-enum-types.c: dh-enum-types.c.template $(libdevhelp_public_headers) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template dh-enum-types.c.template $(libdevhelp_public_headers)) > $@

EXTRA_DIST =				\
	dh-enum-types.c.template	\
	dh-enum-types.h.template	\
	$(NULL)

CLEANFILES = $(BUILT_SOURCES)

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = Devhelp-3.0.gir

INTROSPECTION_SCANNER_ENV = CC="$(CC)"

Devhelp-3.0.gir: libdevhelp-3.la $(BUILT_SOURCES)
INTROSPECTION_SCANNER_ARGS =		\
	-I$(top_srcdir)			\
	-I$(top_builddir)		\
	--identifier-prefix Dh		\
	--identifier-prefix dh		\
	--c-include=devhelp/devhelp.h	\
	--warn-all
Devhelp_3_0_gir_CFLAGS = $(DEVHELP_CFLAGS)
Devhelp_3_0_gir_SCANNERFLAGS = $(WARN_SCANNERFLAGS)
Devhelp_3_0_gir_LIBS = libdevhelp-3.la
Devhelp_3_0_gir_FILES =				\
	$(libdevhelp_public_headers)		\
	$(libdevhelp_public_c_files)		\
	$(libdevhelp_built_public_headers)	\
	$(libdevhelp_built_public_c_files)

Devhelp_3_0_gir_INCLUDES = Gtk-3.0 WebKit2-4.0
Devhelp_3_0_gir_PACKAGES = gtk+-3.0 webkit2gtk-4.0
Devhelp_3_0_gir_EXPORT_PACKAGES = libdevhelp-3.0

girdir = $(datadir)/gir-1.0
gir_DATA = Devhelp-3.0.gir

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = Devhelp-3.0.typelib

CLEANFILES +=		\
	$(gir_DATA)	\
	$(typelib_DATA)

endif # HAVE_INTROSPECTION

-include $(top_srcdir)/git.mk
