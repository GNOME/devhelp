
INCLUDES = \
  @EGG_CFLAGS@ \
  -I$(top_srdir) \
  -I$(top_builddir) \
  -DEGG_COMPILATION \
  -DGTK_DISABLE_DEPRECATED \
  -DGDK_DISABLE_DEPRECATED \
  -DG_DISABLE_DEPRECATED

noinst_LTLIBRARIES = libegg-toolbar.la

libegg_toolbar_la_LIBADD = @EGG_LIBS@
libegg_toolbar_la_SOURCES = \
  eggtoolbar.c \
  eggtoolitem.c \
  eggseparatortoolitem.c \
  eggtoolbutton.c \
  eggtoggletoolbutton.c

noinst_HEADERS = \
  eggtoolbar.h \
  eggtoolitem.h	 \
  eggseparatortoolitem.h \
  eggtoolbutton.h \
  eggtoggletoolbutton.h

noinst_PROGRAMS = test-toolbar

test_toolbar_SOURCES = test-toolbar.c

test_toolbar_LDADD = @EGG_LIBS@ \
  $(top_builddir)/libegg/toolbar/libegg-toolbar.la
