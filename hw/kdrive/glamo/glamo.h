/*
 * Copyright © 2007 OpenMoko, Inc.
 *
 * This driver is based on Xati,
 * Copyright © 2003 Eric Anholt
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Edited by:
 *   Dodji SEKETELI <dodji@openedhand.com>
 */

#ifndef _GLAMO_H_
#define _GLAMO_H_

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif

#include <fbdev.h>
#include "kxv.h"
#include "exa.h"

#define GLAMO_REG_BASE(c)		((c)->attr.address[0])
#define GLAMO_REG_SIZE(c)		(0x2400)

#ifdef __powerpc__

static __inline__ void
MMIO_OUT16(__volatile__ void *base, const unsigned long offset,
	   const unsigned int val)
{
	__asm__ __volatile__(
			"stwbrx %1,%2,%3\n\t"
			"eieio"
			: "=m" (*((volatile unsigned char *)base+offset))
			: "r" (val), "b" (base), "r" (offset));
}

static __inline__ CARD32
MMIO_IN16(__volatile__ void *base, const unsigned long offset)
{
	register unsigned int val;
	__asm__ __volatile__(
			"lwbrx %0,%1,%2\n\t"
			"eieio"
			: "=r" (val)
			: "b" (base), "r" (offset),
			"m" (*((volatile unsigned char *)base+offset)));
	return val;
}

#elif defined(__arm__) /* && !defined(__ARM_EABI__) */

static __inline__ void
MMIO_OUT16(__volatile__ void *base, const unsigned long offset,
       const unsigned int val)
{
    __asm__ __volatile__(
            "strh %0, [%1, +%2]"
            :
            : "r" (val), "r" (base), "r" (offset)
            : "memory" );
}

static __inline__ CARD16
MMIO_IN16(__volatile__ void *base, const unsigned long offset)
{
    register unsigned int val;
    __asm__ __volatile__(
            "ldrh %0, [%1, +%2]"
            : "=r" (val)
            : "r" (base), "r" (offset)
            : "memory");
    return val;
}

#else

#define MMIO_OUT16(mmio, a, v)		(*(VOL16 *)((mmio) + (a)) = (v))
#define MMIO_IN16(mmio, a)		(*(VOL16 *)((mmio) + (a)))

#endif

typedef volatile CARD8	VOL8;
typedef volatile CARD16	VOL16;
typedef volatile CARD32	VOL32;

struct backend_funcs {
	void    (*cardfini)(KdCardInfo *);
	void    (*scrfini)(KdScreenInfo *);
	Bool    (*initScreen)(ScreenPtr);
	Bool    (*finishInitScreen)(ScreenPtr pScreen);
	Bool	(*createRes)(ScreenPtr);
	void    (*preserve)(KdCardInfo *);
	void    (*restore)(KdCardInfo *);
	Bool    (*dpms)(ScreenPtr, int);
	Bool    (*enable)(ScreenPtr);
	void    (*disable)(ScreenPtr);
	void    (*getColors)(ScreenPtr, int, int, xColorItem *);
	void    (*putColors)(ScreenPtr, int, int, xColorItem *);
#ifdef RANDR
	Bool	(*randrSetConfig) (ScreenPtr, Rotation, int, RRScreenSizePtr);
#endif
};

typedef struct _GLAMOCardInfo {
	union {
		FbdevPriv fbdev;
	} backend_priv;
	struct backend_funcs backend_funcs;

	char *reg_base;
	Bool is_3362;
} GLAMOCardInfo;

#define getGLAMOCardInfo(kd)	((GLAMOCardInfo *) ((kd)->card->driver))
#define GLAMOCardInfo(kd)		GLAMOCardInfo *glamoc = getGLAMOCardInfo(kd)

