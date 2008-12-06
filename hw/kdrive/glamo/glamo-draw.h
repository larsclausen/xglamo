/*
 * Copyright © 2007 OpenMoko, Inc.
 *
 * This driver is based on Xati,
 * Copyright © 2004 Eric Anholt
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

#ifndef _GLAMO_DRAW_H_
#define _GLAMO_DRAW_H_

void GLAMOWaitIdle(GLAMOScreenInfo *glamos);

#define GLAMO_TRACE_FALL 1
#define GLAMO_TRACE_DRAW 1

#if GLAMO_TRACE_FALL
#define GLAMO_FALLBACK(x)			\
do {					\
	ErrorF("%s: ", __FUNCTION__);	\
	ErrorF x;			\
	return FALSE;			\
} while (0)
#else
#define GLAMO_FALLBACK(x) return FALSE
#endif

#if GLAMO_TRACE_DRAW
#define ENTER_DRAW(pix) GLAMOEnterDraw(pix, __FUNCTION__)
#define LEAVE_DRAW(pix) GLAMOLeaveDraw(pix, __FUNCTION__)

void
GLAMOEnterDraw (PixmapPtr pPixmap, const char *function);

void
GLAMOLeaveDraw (PixmapPtr pPixmap, const char *function);
#else /* GLAMO_TRACE */
#define ENTER_DRAW(pix)
#define LEAVE_DRAW(pix)
#endif /* !GLAMO_TRACE */

#endif /* _GLAMO_DRAW_H_ */
