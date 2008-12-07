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
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "glamo-log.h"
#include "glamo.h"
#include "glamo-regs.h"
#include "glamo-cmdq.h"
#include "glamo-draw.h"
#include "kaa.h"

static const CARD8 GLAMOSolidRop[16] = {
    /* GXclear      */      0x00,         /* 0 */
    /* GXand        */      0xa0,         /* src AND dst */
    /* GXandReverse */      0x50,         /* src AND NOT dst */
    /* GXcopy       */      0xf0,         /* src */
    /* GXandInverted*/      0x0a,         /* NOT src AND dst */
    /* GXnoop       */      0xaa,         /* dst */
    /* GXxor        */      0x5a,         /* src XOR dst */
    /* GXor         */      0xfa,         /* src OR dst */
    /* GXnor        */      0x05,         /* NOT src AND NOT dst */
    /* GXequiv      */      0xa5,         /* NOT src XOR dst */
    /* GXinvert     */      0x55,         /* NOT dst */
    /* GXorReverse  */      0xf5,         /* src OR NOT dst */
    /* GXcopyInverted*/     0x0f,         /* NOT src */
    /* GXorInverted */      0xaf,         /* NOT src OR dst */
    /* GXnand       */      0x5f,         /* NOT src OR NOT dst */
    /* GXset        */      0xff,         /* 1 */
};

static const CARD8 GLAMOBltRop[16] = {
    /* GXclear      */      0x00,         /* 0 */
    /* GXand        */      0x88,         /* src AND dst */
    /* GXandReverse */      0x44,         /* src AND NOT dst */
    /* GXcopy       */      0xcc,         /* src */
    /* GXandInverted*/      0x22,         /* NOT src AND dst */
    /* GXnoop       */      0xaa,         /* dst */
    /* GXxor        */      0x66,         /* src XOR dst */
    /* GXor         */      0xee,         /* src OR dst */
    /* GXnor        */      0x11,         /* NOT src AND NOT dst */
    /* GXequiv      */      0x99,         /* NOT src XOR dst */
    /* GXinvert     */      0x55,         /* NOT dst */
    /* GXorReverse  */      0xdd,         /* src OR NOT dst */
    /* GXcopyInverted*/     0x33,         /* NOT src */
    /* GXorInverted */      0xbb,         /* NOT src OR dst */
    /* GXnand       */      0x77,         /* NOT src OR NOT dst */
    /* GXset        */      0xff,         /* 1 */
};

static GLAMOScreenInfo *accel_glamos;
static CARD32 settings, color, src_pitch_offset, dst_pitch_offset;

int sample_count;
float sample_offsets_x[255];
float sample_offsets_y[255];

/********************************
 * exa entry points declarations
 ********************************/

Bool
GLAMOExaPrepareSolid(PixmapPtr      pPixmap,
		     int            alu,
		     Pixel          planemask,
		     Pixel          fg);

void
GLAMOExaSolid(PixmapPtr pPixmap, int x1, int y1, int x2, int y2);

void
GLAMOExaDoneSolid(PixmapPtr pPixmap);

void
GLAMOExaCopy(PixmapPtr pDstPixmap,
	     int    srcX,
	     int    srcY,
	     int    dstX,
	     int    dstY,
	     int    width,
	     int    height);

void
GLAMOExaDoneCopy(PixmapPtr pDstPixmap);

Bool
GLAMOExaCheckComposite(int op,
	       PicturePtr   pSrcPicture,
	       PicturePtr   pMaskPicture,
	       PicturePtr   pDstPicture);


Bool
GLAMOExaPrepareComposite(int                op,
			 PicturePtr         pSrcPicture,
			 PicturePtr         pMaskPicture,
			 PicturePtr         pDstPicture,
			 PixmapPtr          pSrc,
			 PixmapPtr          pMask,
			 PixmapPtr          pDst);

void
GLAMOExaComposite(PixmapPtr pDst,
		 int srcX,
		 int srcY,
		 int maskX,
		 int maskY,
		 int dstX,
		 int dstY,
		 int width,
		 int height);

