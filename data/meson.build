install_data(
        ['org.gnome.devhelp.gschema.xml',
         'org.gnome.libdevhelp-3.gschema.xml'],
        install_dir : join_paths(get_option('prefix'), get_option('datadir'), 'glib-2.0', 'schemas')
)

appdata = 'org.gnome.Devhelp.appdata.xml'
I18N.merge_file(
        appdata,
        input : appdata + '.in',
        output : appdata,
        po_dir : '../po/',
        install : true,
        install_dir : join_paths(get_option('prefix'), get_option('datadir'), 'metainfo')
)
