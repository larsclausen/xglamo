AM_CFLAGS = $(DIX_CFLAGS) $(XSERVER_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/hw/xfree86/os-support

SUBDIRS = safeAlpha accel

noinst_LTLIBRARIES = librootless.la
librootless_la_SOURCES = \
	rootlessCommon.c \
	rootlessGC.c \
	rootlessScreen.c \
	rootlessValTree.c \
	rootlessWindow.c

EXTRA_DIST = \
	README.txt \
	rootless.h \
	rootlessCommon.h \
	rootlessConfig.h \
	rootlessWindow.h 