Bool
GLAMOExaPrepareCopy(PixmapPtr       pSrcPixmap,
		    PixmapPtr       pDstPixmap,
		    int             dx,
		    int             dy,
		    int             alu,
		    Pixel           planemask);

void
GLAMOExaDoneComposite(PixmapPtr pDst);


Bool
GLAMOExaUploadToScreen(PixmapPtr pDst,
		       int x,
		       int y,
		       int w,
		       int h,
		       char *src,
		       int src_pitch);
Bool
GLAMOExaDownloadFromScreen(PixmapPtr pSrc,
			   int x,  int y,
			   int w,  int h,
			   char *dst,
			   int dst_pitch);

void
GLAMOExaWaitMarker (ScreenPtr pScreen, int marker);

static void
MarkForWait(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	if (glamos->use_exa) {
		exaMarkSync(pScreen);
	} else {
		kaaMarkSync(pScreen);
	}
}

static void
WaitSync(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	if (glamos->use_exa) {
		exaWaitSync(pScreen);
	} else {
		kaaWaitSync(pScreen);
	}
}

void
GLAMODrawSetup(ScreenPtr pScreen)
{
	GLAMOEngineEnable(pScreen, GLAMO_ENGINE_2D);
	GLAMOEngineReset(pScreen, GLAMO_ENGINE_2D);
}

static void
GLAMOWaitMarker(ScreenPtr pScreen, int marker)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);

	GLAMO_LOG("enter\n");
	GLAMOEngineWait(pScreen, GLAMO_ENGINE_ALL);
	GLAMO_LOG("leave\n");
}

static Bool
GLAMOPrepareSolid(PixmapPtr pPix, int alu, Pixel pm, Pixel fg)
{
	KdScreenPriv(pPix->drawable.pScreen);
	GLAMOScreenInfo(pScreenPriv);
	CARD32 offset, pitch;
	FbBits mask;
	RING_LOCALS;

	if (pPix->drawable.bitsPerPixel != 16) {
		GLAMO_LOG("pPix->drawable.bitsPerPixel:%d\n",
			  pPix->drawable.bitsPerPixel);
		GLAMO_FALLBACK(("Only 16bpp is supported\n"));
	}

	mask = FbFullMask(16);
	if ((pm & mask) != mask)
		GLAMO_FALLBACK(("Can't do planemask 0x%08x\n", (unsigned int) pm));

	accel_glamos = glamos;

	glamos->settings = GLAMOSolidRop[alu] << 8;
	glamos->src_offset = ((CARD8 *) pPix->devPrivate.ptr -
			pScreenPriv->screen->memory_base);
	glamos->src_pitch = pPix->devKind;
	glamos->srcPixmap = pPix;
	glamos->foreground = fg;

	GLAMO_LOG("enter:, src_offset:%#x, src_pitch:%d, fg%#x\n",
		  glamos->foreground,glamos->src_pitch, glamos->src_offset);

	/*
	BEGIN_CMDQ(12);
	OUT_REG(GLAMO_REG_2D_DST_ADDRL, offset & 0xffff);
	OUT_REG(GLAMO_REG_2D_DST_ADDRH, (offset >> 16) & 0x7f);
	OUT_REG(GLAMO_REG_2D_DST_PITCH, pitch);
	OUT_REG(GLAMO_REG_2D_DST_HEIGHT, pPix->drawable.height);
	OUT_REG(GLAMO_REG_2D_PAT_FG, fg);
	OUT_REG(GLAMO_REG_2D_COMMAND2, settings);
	END_CMDQ();
	*/

	kaaMarkSync(glamos->screen->pScreen);
	GLAMO_LOG("leave\n");

	return TRUE;
}

