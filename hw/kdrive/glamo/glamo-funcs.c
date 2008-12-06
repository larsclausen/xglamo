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

#include "glamo-log.h"
#include "glamo.h"
#include "glamo-funcs.h"
#include "glamo-regs.h"
#include "glamo-cmdq.h"

void
GLAMOOutReg(ScreenPtr pScreen, unsigned short reg, unsigned short val)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);
	GLAMO_LOG("mark: pScreen:%#x, reg:%#x, val:%#x, reg_base:%#x\n",
		  pScreen, reg, val, glamoc->reg_base);
	if (!glamoc->reg_base) {
		GLAMO_LOG_ERROR("got null glamoc->reg_base\n");
		return;
	}
	MMIO_OUT16(glamoc->reg_base, reg, val);
}

unsigned short
GLAMOInReg(ScreenPtr pScreen, unsigned short reg)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);
	GLAMO_LOG("mark: pScreen:%#x, reg:%#x, reg_base:%#x\n",
		  pScreen, reg, glamoc->reg_base);
	if (!glamoc->reg_base) {
		GLAMO_LOG_ERROR("got null glamoc->reg_base\n");
		return 0;
	}
	return MMIO_IN16(glamoc->reg_base, reg);
}

void
GLAMOSetBitMask(ScreenPtr pScreen, int reg, int mask, int val)
{
	int old;
	old = GLAMOInReg(pScreen, reg);
	old &= ~mask;
	old |= val & mask;
	GLAMO_LOG("mark\n");
	GLAMOOutReg(pScreen, reg, old);
}

void
setCmdMode (ScreenPtr pScreen, Bool on)
{
	if (on) {
		GLAMO_LOG("mark\n");
		/*TODO: busy waiting is bad*/
		while (!GLAMOInReg(pScreen, GLAMO_REG_LCD_STATUS1)
		       & (1 << 15)) {
			GLAMO_LOG("mark\n");
			usleep(1 * 1000);
		}
		GLAMO_LOG("mark\n");
		GLAMOOutReg(pScreen,
			    GLAMO_REG_LCD_COMMAND1,
			    GLAMO_LCD_CMD_TYPE_DISP |
			    GLAMO_LCD_CMD_DATA_FIRE_VSYNC);
		GLAMO_LOG("mark\n");
		while (!GLAMOInReg(pScreen, GLAMO_REG_LCD_STATUS2)
		       & (1 << 12)) {
			GLAMO_LOG("mark\n");
			usleep(1 * 1000);
		}
		/* wait */
		GLAMO_LOG("mark\n");
		usleep(100 * 1000);
	} else {
		GLAMO_LOG("mark\n");
		GLAMOOutReg(pScreen,
			    GLAMO_REG_LCD_COMMAND1,
			    GLAMO_LCD_CMD_TYPE_DISP |
			    GLAMO_LCD_CMD_DATA_DISP_SYNC);
		GLAMO_LOG("mark\n");
		GLAMOOutReg(pScreen,
			    GLAMO_REG_LCD_COMMAND1,
			    GLAMO_LCD_CMD_TYPE_DISP |
			    GLAMO_LCD_CMD_DATA_DISP_FIRE);
	}
}

#define STATUS_ENABLED  0x1
static int engine_status[NB_GLAMO_ENGINES];

void
GLAMOResetEngine(ScreenPtr pScreen, enum GLAMOEngine engine)
{
	int reg, mask;

	GLAMO_LOG("enter\n");

	if (!(engine_status[engine] & STATUS_ENABLED))
		return;

	switch (engine) {
		case GLAMO_ENGINE_MPEG:
			reg = GLAMO_REG_CLOCK_MPEG;
			mask = GLAMO_CLOCK_MPEG_DEC_RESET;
			break;
		case GLAMO_ENGINE_ISP:
			reg = GLAMO_REG_CLOCK_ISP;
			mask = GLAMO_CLOCK_ISP2_RESET;
			break; 
		case GLAMO_ENGINE_CMDQ:
			reg = GLAMO_REG_CLOCK_2D;
			mask = GLAMO_CLOCK_2D_CMDQ_RESET;
			break;
		case GLAMO_ENGINE_2D:
			reg = GLAMO_REG_CLOCK_2D;
			mask = GLAMO_CLOCK_2D_RESET;
			break;
	}

	GLAMOSetBitMask(pScreen, reg, mask, 0xffff);
	usleep(1000);
	GLAMOSetBitMask(pScreen, reg, mask, 0);
	usleep(1000);

	GLAMO_LOG("leave\n");
}

