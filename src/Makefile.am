AM_CPPFLAGS = 						\
	-DLOCALEDIR=\""$(datadir)/locale"\" 	  	\
	-DPREFIX="\"$(prefix)"\"		 	\
	-DDATADIR=\""$(datadir)"\"                      \
	-DG_LOG_DOMAIN=\"Devhelp\"			\
	-DG_DISABLE_DEPRECATED				\
	-DGDK_DISABLE_DEPRECATED			\
	-DGTK_DISABLE_DEPRECATED			\
	$(WARN_CFLAGS)

bin_PROGRAMS = devhelp

devhelp_SOURCES =					\
	dh-main.c

devhelp_CPPFLAGS =					\
	$(AM_CPPFLAGS)

devhelp_CFLAGS =					\
	$(DEVHELP_CFLAGS)

devhelp_LDADD = 					\
        $(DEVHELP_LIBS)					\
        libdevhelp-1.la

lib_LTLIBRARIES = libdevhelp-1.la

devhelpincludedir = $(includedir)/devhelp-1.0/devhelp
devhelpinclude_HEADERS = 				\
	dh-assistant.h					\
	dh-assistant-view.h				\
	dh-base.h					\
	dh-book-tree.h					\
	dh-error.h					\
	dh-keyword-model.h				\
	dh-link.h					\
	dh-search.h					\
	dh-window.h

libdevhelp_1_la_SOURCES =				\
	bacon-message-connection.c			\
	bacon-message-connection.h			\
	dh-marshal.c					\
	dh-marshal.h					\
	dh-assistant.c					\
	dh-assistant-view.c				\
	dh-base.c					\
	dh-book-tree.c					\
	dh-error.c					\
	dh-keyword-model.c				\
	dh-link.c					\
	dh-search.c					\
	dh-parser.c					\
	dh-preferences.c				\
	dh-util.c					\
	dh-window.c					\
	dh-parser.h					\
	dh-preferences.h                                \
	dh-util.h					\
	eggfindbar.c					\
	eggfindbar.h					\
	ige-conf.h

if HAVE_PLATFORM_OSX
libdevhelp_1_la_SOURCES +=				\
	ige-conf-mac.m
endif
if HAVE_PLATFORM_X11
libdevhelp_1_la_SOURCES +=				\
	ige-conf-gconf.c
endif

libdevhelp_1_la_CPPFLAGS =				\
	$(AM_CPPFLAGS)					\
	$(ZLIB_CPPFLAGS)

libdevhelp_1_la_CFLAGS =				\
	$(LIBDEVHELP_CFLAGS)

libdevhelp_1_la_OBJCFLAGS =				\
	$(LIBDEVHELP_CFLAGS)

libdevhelp_1_la_LIBADD =				\
	$(LIBDEVHELP_LIBS)				\
	$(ZLIB_LIBS)

libdevhelp_1_la_LDFLAGS =				\
	$(ZLIB_LDFLAGS)

MARSHAL=dh
$(MARSHAL)-marshal.h: $(MARSHAL)-marshal.list
	(cd $(srcdir) && \
	$(GLIB_GENMARSHAL) --prefix=_$(MARSHAL)_marshal $(MARSHAL)-marshal.list --header) > xgen-gmh \
	&& (cmp -s xgen-gmh $(MARSHAL)-marshal.h || cp xgen-gmh $(MARSHAL)-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~

$(MARSHAL)-marshal.c: $(MARSHAL)-marshal.list
	(cd $(srcdir) && \
	echo "#include \"$(MARSHAL)-marshal.h\""; \
	$(GLIB_GENMARSHAL) --prefix=_$(MARSHAL)_marshal $(MARSHAL)-marshal.list --body) > xgen-gmc \
	&& cp xgen-gmc $(MARSHAL)-marshal.c \
	&& rm -f xgen-gmc xgen-gmc~

BUILT_SOURCES = 			\
	$(MARSHAL)-marshal.h		\
	$(MARSHAL)-marshal.c		\
	$(NULL)

EXTRA_DIST =				\
	$(MARSHAL)-marshal.list		\
	$(NULL)
