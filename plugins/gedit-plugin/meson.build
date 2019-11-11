gedit_plugin_dir = join_paths(get_option('prefix'), get_option('libdir'), 'gedit/plugins')

install_data(
  'devhelp.py',
  install_dir : gedit_plugin_dir
)

plugin_info_file = 'devhelp.plugin'
I18N.merge_file(
  plugin_info_file,
  type : 'desktop',
  input : plugin_info_file + '.desktop.in',
  output : plugin_info_file,
  po_dir : '../../po/',
  install : true,
  install_dir : gedit_plugin_dir
)
