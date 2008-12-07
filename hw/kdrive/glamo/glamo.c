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
	FbdevPriv *priv = screen->card->driver;
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

#ifdef RANDR
static KdMonitorTiming  glamoMonitorTimings[] = {
	{/*horiz size*/640,  /*vert size*/480, /*frame rate(Hz)*/23,
	 /*pixel clock(KHz)*/12250,
	 /*horiz back porsh(pixels)*/8, /*horiz front porsh(pixels)*/16,
	 /*hblank(pixels)*/32, /*horiz polarity*/KdSyncNegative,
	 /*vertical back porsh(pixels)*/2, /*vert front porsh(pixels)*/16,
	 /*vblank(pixels)*/20, /*horiz polarity*/KdSyncNegative,
        },
	{/*horiz size*/480,  /*vert size*/640, /*frame rate(Hz)*/60,
	 /*pixel clock(KHz)*/24500,
	 /*horiz back porsh(pixels)*/8, /*horiz front porsh(pixels)*/16,
	 /*hblank(pixels)*/32, /*horiz polarity*/KdSyncNegative,
	 /*vert back porsh(pixels)*/2, /*vert front porsh(pixels)*/16,
	 /*vblank(pixels)*/20, /*horiz polarity*/KdSyncNegative,
        },
	{/*horiz size*/320,  /*vert size*/240, /*frame rate(Hz)*/23,
	 /*pixel clock(KHz)*/12250,
	 /*horiz back porsh(pixels)*/8, /*horiz front porsh(pixels)*/16,
	 /*hblank(pixels)*/32, /*horiz polarity*/KdSyncNegative,
	 /*horiz back porsh(pixels)*/2, /*horiz front porsh(pixels)*/16,
	 /*vblank(pixels)*/20, /*horiz polarity*/KdSyncNegative,
        },
	{/*horiz size*/240,  /*vert size*/320, /*frame rate(Hz)*/60,
	 /*pixel clock(KHz)*/24500,
	 /*horiz back porsh(pixels)*/8, /*horiz front porsh(pixels)*/16,
	 /*hblank(pixels)*/32, /*horiz polarity*/KdSyncNegative,
	 /*horiz back porsh(pixels)*/2, /*horiz front porsh(pixels)*/16,
	 /*vblank(pixels)*/20, /*horiz polarity*/KdSyncNegative,
        },
};


#define NB_MONITOR_TIMINGS(var) sizeof(var)/sizeof(var[0])

static void
glamoConvertMonitorTiming (const KdMonitorTiming *t,
			   struct fb_var_screeninfo *var)
{
	GLAMO_LOG("enter\n");
	var->xres = t->horizontal;
	var->yres = t->vertical;
	var->xres_virtual = t->horizontal;
	var->yres_virtual = t->vertical;
	var->xoffset = 0;
	var->yoffset = 0;
	var->pixclock = t->clock ? 1000000000 / t->clock : 0;
	var->left_margin = t->hbp;
	var->right_margin = t->hfp;
	var->upper_margin = t->vbp;
	var->lower_margin = t->vfp;
	var->hsync_len = t->hblank - t->hfp - t->hbp;
	var->vsync_len = t->vblank - t->vfp - t->vbp;

	var->sync = 0;
	var->vmode = 0;

	if (t->hpol == KdSyncPositive)
		var->sync |= FB_SYNC_HOR_HIGH_ACT;
	if (t->vpol == KdSyncPositive)
		var->sync |= FB_SYNC_VERT_HIGH_ACT;
	GLAMO_LOG("leave\n");
}

static Bool
glamoFindMatchingMode (int horizontal,
		       int vertical,
		       KdMonitorTiming **timing)
{
	int i;
	Bool found = FALSE;
	GLAMO_LOG("enter\n");

	if (!timing)
		goto out;
	for (i=0; i < NB_MONITOR_TIMINGS(glamoMonitorTimings); i++) {
		if (glamoMonitorTimings[i].horizontal == horizontal
		    && glamoMonitorTimings[i].vertical == vertical) {
		*timing = &glamoMonitorTimings[i];
		found = TRUE;
		goto out;
		}
	}

out:
	GLAMO_LOG("leave\n");
	return found;
}

