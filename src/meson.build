# SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
# SPDX-License-Identifier: GPL-3.0-or-later

devhelp_app_sources = [
  'dh-app.c',
  'dh-assistant.c',
  'dh-main.c',
  'dh-preferences.c',
  'dh-settings-app.c',
  'dh-util-app.c',
  'dh-window.c'
]

devhelp_app_sources += GNOME.compile_resources(
  'dh-resources',
  'dh.gresource.xml'
)

executable(
  'devhelp',
  devhelp_app_sources,
  dependencies: [DEVHELP_APP_DEPS, LIBDEVHELP_SHARED_LIB_DEP],
  install: true
)
