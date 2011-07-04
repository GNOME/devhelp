AC_DEFUN([IGE_PLATFORM_CHECK],[
    gdk_targets=`$PKG_CONFIG --variable=targets gtk+-3.0`

    IGE_PLATFORM=

    for gdk_target in $gdk_targets; do
        if test "x$gdk_target" = "xquartz"; then
            AC_MSG_CHECKING([checking for Mac OS X support])
            carbon_ok=no
            AC_TRY_CPP([
            #include <Carbon/Carbon.h>
            #include <CoreServices/CoreServices.h>
            ], carbon_ok=yes)
            AC_MSG_RESULT($carbon_ok)
            if test $carbon_ok = yes; then
              IGE_PLATFORM=osx
              IGE_PLATFORM_NAME="GTK+ OS X"
              AC_DEFINE(HAVE_PLATFORM_OSX, 1, [whether GTK+ OS X is available])
              break
            fi
        elif test "x$gdk_target" = "xx11"; then
            IGE_PLATFORM=x11
            IGE_PLATFORM_NAME="GTK+ X11"
            AC_DEFINE(HAVE_PLATFORM_X11, 1, [whether GTK+ X11 is available])
            break
        fi
    done

    if test "x$IGE_PLATFORM" = "x"; then
        AC_MSG_ERROR([Could not detect the platform])
    fi

    AM_CONDITIONAL(HAVE_PLATFORM_OSX, test $IGE_PLATFORM = osx)
    AM_CONDITIONAL(HAVE_PLATFORM_X11, test $IGE_PLATFORM = x11)
])

