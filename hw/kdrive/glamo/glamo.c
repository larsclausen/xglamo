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
 * modified by:
 *   Dodji Seketeli <dodji@openedhand.com>
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif

#include "glamo.h"
#include "glamo-log.h"

static Bool
GLAMOCardInit(KdCardInfo *card)
{
	GLAMOCardInfo *glamoc;
	Bool initialized = FALSE;

	glamoc = xcalloc(sizeof(GLAMOCardInfo), 1);
	if (glamoc == NULL)
		return FALSE;

	if (!initialized
	    && fbdevInitialize(card, &glamoc->backend_priv.fbdev)) {
		initialized = TRUE;
		glamoc->backend_funcs.cardfini = fbdevCardFini;
		glamoc->backend_funcs.scrfini = fbdevScreenFini;
		glamoc->backend_funcs.initScreen = fbdevInitScreen;
		glamoc->backend_funcs.finishInitScreen = fbdevFinishInitScreen;
		glamoc->backend_funcs.createRes = fbdevCreateResources;
		glamoc->backend_funcs.preserve = fbdevPreserve;
		glamoc->backend_funcs.restore = fbdevRestore;
		glamoc->backend_funcs.dpms = fbdevDPMS;
		glamoc->backend_funcs.enable = fbdevEnable;
		glamoc->backend_funcs.disable = fbdevDisable;
		glamoc->backend_funcs.getColors = fbdevGetColors;
		glamoc->backend_funcs.putColors = fbdevPutColors;
#ifdef RANDR
		glamoc->backend_funcs.randrSetConfig = fbdevRandRSetConfig;
#endif
	}
	if (!initialized || !GLAMOMapReg(card, glamoc)) {
		xfree(glamoc);
		return FALSE;
	}

	card->driver = glamoc;

	glamoc->is_3362 = TRUE;
	ErrorF("Using GLAMO 3362 card\n");

	return TRUE;
}

static void
GLAMOCardFini(KdCardInfo *card)
{
	GLAMOCardInfo *glamoc = (GLAMOCardInfo *)card->driver;

	GLAMOUnmapReg(card, glamoc);
	glamoc->backend_funcs.cardfini(card);
}

/*
 * Once screen->off_screen_base is set, this function
 * allocates the remaining memory appropriately
 */

static void
GLAMOSetOffscreen (KdScreenInfo *screen)
{
	GLAMOCardInfo(screen);
	int screen_size;
	char *mmio = glamoc->reg_base;

	GLAMO_LOG("enter\n");
	screen->fb[0].byteStride =
		screen->width * screen->fb[0].bitsPerPixel / 8;
	screen->fb[0].pixelStride =
		screen->fb[0].byteStride * 8 / screen->fb[0].depth;

	GLAMO_LOG("mmio:%#x, bytestride: %d\n",
		  mmio, screen->fb[0].byteStride);

	/* check (and adjust) pitch */
	if (mmio)
	{
		GLAMO_LOG("mark\n");
		int	byteStride = screen->fb[0].byteStride;
		int	bitStride;
		int	pixelStride;
		int	bpp = screen->fb[0].bitsPerPixel;

		/*
		 * Ensure frame buffer is correctly aligned
		 */
		if (byteStride & 0x3f)
		{
			GLAMO_LOG("mark\n");
			byteStride = (byteStride + 0x3f) & ~0x3f;
			bitStride = byteStride * 8;
			pixelStride = bitStride / bpp;

			screen->fb[0].byteStride = byteStride;
			screen->fb[0].pixelStride = pixelStride;
		}
	}

	screen_size = screen->fb[0].byteStride * screen->height;
	screen->off_screen_base = screen_size;
	GLAMO_LOG("leave\n");
}

static Bool
GLAMOScreenInit(KdScreenInfo *screen)
{
	GLAMOScreenInfo *glamos;
	GLAMOCardInfo(screen);
	Bool success = FALSE;

	glamos = xcalloc(sizeof(GLAMOScreenInfo), 1);
	if (glamos == NULL)
		return FALSE;

	glamos->glamoc = glamoc;
	glamos->screen = screen;
	screen->driver = glamos;

	if (screen->fb[0].depth == 0)
		screen->fb[0].depth = 16;

	success = fbdevScreenInitialize(screen, &glamos->backend_priv.fbdev);

	if (!success) {
		screen->driver = NULL;
		xfree(glamos);
		return FALSE;
	}

	GLAMOSetOffscreen (screen);

	return TRUE;
}

static void
GLAMOScreenFini(KdScreenInfo *screen)
{
	GLAMOScreenInfo *glamos = (GLAMOScreenInfo *)screen->driver;
	GLAMOCardInfo *glamoc = screen->card->driver;

#ifdef XV
	GLAMOFiniVideo(screen->pScreen);
#endif

	glamoc->backend_funcs.scrfini(screen);
	xfree(glamos);
	screen->driver = 0;
}

