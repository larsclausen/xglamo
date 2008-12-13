#include "cursorstr.h"

#include "glamo.h"
#include "glamo-regs.h"

#define GLAMO_CURSOR_SIZE 64

static Bool glamoRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
static Bool glamoUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
static void glamoSetCursor(ScreenPtr pScreen, CursorPtr pCursor, int x, int y);
static void glamoMoveCursor(ScreenPtr pScreen, int x, int y);

static miPointerSpriteFuncRec glamoSpriteFuncs = {
    glamoRealizeCursor,
    glamoUnrealizeCursor,
    glamoSetCursor,
    glamoMoveCursor,
};

static void
glamoQueryBestSize (int class,
                    unsigned short *pwidth, unsigned short *pheight,
                    ScreenPtr pScreen)
{
    switch (class)
    {
    case CursorShape:
        if (*pwidth > GLAMO_CURSOR_SIZE)
            *pwidth = GLAMO_CURSOR_SIZE;
        if (*pheight > GLAMO_CURSOR_SIZE)
            *pheight = GLAMO_CURSOR_SIZE;
        if (*pwidth > pScreen->width)
            *pwidth = pScreen->width;
        if (*pheight > pScreen->height)
            *pheight = pScreen->height;
    break;
    default:
        fbQueryBestSize (class, pwidth, pheight, pScreen);
    break;
    }
}


Bool
GLAMOCursorInit(ScreenPtr pScreen) {
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
    char *mmio = glamos->glamoc->reg_base;

    glamos->cursor.area = KdOffscreenAlloc(pScreen, GLAMO_CURSOR_SIZE *
                                           GLAMO_CURSOR_SIZE >> 2,
                                           glamos->kaa.offsetAlign,
                                           TRUE, NULL, NULL);
    if (!glamos->cursor.area)
        return FALSE;

    if (!miPointerInitialize(pScreen, &glamoSpriteFuncs, &kdPointerScreenFuncs,
                             FALSE))
        return FALSE;

    glamos->cursor.pCursor = NULL;

    MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_BASE1, glamos->cursor.area->offset & 0xffff);
    MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_BASE2, (glamos->cursor.area->offset >> 16) & 0xffff);

    pScreen->QueryBestSize = glamoQueryBestSize;

    return TRUE;
}

void
GLAMOCursorFini(ScreenPtr pScreen) {
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
    if (glamos->glamoc->reg_base)
        MMIOSetBitMask(glamos->glamoc->reg_base, GLAMO_REG_LCD_MODE1,
                       GLAMO_LCD_MODE1_CURSOR_EN, 0x0);
    if (glamos->cursor.area) {
        KdOffscreenFree(pScreen, glamos->cursor.area);
        glamos->cursor.area = NULL;
    }
}

void
GLAMOCursorEnable(ScreenPtr pScreen) {
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
    MMIOSetBitMask(glamos->glamoc->reg_base, GLAMO_REG_LCD_MODE1,
                   GLAMO_LCD_MODE1_CURSOR_EN, 0xffff);
}

void
GLAMOCursorDisable(ScreenPtr pScreen) {
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
    MMIOSetBitMask(glamos->glamoc->reg_base, GLAMO_REG_LCD_MODE1,
                   GLAMO_LCD_MODE1_CURSOR_EN, 0);
}


void
GLAMOCursorRecolor(ScreenPtr pScreen) {
}

static Bool
glamoRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor) {
    return TRUE;
}

static Bool
glamoUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor) {
    return TRUE;
}

static void
glamoSetCursor(ScreenPtr pScreen, CursorPtr pCursor, int x, int y) {
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
    char *mmio = glamos->glamoc->reg_base;

    glamos->cursor.pCursor = pCursor;

    if (pCursor == NullCursor) {
        MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_X_SIZE, 0);
    } else {
        unsigned short x, y;
        unsigned char *dst, *mask, *src;
        unsigned char *dst_line, *mask_line, *src_line;
        unsigned char m, s;
        unsigned short fg, bg;
        unsigned short dst_pitch = ((pCursor->bits->width + 7) >> 2) & ~1;
        unsigned short src_pitch = BitmapBytePad(pCursor->bits->width);

        dst = pScreenPriv->screen->memory_base + glamos->cursor.area->offset;

        mask = pCursor->bits->mask;
        src = pCursor->bits->source;

        /* 01 - invisible
           00 - background color
           10 - foreground color
           11 - invert */

        for (y = pCursor->bits->height; y; --y) {
            src_line = src;
            mask_line = mask;
            dst_line = dst;
            for (x = (pCursor->bits->width + 3) / 4; x; --x) {
                m = ~(*mask_line);
                s = *src_line & *mask_line;
                *dst_line = ((m & 128) >> 7) | ((m & 64) >> 4) |
                            ((m & 32) >> 1) | ((m & 16) << 2) |
                            ((s & 128) >> 6) | ((s & 64) >> 3) |
                            ((s & 32)) | ((s & 16) << 3);
                ++dst_line;
                *dst_line = ((m & 8) >> 3) | ((m & 4)) |
                            ((m & 2) << 3) | ((m & 1) << 6) |
                            ((s & 8) >> 2) | ((s & 4) << 1) |
                            ((s & 2) << 4) | ((s & 1) << 7);
                ++dst_line;
                ++src_line;
                ++mask_line;
            }
            dst += dst_pitch;
            src += src_pitch;
            mask += src_pitch;
        }

        glamoMoveCursor(pScreen, x, y);

        MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_PITCH, dst_pitch);
        MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_X_SIZE, pCursor->bits->width);
        MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_Y_SIZE, pCursor->bits->height);

        bg = ((pCursor->backRed >> 11) << 11) |
             ((pCursor->backGreen >> 10) << 5) |
             (pCursor->backBlue >> 11);
        fg = ((pCursor->foreRed >> 11) << 11) |
             ((pCursor->foreGreen >> 10) << 5) |
             (pCursor->foreBlue >> 11);

        MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_FG_COLOR, fg);
        MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_DST_COLOR, fg);
        MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_BG_COLOR, bg);

/*      For some reason this crashes the glamo chip.
        MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_PRESET,
                  (pCursor->bits->xhot << 8) | (pCursor->bits->yhot & 0xff));
*/
        MMIOSetBitMask(mmio, GLAMO_REG_LCD_MODE1, GLAMO_LCD_MODE1_CURSOR_EN, 0xffff);
    }
}

static void
glamoMoveCursor(ScreenPtr pScreen, int x, int y) {
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
    char *mmio = glamos->glamoc->reg_base;
    if (glamos->cursor.pCursor) {
        unsigned char xoff, yoff;
        x -= glamos->cursor.pCursor->bits->xhot;
        y -= glamos->cursor.pCursor->bits->yhot;
        if (x < 0) {
            xoff = -x;
            x = 0;
        } else {
            xoff = 0;
        }
        if (y < 0) {
            yoff = -y;
            y = 0;
        } else {
            yoff = 0;
        }

/*      For some reason this crashes the glamo chip.
        MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_PRESET, (xoff << 8) & yoff);
*/
    }
    MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_X_POS, x);
    MMIO_OUT16(mmio, GLAMO_REG_LCD_CURSOR_Y_POS, y);
}
