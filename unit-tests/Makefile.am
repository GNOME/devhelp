AM_CPPFLAGS = 			\
	-I$(top_srcdir)/src 	\
	-I$(top_builddir)/src 	\
	$(WARN_CFLAGS) 		\
	$(DEVHELP_CFLAGS)

AM_LDFLAGS = $(WARN_LDFLAGS)

LDADD = $(top_builddir)/src/libdevhelp-3.la	\
	$(DEVHELP_LIBS)				\
	$(LIBM)

UNIT_TEST_PROGS =

UNIT_TEST_PROGS += test-link
test_link_SOURCES = test-link.c

noinst_PROGRAMS = $(UNIT_TEST_PROGS)
TESTS = $(UNIT_TEST_PROGS)

-include $(top_srcdir)/git.mk
