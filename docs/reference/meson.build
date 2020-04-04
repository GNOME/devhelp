subdir('xml')

configure_file(
  input: 'devhelp-sections.txt',
  output: 'devhelp-@0@-sections.txt'.format(LIBDEVHELP_API_VERSION),
  copy: true
)

gtkdoc_module_name = 'devhelp-@0@'.format(LIBDEVHELP_API_VERSION)
html_dir = get_option('prefix') / GNOME.gtkdoc_html_dir(gtkdoc_module_name)

glib_docpath = dependency('glib-2.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/glib'
gobject_docpath = dependency('gobject-2.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gobject'
gio_docpath = dependency('gio-2.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gio'
gtk_docpath = dependency('gtk+-3.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gtk3'
webkitgtk_docpath = dependency('webkit2gtk-4.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/webkit2gtk-4.0'

GNOME.gtkdoc(
  gtkdoc_module_name,
  main_xml: 'devhelp-docs.xml',
  src_dir: include_directories('../../devhelp/'),
  dependencies: LIBDEVHELP_SHARED_LIB_DEP,
  scan_args: ['--rebuild-types'],
  gobject_typesfile: 'devhelp-@0@.types'.format(LIBDEVHELP_API_VERSION),
  fixxref_args: [
    '--html-dir=@0@'.format(html_dir),
    '--extra-dir=@0@'.format(glib_docpath),
    '--extra-dir=@0@'.format(gobject_docpath),
    '--extra-dir=@0@'.format(gio_docpath),
    '--extra-dir=@0@'.format(gtk_docpath),
    '--extra-dir=@0@'.format(webkitgtk_docpath)
  ],
  content_files: [
    'api-breaks.xml'
  ],
  ignore_headers: [
    'dh-book-list-simple.h',
    'dh-dconf-migration.h',
    'dh-error.h',
    'dh-parser.h',
    'dh-search-context.h',
    'dh-util-lib.h'
  ],
  install: true
)
