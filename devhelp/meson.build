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

install_headers_first_subdir = 'devhelp-@0@'.format(LIBDEVHELP_API_VERSION)
install_headers_subdir = join_paths(install_headers_first_subdir, 'devhelp')
install_headers_fulldir = join_paths(
  get_option('prefix'),
  get_option('includedir'),
  install_headers_subdir
)

libdevhelp_enums = GNOME.mkenums_simple(
  'dh-enum-types',
  sources: libdevhelp_public_headers,
  install_header: true,
  install_dir: install_headers_fulldir
)
libdevhelp_enum_header = libdevhelp_enums[1]

libdevhelp_static = static_library(
  'devhelp-static',
  [libdevhelp_public_c_files,
   libdevhelp_private_c_files,
   libdevhelp_enums],
  include_directories: ROOT_INCLUDE_DIR,
  dependencies: LIBDEVHELP_DEPS
)

# For unit tests, to be able to test private functions.
STATIC_LIBDEVHELP_DECLARED_DEP = declare_dependency(
  sources: libdevhelp_enum_header,
  link_with: libdevhelp_static,
  include_directories: [ROOT_INCLUDE_DIR],
  dependencies: LIBDEVHELP_DEPS
)

symbol_map = join_paths(meson.current_source_dir(), 'symbol.map')

libdevhelp = shared_library(
  'devhelp-@0@'.format(LIBDEVHELP_API_VERSION),
  link_whole: libdevhelp_static,
  link_args: '-Wl,--version-script,' + symbol_map,
  link_depends: symbol_map,
  version: LIBDEVHELP_LT_VERSION,
  install: true
)

SHARED_LIBDEVHELP_DECLARED_DEP = declare_dependency(
  sources: libdevhelp_enum_header,
  link_with: libdevhelp,
  include_directories: [ROOT_INCLUDE_DIR, include_directories('.')],
  dependencies: LIBDEVHELP_DEPS
)

install_headers(
  libdevhelp_public_headers,
  subdir: install_headers_subdir
)

PKG_CONFIG.generate(
  filebase: 'libdevhelp-@0@'.format(LIBDEVHELP_API_VERSION_FULL),
  name: meson.project_name(),
  description: meson.project_name(),
  version: meson.project_version(),
  libraries: libdevhelp,
  libraries_private: '-lm',
  subdirs: install_headers_first_subdir,
  requires: LIBDEVHELP_PUBLIC_DEPS,
  requires_private: LIBDEVHELP_PRIVATE_DEPS
)

GNOME.generate_gir(
  libdevhelp,
  export_packages: 'libdevhelp-@0@'.format(LIBDEVHELP_API_VERSION_FULL),
  sources: [
    libdevhelp_public_headers,
    libdevhelp_public_c_files,
    libdevhelp_enums
  ],
  namespace: 'Devhelp',
  nsversion: LIBDEVHELP_API_VERSION_FULL,
  identifier_prefix: 'Dh',
  includes: ['Gtk-3.0', 'WebKit2-4.0'],
  header: 'devhelp/devhelp.h',
  include_directories: ROOT_INCLUDE_DIR,
  install: true
)
