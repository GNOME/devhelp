if get_option('plugin_gedit')
  subdir('gedit-plugin')
endif

if get_option('plugin_emacs')
  install_data(
    'devhelp.el',
    install_dir: get_option('datadir') / 'emacs/site-lisp'
  )
endif

if get_option('plugin_vim')
  install_data(
    'devhelp.vim',
    install_dir: get_option('datadir') / 'vim/vimfiles/plugin'
  )
endif
