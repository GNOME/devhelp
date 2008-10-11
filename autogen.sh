#!/bin/sh
# Run this to generate all the initial makefiles, etc.

CONFIGURE=configure.ac

: ${AUTOCONF=autoconf}
: ${AUTOHEADER=autoheader}
: ${AUTOMAKE=automake-1.9}
: ${ACLOCAL=aclocal-1.9}
: ${INTLTOOLIZE=intltoolize}
: ${LIBTOOLIZE=libtoolize}
: ${GTKDOCIZE=gtkdocize}

#
# Nothing should need changing below.
#

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir

DIE=0

($AUTOCONF --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "You must have autoconf installed to compile this project."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}

(grep "^IT_PROG_INTLTOOL" $srcdir/$CONFIGURE >/dev/null) && {
  ($INTLTOOLIZE --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "You must have intltoolize installed to compile this project."
    echo "Get ftp://ftp.gnome.org/pub/GNOME/stable/sources/intltool/intltool-0.35.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
  }
}

# Check if gtk-doc is explicitly disabled.
for option in $AUTOGEN_CONFIGURE_ARGS $@
do
  case $option in
    -disable-gtk-doc | --disable-gtk-doc)
    enable_gtk_doc=no
  ;;
  esac
done

if test x$enable_gtk_doc != xno; then
  echo "Checking for gtkdocize ... "
  if grep "^GTK_DOC_CHECK" $CONFIGURE > /dev/null; then
    if !($GTKDOCIZE --version) < /dev/null > /dev/null 2>&1; then
      echo
      echo "  You must have gtk-doc installed to compile this project."
      echo "  Install the appropriate package for your distribution,"
      echo "  or get the source tarball at"
      echo "  http://ftp.gnome.org/pub/GNOME/sources/gtk-doc/"
      echo "  You can also use the option --disable-gtk-doc to skip"
      echo "  this test but then you will not be able to generate a"
      echo "  configure script that can build the API documentation."
      DIE=1
    fi
  fi
fi

($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "You must have automake installed to compile this project."
  echo "Get ftp://sourceware.cygnus.com/pub/automake/automake-1.9.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
}

($LIBTOOLIZE --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "You must have libtool installed to compile this project."
  echo "Get ftp://ftp.gnu.org/pub/gnu/libtool-1.5.22.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
}

if grep "^AM_[A-Z0-9_]\{1,\}_GETTEXT" "$CONFIGURE" >/dev/null; then
  if grep "sed.*POTFILES" "$CONFIGURE" >/dev/null; then
    GETTEXTIZE=""
  else
    if grep "^AM_GLIB_GNU_GETTEXT" "$CONFIGURE" >/dev/null; then
      GETTEXTIZE="glib-gettextize"
      GETTEXTIZE_URL="ftp://ftp.gtk.org/pub/gtk/v2.0/glib-2.0.0.tar.gz"
    else
      GETTEXTIZE="gettextize"
      GETTEXTIZE_URL="ftp://alpha.gnu.org/gnu/gettext-0.10.35.tar.gz"
    fi

    $GETTEXTIZE --version < /dev/null > /dev/null 2>&1
    if test $? -ne 0; then
      echo
      echo "You must have $GETTEXTIZE installed to compile this project."
      echo "Get $GETTEXTIZE_URL"
      echo "(or a newer version if it is available)"
      DIE=1
    fi
  fi
fi

if test "$DIE" -eq 1; then
  exit 1
fi

test -f $CONFIGURE || {
  echo "You must run this script in the top-level this project directory"
  exit 1
}

rm -rf autom4te.cache

do_cmd() {
  echo "Running '$@'"
  $@ || exit $?
}

do_cmd $ACLOCAL $ACLOCAL_FLAGS

if grep "^IT_PROG_INTLTOOL" $CONFIGURE >/dev/null; then
  do_cmd $INTLTOOLIZE --copy --force --automake
fi

do_cmd $LIBTOOLIZE --copy --force --automake

if grep "^GTK_DOC_CHECK" $CONFIGURE > /dev/null; then
  if test x$enable_gtk_doc = xno; then
    echo "WARNING: You have disabled gtk-doc."
    echo "         As a result, you will not be able to generate the API"
    echo "         documentation and 'make dist' will not work."
    echo
  else
    do_cmd $GTKDOCIZE --copy --flavour no-tmpl --docdir build
  fi
fi

do_cmd $AUTOHEADER

do_cmd $AUTOMAKE --add-missing -Wall

do_cmd $AUTOCONF

cd $ORIGDIR || exit $?

if test x$NOCONFIGURE = x; then
  echo "Running '$srcdir/configure $@'"
  $srcdir/configure "$@" && echo "Now type 'make' to compile."  || exit 1
else
  echo Skipping configure process.
fi
