AM_CPPFLAGS =				\
	-I$(top_srcdir)/devhelp		\
	-I$(top_builddir)/devhelp	\
	$(WARN_CFLAGS)			\
	$(DEVHELP_CFLAGS)

AM_LDFLAGS = $(WARN_LDFLAGS)

LDADD = $(top_builddir)/devhelp/libdevhelp-core.la	\
	$(DEVHELP_LIBS)					\
	$(LIBM)

UNIT_TEST_PROGS =

UNIT_TEST_PROGS += test-completion
test_completion_SOURCES = test-completion.c

UNIT_TEST_PROGS += test-link
test_link_SOURCES = test-link.c

UNIT_TEST_PROGS += test-search-context
test_search_context_SOURCES = test-search-context.c

UNIT_TEST_PROGS += test-util
test_util_SOURCES = test-util.c

noinst_PROGRAMS = $(UNIT_TEST_PROGS)
TESTS = $(UNIT_TEST_PROGS)

-include $(top_srcdir)/git.mk