typedef struct _GLAMOVideoFrameDisplayInfo {
	/*
	 * the offset of where the yuv video
	 * frame has been saved in offscreen
	 * video ram. The value 0 would mean
	 * the the video frame has been saved
	 * at the beginning of video ram.
	 */
	unsigned int offscreen_frame_addr;
	/*
	 * the fourcc code of the yuv frame
	 */
	int id;
	/*
	 * width/height of the source rectangle
	 * to crop in the yuv buffer.
	 * The cropped rectangle is the rectangle inside
	 * the yuv frame that is to be displayed in the destination
	 * X drawable.
	 */
	short int src_w;
	short int src_h;

	/*
	 * Destination drawable where to display the video frame
	 * that is at offscreen_frame_addr.
	 */
	DrawablePtr dest_drawable;

	/*
	 * The size of the box where to display the video frame, in
	 * the destination drawable.
	 */
	short int drw_w, drw_h;
	/*
	 * the box in which to display the video frame
	 * in the destination drawable. This might seem a bit redundant
	 * with drw_w, drw_h but is actually useful for the kind of
	 * computation that is needed by the XVideo apis.
	 */
	BoxRec dest_box;
	/*
	 * clipping region to display the video frame in, inside 
	 * the drawable.
	 */
	RegionRec clipped_dest_region;
} GLAMOVideoFrameDisplayInfo;

typedef struct _GLAMOPortPriv {
	CARD16 color_key;
	RegionRec clip;
	KdOffscreenArea *off_screen_yuv_buf;
	GLAMOVideoFrameDisplayInfo frame_display_info;
} GLAMOPortPrivRec, *GLAMOPortPrivPtr;

typedef struct _MemBuf {
	int size;
	int used;
	void *address;
} MemBuf;

typedef struct _GLAMOScreenInfo {
	union {
		FbdevScrPriv fbdev;
	} backend_priv;
	KaaScreenInfoRec kaa;
	ExaDriverRec exa;
	Bool use_exa;
	PixmapPtr srcPixmap;
	PixmapPtr dstPixmap;
	CARD32 src_offset;
	CARD32 dst_offset;
	CARD32 src_pitch;
	CARD32 dst_pitch;
	CARD32 settings;
	CARD32 foreground;

	GLAMOCardInfo *glamoc;
	KdScreenInfo *screen;

	int		scratch_offset;
	int		scratch_next;

	KdVideoAdaptorPtr pAdaptor;
	int		num_texture_ports;

	KdOffscreenArea *cmd_queue;	/* mmapped on-device cmd queue. */
	ExaOffscreenArea *exa_cmd_queue;
	CARD16		*ring_addr;	/* Beginning of ring buffer. */
	int		ring_write;	/* Index of write ptr in ring. */
	int		ring_read;	/* Index of read ptr in ring. */
	int		ring_len;
	/*
	 * cmd queue cache in system memory
	 * It is to be flushed to cmd_queue_space
	 * "at once", when we are happy with it.
	 */
	MemBuf		*cmd_queue_cache;
	int		cmd_queue_cache_start;
} GLAMOScreenInfo;

#define getGLAMOScreenInfo(kd)	((GLAMOScreenInfo *) ((kd)->screen->driver))
#define GLAMOScreenInfo(kd)	GLAMOScreenInfo *glamos = getGLAMOScreenInfo(kd)

static inline void
MMIOSetBitMask(char *mmio, CARD32 reg, CARD16 mask, CARD16 val)
{
	CARD16 tmp;

	val &= mask;

	tmp = MMIO_IN16(mmio, reg);
	tmp &= ~mask;
	tmp |= val;

	MMIO_OUT16(mmio, reg, tmp);
}

/* glamo.c */
Bool
GLAMOMapReg(KdCardInfo *card, GLAMOCardInfo *glamoc);

void
GLAMOUnmapReg(KdCardInfo *card, GLAMOCardInfo *glamoc);

void
GLAMODumpRegs(GLAMOScreenInfo *glamos,
	      CARD16 from,
	      CARD16 to);

/* glamo_draw.c */
void
GLAMODrawSetup(ScreenPtr pScreen);

Bool
GLAMODrawInit(ScreenPtr pScreen);

void
GLAMODrawEnable(ScreenPtr pScreen);

void
GLAMODrawDisable(ScreenPtr pScreen);

void
GLAMODrawFini(ScreenPtr pScreen);


/* glamo_video.c */
Bool
GLAMOInitVideo(ScreenPtr pScreen);

Bool
GLAMOVideoSetup(ScreenPtr pScreen);

void
GLAMOVideoTeardown(ScreenPtr pScreen);

void
GLAMOFiniVideo(ScreenPtr pScreen);

extern KdCardFuncs GLAMOFuncs;

#endif /* _GLAMO_H_ */
