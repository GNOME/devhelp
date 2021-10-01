docs_dir = get_option('prefix') / get_option('datadir') / 'doc'

extra_content_files = [
  'api-breaks.md',
]

dependency('gi-docgen', version: '>= 2021.6',
  fallback: ['gi-docgen', 'dummy_dep'],
  required: get_option('gtk_doc'),
)

gidocgen = find_program('gi-docgen', required: get_option('gtk_doc'))

if get_option('gtk_doc')
  toml_data = configuration_data()
  toml_data.set('version', meson.project_version())
  toml_data.set('webkit_abi', webkit_abi)

  devhelp_toml = configure_file(
    input: 'devhelp.toml.in',
    output: 'devhelp-@0@.toml'.format(LIBDEVHELP_API_VERSION),
    configuration: toml_data,
    install: true,
    install_dir: docs_dir / 'devhelp-@0@'.format(LIBDEVHELP_API_VERSION),
  )

  custom_target('devhelp-doc',
    input: [devhelp_toml, libdevhelp_gir],
    output: 'devhelp-@0@'.format(LIBDEVHELP_API_VERSION),
    command: [
      gidocgen,
      'generate',
      '--quiet',
      '--fatal-warnings',
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../../devhelp'),
      '--config=@INPUT0@',
      '--output-dir=@OUTPUT@',
      '--no-namespace-dir',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT1@',
    ],
    depend_files: extra_content_files,
    build_by_default: true,
    install: true,
    install_dir: docs_dir,
  )
endif
