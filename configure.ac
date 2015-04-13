AC_PREREQ([2.64])
AC_INIT([Devhelp],
        [3.16.2],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=devhelp],
        [devhelp],
        [https://wiki.gnome.org/Apps/Devhelp])

# LT Version numbers, remember to change them just *before* a release.
#   (Interfaces removed:    CURRENT++, AGE=0, REVISION=0)
#   (Interfaces added:      CURRENT++, AGE++, REVISION=0)
#   (No interfaces changed:                   REVISION++)
LIBDEVHELP_CURRENT=2
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

GLIB_GSETTINGS

PKG_PROG_PKG_CONFIG

GNOME_COMPILE_WARNINGS([maximum])

AC_CHECK_LIBM
AC_SUBST(LIBM)

# Pick from supported API versions of WebKit2
PKG_CHECK_EXISTS([webkit2gtk-4.0], [
	WEBKIT_PKGCONFIG=webkit2gtk-4.0
], [
	WEBKIT_PKGCONFIG=webkit2gtk-3.0
])
AC_SUBST(WEBKIT_PKGCONFIG)

PKG_CHECK_MODULES(DEVHELP, [
  gthread-2.0 >= 2.10.0
  gtk+-3.0 >= 3.13.4
  $WEBKIT_PKGCONFIG >= 2.0.0
  gio-2.0 >= 2.37.3
])

PKG_CHECK_MODULES(LIBDEVHELP, [
  gtk+-3.0 >= 3.12.0
  gio-2.0 >= 2.37.3
  $WEBKIT_PKGCONFIG >= 2.0.0
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

GETTEXT_PACKAGE=devhelp
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Package name for gettext])

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
data/dtd/Makefile
misc/Makefile
misc/gedit-plugin/Makefile
contrib/Makefile
])

AC_OUTPUT

echo
echo "Prefix:    $prefix"
echo
