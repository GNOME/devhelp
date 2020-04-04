# SPDX-FileCopyrightText: 2018 Sébastien Wilmet <swilmet@gnome.org>
# SPDX-License-Identifier: GPL-3.0-or-later

unit_tests = [
  'test-completion',
  'test-link',
  'test-search-context',
  'test-util'
]

foreach test_name : unit_tests
  test_exe = executable(
    test_name,
    test_name + '.c',
    dependencies: LIBDEVHELP_STATIC_DEP
  )
  test(test_name, test_exe)
endforeach