void
GLAMOEnableEngine(ScreenPtr pScreen, enum GLAMOEngine engine)
{
	GLAMO_LOG("enter\n");
	if (engine_status[engine] & STATUS_ENABLED)
		return;

	GLAMOSetBitMask(pScreen,
			GLAMO_REG_CLOCK_GEN5_1,
			GLAMO_CLOCK_GEN51_EN_DIV_MCLK,
			0xffff);

	switch (engine) {
		case GLAMO_ENGINE_MPEG:
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_CLOCK_MPEG,
					GLAMO_CLOCK_MPEG_EN_X6CLK |
					GLAMO_CLOCK_MPEG_DG_X6CLK |
					GLAMO_CLOCK_MPEG_EN_X4CLK |
					GLAMO_CLOCK_MPEG_DG_X4CLK |
					GLAMO_CLOCK_MPEG_EN_X2CLK |
					GLAMO_CLOCK_MPEG_DG_X2CLK |
					GLAMO_CLOCK_MPEG_EN_X0CLK |
					GLAMO_CLOCK_MPEG_DG_X0CLK,
					0xffff & ~GLAMO_CLOCK_MPEG_DG_X0CLK);
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_CLOCK_MPROC,
					GLAMO_CLOCK_MPROC_EN_M4CLK,
					0xffff);
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_CLOCK_GEN5_1,
					GLAMO_CLOCK_GEN51_EN_DIV_JCLK,
					0xffff);
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_HOSTBUS(2),
					GLAMO_HOSTBUS2_MMIO_EN_MPEG |
					GLAMO_HOSTBUS2_MMIO_EN_MICROP1,
					0xffff);
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_CLOCK_MPROC,
					GLAMO_CLOCK_MPROC_EN_KCLK,
					0xffff);
			break;
		case GLAMO_ENGINE_ISP:
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_CLOCK_ISP,
					GLAMO_CLOCK_ISP_EN_M2CLK |
					GLAMO_CLOCK_ISP_EN_I1CLK,
					0xffff);
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_CLOCK_GEN5_2,
					GLAMO_CLOCK_GEN52_EN_DIV_ICLK,
					0xffff);
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_CLOCK_GEN5_1,
					GLAMO_CLOCK_GEN51_EN_DIV_JCLK,
					0xffff);
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_HOSTBUS(2),
					GLAMO_HOSTBUS2_MMIO_EN_ISP,
					0xffff);
			break;
		case GLAMO_ENGINE_CMDQ:
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_CLOCK_2D,
					GLAMO_CLOCK_2D_EN_M6CLK,
					0xffff);
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_HOSTBUS(2),
					GLAMO_HOSTBUS2_MMIO_EN_CMDQ,
					0xffff);
			break;
		case GLAMO_ENGINE_2D:
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_CLOCK_2D,
					GLAMO_CLOCK_2D_EN_M7CLK |
					GLAMO_CLOCK_2D_EN_GCLK |
					GLAMO_CLOCK_2D_DG_M7CLK |
					GLAMO_CLOCK_2D_DG_GCLK,
					0xffff);
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_HOSTBUS(2),
					GLAMO_HOSTBUS2_MMIO_EN_2D,
					0xffff);
			GLAMOSetBitMask(pScreen,
					GLAMO_REG_CLOCK_GEN5_1,
					GLAMO_CLOCK_GEN51_EN_DIV_GCLK,
					0xffff);
			break;
	}
	usleep(1000);
	engine_status[engine] |= STATUS_ENABLED;
	GLAMO_LOG("leave\n");
}


#ifdef XV
void
GLAMOISPWaitEngineIdle (ScreenPtr pScreen)
{
	GLAMO_LOG("enter\n");
	while (1) {
		int val = GLAMOInReg(pScreen, GLAMO_REG_ISP_STATUS);
		if (val & 0x1) {
			usleep(1 * 1000);
			GLAMO_LOG("isp busy\n");
			continue;
		}
		break;
	}
	GLAMO_LOG("leave\n");
}

