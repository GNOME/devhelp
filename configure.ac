AC_PREREQ([2.64])
AC_INIT([Devhelp],
        [3.6.1],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=devhelp],
        [devhelp],
        [http://live.gnome.org/devhelp])

# LT Version numbers, remember to change them just *before* a release.
#   (Interfaces removed:    CURRENT++, AGE=0, REVISION=0)
#   (Interfaces added:      CURRENT++, AGE++, REVISION=0)
#   (No interfaces changed:                   REVISION++)
LIBDEVHELP_CURRENT=1
LIBDEVHELP_AGE=0
LIBDEVHELP_REVISION=0

AC_SUBST(LIBDEVHELP_CURRENT)
AC_SUBST(LIBDEVHELP_AGE)
AC_SUBST(LIBDEVHELP_REVISION)

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR(src/dh-main.c)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 -Wall foreign no-define tar-ustar no-dist-gzip dist-xz])

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])


PKG_PROG_PKG_CONFIG
AM_GCONF_SOURCE_2

GNOME_COMPILE_WARNINGS([maximum])

dnl -----------------------------------------------------------
dnl Platform (GTK+ X11 or GTK OS X)
dnl -----------------------------------------------------------
IGE_PLATFORM_CHECK
if test "x$IGE_PLATFORM" = xx11; then
   gconf_pkgconfig="gconf-2.0 >= 2.6.0"
   gtkmacintegration_pkgconfig=
else
   gconf_pkgconfig=
   gtkmacintegration_pkgconfig="gtk-mac-integration >= 1.0.0"
fi
AM_CONDITIONAL(GCONF_SCHEMAS_INSTALL, test "x$IGE_PLATFORM" = xx11)

AC_ARG_WITH(webkit2,
        [AC_HELP_STRING([--with-webkit2], [build with WebKit2 [default=yes]])],
        [],[with_webkit2=yes])

if test "x$with_webkit2" != xno; then
   WEBKITGTK_PC_NAME=webkit2gtk-3.0
   WEBKITGTK_REQUIRED_VERSION=1.10.0
   AC_DEFINE([HAVE_WEBKIT2],[1],[Define if building with WebKit2])
else
   WEBKITGTK_PC_NAME=webkitgtk-3.0
   WEBKITGTK_REQUIRED_VERSION=1.6.0
fi
AM_CONDITIONAL(WITH_WEBKIT2, test "x$with_webkit2" != xno)

PKG_CHECK_MODULES(DEVHELP, [
  gthread-2.0 >= 2.10.0
  gtk+-3.0 >= 3.4
  $WEBKITGTK_PC_NAME >= $WEBKITGTK_REQUIRED_VERSION
  gio-2.0 >= 2.32
])

PKG_CHECK_MODULES(LIBDEVHELP, [
  gtk+-3.0 >= 3.4
  gio-2.0 >= 2.32
  $gconf_pkgconfig
  $gtkmacintegration_pkgconfig
  $WEBKITGTK_PC_NAME >= $WEBKITGTK_REQUIRED_VERSION
])

AC_ARG_WITH(zlib, [  --with-zlib=DIR         use zlib in DIR], zlibdir=$with_zlib)
have_zlib=no
if test "x$zlibdir" != x; then
    ZLIB_CPPFLAGS=-I$zlibdir/include
    ZLIB_LDFLAGS=-L$zlibdir/lib
fi
save_CPPFLAGS="$CPPFLAGS"
save_LDFLAGS="$LDFLAGS"
CPPFLAGS="$CPPFLAGS $ZLIB_CPPFLAGS"
LDFLAGS="$LDFLAGS $ZLIB_LDFLAGS"
AC_CHECK_HEADERS(zlib.h, AC_CHECK_LIB(z, gzread, have_zlib=yes))
CPPFLAGS="$save_CPPFLAGS"
LDFLAGS="$save_LDFLAGS"
ZLIB_LIBS=-lz
AC_SUBST(ZLIB_CFLAGS)
AC_SUBST(ZLIB_LDFLAGS)
AC_SUBST(ZLIB_LIBS)

# i18n stuff
IT_PROG_INTLTOOL([0.40.0])

AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE=devhelp
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Package name for gettext])

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

AM_PATH_PYTHON([2.3],[],[no])

AC_CONFIG_FILES([
Makefile
po/Makefile.in
src/Makefile
data/Makefile
data/libdevhelp-3.0.pc
data/devhelp.desktop.in
data/icons/Makefile
data/ui/Makefile
data/dtd/Makefile
misc/Makefile
misc/gedit-plugin/Makefile
contrib/Makefile
])

AC_OUTPUT

echo
echo "Prefix:    $prefix"
echo "Platform:  $IGE_PLATFORM_NAME"
echo "WebKit2:   $with_webkit2"
echo
