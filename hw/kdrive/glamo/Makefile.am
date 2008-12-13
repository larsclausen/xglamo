if KDRIVEFBDEV
FBDEV_INCLUDES =-I$(top_srcdir)/hw/kdrive/fbdev
FBDEV_LIBS = $(top_builddir)/hw/kdrive/fbdev/libfbdev.a
endif


INCLUDES = 					\
	-I$(top_srcdir)/exa			\
	@KDRIVE_INCS@				\
	$(FBDEV_INCLUDES)			\
	@KDRIVE_CFLAGS@

bin_PROGRAMS = Xglamo

noinst_LIBRARIES = libglamo.a

libglamo_a_SOURCES = 	\
	glamo-log.h	\
	glamo-cmdq.c	\
	glamo-cmdq.h	\
	glamo-draw.c	\
	glamo-draw.h	\
	glamo.c		\
	glamo.h		\
	glamo-regs.h	\
	glamo-video.c \
	glamo-funcs.c \
	glamo-funcs.h \
	glamo-randr.c \
	glamo-cursor.c

Xglamo_SOURCES = \
	glamo-stub.c

GLAMO_LIBS =				\
	libglamo.a 			\
	$(top_builddir)/exa/libexa.la   \
	$(FBDEV_LIBS)			\
	@KDRIVE_LIBS@

Xglamo_LDADD = \
	$(GLAMO_LIBS)			\
	@KDRIVE_LIBS@

Xglamo_DEPENDENCIES =	\
	libglamo.a 			\
	$(FBDEV_LIBS)

relink:
	rm -f $(bin_PROGRAMS) && make $(bin_PROGRAMS)
