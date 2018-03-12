devhelp_app_sources = [
        'dh-app.c',
        'dh-assistant.c',
        'dh-main.c',
        'dh-preferences.c',
        'dh-settings-app.c',
        'dh-tab.c',
        'dh-tab-label.c',
        'dh-util-app.c',
        'dh-web-view.c',
        'dh-window.c',
        'tepl-info-bar.c'
]

devhelp_app_sources += GNOME.compile_resources(
        'dh-resources',
        'dh.gresource.xml'
)

executable(
        'devhelp',
        devhelp_app_sources,
        include_directories : [CONFIG_H_INCLUDE_DIR, ROOT_INCLUDE_DIR],
        dependencies : [DEVHELP_APP_DEPS, SHARED_LIBDEVHELP_DECLARED_DEP],
        install : true
)
