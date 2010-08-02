AC_PREREQ(2.60)
AC_INIT([Devhelp],[2.31.90],[http://bugzilla.gnome.org/enter_bug.cgi?product=devhelp],[devhelp])

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
AM_INIT_AUTOMAKE([1.9 no-define -Wall])
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O

IT_PROG_INTLTOOL([0.40.0])

PKG_PROG_PKG_CONFIG
AM_GCONF_SOURCE_2
GNOME_COMPILE_WARNINGS

dnl -----------------------------------------------------------
dnl Platform (GTK+ X11 or GTK OS X)
dnl -----------------------------------------------------------
IGE_PLATFORM_CHECK
if test "x$IGE_PLATFORM" = xx11; then
   gconf_pkgconfig="gconf-2.0 >= 2.6.0"
   libwnck_pkgconfig="libwnck-1.0 >= 2.10.0"
   igemacintegration_pkgconfig=
   REQUIRES_LIBWNCK=libwnck-1.0
   PLATFORM_CFLAGS=
   PLATFORM_LDFLAGS=
else
   gconf_pkgconfig=
   libwnck_pkgconfig=
   igemacintegration_pkgconfig="ige-mac-integration >= 0.8.2"
   REQUIRES_LIBWNCK=
   PLATFORM_CFLAGS=
   PLATFORM_LDFLAGS=
fi
AM_CONDITIONAL(GCONF_SCHEMAS_INSTALL, test "x$IGE_PLATFORM" = xx11)
AC_SUBST(PLATFORM_CFLAGS)
AC_SUBST(PLATFORM_LDFLAGS)
AC_SUBST(REQUIRES_LIBWNCK)

PKG_CHECK_MODULES(DEVHELP, [
  gthread-2.0 >= 2.10.0
  gtk+-2.0 >= 2.10.0
  webkit-1.0 >= 1.1.13
  glib-2.0 >= 2.25.11
])

PKG_CHECK_MODULES(LIBDEVHELP, [
  gtk+-2.0 >= 2.10.0
  $gconf_pkgconfig
  $libwnck_pkgconfig
  $igemacintegration_pkgconfig
  webkit-1.0 >= 1.1.13
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

GETTEXT_PACKAGE=devhelp
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Package name for gettext])
AM_GLIB_GNU_GETTEXT

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

AM_PATH_PYTHON([2.3],[],[no])

AC_CONFIG_FILES([
Makefile
po/Makefile.in
src/Makefile
data/Makefile
data/libdevhelp-2.0.pc
data/devhelp.desktop.in
data/icons/Makefile
data/ui/Makefile
data/dtd/Makefile
misc/Makefile
misc/gedit-plugin/Makefile
misc/gedit-plugin/devhelp/Makefile
contrib/Makefile
])

AC_OUTPUT

echo
echo "Prefix:    $prefix"
echo "Platform:  $IGE_PLATFORM_NAME"
echo
