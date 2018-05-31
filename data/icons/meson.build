icon_sizes = [
        '16x16',
        '22x22',
        '24x24',
        '32x32',
        '48x48',
        '256x256'
]

foreach icon_size : icon_sizes
        install_data(
                join_paths(icon_size, 'org.gnome.Devhelp.png'),
                install_dir : join_paths(
                        get_option('prefix'),
                        get_option('datadir'),
                        'icons/hicolor',
                        icon_size,
                        'apps'
                )
        )
endforeach

install_data(
        'symbolic/org.gnome.Devhelp-symbolic.svg',
        install_dir : join_paths(
                get_option('prefix'),
                get_option('datadir'),
                'icons/hicolor/symbolic/apps'
        )
)
