AM_CPPFLAGS = 					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/devhelp			\
	-I$(top_builddir)/devhelp		\
	-DG_LOG_DOMAIN=\"Devhelp\"		\
	-DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES	\
	$(WARN_CFLAGS)

AM_LDFLAGS =		\
	$(WARN_LDFLAGS)

app_headers =			\
	dh-app.h		\
	dh-assistant.h		\
	dh-tab.h		\
	dh-tab-label.h		\
	dh-web-view.h		\
	dh-window.h		\
	tepl-info-bar.h		\
	$(NULL)

app_c_files =			\
	dh-app.c		\
	dh-assistant.c		\
	dh-main.c		\
	dh-tab.c		\
	dh-tab-label.c		\
	dh-web-view.c		\
	dh-window.c		\
	tepl-info-bar.c		\
	$(NULL)

# The application
bin_PROGRAMS = devhelp

devhelp_SOURCES =	\
	$(app_headers)	\
	$(app_c_files)	\
	$(NULL)

devhelp_CPPFLAGS =	\
	$(AM_CPPFLAGS)

devhelp_CFLAGS =		\
	$(DEVHELP_CFLAGS)

devhelp_LDADD = 				\
	$(DEVHELP_LIBS)				\
	$(LIBM)					\
        $(top_builddir)/devhelp/libdevhelp-3.la

devhelp_LDFLAGS =	\
	$(AM_LDFLAGS)

-include $(top_srcdir)/git.mk