static void
SetOnFlyLUTRegs(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	struct {
		int src_block_x;
		int src_block_y;
		int src_block_w;
		int src_block_h;
		int jpeg_out_y;
		int jpeg_out_x;
		int fifo_full_cnt;
		int in_length;
		int fifo_data_cnt;
		int in_height;
	} onfly;
	RING_LOCALS;

	GLAMO_LOG("enter\n");

	onfly.src_block_y = 32;
	onfly.src_block_x = 32;
	onfly.src_block_w = 36;
	onfly.src_block_h = 35;
	onfly.jpeg_out_y = 32;
	onfly.jpeg_out_x = 32;
	onfly.fifo_full_cnt = onfly.src_block_w * 2 + 2;
	onfly.in_length = onfly.jpeg_out_x + 3;
	onfly.fifo_data_cnt = onfly.src_block_w * onfly.src_block_h / 2;
	onfly.in_height = onfly.jpeg_out_y + 2;

	BEGIN_CMDQ(10);
	OUT_REG(GLAMO_REG_ISP_ONFLY_MODE1,
		onfly.src_block_y << 10 | onfly.src_block_x << 2);
	OUT_REG(GLAMO_REG_ISP_ONFLY_MODE2,
		onfly.src_block_h << 8 | onfly.src_block_w);
	OUT_REG(GLAMO_REG_ISP_ONFLY_MODE3,
		onfly.jpeg_out_y << 8 | onfly.jpeg_out_x);
	OUT_REG(GLAMO_REG_ISP_ONFLY_MODE4,
		onfly.fifo_full_cnt << 8 | onfly.in_length);
	OUT_REG(GLAMO_REG_ISP_ONFLY_MODE5,
		onfly.fifo_data_cnt << 6 | onfly.in_height);
	END_CMDQ();
	GLAMO_LOG("leave\n");
}

static void
SetScalingWeightMatrixRegs(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	int left = 1 << 14;
	RING_LOCALS;

	GLAMO_LOG("enter\n");

	/* nearest */

	BEGIN_CMDQ(12);
	OUT_BURST(GLAMO_REG_ISP_DEC_SCALEH_MATRIX, 10);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEH_MATRIX +  0, left);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEH_MATRIX +  2, 0);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEH_MATRIX +  4, left);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEH_MATRIX +  6, 0);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEH_MATRIX +  8, left);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEH_MATRIX + 10, 0);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEH_MATRIX + 12, left);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEH_MATRIX + 14, 0);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEH_MATRIX + 16, left);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEH_MATRIX + 18, 0);
	END_CMDQ();

	BEGIN_CMDQ(12);
	OUT_BURST(GLAMO_REG_ISP_DEC_SCALEV_MATRIX, 10);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEV_MATRIX +  0, left);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEV_MATRIX +  2, 0);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEV_MATRIX +  4, left);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEV_MATRIX +  6, 0);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEV_MATRIX +  8, left);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEV_MATRIX + 10, 0);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEV_MATRIX + 12, left);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEV_MATRIX + 14, 0);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEV_MATRIX + 16, left);
	OUT_BURST_REG(GLAMO_REG_ISP_DEC_SCALEV_MATRIX + 18, 0);
	END_CMDQ();
	GLAMO_LOG("leave\n");
}

static void
GLAMOISPYuvRgbPipelineInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	unsigned short en3;
	RING_LOCALS;

	GLAMO_LOG("enter.glamos:%#x\n", glamos);

	BEGIN_CMDQ(18);

	/*
	 * set the ISP into YUV 4:2:0 planar mode,
	 * enable scaling.
	 */
	en3 = GLAMO_ISP_EN3_PLANE_MODE |
		GLAMO_ISP_EN3_YUV_INPUT |
		GLAMO_ISP_EN3_YUV420 |
		GLAMO_ISP_EN3_SCALE_IMPROVE;

	OUT_REG(GLAMO_REG_ISP_EN3, en3);

	/*
	 * In 8.8 fixed point,
	 *
	 *  R = Y + 1.402 (Cr-128)
	 *    = Y + 0x0167 Cr - 0xb3
	 *
	 *  G = Y - 0.34414 (Cb-128) - 0.71414 (Cr-128)
	 *    = Y - 0x0058 Cb - 0x00b6 Cr + 0x89
	 *
	 *  B = Y + 1.772 (Cb-128)
	 *    = Y + 0x01c5 Cb - 0xe2
	 */

	OUT_REG(GLAMO_REG_ISP_YUV2RGB_11, 0x0167);
	OUT_REG(GLAMO_REG_ISP_YUV2RGB_21, 0x01c5);
	OUT_REG(GLAMO_REG_ISP_YUV2RGB_32, 0x00b6);
	OUT_REG(GLAMO_REG_ISP_YUV2RGB_33, 0x0058);
	OUT_REG(GLAMO_REG_ISP_YUV2RGB_RG, 0xb3 << 8 | 0x89);
	OUT_REG(GLAMO_REG_ISP_YUV2RGB_B, 0xe2);

	OUT_REG(GLAMO_REG_ISP_PORT1_DEC_EN, GLAMO_ISP_PORT1_EN_OUTPUT);
	OUT_REG(GLAMO_REG_ISP_PORT2_EN, GLAMO_ISP_PORT2_EN_DECODE);

	END_CMDQ();

	SetOnFlyLUTRegs(pScreen);
	SetScalingWeightMatrixRegs(pScreen);

	GLAMO_LOG("leave\n");
}

