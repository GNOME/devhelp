# Convention:
# - Local variables in lower_case.
# - Global variables in UPPER_CASE.
# See: https://github.com/mesonbuild/meson/issues/2607

project('devhelp', 'c',
        meson_version : '>= 0.43',
        version : '3.28.1')

DEVHELP_DEPS = [
        dependency('gio-2.0', version : '>= 2.40'),
        dependency('gtk+-3.0', version : '>= 3.22'),
        dependency('webkit2gtk-4.0', version : '>= 2.19.2'),
        dependency('gsettings-desktop-schemas')
]

add_global_arguments('-DG_LOG_DOMAIN="@0@"'.format(meson.project_name()),
                     language : 'c')

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

subdir('devhelp')
