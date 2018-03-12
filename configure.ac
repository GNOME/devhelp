AC_PREREQ([2.64])
AC_INIT([Devhelp],
        [3.28.1],
        [https://bugzilla.gnome.org/browse.cgi?product=devhelp],
        [devhelp],
        [https://wiki.gnome.org/Apps/Devhelp])

AX_REQUIRE_DEFINED([AX_IS_RELEASE])
AX_IS_RELEASE([git-directory])

# LT Version numbers, remember to change them just *before* a release.
# A good time to update it is for the GNOME x.y.90 version, at API freeze,
# so that packagers have the time to update the package.
#   (Interfaces removed:    CURRENT++, AGE=0, REVISION=0)
#   (Interfaces added:      CURRENT++, AGE++, REVISION=0)
#   (No interfaces changed:                   REVISION++)
LIBDEVHELP_LT_CURRENT=5
LIBDEVHELP_LT_REVISION=0
LIBDEVHELP_LT_AGE=0
LIBDEVHELP_LT_VERSION="$LIBDEVHELP_LT_CURRENT:$LIBDEVHELP_LT_REVISION:$LIBDEVHELP_LT_AGE"
AC_SUBST([LIBDEVHELP_LT_VERSION])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR(src/dh-main.c)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.14 -Wall foreign no-define tar-ustar no-dist-gzip dist-xz -Wno-portability])

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AC_PATH_PROG([GLIB_MKENUMS], [glib-mkenums])
AC_PATH_PROG([GLIB_COMPILE_RESOURCES], [glib-compile-resources])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

AX_REQUIRE_DEFINED([APPSTREAM_XML])
APPSTREAM_XML

AX_REQUIRE_DEFINED([GLIB_GSETTINGS])
GLIB_GSETTINGS

AX_REQUIRE_DEFINED([GOBJECT_INTROSPECTION_CHECK])
GOBJECT_INTROSPECTION_CHECK([1.30.0])

AX_REQUIRE_DEFINED([YELP_HELP_INIT])
YELP_HELP_INIT

# Compile warnings. There can be deprecation warnings at any time, so disable
# -Werror by default with the [yes] argument.
AX_REQUIRE_DEFINED([AX_COMPILER_FLAGS])
AX_COMPILER_FLAGS([WARN_CFLAGS], [WARN_LDFLAGS], [yes])

LT_LIB_M

AX_REQUIRE_DEFINED([AX_PKG_CHECK_MODULES])
AX_PKG_CHECK_MODULES([DEVHELP],
		     [gio-2.0 >= 2.40  gtk+-3.0 >= 3.22  webkit2gtk-4.0 >= 2.19.2],
		     [gsettings-desktop-schemas])

# i18n stuff
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.7])

GETTEXT_PACKAGE=devhelp
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Package name for gettext])

AM_PATH_PYTHON([2.3],[],[no])

# Flatpak
AC_ARG_ENABLE([flatpak-build],
	      [AS_HELP_STRING([--enable-flatpak-build], [Enable Flatpak build mode])],
	      [flatpak_build=${enableval}],
	      [flatpak_build=no])

if test "x${flatpak_build}" = xyes; then
	AC_DEFINE([FLATPAK_BUILD], 1, [Whether Flatpak build mode is enabled])
fi

# GTK-Doc
AX_REQUIRE_DEFINED([GTK_DOC_CHECK])
GTK_DOC_CHECK([1.25],[--flavour no-tmpl])

AX_REQUIRE_DEFINED([PKG_CHECK_VAR])
PKG_CHECK_VAR([WEBKITGTK_PREFIX], [webkit2gtk-4.0], [prefix])

AC_CONFIG_FILES([
	Makefile
	data/libdevhelp-3.0.pc
	data/Makefile
	data/icons/Makefile
	devhelp/Makefile
	docs/Makefile
	docs/reference/Makefile
	help/Makefile
	plugins/Makefile
	plugins/gedit-plugin/Makefile
	po/Makefile.in
	src/Makefile
	unit-tests/Makefile
])

AC_OUTPUT

# Print a summary of the configuration
echo "
Configuration:

	${PACKAGE_NAME} version ${PACKAGE_VERSION}

	Source code location:	${srcdir}
	Prefix:			${prefix}
	Compiler:		${CC}
	API documentation:	${enable_gtk_doc}
	GObject introspection:	${found_introspection}
	Flatpak build mode:	${flatpak_build}
"
