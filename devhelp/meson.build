libdevhelp_public_headers = [
        'devhelp.h',
        'dh-assistant-view.h',
        'dh-book.h',
        'dh-book-manager.h',
        'dh-book-tree.h',
        'dh-completion.h',
        'dh-init.h',
        'dh-keyword-model.h',
        'dh-link.h',
        'dh-sidebar.h'
]

libdevhelp_public_c_files = [
        'dh-assistant-view.c',
        'dh-book.c',
        'dh-book-manager.c',
        'dh-book-tree.c',
        'dh-completion.c',
        'dh-init.c',
        'dh-keyword-model.c',
        'dh-link.c',
        'dh-sidebar.c'
]

libdevhelp_private_headers = [
        'dh-error.h',
        'dh-parser.h',
        'dh-search-context.h',
        'dh-settings.h',
        'dh-util-lib.h'
]

libdevhelp_private_c_files = [
        'dh-error.c',
        'dh-parser.c',
        'dh-search-context.c',
        'dh-settings.c',
        'dh-util-lib.c'
]

install_headers_first_subdir = 'devhelp-@0@'.format(DEVHELP_API_VERSION)
install_headers_subdir = join_paths(install_headers_first_subdir, 'devhelp')
install_headers_fulldir = join_paths(
        get_option('prefix'),
        get_option('includedir'),
        install_headers_subdir
)

libdevhelp_enums = GNOME.mkenums_simple(
        'dh-enum-types',
        sources : libdevhelp_public_headers,
        install_header : true,
        install_dir : install_headers_fulldir
)
libdevhelp_enum_header = libdevhelp_enums[1]

libdevhelp_static = static_library(
        'devhelp-static',
        [libdevhelp_public_c_files,
         libdevhelp_private_c_files,
         libdevhelp_enums],
        include_directories : [CONFIG_H_INCLUDE_DIR, ROOT_INCLUDE_DIR],
        dependencies : LIBDEVHELP_DEPS
)

# For unit tests, to be able to test private functions.
STATIC_LIBDEVHELP_DECLARED_DEP = declare_dependency(
        sources : libdevhelp_enum_header,
        link_with : libdevhelp_static,
        include_directories : [ROOT_INCLUDE_DIR],
        dependencies : LIBDEVHELP_DEPS
)

symbol_map = join_paths(meson.current_source_dir(), 'symbol.map')

ldflag = '-Wl,--version-script'
c_compiler = meson.get_compiler('c')

ldflags = []
if c_compiler.has_argument(ldflag)
        ldflags += '@0@,@1@'.format(ldflag, symbol_map)
endif

libdevhelp = shared_library(
        'devhelp-@0@'.format(DEVHELP_API_VERSION),
        link_whole : libdevhelp_static,
        link_args : ldflags,
        link_depends : symbol_map,
        version : LIBDEVHELP_LT_VERSION,
        install : true
)

SHARED_LIBDEVHELP_DECLARED_DEP = declare_dependency(
        sources : libdevhelp_enum_header,
        link_with : libdevhelp,
        include_directories : [ROOT_INCLUDE_DIR, include_directories('.')],
        dependencies : LIBDEVHELP_DEPS
)

install_headers(
        libdevhelp_public_headers,
        subdir : install_headers_subdir
)

PKG_CONFIG.generate(
        filebase : 'libdevhelp-@0@'.format(DEVHELP_API_VERSION_FULL),
        name : meson.project_name(),
        description : meson.project_name(),
        version : meson.project_version(),
        libraries : libdevhelp,
        subdirs : install_headers_first_subdir,
        requires : LIBDEVHELP_DEPS_STR
)
