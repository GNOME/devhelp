# SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
# SPDX-License-Identifier: GPL-3.0-or-later

libdevhelp_public_headers = [
  'devhelp.h',
  'dh-application-window.h',
  'dh-assistant-view.h',
  'dh-book.h',
  'dh-book-list.h',
  'dh-book-list-builder.h',
  'dh-book-list-directory.h',
  'dh-book-manager.h',
  'dh-book-tree.h',
  'dh-completion.h',
  'dh-init.h',
  'dh-keyword-model.h',
  'dh-link.h',
  'dh-notebook.h',
  'dh-profile.h',
  'dh-profile-builder.h',
  'dh-search-bar.h',
  'dh-settings.h',
  'dh-settings-builder.h',
  'dh-sidebar.h',
  'dh-tab.h',
  'dh-tab-label.h',
  'dh-web-view.h'
]

libdevhelp_public_c_files = [
  'dh-application-window.c',
  'dh-assistant-view.c',
  'dh-book.c',
  'dh-book-list.c',
  'dh-book-list-builder.c',
  'dh-book-list-directory.c',
  'dh-book-manager.c',
  'dh-book-tree.c',
  'dh-completion.c',
  'dh-init.c',
  'dh-keyword-model.c',
  'dh-link.c',
  'dh-notebook.c',
  'dh-profile.c',
  'dh-profile-builder.c',
  'dh-search-bar.c',
  'dh-settings.c',
  'dh-settings-builder.c',
  'dh-sidebar.c',
  'dh-tab.c',
  'dh-tab-label.c',
  'dh-web-view.c'
]

libdevhelp_private_c_files = [
  'dh-book-list-simple.c',
  'dh-error.c',
  'dh-parser.c',
  'dh-search-context.c',
  'dh-util-lib.c'
]

headers_install_dir = get_option('includedir') / 'devhelp-@0@/devhelp/'.format(LIBDEVHELP_API_VERSION)
install_headers(
  libdevhelp_public_headers,
  install_dir: headers_install_dir
)

libdevhelp_enum_types = GNOME.mkenums_simple(
  'dh-enum-types',
  sources: libdevhelp_public_headers,
  install_header: true,
  install_dir: headers_install_dir
)

libdevhelp_static_lib = static_library(
  'devhelp-static',
  [libdevhelp_public_c_files,
   libdevhelp_private_c_files,
   libdevhelp_enum_types],
  pic: true, # libdevhelp_static_lib is linked in a shared library.
  include_directories: ROOT_INCLUDE_DIR,
  dependencies: LIBDEVHELP_DEPS
)

# For unit tests, to be able to test private functions.
LIBDEVHELP_STATIC_DEP = declare_dependency(
  include_directories: ROOT_INCLUDE_DIR,
  link_with: libdevhelp_static_lib,
  sources: libdevhelp_enum_types[1],
  dependencies: LIBDEVHELP_DEPS
)

symbol_map = meson.current_source_dir() / 'symbol.map'

libdevhelp_shared_lib = shared_library(
  'devhelp-@0@'.format(LIBDEVHELP_API_VERSION),
  link_whole: libdevhelp_static_lib,
  link_args: '-Wl,--version-script,' + symbol_map,
  link_depends: symbol_map,
  version: LIBDEVHELP_LT_VERSION,
  install: true
)

LIBDEVHELP_SHARED_LIB_DEP = declare_dependency(
  include_directories: ROOT_INCLUDE_DIR,
  link_with: libdevhelp_shared_lib,
  sources: libdevhelp_enum_types[1],
  dependencies: LIBDEVHELP_DEPS
)

PKG_CONFIG.generate(
  filebase: 'libdevhelp-@0@'.format(LIBDEVHELP_API_VERSION_FULL),
  name: meson.project_name(),
  description: meson.project_name(),
  libraries: libdevhelp_shared_lib,
  libraries_private: '-lm',
  subdirs: 'devhelp-@0@'.format(LIBDEVHELP_API_VERSION),
  requires: LIBDEVHELP_PUBLIC_DEPS,
  requires_private: LIBDEVHELP_PRIVATE_DEPS
)

GNOME.generate_gir(
  libdevhelp_shared_lib,
  export_packages: 'libdevhelp-@0@'.format(LIBDEVHELP_API_VERSION_FULL),
  header: 'devhelp/devhelp.h',
  identifier_prefix: 'Dh',
  include_directories: ROOT_INCLUDE_DIR,
  includes: ['Gtk-3.0', 'WebKit2-4.0'],
  install: true,
  namespace: 'Devhelp',
  nsversion: LIBDEVHELP_API_VERSION_FULL,
  sources: [
    libdevhelp_public_headers,
    libdevhelp_public_c_files,
    libdevhelp_enum_types
  ]
)
