
INCLUDES = \
  @EGG_CFLAGS@ \
  -DEGG_COMPILATION \
  -DGTK_DISABLE_DEPRECATED \
  -DGDK_DISABLE_DEPRECATED \
  -DG_DISABLE_DEPRECATED


noinst_LTLIBRARIES = libegg-menu.la

libegg_menu_la_LIBADD = ../toolbar/libegg-toolbar.la @EGG_LIBS@
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

noinst_PROGRAMS = test-actions test-merge

test_actions_SOURCES = test-actions.c
test_actions_LDFLAGS = libegg-menu.la @EGG_LIBS@
test_actions_DEPENDENCIES = libegg-menu.la

test_merge_SOURCES = test-merge.c
test_merge_LDFLAGS = libegg-menu.la @EGG_LIBS@
test_merge_DEPENDENCIES = libegg-menu.la