static const char *LCD_RESOLUTION_CFG =
	"/sys/devices/platform/glamo3362.0/glamo-spi-gpio.0/spi2.0/state";
static const char QVGA_MODE[] = "qvga-normal";
static const char VGA_MODE[] = "normal";

static Bool
glamoSetLCDResolution (int width, int height)
{
	Bool is_ok = FALSE;
	int fd = 0;
	int scalX, scalY;

	GLAMO_LOG("enter\n");

	if ((fd = open(LCD_RESOLUTION_CFG, O_RDWR)) < 0) {
		GLAMO_LOG_ERROR("failed to open %s\n", LCD_RESOLUTION_CFG);
		goto out;
	}
	if ((width == 240 && height == 320)
	     || (width == 320 && height == 240)) {
		scalX = scalY = 2;
		write(fd, QVGA_MODE, sizeof(QVGA_MODE));
		GLAMO_LOG("set mode to %s\n", QVGA_MODE);
	} else if ((width == 480 && height == 640)
		   || (width == 640 && height == 480)) {
		scalX = scalY = 1;
		write(fd, VGA_MODE, sizeof(VGA_MODE));
		GLAMO_LOG("set mode to %s\n", VGA_MODE);
	} else {
		GLAMO_LOG_ERROR("unknown mode: (%dx%d)\n", width, height);
		goto out;
	}
/*	KdSetPointerScaling(scalX, scalY);*/
	is_ok = TRUE;
out:
	if (fd > 0)
		close(fd);
	GLAMO_LOG("leave. is_ok:%d\n", is_ok);
	return is_ok;
}