Bool
GLAMOMapReg(KdCardInfo *card, GLAMOCardInfo *glamoc)
{
	glamoc->reg_base = (char *)KdMapDevice(GLAMO_REG_BASE(card),
	    GLAMO_REG_SIZE(card));

	if (glamoc->reg_base == NULL)
		return FALSE;

	KdSetMappedMode(GLAMO_REG_BASE(card), GLAMO_REG_SIZE(card),
	    KD_MAPPED_MODE_REGISTERS);

	return TRUE;
}

void
GLAMOUnmapReg(KdCardInfo *card, GLAMOCardInfo *glamoc)
{
	if (glamoc->reg_base) {
		KdResetMappedMode(GLAMO_REG_BASE(card), GLAMO_REG_SIZE(card),
		    KD_MAPPED_MODE_REGISTERS);
		KdUnmapDevice((void *)glamoc->reg_base, GLAMO_REG_SIZE(card));
		glamoc->reg_base = 0;
	}
}

void
GLAMODumpRegs(GLAMOScreenInfo *glamos,
	      CARD16 from,
	      CARD16 to)
{
#ifdef DEBUG
	GLAMOCardInfo *glamoc = glamos->glamoc;
	int i=0;
	for (i=from; i <= to; i += 2) {
		GLAMO_LOG("reg:%#x, val:%#x\n",
			  glamoc->reg_base+i,
			  *(VOL16*)(glamoc->reg_base+i));
	}
#endif
}

static Bool
GLAMOInitScreen(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);

#ifdef XV
	GLAMOInitVideo(pScreen);
#endif
	return glamoc->backend_funcs.initScreen(pScreen);
}

static Bool
GLAMOFinishInitScreen(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);

	if (!glamoc->backend_funcs.finishInitScreen(pScreen))
		return FALSE;
#ifdef RANDR
	if (!GLAMORandRInit (pScreen))
		return FALSE;
#endif
	return TRUE;
}

static Bool
GLAMOCreateResources(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);

	return glamoc->backend_funcs.createRes(pScreen);
}

static void
GLAMOPreserve(KdCardInfo *card)
{
	GLAMOCardInfo *glamoc = card->driver;

	glamoc->backend_funcs.preserve(card);
}

static void
GLAMORestore(KdCardInfo *card)
{
	GLAMOCardInfo *glamoc = card->driver;

	glamoc->backend_funcs.restore(card);
}

static Bool
GLAMODPMS(ScreenPtr pScreen, int mode)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);

	return glamoc->backend_funcs.dpms(pScreen, mode);
}

static Bool
GLAMOEnable(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);

	GLAMO_LOG("enter\n");

	if (!glamoc->backend_funcs.enable(pScreen)) {
		GLAMO_LOG_ERROR("failed to enable screen\n");
		return FALSE;
	}

	if ((glamoc->reg_base == NULL) && !GLAMOMapReg(pScreenPriv->screen->card,
	    glamoc)) {
		GLAMO_LOG_ERROR("failed to map glamo registers\n");
		return FALSE;
	}

	GLAMOSetOffscreen (pScreenPriv->screen);
	GLAMO_LOG("leave\n");

	return TRUE;
}

static void
GLAMODisable(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);

	GLAMOUnmapReg(pScreenPriv->card, glamoc);

	glamoc->backend_funcs.disable(pScreen);
}

static void
GLAMOGetColors(ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);

	glamoc->backend_funcs.getColors(pScreen, fb, n, pdefs);
}

static void
GLAMOPutColors(ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);

	glamoc->backend_funcs.putColors(pScreen, fb, n, pdefs);
}

KdCardFuncs GLAMOFuncs = {
	GLAMOCardInit,		/* cardinit */
	GLAMOScreenInit,	/* scrinit */
	GLAMOInitScreen,	/* initScreen */
	GLAMOFinishInitScreen,	/* finishInitScreen */
	GLAMOCreateResources,	/* createRes */
	GLAMOPreserve,		/* preserve */
	GLAMOEnable,		/* enable */
	GLAMODPMS,		/* dpms */
	GLAMODisable,		/* disable */
	GLAMORestore,		/* restore */
	GLAMOScreenFini,	/* scrfini */
	GLAMOCardFini,		/* cardfini */

	0,			/* initCursor */
	0,			/* enableCursor */
	0,			/* disableCursor */
	0,			/* finiCursor */
	0,			/* recolorCursor */

	GLAMODrawInit,		/* initAccel */
	GLAMODrawEnable,	/* enableAccel */
	GLAMODrawDisable,	/* disableAccel */
	GLAMODrawFini,		/* finiAccel */

	GLAMOGetColors,		/* getColors */
	GLAMOPutColors,		/* putColors */
};
