INCLUDES = \
	$(EGG_CFLAGS)

noinst_LTLIBRARIES=libeggutil.la


eggmarshalers.h:
	cd $(srcdir) \
	&& $(GLIB_GENMARSHAL) --prefix=_egg_marshal eggmarshalers.list --header > xgen-emh \
	&& cp xgen-emh eggmarshalers.h \
	&& rm -f xgen-emh xgen-emh~

eggmarshalers.c:
	cd $(srcdir) \
	&& $(GLIB_GENMARSHAL) --prefix=_egg_marshal eggmarshalers.list --body > xgen-emc \
	&& cp xgen-emc eggmarshalers.c \
	&& rm -f xgen-emc xgen-emc~

egg-marshal.c: eggmarshalers.h eggmarshalers.c

libeggutil_la_SOURCES =	\
	egg-marshal.c

noinst_HEADERS = \
	eggmarshalers.h		\
	eggintl.h

EXTRA_DIST=			\
	eggmarshalers.list