static void
GLAMOSolid(int x1, int y1, int x2, int y2)
{
	GLAMO_LOG("enter: (x1,y1,x2,y2):(%d,%d,%d,%d)\n",
		  x1, y1, x2, y2);
	GLAMOScreenInfo *glamos = accel_glamos;
	RING_LOCALS;

	BEGIN_CMDQ(26);
	OUT_REG(GLAMO_REG_2D_DST_ADDRL, glamos->src_offset & 0xffff);
	OUT_REG(GLAMO_REG_2D_DST_ADDRH, (glamos->src_offset >> 16) & 0x7f);
	OUT_REG(GLAMO_REG_2D_DST_PITCH, glamos->src_pitch);
	OUT_REG(GLAMO_REG_2D_DST_HEIGHT, glamos->srcPixmap->drawable.height);
	OUT_REG(GLAMO_REG_2D_PAT_FG, glamos->foreground);
	OUT_REG(GLAMO_REG_2D_COMMAND2, glamos->settings);
	OUT_REG(GLAMO_REG_2D_DST_X, x1);
	OUT_REG(GLAMO_REG_2D_DST_Y, y1);
	OUT_REG(GLAMO_REG_2D_RECT_WIDTH, x2 - x1);
	OUT_REG(GLAMO_REG_2D_RECT_HEIGHT, y2 - y1);
	OUT_REG(GLAMO_REG_2D_COMMAND3, 0);
	OUT_REG(GLAMO_REG_2D_ID1, 0);
	OUT_REG(GLAMO_REG_2D_ID2, 0);
	END_CMDQ();
	GLAMO_LOG("leave\n");
}

static void
GLAMODoneSolid(void)
{
	GLAMOScreenInfo *glamos = accel_glamos;
	kaaWaitSync(glamos->screen->pScreen);
	if (glamos->cmd_queue_cache)
		GLAMOFlushCMDQCache(glamos, 1);
}

static Bool
GLAMOPrepareCopy(PixmapPtr pSrc, PixmapPtr pDst,
		 int dx, int dy, int alu, Pixel pm)
{
	KdScreenPriv(pDst->drawable.pScreen);
	GLAMOScreenInfo(pScreenPriv);
	CARD32 src_offset, src_pitch;
	CARD32 dst_offset, dst_pitch;
	FbBits mask;
	RING_LOCALS;

	GLAMO_LOG("enter\n");

	if (pSrc->drawable.bitsPerPixel != 16 ||
	    pDst->drawable.bitsPerPixel != 16) {
		GLAMO_LOG("pSrc->drawable.bitsPerPixel:%d\n"
			  "pDst->drawable.bitsPerPixel:%d\n",
			  pSrc->drawable.bitsPerPixel,
			  pDst->drawable.bitsPerPixel);
		GLAMO_FALLBACK(("Only 16bpp is supported"));
	}

	mask = FbFullMask(16);
	if ((pm & mask) != mask)
		GLAMO_FALLBACK(("Can't do planemask 0x%08x", (unsigned int) pm));

	accel_glamos = glamos;
	glamos->src_offset = ((CARD8 *) pSrc->devPrivate.ptr -
				pScreenPriv->screen->memory_base);
	glamos->src_pitch = pSrc->devKind;
	glamos->dst_offset = ((CARD8 *) pDst->devPrivate.ptr -
				pScreenPriv->screen->memory_base);
	glamos->dst_pitch = pDst->devKind;
	glamos->settings = GLAMOBltRop[alu] << 8;
	glamos->srcPixmap = pSrc;
	glamos->dstPixmap = pDst;

	kaaMarkSync(pDst->drawable.pScreen);
	GLAMO_LOG("leave\n");

	return TRUE;
}