static Bool
glamoSetScannoutGeometry (ScreenPtr pScreen,
			  Rotation rotation,
			  RRScreenSizePtr size)
{
	KdScreenPriv(pScreen);
        KdScreenInfo *screen = pScreenPriv->screen;
	GLAMOCardInfo(screen);
	FbdevPriv *priv = screen->card->driver;
	struct fb_var_screeninfo	var;
	int byte_stride=0, k = 0, is_ok = FALSE;
	KdPointerMatrix m;
	Bool is_portrait = TRUE, orientation_will_change = FALSE;
        int new_xres = 0, new_yres = 0;

	GLAMO_LOG("enter: Absolute rotation to honour:%d\n", rotation);

	k = ioctl(priv->fd, FBIOGET_VSCREENINFO, &var);
	if (k < 0) {
		GLAMO_LOG_ERROR("failed to get fb info\n");
		is_ok = FALSE;
		goto out;
	}
	GLAMO_LOG("geometry reported by fb: (%dx%d),r:%d\n",
		  var.xres, var.yres, var.rotate);

	var.xres = size->width;
	var.yres = size->height;

	if (var.yres >= var.xres)
		is_portrait = TRUE;
	else
		is_portrait = FALSE;
	GLAMO_LOG("is_portrait:%d\n", is_portrait);

	switch (rotation) {
		case RR_Rotate_0:
			var.rotate = FB_ROTATE_UR;
			break;
		case RR_Rotate_90:
			var.rotate = FB_ROTATE_CW;
			break;
		case RR_Rotate_180:
			var.rotate = FB_ROTATE_UD;
			break;
		case RR_Rotate_270:
			var.rotate = FB_ROTATE_CCW;
			break;
		default:
			var.rotate = FB_ROTATE_UR;
			break;
	}
	GLAMO_LOG("geometry requested by client app: (%dx%d),r:%d\n",
		  var.xres, var.yres, var.rotate);

	if ((is_portrait && (var.rotate == FB_ROTATE_CW
			    || var.rotate == FB_ROTATE_CCW))
	    ||(!is_portrait) && (var.rotate == FB_ROTATE_UR
			       || var.rotate == FB_ROTATE_UD)) {
		orientation_will_change = TRUE;
	}
	GLAMO_LOG("will orientation change ?:%d\n",
		  orientation_will_change);

	if (orientation_will_change) {
		new_xres = var.yres;
		new_yres = var.xres;
	} else {
		new_xres = var.xres;
		new_yres = var.yres;
	}

	/*
	 * if we want to get back to "normal" (portrait) orientation
	 * from landscape orientation, then it means we need to
	 * to set orientation back to portrait ourselves.
	 */
	if (orientation_will_change
	    && !is_portrait
	    && (var.rotate == FB_ROTATE_UR || FB_ROTATE_UD)) {
		var.xres = size->height;
		var.yres = size->width;
		new_xres = var.xres;
		new_yres = var.yres;
		GLAMO_LOG("getting back to portrait. geo:(%dx%d),r:%d\n",
			  var.xres, var.yres, var.rotate);
	}
	if (orientation_will_change) {
		KdMonitorTiming *t = NULL;
		if (glamoFindMatchingMode(new_xres, new_yres, &t) && t) {
			struct fb_var_screeninfo mode;
			memset(&mode, 0, sizeof(mode));
			glamoConvertMonitorTiming(t, &mode);
			GLAMO_LOG("setting pixclock to:%d\n", mode.pixclock);
			var.pixclock = mode.pixclock;
		} else {
			GLAMO_LOG_ERROR("failed to get mode for (%dx%d)\n",
					var.yres, var.xres);
		}
	}

	memcpy(&priv->var, &var, sizeof (priv->var));
	k = ioctl(priv->fd, FBIOPUT_VSCREENINFO, &priv->var);
	if (k < 0) {
		GLAMO_LOG_ERROR("failed to set variable fb info\n");
		is_ok = FALSE;
		goto out;
	}
	/*
	 * framebuffer geometry have maybe changed due to rotation.
	 * So, read it back.
	 */
	k = ioctl(priv->fd, FBIOGET_VSCREENINFO, &priv->var);
	if (k < 0) {
		GLAMO_LOG_ERROR("failed to get fb info\n");
		is_ok = FALSE;
		goto out;
	}
	k = ioctl(priv->fd, FBIOGET_FSCREENINFO, &priv->fix);
	if (k < 0) {
		GLAMO_LOG_ERROR("failed to get fb info\n");
		is_ok = FALSE;
		goto out;
	}
	GLAMO_LOG("recorded geometry is: (%dx%d:%d),(%dx%d),r:%d\n",
		  priv->var.xres, priv->var.yres, priv->fix.line_length,
		  priv->var.xres_virtual, priv->var.yres_virtual,
		  priv->var.rotate);
	screen->randr = rotation;

	memset(&m, 0, sizeof(m));
	KdComputePointerMatrix(&m, screen->randr, screen->width, screen->height);
	KdSetPointerMatrix(&m);

	pScreen->width = screen->width = priv->var.xres;
	pScreen->height = screen->height = priv->var.yres;
	screen->fb[0].byteStride =
			screen->width * screen->fb[0].bitsPerPixel / 8;

	/*TODO: not yet supported by glamo fb module*/
	pScreen->mmWidth = priv->var.width;
	pScreen->mmHeight = priv->var.height;
	is_ok = TRUE;
out:
	GLAMO_LOG("leave: is_ok:%d\n", is_ok);
	return is_ok;
}

