
INCLUDES = \
  $(EGG_CFLAGS) \
  -DEGG_COMPILATION \
  -DGTK_DISABLE_DEPRECATED \
  -DGDK_DISABLE_DEPRECATED \
  -DG_DISABLE_DEPRECATED


noinst_LTLIBRARIES = libegg-menu.la

libegg_menu_la_LIBADD = ../toolbar/libegg-toolbar.la $(EGG_LIBS)
libegg_menu_la_SOURCES = \
  egg-action.c \
  egg-action-group.c \
  egg-toggle-action.c \
  egg-radio-action.c \
  egg-markup.c \
  egg-menu-merge.c \
  egg-accel-dialog.c

noinst_HEADERS = \
  egg-menu.h \
  egg-action.h \
  egg-action-group.h \
  egg-toggle-action.h \
  egg-radio-action.h \
  egg-markup.h \
  egg-menu-merge.h \
  egg-accel-dialog.h

