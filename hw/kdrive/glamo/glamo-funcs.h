/*
 * Copyright © 2007 OpenMoko, Inc.
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
 * Author:
 *  Dodji Seketeli <dodji@openedhand.com>
 */
#ifndef _GLAMO_FUNCS_H_
#define _GLAMO_FUNCS_H_

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif

#include <assert.h>
#include "os.h"
#include "kdrive.h"

void glamoOutReg(ScreenPtr pScreen, unsigned short reg, unsigned short val);

unsigned short glamoInReg(ScreenPtr pScreen, unsigned short reg);

void glamoSetBitMask(ScreenPtr pScreen, int reg, int mask, int val);

void setCmdMode (ScreenPtr pScreen, Bool on);

Bool glamoRotateLCD (ScreenPtr pScreen, Rotation rotation);

#ifdef XV

void GLAMOISPWaitEngineIdle (ScreenPtr pScreen);
void GLAMOISPEngineInit (ScreenPtr pScreen);
void GLAMOISPDisplayYUVPlanarFrame (ScreenPtr pScreen,
				    unsigned int y_addr,
				    unsigned int u_addr,
				    unsigned int v_addr,
				    short y_pitch,
				    short uv_pitch,
				    short src_crop_rect_width,
				    short src_crop_rect_height,
				    unsigned int dst_addr,
				    short dst_pitch,
				    short dst_rect_width,
				    short dst_rect_height,
				    short scale_w,
				    short scale_h);

void GLAMOISPSetColorKeyOverlay(ScreenPtr	pScreen,
				CARD32	start_addr/*addr on 23bits*/,
				CARD16	x /*12bits*/,
				CARD16	y /*12bits*/,
				CARD16	width /*12bits*/,
				CARD16	height /*12bits*/,
				CARD16	pitch /*12bits*/,
				CARD8	red_key /*5bits*/,
				CARD8	green_key /*6bits*/,
				CARD8	blue_key /*5bits*/);

void GLAMOISPSetColorKeyOverlay2(ScreenPtr	pScreen,
				 CARD32	start_addr/*addr on 23bits*/,
				 CARD16	x /*12bits*/,
				 CARD16	y /*12bits*/,
				 CARD16	width /*12bits*/,
				 CARD16	height /*12bits*/,
				 CARD16	pitch /*12bits*/,
				 CARD16	color_key /*16bits*/);
#endif /*XV*/

#endif /*_GLAMO_FUNCS_H_*/

