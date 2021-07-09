# Convention:
# - Local variables in lower_case.
# - Global variables in UPPER_CASE.
# See: https://github.com/mesonbuild/meson/issues/2607

project(
  'devhelp', 'c',
  meson_version: '>= 0.53',
  version: '41.beta',
  default_options: ['warning_level=2']
)

GNOME = import('gnome')
PKG_CONFIG = import('pkgconfig')
I18N = import('i18n')

# API version, used for parallel installability.
LIBDEVHELP_API_VERSION = '3'

# It would be nice to get rid of this variant, to use only the short version
# everywhere.
LIBDEVHELP_API_VERSION_FULL = '3.0'

# Libtool versioning
#
# A good time to update it is for the GNOME x.y.90 version, at API freeze, so
# that packagers have the time to update the package. For other development
# releases (if the minor package version is odd), keep the same Libtool
# version.
#
# To update the Libtool version, apply the following algorithm step by step:
# 1. If the library source code has changed at all since the last
#    update, then increment REVISION.
# 2. If any exported functions or data have been added, removed, or
#    changed since the last update, increment CURRENT and set REVISION
#    to 0.
# 3. If any exported functions or data have been added since the last
#    public release, increment AGE.
# 4. If any exported functions or data have been removed since the last
#    public release, set AGE to 0.
#
# When incrementing the API version (usually for a new major package version),
# set CURRENT, REVISION and AGE to 0 since it's like a new library.
lt_current = 6
lt_revision = 3
lt_age = 0
LIBDEVHELP_LT_VERSION = '@0@.@1@.@2@'.format(lt_current, lt_revision, lt_age)

LIBDEVHELP_PUBLIC_DEPS = [
  dependency('gio-2.0', version: '>= 2.64'),
  dependency('gtk+-3.0', version: '>= 3.22'),
  dependency('webkit2gtk-4.0', version: '>= 2.26'),
]
LIBDEVHELP_PRIVATE_DEPS = [
  dependency('gsettings-desktop-schemas'),
]
LIBDEVHELP_DEPS = [
  LIBDEVHELP_PUBLIC_DEPS,
  LIBDEVHELP_PRIVATE_DEPS,
  meson.get_compiler('c').find_library('m'),
]

DEVHELP_APP_DEPS = [
  LIBDEVHELP_DEPS,
  dependency('gsettings-desktop-schemas'),
]

devhelp_version = meson.project_version().split('.')
devhelp_is_devel = devhelp_version[1] in ['alpha', 'beta', 'rc']
devel_build = get_option('profile') == 'devel' or devhelp_is_devel

if devel_build
  APPLICATION_ID = 'org.gnome.Devhelp.Devel'
else
  APPLICATION_ID = 'org.gnome.Devhelp'
endif

# config.h

config_data = configuration_data()
GETTEXT_PACKAGE_NAME = meson.project_name()
config_data.set_quoted('GETTEXT_PACKAGE', GETTEXT_PACKAGE_NAME)
config_data.set_quoted('DATADIR', get_option('prefix') / get_option('datadir'))
config_data.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))
config_data.set_quoted('PACKAGE_VERSION', meson.project_version())
config_data.set_quoted('LIBDEVHELP_API_VERSION', LIBDEVHELP_API_VERSION)
config_data.set_quoted('APPLICATION_ID', APPLICATION_ID)
config_data.set10('DEVEL_BUILD', devel_build)
config_data.set10('FLATPAK_BUILD', get_option('flatpak_build'))

configure_file(
  output: 'config.h',
  configuration: config_data
)

# Misc

# For #include <devhelp/something.h> or #include "config.h".
ROOT_INCLUDE_DIR = include_directories('.')

add_project_arguments(
  '-DG_LOG_DOMAIN="@0@"'.format(meson.project_name()),
  language: 'c'
)

#####
# CFLAGS
# Try to mimic the AX_COMPILER_FLAGS Autotools macro.
# Some flags are missing when using only the builtin warning_level meson option,
# even at the maximum level.
# The following warning_cflags suppose that warning_level=2.

warning_cflags = [
  '-fno-strict-aliasing',
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
add_project_arguments(supported_warning_cflags, language: 'c')
##### end CFLAGS

subdir('data')
subdir('plugins')
subdir('po')
subdir('help')
subdir('devhelp')
subdir('src')
subdir('unit-tests')

if get_option('gtk_doc')
  subdir('docs/reference')
endif

meson.add_install_script(
  find_program('build-aux/meson/meson_post_install.py'),
  get_option('prefix') / get_option('datadir'),
)

summary('Prefix', get_option('prefix'))
summary('Libdir', get_option('prefix') / get_option('libdir'))
summary('Datadir', get_option('prefix') / get_option('datadir'))

summary('Debug', get_option('debug'), section: 'Build')
summary('Optimization', get_option('optimization'), section: 'Build')
summary('API documentation', get_option('gtk_doc'), section: 'Build')
summary('Flatpak build mode', get_option('flatpak_build'), section: 'Build')
summary('Development build', devel_build, section: 'Build')

summary('Plugin for Emacs', get_option('plugin_emacs'), section: 'Plugins')
summary('Plugin for gedit', get_option('plugin_gedit'), section: 'Plugins')
summary('Plugin for Vim', get_option('plugin_vim'), section: 'Plugins')
