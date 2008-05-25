SUBDIRS = po src data misc contrib

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdevhelp-1.0.pc

INTLTOOL =                      \
	intltool-extract.in     \
	intltool-merge.in       \
	intltool-update.in

EXTRA_DIST =			\
	$(INTLTOOL)             \
	autogen.sh		\
	libdevhelp-1.0.pc.in    \
	misc/devhelp.el	        \
	misc/html2funcs.py	\
	misc/html2xml.py	\
	misc/devhelp.vim

DISTCLEANFILES = intltool-extract  \
		 intltool-merge    \
		 intltool-update
