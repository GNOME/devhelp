subdir('xml')

GNOME.gtkdoc(
        'devhelp',
        main_xml : 'devhelp-docs.xml',
        src_dir : include_directories('../../devhelp/'),
        dependencies : SHARED_LIBDEVHELP_DECLARED_DEP,
        scan_args : ['--rebuild-types'],
        gobject_typesfile : 'devhelp.types',
        content_files : [
                'api-breaks.xml'
        ],
        ignore_headers : [
                'dh-dconf-migration.h',
                'dh-error.h',
                'dh-parser.h',
                'dh-search-context.h',
                'dh-settings-builder.h',
                'dh-util-lib.h'
        ],
        install : true
)
