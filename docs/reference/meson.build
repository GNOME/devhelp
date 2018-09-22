subdir('xml')

configure_file(
        input : 'devhelp-sections.txt',
        output : 'devhelp-@0@-sections.txt'.format(LIBDEVHELP_API_VERSION),
        copy : true
)

gtkdoc_module_name = 'devhelp-@0@'.format(LIBDEVHELP_API_VERSION)
html_dir = join_paths(get_option('prefix'), GNOME.gtkdoc_html_dir(gtkdoc_module_name))

webkitgtk_dep = dependency(WEBKITGTK_DEP_STR)
webkitgtk_prefix = webkitgtk_dep.get_pkgconfig_variable('prefix')

GNOME.gtkdoc(
        gtkdoc_module_name,
        main_xml : 'devhelp-docs.xml',
        src_dir : include_directories('../../devhelp/'),
        dependencies : SHARED_LIBDEVHELP_DECLARED_DEP,
        scan_args : ['--rebuild-types'],
        gobject_typesfile : 'devhelp-@0@.types'.format(LIBDEVHELP_API_VERSION),
        fixxref_args : [
                '--html-dir=@0@'.format(html_dir),
                '--extra-dir=@0@/share/gtk-doc/html/@1@'.format(webkitgtk_prefix, WEBKITGTK_DEP_STR)
        ],
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
