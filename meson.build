# Convention:
# - Local variables in lower_case.
# - Global variables in UPPER_CASE.
# See: https://github.com/mesonbuild/meson/issues/2607

project(
        'devhelp', 'c',
        meson_version : '>= 0.43',
        version : '3.28.1'
)

GNOME = import('gnome')
PKG_CONFIG = import('pkgconfig')
I18N = import('i18n')

# API version, used for parallel installability.
DEVHELP_API_VERSION = '3'

# It would be nice to get rid of this variant, to use only the short version
# everywhere.
DEVHELP_API_VERSION_FULL = '3.0'

# Libtool version numbers, remember to change them just *before* a release.
# A good time to update it is for the GNOME x.y.90 version, at API freeze,
# so that packagers have the time to update the package.
#   (Interfaces removed:    CURRENT++, AGE=0, REVISION=0)
#   (Interfaces added:      CURRENT++, AGE++, REVISION=0)
#   (No interfaces changed:                   REVISION++)
lt_current=5
lt_revision=0
lt_age=0
LIBDEVHELP_LT_VERSION='@0@.@1@.@2@'.format(lt_current, lt_revision, lt_age)

libdevhelp_deps_array = [
        ['gio-2.0', '>= 2.56'],
        ['gtk+-3.0', '>= 3.22'],
        ['webkit2gtk-4.0', '>= 2.19.2']
]

LIBDEVHELP_DEPS_STR = []
LIBDEVHELP_DEPS = []
foreach dep : libdevhelp_deps_array
        dep_name = dep[0]
        dep_version = dep[1]
        LIBDEVHELP_DEPS_STR += '@0@ @1@'.format(dep_name, dep_version)
        LIBDEVHELP_DEPS += dependency(dep_name, version : dep_version)
endforeach

DEVHELP_APP_DEPS = [
        LIBDEVHELP_DEPS,
        dependency('gsettings-desktop-schemas'),
        meson.get_compiler('c').find_library('m')
]

# config.h

config_data = configuration_data()
GETTEXT_PACKAGE_NAME = meson.project_name()
config_data.set_quoted(
        'GETTEXT_PACKAGE',
        GETTEXT_PACKAGE_NAME,
        description : 'Define to the gettext package name.'
)
config_data.set_quoted(
        'DATADIR',
        join_paths(get_option('prefix'), get_option('datadir'))
)
config_data.set_quoted(
        'LOCALEDIR',
        join_paths(get_option('prefix'), get_option('localedir'))
)
config_data.set_quoted('PACKAGE_VERSION', meson.project_version())
config_data.set('FLATPAK_BUILD', get_option('flatpak_build'))

configure_file(
        output : 'config.h',
        configuration : config_data
)

CONFIG_H_INCLUDE_DIR = include_directories('.')

# Misc

# For #include <devhelp/something.h>
ROOT_INCLUDE_DIR = include_directories('.')

add_global_arguments(
        '-DG_LOG_DOMAIN="@0@"'.format(meson.project_name()),
        language : 'c'
)

#####
# CFLAGS
# (This is one of the ugly parts of Meson, currently.)
# The same as the AX_COMPILER_FLAGS Autotools macro.

warning_cflags = [
        '-fno-strict-aliasing',
        '-Wall',
        '-Wextra',
        '-Wundef',
        '-Wnested-externs',
        '-Wwrite-strings',
        '-Wpointer-arith',
        '-Wmissing-declarations',
        '-Wmissing-prototypes',
        '-Wstrict-prototypes',
        '-Wredundant-decls',
        '-Wno-unused-parameter',
        '-Wno-missing-field-initializers',
        '-Wdeclaration-after-statement',
        '-Wformat=2',
        '-Wold-style-definition',
        '-Wcast-align',
        '-Wformat-nonliteral',
        '-Wformat-security',
        '-Wsign-compare',
        '-Wstrict-aliasing',
        '-Wshadow',
        '-Winline',
        '-Wpacked',
        '-Wmissing-format-attribute',
        '-Wmissing-noreturn',
        '-Winit-self',
        '-Wredundant-decls',
        '-Wmissing-include-dirs',
        '-Wunused-but-set-variable',
        '-Warray-bounds',
        '-Wimplicit-function-declaration',
        '-Wreturn-type',
        '-Wswitch-enum',
        '-Wswitch-default',
        '-Wduplicated-cond',
        '-Wduplicated-branches',
        '-Wlogical-op',
        '-Wrestrict',
        '-Wnull-dereference',
        '-Wjump-misses-init',
        '-Wdouble-promotion'
]

c_compiler = meson.get_compiler('c')
supported_warning_cflags = c_compiler.get_supported_arguments(warning_cflags)
add_global_arguments(supported_warning_cflags, language : 'c')
##### end CFLAGS

subdir('data')
subdir('plugins/gedit-plugin')
subdir('po')
subdir('help')
subdir('devhelp')
subdir('src')
subdir('unit-tests')

if get_option('gtk_doc')
        subdir('docs/reference')
endif

meson.add_install_script(
        'meson_post_install.py',
        join_paths(get_option('prefix'), get_option('datadir'))
)

# Print a summary of the configuration
output = 'Configuration:\n\n'
output += '        Devhelp version ' + meson.project_version() + '\n\n'
output += '        Prefix:             ' + get_option('prefix') + '\n'
output += '        API documentation:  ' + get_option('gtk_doc').to_string() + '\n'
output += '        Flatpak build mode: ' + get_option('flatpak_build').to_string() + '\n'
message(output)
