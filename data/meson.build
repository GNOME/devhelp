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

desktop_file = 'org.gnome.Devhelp.desktop'
I18N.merge_file(
        desktop_file,
        type : 'desktop',
        input : desktop_file + '.in',
        output : desktop_file,
        po_dir : '../po/',
        install : true,
        install_dir : join_paths(get_option('prefix'), get_option('datadir'), 'applications')
)

service_file = 'org.gnome.Devhelp.service'
service_conf = configuration_data()
service_conf.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))
configure_file(
        input : service_file + '.in',
        output : service_file,
        configuration : service_conf,
        install : true,
        install_dir : join_paths(get_option('prefix'), get_option('datadir'), 'dbus-1/services')
)

install_data(
        ['assistant.css', 'assistant.js'],
        install_dir : join_paths(get_option('prefix'), get_option('datadir'), 'devhelp', 'assistant')
)

install_data(
        'devhelp.1',
        install_dir : join_paths(get_option('prefix'), get_option('mandir'), 'man1')
)