static void
GLAMOCopy(int srcX, int srcY, int dstX, int dstY, int w, int h)
{
	GLAMOScreenInfo *glamos = accel_glamos;
	RING_LOCALS;

	GLAMO_LOG("enter: src(%d,%d), dst(%d,%d), wxh(%dx%d)\n",
		  srcX, srcY, dstX, dstY, w, h);
	GLAMO_LOG("src_offset:%#x, dst_offset:%#x\n",
		  glamos->src_offset, glamos->dst_offset);

	BEGIN_CMDQ(34);

	OUT_REG(GLAMO_REG_2D_SRC_ADDRL, glamos->src_offset & 0xffff);
	OUT_REG(GLAMO_REG_2D_SRC_ADDRH, (glamos->src_offset >> 16) & 0x7f);
	OUT_REG(GLAMO_REG_2D_SRC_PITCH, glamos->src_pitch & 0x7ff);
	OUT_REG(GLAMO_REG_2D_SRC_X, srcX & 0x7ff);
	OUT_REG(GLAMO_REG_2D_SRC_Y, srcY & 0x7ff);
	OUT_REG(GLAMO_REG_2D_DST_X, dstX & 0x7ff);
	OUT_REG(GLAMO_REG_2D_DST_Y, dstY & 0x7ff);
	OUT_REG(GLAMO_REG_2D_DST_ADDRL, glamos->dst_offset & 0xffff);
	OUT_REG(GLAMO_REG_2D_DST_ADDRH, (glamos->dst_offset >> 16) & 0x7f);
	OUT_REG(GLAMO_REG_2D_DST_PITCH, glamos->dst_pitch & 0x7ff);
	OUT_REG(GLAMO_REG_2D_DST_HEIGHT,
		glamos->dstPixmap->drawable.height & 0x3ff);
	OUT_REG(GLAMO_REG_2D_RECT_WIDTH, w & 0x3ff);
	OUT_REG(GLAMO_REG_2D_RECT_HEIGHT, h & 0x3ff);

	OUT_REG(GLAMO_REG_2D_COMMAND2, glamos->settings & 0xffff);

	OUT_REG(GLAMO_REG_2D_COMMAND3, 0);
	OUT_REG(GLAMO_REG_2D_ID1, 0);
	OUT_REG(GLAMO_REG_2D_ID2, 0);
	END_CMDQ();

	GLAMO_LOG("leave\n");
}

static void
GLAMODoneCopy(void)
{
	GLAMOScreenInfo *glamos = accel_glamos;
	GLAMO_LOG("enter\n");
	kaaWaitSync(glamos->screen->pScreen);
	kaaMarkSync(glamos->screen->pScreen);
	if (glamos->cmd_queue_cache)
		GLAMOFlushCMDQCache(glamos, 1);
	kaaWaitSync(glamos->screen->pScreen);
	GLAMO_LOG("leave\n");
}

static Bool
GLAMOUploadToScreen(PixmapPtr pDst, char *src, int src_pitch)
{
	int width, height, bpp, i;
	CARD8 *dst_offset;
	int dst_pitch;

        GLAMO_LOG("enter\n");
	dst_offset = (CARD8 *)pDst->devPrivate.ptr;
	dst_pitch = pDst->devKind;
	width = pDst->drawable.width;
	height = pDst->drawable.height;
	bpp = pDst->drawable.bitsPerPixel;
	bpp /= 8;

	GLAMO_LOG("wxh(%dx%d), bpp:%d, dst_pitch:%d, src_pitch:%d\n",
		  width, height, bpp, dst_pitch, src_pitch);

	for (i = 0; i < height; i++)
	{
		memcpy(dst_offset, src, width * bpp);

		dst_offset += dst_pitch;
		src += src_pitch;
	}

	return TRUE;
}

static void
GLAMOBlockHandler(pointer blockData, OSTimePtr timeout, pointer readmask)
{
	ScreenPtr pScreen = (ScreenPtr) blockData;
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);

	/* When the server is going to sleep,
	 * make sure that the cmd queue cache
	 * has been flushed.
	 */
	if (glamos->use_exa)
		exaWaitSync(pScreen);
	else
		kaaWaitSync(pScreen);
	if (glamos->cmd_queue_cache)
		GLAMOFlushCMDQCache(glamos, 1);
}

static void
GLAMOWakeupHandler(pointer blockData, int result, pointer readmask)
{
}

