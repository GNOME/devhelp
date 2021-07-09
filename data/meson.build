subdir('icons')

install_data(
  'org.gnome.devhelp.gschema.xml',
  install_dir: get_option('datadir') / 'glib-2.0/schemas'
)

libdevhelp_gschema_conf = configuration_data()
libdevhelp_gschema_conf.set('LIBDEVHELP_API_VERSION', LIBDEVHELP_API_VERSION)
configure_file(
  input: 'org.gnome.libdevhelp.gschema.xml.in',
  output: 'org.gnome.libdevhelp-@0@.gschema.xml'.format(LIBDEVHELP_API_VERSION),
  configuration: libdevhelp_gschema_conf,
  install: true,
  install_dir: get_option('datadir') / 'glib-2.0/schemas'
)

appdata_conf = configuration_data()
appdata_conf.set('application_id', APPLICATION_ID)
appdata = APPLICATION_ID + '.appdata.xml'
appdata_file = I18N.merge_file(
  appdata,
  input: configure_file(
    input: 'org.gnome.Devhelp.appdata.xml.in.in',
    output: APPLICATION_ID + '.appdata.xml.in',
    configuration: appdata_conf,
  ),
  output: appdata,
  po_dir: '../po/',
  install: true,
  install_dir: get_option('datadir') / 'metainfo'
)

appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test('validate-appdata',
    appstream_util,
    args: ['validate-relax', '--nonet', appdata_file],
    workdir: meson.current_build_dir(),
    depends: appdata_file,
  )
endif

desktop_conf = configuration_data()
desktop_conf.set('application_id', APPLICATION_ID)
desktop = APPLICATION_ID + '.desktop'
desktop_file = I18N.merge_file(
  desktop,
  type: 'desktop',
  input: configure_file(
    input: 'org.gnome.Devhelp.desktop.in.in',
    output: APPLICATION_ID + '.desktop.in',
    configuration: desktop_conf,
  ),
  output: desktop,
  po_dir: '../po/',
  install: true,
  install_dir: get_option('datadir') / 'applications'
)

desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test('validate-desktop',
    desktop_file_validate,
    args: desktop,
    workdir: meson.current_build_dir(),
    depends: desktop_file,
  )
endif

service_file = APPLICATION_ID + '.service'
service_conf = configuration_data()
service_conf.set('application_id', APPLICATION_ID)
service_conf.set('bindir', get_option('prefix') / get_option('bindir'))
configure_file(
  input: 'org.gnome.Devhelp.service.in',
  output: service_file,
  configuration: service_conf,
  install: true,
  install_dir: get_option('datadir') / 'dbus-1/services'
)

install_data([
    'assistant.css',
    'assistant.js',
  ],
  install_dir: get_option('datadir') / 'devhelp/assistant'
)

install_man('devhelp.1')
