libdri_la_LTLIBRARIES = libdri.la
libdri_la_CFLAGS = -I$(top_srcdir)/hw/xfree86/common \
                   -I$(top_srcdir)/hw/xfree86/os-support \
                   -I$(top_srcdir)/hw/xfree86/os-support/bus \
                   -I$(top_srcdir)/glx \
                   -I$(top_srcdir)/GL/include \
                   -I$(top_builddir)/GL/include \
                   -DHAVE_XORG_CONFIG_H \
                   @DIX_CFLAGS@ @XORG_CFLAGS@ @DRIPROTO_CFLAGS@ \
                   @LIBDRM_CFLAGS@ \
                   @GL_CFLAGS@
libdri_la_LDFLAGS = -module -avoid-version @LIBDRM_LIBS@
libdri_ladir = $(moduledir)/extensions
libdri_la_SOURCES = \
	dri.c \
	dri.h \
	drimodule.c \
	dristruct.h \
	sarea.h \
	xf86dri.c

sdk_HEADERS = dri.h sarea.h dristruct.h