Bool
GLAMODrawKaaInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	KdScreenInfo *screen = pScreenPriv->screen;
	int offscreen_memory_size = 0;  

	offscreen_memory_size =
            screen->memory_size - screen->off_screen_base;

	LogMessage(X_INFO,
		   "vram size:%d, "
		   "onscreen vram size:%d, "
		   "offscreen vram size:%d\n",
		   screen->memory_size,
		   screen->off_screen_base,
		   offscreen_memory_size);

	GLAMO_LOG("enter\n");

	RegisterBlockAndWakeupHandlers(GLAMOBlockHandler, GLAMOWakeupHandler,
	    pScreen);

	memset(&glamos->kaa, 0, sizeof(KaaScreenInfoRec));
	glamos->kaa.waitMarker = GLAMOWaitMarker;
	glamos->kaa.PrepareSolid = GLAMOPrepareSolid;
	glamos->kaa.Solid = GLAMOSolid;
	glamos->kaa.DoneSolid = GLAMODoneSolid;
	glamos->kaa.PrepareCopy = GLAMOPrepareCopy;
	glamos->kaa.Copy = GLAMOCopy;
	glamos->kaa.DoneCopy = GLAMODoneCopy;
	glamos->kaa.UploadToScreen = GLAMOUploadToScreen;

	if (offscreen_memory_size > 0) {
		glamos->kaa.flags = KAA_OFFSCREEN_PIXMAPS;
	}
	glamos->kaa.offsetAlign = 16;
	glamos->kaa.pitchAlign = 16;

	if (!kaaDrawInit(pScreen, &glamos->kaa)) {
		GLAMO_LOG_ERROR("failed to init kaa\n");
		return FALSE;
	}

	GLAMO_LOG("leave\n");
	return TRUE;
}

/**
 * exaDDXDriverInit is required by the top-level EXA module, and is used by
 * the xorg DDX to hook in its EnableDisableFB wrapper.  We don't need it, since
 * we won't be enabling/disabling the FB.
 */
#ifndef GetGLAMOExaPriv
#define GetGLAMOExaPriv(pScreen) \
(GLAMOScreenInfo*)dixLookupPrivate(&pScreen->devPrivates, glamoScreenPrivateKey)
#endif
void
exaDDXDriverInit(ScreenPtr pScreen)
{
}

static DevPrivateKey glamoScreenPrivateKey = &glamoScreenPrivateKey;

Bool
GLAMODrawExaInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	KdScreenInfo *screen = pScreenPriv->screen;
	int offscreen_memory_size = 0;
	char *use_exa = NULL;
	Bool success = FALSE;

	GLAMO_LOG("enter\n");

	memset(&glamos->exa, 0, sizeof(ExaDriverRec));
	glamos->exa.memoryBase = screen->memory_base;
	glamos->exa.memorySize = screen->memory_size;
	glamos->exa.offScreenBase = screen->off_screen_base;

	glamos->exa.exa_major = 2;
	glamos->exa.exa_minor = 0;

	glamos->exa.PrepareSolid = GLAMOExaPrepareSolid;
	glamos->exa.Solid = GLAMOExaSolid;
	glamos->exa.DoneSolid = GLAMOExaDoneSolid;

	glamos->exa.PrepareCopy = GLAMOExaPrepareCopy;
	glamos->exa.Copy = GLAMOExaCopy;
	glamos->exa.DoneCopy = GLAMOExaDoneCopy;

	glamos->exa.CheckComposite = GLAMOExaCheckComposite;
	glamos->exa.PrepareComposite = GLAMOExaPrepareComposite;
	glamos->exa.Composite = GLAMOExaComposite;
	glamos->exa.DoneComposite = GLAMOExaDoneComposite;


	glamos->exa.DownloadFromScreen = GLAMOExaDownloadFromScreen;
	glamos->exa.UploadToScreen = GLAMOExaUploadToScreen;

	/*glamos->exa.MarkSync = GLAMOExaMarkSync;*/
	glamos->exa.WaitMarker = GLAMOExaWaitMarker;

	glamos->exa.pixmapOffsetAlign = 1;
	glamos->exa.pixmapPitchAlign = 1;

	glamos->exa.maxX = 640;
	glamos->exa.maxY = 640;

	glamos->exa.flags = EXA_OFFSCREEN_PIXMAPS;

	RegisterBlockAndWakeupHandlers(GLAMOBlockHandler,
				       GLAMOWakeupHandler,
				       pScreen);

    dixSetPrivate(&pScreen->devPrivates, glamoScreenPrivateKey, glamos);
	success = exaDriverInit(pScreen, &glamos->exa);
	if (success) {
		ErrorF("Initialized EXA acceleration\n");
	} else {
		ErrorF("Failed to initialize EXA acceleration\n");
	}
	GLAMO_LOG("leave\n");

	return success;
}

