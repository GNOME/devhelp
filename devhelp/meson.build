libdevhelp_public_headers = [
        'dh-book.h',
        'dh-book-manager.h',
        'dh-completion.h',
        'dh-init.h',
        'dh-link.h'
]

libdevhelp_public_c_files = [
        'dh-book.c',
        'dh-book-manager.c',
        'dh-completion.c',
        'dh-init.c',
        'dh-link.c'
]

libdevhelp_private_headers = [
        'dh-error.h',
        'dh-parser.h',
        'dh-settings.h',
        'dh-util-lib.h'
]

libdevhelp_private_c_files = [
        'dh-error.c',
        'dh-parser.c',
        'dh-settings.c',
        'dh-util-lib.c'
]

library('devhelp-3',
        [libdevhelp_public_headers,
         libdevhelp_public_c_files,
         libdevhelp_private_headers,
         libdevhelp_private_c_files],
        dependencies : DEVHELP_DEPS,
        include_directories : [CONFIG_H_INCLUDE_DIR, ROOT_INCLUDE_DIR],
        install : true)
