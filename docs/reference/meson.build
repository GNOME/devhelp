subdir('xml')

configure_file(
        input : 'devhelp-sections.txt',
        output : 'devhelp-@0@-sections.txt'.format(LIBDEVHELP_API_VERSION),
        configuration : configuration_data() # TODO use "copy : true" when Meson 0.47 is released.
)

GNOME.gtkdoc(
        'devhelp-@0@'.format(LIBDEVHELP_API_VERSION),
        main_xml : 'devhelp-docs.xml',
        src_dir : include_directories('../../devhelp/'),
        dependencies : SHARED_LIBDEVHELP_DECLARED_DEP,
        scan_args : ['--rebuild-types'],
        gobject_typesfile : 'devhelp-@0@.types'.format(LIBDEVHELP_API_VERSION),
        content_files : [
                'api-breaks.xml'
        ],
        ignore_headers : [
                'dh-book-list-simple.h',
                'dh-dconf-migration.h',
                'dh-error.h',
                'dh-parser.h',
                'dh-search-context.h',
                'dh-util-lib.h'
        ],
        install : true
)