static void
GLAMOISPColorKeyOverlayInit(ScreenPtr pScreen)
{
	GLAMO_RETURN_IF_FAIL (pScreen);

	/*GLAMOSetBitMask(pScreen,
			GLAMO_REG_ISP_EN2,
			GLAMO_ISP_EN2_OVERLAY,
			0x0001);*/
	GLAMOSetBitMask(pScreen,
			GLAMO_REG_ISP_EN4,
			GLAMO_ISP_EN4_OVERLAY|GLAMO_ISP_EN4_LCD_OVERLAY,
			0x0003);
}

void
GLAMOISPSetColorKeyOverlay(ScreenPtr	pScreen,
			   CARD32	start_addr/*addr on 23bits*/,
			   CARD16	x /*12bits*/,
			   CARD16	y /*12bits*/,
			   CARD16	width /*12bits*/,
			   CARD16	height /*12bits*/,
			   CARD16	pitch /*12bits*/,
			   CARD8	red_key /*5bits*/,
			   CARD8	green_key /*6bits*/,
			   CARD8	blue_key /*5bits*/)
{
	unsigned short green_red_keys = 0;
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	RING_LOCALS;

	GLAMO_LOG("enter. start_addr:%#x, (x,y):(%hd,%hd), "
		  "width,height:(%dx%d), pitch:%d\n"
		  "red, green, blue:(%d, %d, %d)\n",
		  start_addr, x, y, width, height, pitch,
		  red_key, green_key, blue_key);

	green_red_keys = (green_key << 8+2) & 0xff00;
	green_red_keys |= (red_key << 3) & 0x00ff;

	BEGIN_CMDQ(18);

	OUT_REG(GLAMO_REG_ISP_OVERLAY_GR_KEY, green_red_keys);
	OUT_REG(GLAMO_REG_ISP_OVERLAY_B_KEY, (blue_key << 3) && 0x00ff);
	OUT_REG(GLAMO_REG_ISP_OVERLAY_ADDRL, start_addr & 0x00ff);
	OUT_REG(GLAMO_REG_ISP_OVERLAY_ADDRH, start_addr & 0x7f00);
	OUT_REG(GLAMO_REG_ISP_OVERLAY_X, x & 0x0fff);
	OUT_REG(GLAMO_REG_ISP_OVERLAY_Y, y & 0x0fff);
	OUT_REG(GLAMO_REG_ISP_OVERLAY_WIDTH, width & 0x0fff);
	OUT_REG(GLAMO_REG_ISP_OVERLAY_HEIGHT, width & 0x0fff);
	OUT_REG(GLAMO_REG_ISP_OVERLAY_PITCH, pitch & 0x0fff);
	/*TODO: no idea what this one is*/
	/*OUT_REG(GLAMO_REG_ISP_OVERLAY_BLOCK_XY, 0);*/

	END_CMDQ();

	GLAMO_LOG("leave\n");
}

void
GLAMOISPSetColorKeyOverlay2(ScreenPtr	pScreen,
			    CARD32	start_addr/*addr on 23bits*/,
			    CARD16	x /*12bits*/,
			    CARD16	y /*12bits*/,
			    CARD16	width /*12bits*/,
			    CARD16	height /*12bits*/,
			    CARD16	pitch /*12bits*/,
			    CARD16	color_key /*16bits*/)
{
	CARD8 red, green, blue;

	/*assume color key has rgb 565 format*/
	red = (color_key >> 11);
	green = (color_key >> 5) & 0x003f;
	blue = (color_key) & 0x001f;
	GLAMOISPSetColorKeyOverlay(pScreen, start_addr, x, y,
				   width, height, pitch,
				   red, green, blue);
}

