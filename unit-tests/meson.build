# SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
# SPDX-License-Identifier: GPL-3.0-or-later

unit_tests = [
  'test-completion',
  'test-link',
  'test-search-context',
  'test-util'
]

foreach unit_test : unit_tests
  exe = executable(
    unit_test,
    unit_test + '.c',
    include_directories: ROOT_INCLUDE_DIR,
    dependencies: [LIBDEVHELP_DEPS, STATIC_LIBDEVHELP_DECLARED_DEP]
  )

  test(unit_test, exe)
endforeach