Bool
GLAMODrawInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	KdScreenInfo *screen = pScreenPriv->screen;
	int offscreen_memory_size = 0;
	char *use_exa = NULL;

	LogMessage(X_INFO, "Screen: %d/%d depth/bpp\n",
		  pScreenPriv->screen->fb[0].depth,
		  pScreenPriv->screen->fb[0].bitsPerPixel);

	use_exa = getenv("USE_EXA");
	if (use_exa && !strcmp(use_exa, "yes")) {
		glamos->use_exa = TRUE;
		return GLAMODrawExaInit(pScreen);
	} else {
		glamos->use_exa = FALSE;
		return GLAMODrawKaaInit(pScreen);
	}
}

#if 0
static void
GLAMOScratchSave(ScreenPtr pScreen, KdOffscreenArea *area)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);

	glamos->scratch_area = NULL;
}
#endif

void
GLAMODrawEnable(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);

	GLAMO_LOG("enter\n");
	GLAMOCMDQCacheSetup(pScreen);
	GLAMODrawSetup(pScreen);
	GLAMOEngineWait(pScreen, GLAMO_ENGINE_ALL);
	GLAMO_LOG("leave\n");


}


void
GLAMODrawDisable(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	if (!glamos->use_exa) {
		kaaWaitSync(pScreen);
	}
	GLAMOCMQCacheTeardown(pScreen);
}

void
GLAMODrawFini(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	RemoveBlockAndWakeupHandlers(GLAMOBlockHandler, GLAMOWakeupHandler,
	    pScreen);

	if (!glamos->use_exa) {
		kaaDrawFini(pScreen);
	}
}

/***************************************
 * <glamo exa entry point definitions>
 ***************************************/
Bool
GLAMOExaPrepareSolid(PixmapPtr      pPix,
		     int            alu,
		     Pixel          pm,
		     Pixel          fg)
{
	KdScreenPriv(pPix->drawable.pScreen);
	GLAMOScreenInfo(pScreenPriv);
	CARD32 offset, pitch;
	FbBits mask;
	RING_LOCALS;

	if (pPix->drawable.bitsPerPixel != 16)
		GLAMO_FALLBACK(("Only 16bpp is supported\n"));

	mask = FbFullMask(16);
	if ((pm & mask) != mask)
		GLAMO_FALLBACK(("Can't do planemask 0x%08x\n",
				(unsigned int) pm));

	settings = GLAMOSolidRop[alu] << 8;
	offset = exaGetPixmapOffset(pPix);
	pitch = pPix->devKind;

	GLAMO_LOG("enter.pitch:%d\n", pitch);

	BEGIN_CMDQ(12);
	OUT_REG(GLAMO_REG_2D_DST_ADDRL, offset & 0xffff);
	OUT_REG(GLAMO_REG_2D_DST_ADDRH, (offset >> 16) & 0x7f);
	OUT_REG(GLAMO_REG_2D_DST_PITCH, pitch);
	OUT_REG(GLAMO_REG_2D_DST_HEIGHT, pPix->drawable.height);
	OUT_REG(GLAMO_REG_2D_PAT_FG, fg);
	OUT_REG(GLAMO_REG_2D_COMMAND2, settings);
	END_CMDQ();
	GLAMO_LOG("leave\n");

	return TRUE;
}

void
GLAMOExaSolid(PixmapPtr pPix, int x1, int y1, int x2, int y2)
{
	GLAMO_LOG("enter\n");
	GLAMOScreenInfo *glamos = GetGLAMOExaPriv(pPix->drawable.pScreen);
	RING_LOCALS;

	BEGIN_CMDQ(14);
	OUT_REG(GLAMO_REG_2D_DST_X, x1);
	OUT_REG(GLAMO_REG_2D_DST_Y, y1);
	OUT_REG(GLAMO_REG_2D_RECT_WIDTH, x2 - x1);
	OUT_REG(GLAMO_REG_2D_RECT_HEIGHT, y2 - y1);
	OUT_REG(GLAMO_REG_2D_COMMAND3, 0);
	OUT_REG(GLAMO_REG_2D_ID1, 0);
	OUT_REG(GLAMO_REG_2D_ID2, 0);
	END_CMDQ();
	GLAMO_LOG("leave\n");
}

