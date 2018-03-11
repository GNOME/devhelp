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

install_headers_subdir = 'devhelp-@0@/devhelp'.format(DEVHELP_API_VERSION)
install_headers_fulldir = join_paths(get_option('prefix'),
                                     get_option('includedir'),
                                     install_headers_subdir)

libdevhelp_enums = GNOME.mkenums_simple('dh-enum-types',
                                        sources : libdevhelp_public_headers,
                                        install_header : true,
                                        install_dir : install_headers_fulldir)

library('devhelp-@0@'.format(DEVHELP_API_VERSION),
        [libdevhelp_public_headers,
         libdevhelp_public_c_files,
         libdevhelp_private_headers,
         libdevhelp_private_c_files,
         libdevhelp_enums],
        dependencies : DEVHELP_DEPS,
        include_directories : [CONFIG_H_INCLUDE_DIR, ROOT_INCLUDE_DIR],
        install : true)

install_headers(libdevhelp_public_headers,
                subdir : install_headers_subdir)
