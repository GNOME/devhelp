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
                include_directories : ROOT_INCLUDE_DIR,
                dependencies : [LIBDEVHELP_DEPS, LIBDEVHELP_STATIC_DEP]
        )

        test(unit_test, exe)
endforeach
