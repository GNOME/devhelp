AM_CPPFLAGS = 			\
	-I$(top_srcdir)/src 	\
	-I$(top_builddir)/src 	\
	$(WARN_CFLAGS) 		\
	$(DEVHELP_CFLAGS)

AM_LDFLAGS = $(WARN_LDFLAGS)

LDADD = $(top_builddir)/src/libdevhelp-core.la	\
	$(DEVHELP_LIBS)				\
	$(LIBM)

UNIT_TEST_PROGS =

UNIT_TEST_PROGS += test-link
test_link_SOURCES = test-link.c

UNIT_TEST_PROGS += test-util
test_util_SOURCES = test-util.c

noinst_PROGRAMS = $(UNIT_TEST_PROGS)
TESTS = $(UNIT_TEST_PROGS)

-include $(top_srcdir)/git.mk
