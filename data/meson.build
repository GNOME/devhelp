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

appdata = 'org.gnome.Devhelp.appdata.xml'
I18N.merge_file(
  appdata,
  input: appdata + '.in',
  output: appdata,
  po_dir: '../po/',
  install: true,
  install_dir: get_option('datadir') / 'metainfo'
)

desktop_file = 'org.gnome.Devhelp.desktop'
I18N.merge_file(
  desktop_file,
  type: 'desktop',
  input: desktop_file + '.in',
  output: desktop_file,
  po_dir: '../po/',
  install: true,
  install_dir: get_option('datadir') / 'applications'
)

desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test('validate-desktop',
    desktop_file_validate,
    args: desktop_file,
    workdir: meson.current_build_dir(),
  )
endif

service_file = 'org.gnome.Devhelp.service'
service_conf = configuration_data()
service_conf.set('bindir', get_option('prefix') / get_option('bindir'))
configure_file(
  input: service_file + '.in',
  output: service_file,
  configuration: service_conf,
  install: true,
  install_dir: get_option('datadir') / 'dbus-1/services'
)

install_data(
  ['assistant.css', 'assistant.js'],
  install_dir: get_option('datadir') / 'devhelp/assistant'
)

install_man('devhelp.1')