void
GLAMOExaDoneSolid(PixmapPtr pPix)
{
	GLAMOScreenInfo *glamos = GetGLAMOExaPriv(pPix->drawable.pScreen);
	exaWaitSync(glamos->screen->pScreen);
	if (glamos->cmd_queue_cache)
		GLAMOFlushCMDQCache(glamos, 1);
}

Bool
GLAMOExaPrepareCopy(PixmapPtr       pSrc,
		    PixmapPtr       pDst,
		    int             dx,
		    int             dy,
		    int             alu,
		    Pixel           pm)
{
	KdScreenPriv(pDst->drawable.pScreen);
	GLAMOScreenInfo *glamos = GetGLAMOExaPriv(pDst->drawable.pScreen);
	CARD32 src_offset, src_pitch;
	CARD32 dst_offset, dst_pitch;
	FbBits mask;
	RING_LOCALS;

	GLAMO_LOG("enter\n");

	if (pSrc->drawable.bitsPerPixel != 16 ||
	    pDst->drawable.bitsPerPixel != 16)
		GLAMO_FALLBACK(("Only 16bpp is supported"));

	mask = FbFullMask(16);
	if ((pm & mask) != mask) {
		GLAMO_FALLBACK(("Can't do planemask 0x%08x",
				(unsigned int) pm));
	}

	glamos->src_offset = exaGetPixmapOffset(pSrc);
	glamos->src_pitch = pSrc->devKind;

	glamos->dst_offset = exaGetPixmapOffset(pDst);
	glamos->dst_pitch = pDst->devKind;
	GLAMO_LOG("src_offset:%d, src_pitch:%d, "
		  "dst_offset:%d, dst_pitch:%d, mem_base:%#x\n",
		  glamos->src_offset,
		  glamos->src_pitch,
		  glamos->dst_offset,
		  glamos->dst_pitch,
		  pScreenPriv->screen->memory_base);

	glamos->settings = GLAMOBltRop[alu] << 8;
	exaMarkSync(pDst->drawable.pScreen);
	GLAMO_LOG("leave\n");
	return TRUE;
}

void
GLAMOExaCopy(PixmapPtr       pDst,
	      int    srcX,
	      int    srcY,
	      int    dstX,
	      int    dstY,
	      int    width,
	      int    height)
{
	GLAMOScreenInfo *glamos = GetGLAMOExaPriv(pDst->drawable.pScreen);
	RING_LOCALS;

	GLAMO_LOG("enter (%d,%d,%d,%d),(%dx%d)\n",
		  srcX, srcY, dstX, dstY,
		  width, height);

	BEGIN_CMDQ(34);

	OUT_REG(GLAMO_REG_2D_SRC_ADDRL, glamos->src_offset & 0xffff);
	OUT_REG(GLAMO_REG_2D_SRC_ADDRH, (glamos->src_offset >> 16) & 0x7f);
	OUT_REG(GLAMO_REG_2D_SRC_PITCH, glamos->src_pitch);

	OUT_REG(GLAMO_REG_2D_DST_ADDRL, glamos->dst_offset & 0xffff);
	OUT_REG(GLAMO_REG_2D_DST_ADDRH, (glamos->dst_offset >> 16) & 0x7f);
	OUT_REG(GLAMO_REG_2D_DST_PITCH, glamos->dst_pitch);
	OUT_REG(GLAMO_REG_2D_DST_HEIGHT, pDst->drawable.height);

	OUT_REG(GLAMO_REG_2D_COMMAND2, glamos->settings);

	OUT_REG(GLAMO_REG_2D_SRC_X, srcX);
	OUT_REG(GLAMO_REG_2D_SRC_Y, srcY);
	OUT_REG(GLAMO_REG_2D_DST_X, dstX);
	OUT_REG(GLAMO_REG_2D_DST_Y, dstY);
	OUT_REG(GLAMO_REG_2D_RECT_WIDTH, width);
	OUT_REG(GLAMO_REG_2D_RECT_HEIGHT, height);
	OUT_REG(GLAMO_REG_2D_COMMAND3, 0);
	OUT_REG(GLAMO_REG_2D_ID1, 0);
	OUT_REG(GLAMO_REG_2D_ID2, 0);
	END_CMDQ();
	GLAMO_LOG("leave\n");
}