void
GLAMOISPEngineInit (ScreenPtr pScreen)
{
	static Bool isp_enabled, isp_reset;

	GLAMO_LOG("enter\n");
	if (!isp_enabled) {
		GLAMOEnableEngine(pScreen, GLAMO_ENGINE_ISP);
		isp_enabled = TRUE;
		GLAMO_LOG("enabled ISP\n");
	}
	if (!isp_reset) {
		GLAMOResetEngine(pScreen, GLAMO_ENGINE_ISP);
		GLAMO_LOG("reset ISP\n");
	}
	GLAMOISPYuvRgbPipelineInit(pScreen);
	/*GLAMOISPColorKeyOverlayInit(pScreen);*/
	GLAMO_LOG("leave\n");
}

void
GLAMOISPDisplayYUVPlanarFrame (ScreenPtr pScreen,
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
			       short scale_h)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	int en3;
	RING_LOCALS;


	GLAMO_LOG("enter: y_addr:%#x, u_addr:%#x, v_addr:%#x\n"
		  "y_pitch:%hd, uv_pitch:%hd\n"
		  "src_crop_rect(%hdx%hd)\n"
		  "dst_addr:%#x, dst_pitch:%hd\n"
		  "dst_rect(%hdx%hd), dst_scale(%hdx%hd)\n",
		  y_addr, u_addr, v_addr,
		  y_pitch, uv_pitch,
		  src_crop_rect_width, src_crop_rect_height,
		  dst_addr, dst_pitch,
		  dst_rect_width, dst_rect_height,
		  scale_w, scale_h);


	/*scale_w <<= 11;*/
	/*scale_h <<= 11;*/


	BEGIN_CMDQ(38);

	/*
	 * set Y, U, V pitches.
	 */
	OUT_REG(GLAMO_REG_ISP_DEC_PITCH_Y,  y_pitch & 0x1fff);
	OUT_REG(GLAMO_REG_ISP_DEC_PITCH_UV, uv_pitch & 0x1fff);
	OUT_REG(GLAMO_REG_ISP_PORT1_DEC_PITCH, dst_pitch & 0x1fff);

	/*
	 * set yuv starting addresses, pitches and crop rect size
	 */
	OUT_REG(GLAMO_REG_ISP_DEC_Y_ADDRL, y_addr & 0xffff);
	OUT_REG(GLAMO_REG_ISP_DEC_Y_ADDRH, (y_addr >> 16) & 0x7f);

	OUT_REG(GLAMO_REG_ISP_DEC_U_ADDRL, u_addr & 0xffff);
	OUT_REG(GLAMO_REG_ISP_DEC_U_ADDRH, (u_addr >> 16) & 0x7f);

	OUT_REG(GLAMO_REG_ISP_DEC_V_ADDRL, v_addr & 0xffff);
	OUT_REG(GLAMO_REG_ISP_DEC_V_ADDRH, (v_addr >> 16) & 0x7f);

	OUT_REG(GLAMO_REG_ISP_DEC_HEIGHT, src_crop_rect_height & 0x1fff);
	OUT_REG(GLAMO_REG_ISP_DEC_WIDTH, src_crop_rect_width & 0x1fff);

	/*
	 * set output coordinates/sizes and scaling
	 */
	OUT_REG(GLAMO_REG_ISP_PORT1_DEC_0_ADDRL, dst_addr & 0xffff);
	OUT_REG(GLAMO_REG_ISP_PORT1_DEC_0_ADDRH, (dst_addr >> 16) & 0x7f);

	OUT_REG(GLAMO_REG_ISP_PORT1_DEC_WIDTH, dst_rect_width & 0x1fff);
	OUT_REG(GLAMO_REG_ISP_PORT1_DEC_HEIGHT, dst_rect_height & 0x1fff);

	OUT_REG(GLAMO_REG_ISP_DEC_SCALEH, scale_w);
	OUT_REG(GLAMO_REG_ISP_DEC_SCALEV, scale_h);

	OUT_REG(GLAMO_REG_ISP_EN1, GLAMO_ISP_EN1_FIRE_ISP);
	OUT_REG(GLAMO_REG_ISP_EN1, 0);

	END_CMDQ();

        GLAMOEngineWait(pScreen, GLAMO_ENGINE_ALL);


	GLAMO_LOG("leave\n");

}

#endif /*XV*/