static Bool
GLAMORandRSetConfig (ScreenPtr		pScreen,
		     Rotation		randr,
		     int		rate,
		     RRScreenSizePtr	size)
{
	KdScreenPriv(pScreen);
	KdScreenInfo *screen = pScreenPriv->screen;
	GLAMOCardInfo *glamoc = screen->card->driver;
	Bool enabled = TRUE;
	int byte_stride = 0;

	GLAMO_LOG ("enter, randr:%d, (sizew,sizeh):(%d,%d)\n",
		   randr, size->width, size->height);

	enabled = pScreenPriv->enabled;
	if (enabled) {
		KdDisableScreen(pScreen);
	}
	if (pScreen->width * pScreen->height
	    != size->width * size->height) {
		/*resolution changes*/
		glamoSetLCDResolution(size->width, size->height);
	}
	if (!glamoSetScannoutGeometry(pScreen, randr, size)) {
		GLAMO_LOG_ERROR("failed to rotate LCD\n");
		return FALSE;
	}
	/*
	 * Set frame buffer mapping
	 */
	byte_stride = screen->width * screen->fb[0].bitsPerPixel / 8;
	GLAMO_LOG ("(w,h,pitch):(%d,%d,%d)\n",
		   screen->width,
		   screen->height,
		   screen->fb[0].byteStride);
	(*pScreen->ModifyPixmapHeader) (fbGetScreenPixmap (pScreen),
					pScreen->width,
					pScreen->height,
					screen->fb[0].depth,
					screen->fb[0].bitsPerPixel,
					byte_stride,
					screen->fb[0].frameBuffer);
	if (enabled) {
		KdEnableScreen(pScreen);
	}
	GLAMO_LOG ("leave\n");
	return TRUE;
}

static Bool
GLAMORandRGetInfo(ScreenPtr pScreen, Rotation *rotations)
{
#define TAB_LEN(tab) sizeof((tab))/sizeof((tab)[0])
	KdScreenPriv(pScreen);
	KdScreenInfo *screen = pScreenPriv->screen;
	unsigned i = 0;
	RRScreenSizePtr size = NULL;
	struct {int width, height, rate;} sizes[] = {
		{480, 640, 50},
		{640, 480, 25},
		{240, 320, 50},
		{320, 240, 50},
		{0, 0}
	};

	GLAMO_LOG("enter\n");

	*rotations = RR_Rotate_All|RR_Reflect_All;

	for (i=0; i < TAB_LEN(sizes); ++i) {
		if (sizes[i].width == 0 || sizes[i].height == 0) {
			continue;
		}
		size = RRRegisterSize(pScreen,
				      sizes[i].width,
				      sizes[i].height,
				      (sizes[i].width * screen->width_mm)
				       /screen->width,
				      (sizes[i].height *screen->height_mm)
				       /screen->height);
		if (!size) {
			GLAMO_LOG_ERROR("got null size when "
					"registering size (%dx%d)\n",
					sizes[i].width, sizes[i].height);
			continue;
		}
		if (!RRRegisterRate(pScreen, size, sizes[i].rate)) {
			GLAMO_LOG_ERROR("failed to register rate %d"
					" for size (%dx%d)\n",
					sizes[i].rate,
					sizes[i].width,
					sizes[i].height);
		}
	}
	size = RRRegisterSize(pScreen,
			      screen->width, screen->height,
			      screen->width_mm, screen->height_mm);
	RRSetCurrentConfig(pScreen,
			   screen->randr,
			   size->pRates[0].rate,
			   size);

	GLAMO_LOG("leave. current size:(%dx%d)\n",
                  size->width, size->height);

	return TRUE;
}


static Bool
GLAMORandRInit (ScreenPtr pScreen)
{
	rrScrPrivPtr    pScrPriv;

	GLAMO_LOG("enter\n");

	pScrPriv = rrGetScrPriv(pScreen);
	pScrPriv->rrSetConfig = GLAMORandRSetConfig;
	pScrPriv->rrGetInfo = GLAMORandRGetInfo;

	GLAMO_LOG("leave \n");

	return TRUE;
}
#endif /*RANDR*/

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
	GLAMOCardInfo *glamoc = glamos->glamoc;
	int i=0;
	for (i=from; i <= to; i += 2) {
		GLAMO_LOG("reg:%#x, val:%#x\n",
			  glamoc->reg_base+i,
			  *(VOL16*)(glamoc->reg_base+i));
	}
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