void
GLAMOExaDoneCopy(PixmapPtr pDst)
{
	GLAMOScreenInfo *glamos = GetGLAMOExaPriv(pDst->drawable.pScreen);
	GLAMO_LOG("enter\n");
	exaWaitSync(glamos->screen->pScreen);
	if (glamos->cmd_queue_cache)
		GLAMOFlushCMDQCache(glamos, 1);
	GLAMO_LOG("leave\n");
}

Bool
GLAMOExaCheckComposite(int op,
		       PicturePtr   pSrcPicture,
		       PicturePtr   pMaskPicture,
		       PicturePtr   pDstPicture)
{
	return FALSE;
}

Bool
GLAMOExaPrepareComposite(int                op,
			 PicturePtr         pSrcPicture,
			 PicturePtr         pMaskPicture,
			 PicturePtr         pDstPicture,
			 PixmapPtr          pSrc,
			 PixmapPtr          pMask,
			 PixmapPtr          pDst)
{
	return FALSE;
}

void
GLAMOExaComposite(PixmapPtr pDst,
		 int srcX,
		 int srcY,
		 int maskX,
		 int maskY,
		 int dstX,
		 int dstY,
		 int width,
		 int height)
{
}

void
GLAMOExaDoneComposite(PixmapPtr pDst)
{
}

Bool
GLAMOExaUploadToScreen(PixmapPtr pDst,
		       int x,
		       int y,
		       int w,
		       int h,
		       char *src,
		       int src_pitch)
{
	int bpp, i;
	CARD8 *dst_offset;
	int dst_pitch;
	KdScreenPriv(pDst->drawable.pScreen);
	GLAMOScreenInfo *glamos = GetGLAMOExaPriv(pDst->drawable.pScreen);

	GLAMO_LOG("enter\n");
	bpp = pDst->drawable.bitsPerPixel / 8;
	dst_pitch = pDst->devKind;
	dst_offset = glamos->exa.memoryBase + exaGetPixmapOffset(pDst)
						+ x*bpp + y*dst_pitch;

	GLAMO_LOG("dst_pitch:%d, src_pitch\n", dst_pitch, src_pitch);
	for (i = 0; i < h; i++) {
		memcpy(dst_offset, src, w*bpp);
		dst_offset += dst_pitch;
		src += src_pitch;
	}

	return TRUE;
}

Bool
GLAMOExaDownloadFromScreen(PixmapPtr pSrc,
			   int x,  int y,
			   int w,  int h,
			   char *dst,
			   int dst_pitch)
{
	int bpp, i;
	CARD8 *dst_offset, *src;
	int src_pitch;
	KdScreenPriv(pSrc->drawable.pScreen);
	GLAMOScreenInfo *glamos = GetGLAMOExaPriv(pSrc->drawable.pScreen);

        GLAMO_LOG("enter\n");
	bpp = pSrc->drawable.bitsPerPixel;
	bpp /= 8;
	src_pitch = pSrc->devKind;
	src = glamos->exa.memoryBase + exaGetPixmapOffset(pSrc) +
						x*bpp + y*src_pitch;
	dst_offset = dst ;

	GLAMO_LOG("dst_pitch:%d, src_pitch\n", dst_pitch, src_pitch);
	for (i = 0; i < h; i++) {
		memcpy(dst_offset, src, w*bpp);
		dst_offset += dst_pitch;
		src += src_pitch;
	}

	return TRUE;
}

void
GLAMOExaWaitMarker (ScreenPtr pScreen, int marker)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo *glamos = GetGLAMOExaPriv(pScreen);

	GLAMO_LOG("enter\n");
	GLAMOEngineWait(pScreen, GLAMO_ENGINE_ALL);
	GLAMO_LOG("leave\n");
}

/**********************
 * </glamo exa functions>
 **********************/
