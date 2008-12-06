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

#include <sys/time.h>

#include "glamo-log.h"
#include "glamo.h"
#include "glamo-regs.h"
#include "glamo-cmdq.h"
#include "glamo-draw.h"

static void GLAMOCMDQResetCP(ScreenPtr pScreen);
#ifndef NDEBUG
static void
GLAMODebugFifo(GLAMOScreenInfo *glamos)
{
	GLAMOCardInfo *glamoc = glamos->glamoc;
	char *mmio = glamoc->reg_base;
	CARD32 offset;

	ErrorF("GLAMO_REG_CMDQ_STATUS: 0x%04x\n",
	    MMIO_IN16(mmio, GLAMO_REG_CMDQ_STATUS));

	offset = MMIO_IN16(mmio, GLAMO_REG_CMDQ_WRITE_ADDRL);
	offset |= (MMIO_IN16(mmio, GLAMO_REG_CMDQ_WRITE_ADDRH) << 16) & 0x7;
	ErrorF("GLAMO_REG_CMDQ_WRITE_ADDR: 0x%08x\n", (unsigned int) offset);

	offset = MMIO_IN16(mmio, GLAMO_REG_CMDQ_READ_ADDRL);
	offset |= (MMIO_IN16(mmio, GLAMO_REG_CMDQ_READ_ADDRH) << 16) & 0x7;
	ErrorF("GLAMO_REG_CMDQ_READ_ADDR: 0x%08x\n", (unsigned int) offset);
}
#endif

void
GLAMOEngineReset(ScreenPtr pScreen, enum GLAMOEngine engine)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);
	CARD32 reg;
	CARD16 mask;
	char *mmio = glamoc->reg_base;

	if (!mmio)
		return;

	switch (engine) {
		case GLAMO_ENGINE_CMDQ:
			reg = GLAMO_REG_CLOCK_2D;
			mask = GLAMO_CLOCK_2D_CMDQ_RESET;
			break;
		case GLAMO_ENGINE_ISP:
			reg = GLAMO_REG_CLOCK_ISP;
			mask = GLAMO_CLOCK_ISP2_RESET;
			break;
		case GLAMO_ENGINE_2D:
			reg = GLAMO_REG_CLOCK_2D;
			mask = GLAMO_CLOCK_2D_RESET;
			break;
		default:
			return;
			break;
	}
	MMIOSetBitMask(mmio, reg, mask, 0xffff);
	usleep(5);
	MMIOSetBitMask(mmio, reg, mask, 0);
	usleep(5);

}

void
GLAMOEngineDisable(ScreenPtr pScreen, enum GLAMOEngine engine)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);
	char *mmio = glamoc->reg_base;

	if (!mmio)
		return;
	switch (engine) {
		case GLAMO_ENGINE_CMDQ:
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_2D,
					GLAMO_CLOCK_2D_EN_M6CLK,
					0);
			MMIOSetBitMask(mmio, GLAMO_REG_HOSTBUS(2),
					GLAMO_HOSTBUS2_MMIO_EN_CMDQ,
					0);
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_GEN5_1,
					GLAMO_CLOCK_GEN51_EN_DIV_MCLK,
					0);
			break;
		case GLAMO_ENGINE_ISP:
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_ISP,
					GLAMO_CLOCK_ISP_EN_M2CLK |
					GLAMO_CLOCK_ISP_EN_I1CLK,
					0);
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_GEN5_2,
					GLAMO_CLOCK_GEN52_EN_DIV_ICLK,
					0);
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_GEN5_1,
					GLAMO_CLOCK_GEN51_EN_DIV_JCLK,
					0);
			MMIOSetBitMask(mmio, GLAMO_REG_HOSTBUS(2),
					GLAMO_HOSTBUS2_MMIO_EN_ISP,
					0);
			break;
		case GLAMO_ENGINE_2D:
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_2D,
					GLAMO_CLOCK_2D_EN_M7CLK |
					GLAMO_CLOCK_2D_EN_GCLK |
					GLAMO_CLOCK_2D_DG_M7CLK |
					GLAMO_CLOCK_2D_DG_GCLK,
					0);
			MMIOSetBitMask(mmio, GLAMO_REG_HOSTBUS(2),
					GLAMO_HOSTBUS2_MMIO_EN_2D,
					0);
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_GEN5_1,
					GLAMO_CLOCK_GEN51_EN_DIV_GCLK,
					0);
			break;
		default:
			break;
	}
	return;
}

void
GLAMOEngineEnable(ScreenPtr pScreen, enum GLAMOEngine engine)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);
	char *mmio = glamoc->reg_base;

	if (!mmio)
		return;

	switch (engine) {
		case GLAMO_ENGINE_CMDQ:
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_2D,
					GLAMO_CLOCK_2D_EN_M6CLK,
					0xffff);
			MMIOSetBitMask(mmio, GLAMO_REG_HOSTBUS(2),
					GLAMO_HOSTBUS2_MMIO_EN_CMDQ,
					0xffff);
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_GEN5_1,
					GLAMO_CLOCK_GEN51_EN_DIV_MCLK,
					0xffff);
			break;
		case GLAMO_ENGINE_ISP:
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_ISP,
					GLAMO_CLOCK_ISP_EN_M2CLK |
					GLAMO_CLOCK_ISP_EN_I1CLK,
					0xffff);
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_GEN5_2,
					GLAMO_CLOCK_GEN52_EN_DIV_ICLK,
					0xffff);
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_GEN5_1,
					GLAMO_CLOCK_GEN51_EN_DIV_JCLK,
					0xffff);
			MMIOSetBitMask(mmio, GLAMO_REG_HOSTBUS(2),
					GLAMO_HOSTBUS2_MMIO_EN_ISP,
					0xffff);
			break;
		case GLAMO_ENGINE_2D:
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_2D,
					GLAMO_CLOCK_2D_EN_M7CLK |
					GLAMO_CLOCK_2D_EN_GCLK |
					GLAMO_CLOCK_2D_DG_M7CLK |
					GLAMO_CLOCK_2D_DG_GCLK,
					0xffff);
			MMIOSetBitMask(mmio, GLAMO_REG_HOSTBUS(2),
					GLAMO_HOSTBUS2_MMIO_EN_2D,
					0xffff);
			MMIOSetBitMask(mmio, GLAMO_REG_CLOCK_GEN5_1,
					GLAMO_CLOCK_GEN51_EN_DIV_GCLK,
					0xffff);
			break;
		default:
			break;
	}
}

int
GLAMOEngineBusy(ScreenPtr pScreen, enum GLAMOEngine engine)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);
	GLAMOScreenInfo(pScreenPriv);
	char *mmio = glamoc->reg_base;
	CARD16 status, mask, val;

	if (!mmio)
		return FALSE;

	if (glamos->cmd_queue_cache != NULL)
		GLAMOFlushCMDQCache(glamos, 0);

	switch (engine)
	{
		case GLAMO_ENGINE_CMDQ:
			mask = 0x3;
			val  = mask;
			break;
		case GLAMO_ENGINE_ISP:
			mask = 0x3 | (1 << 8);
			val  = 0x3;
			break;
		case GLAMO_ENGINE_2D:
			mask = 0x3 | (1 << 4);
			val  = 0x3;
			break;
		case GLAMO_ENGINE_ALL:
		default:
			mask = 1 << 2;
			val  = mask;
			break;
	}

	status = MMIO_IN16(mmio, GLAMO_REG_CMDQ_STATUS);

	return !((status & mask) == val);
}

static void
GLAMOEngineWaitReal(ScreenPtr pScreen,
		   enum GLAMOEngine engine,
		   Bool do_flush)
{
	KdScreenPriv(pScreen);
	GLAMOCardInfo(pScreenPriv);
	GLAMOScreenInfo(pScreenPriv);
	char *mmio = glamoc->reg_base;
	CARD16 status, mask, val;
	TIMEOUT_LOCALS;

	if (!mmio)
		return;

	if (glamos->cmd_queue_cache != NULL && do_flush)
		GLAMOFlushCMDQCache(glamos, 0);

	switch (engine)
	{
		case GLAMO_ENGINE_CMDQ:
			mask = 0x3;
			val  = mask;
			break;
		case GLAMO_ENGINE_ISP:
			mask = 0x3 | (1 << 8);
			val  = 0x3;
			break;
		case GLAMO_ENGINE_2D:
			mask = 0x3 | (1 << 4);
			val  = 0x3;
			break;
		case GLAMO_ENGINE_ALL:
		default:
			mask = 1 << 2;
			val  = mask;
			break;
	}

	WHILE_NOT_TIMEOUT(5) {
		status = MMIO_IN16(mmio, GLAMO_REG_CMDQ_STATUS);
		if ((status & mask) == val)
			break;
	}
	if (TIMEDOUT()) {
		GLAMO_LOG_ERROR("Timeout idling accelerator "
				"(0x%x), resetting...\n",
				status);
		GLAMODumpRegs(glamos, 0x1600, 0x1612);
		GLAMOEngineReset(glamos->screen->pScreen, GLAMO_ENGINE_CMDQ);
		GLAMODrawSetup(glamos->screen->pScreen);
	}
}

void
GLAMOEngineWait(ScreenPtr pScreen,
		enum GLAMOEngine engine)
{
	GLAMOEngineWaitReal(pScreen, engine, TRUE);
}

MemBuf *
GLAMOCreateCMDQCache(GLAMOScreenInfo *glamos)
{
	MemBuf *buf;

	buf = (MemBuf *)xcalloc(1, sizeof(MemBuf));
	if (buf == NULL)
		return NULL;

	/*buf->size = glamos->ring_len / 2;*/
	buf->size = glamos->ring_len;
	buf->address = xcalloc(1, buf->size);
	if (buf->address == NULL) {
		xfree(buf);
		return NULL;
	}
	buf->used = 0;

	return buf;
}

static void
GLAMODispatchCMDQCache(GLAMOScreenInfo *glamos)
{
	GLAMOCardInfo *glamoc = glamos->glamoc;
	MemBuf *buf = glamos->cmd_queue_cache;
	char *mmio = glamoc->reg_base;
	CARD16 *addr;
	int count, ring_count;
	TIMEOUT_LOCALS;

	addr = (CARD16 *)((char *)buf->address + glamos->cmd_queue_cache_start);
	count = (buf->used - glamos->cmd_queue_cache_start) / 2;
	ring_count = glamos->ring_len / 2;
	if (count + glamos->ring_write >= ring_count) {
		GLAMOCMDQResetCP(glamos->screen->pScreen);
		glamos->ring_write = 0;
	}

	WHILE_NOT_TIMEOUT(.5) {
		if (count <= 0)
			break;

		glamos->ring_addr[glamos->ring_write] = *addr;
		glamos->ring_write++; addr++;
		if (glamos->ring_write >= ring_count) {
			GLAMO_LOG_ERROR("wrapped over ring_write\n");
			GLAMODumpRegs(glamos, 0x1600, 0x1612);
			glamos->ring_write = 0;
		}
		count--;
	}
	if (TIMEDOUT()) {
		GLAMO_LOG_ERROR("Timeout submitting packets, "
				"resetting...\n");
		GLAMODumpRegs(glamos, 0x1600, 0x1612);
		GLAMOEngineReset(glamos->screen->pScreen, GLAMO_ENGINE_CMDQ);
		GLAMODrawSetup(glamos->screen->pScreen);
	}

	MMIO_OUT16(mmio, GLAMO_REG_CMDQ_WRITE_ADDRH,
			 (glamos->ring_write >> 15) & 0x7);
	MMIO_OUT16(mmio, GLAMO_REG_CMDQ_WRITE_ADDRL,
			 (glamos->ring_write <<  1) & 0xffff);
	GLAMOEngineWaitReal(glamos->screen->pScreen,
			    GLAMO_ENGINE_CMDQ, FALSE);
}

void
GLAMOFlushCMDQCache(GLAMOScreenInfo *glamos, Bool discard)
{
	MemBuf *buf = glamos->cmd_queue_cache;

	if ((glamos->cmd_queue_cache_start == buf->used) && !discard)
		return;
	GLAMODispatchCMDQCache(glamos);

	buf->used = 0;
	glamos->cmd_queue_cache_start = 0;
}

#define CQ_LEN 255
static void
GLAMOCMDQResetCP(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	GLAMOCardInfo(pScreenPriv);
	char *mmio = glamoc->reg_base;
	int cq_len = CQ_LEN;
	CARD32 queue_offset = 0;

	/* make the decoder happy? */
	memset((char*)glamos->ring_addr, 0, glamos->ring_len+2);

	GLAMOEngineReset(glamos->screen->pScreen, GLAMO_ENGINE_CMDQ);

	if (glamos->use_exa) {
		queue_offset = glamos->exa_cmd_queue->offset;
	} else {
		queue_offset = glamos->cmd_queue->offset;
	}
	MMIO_OUT16(mmio, GLAMO_REG_CMDQ_BASE_ADDRL,
		   queue_offset & 0xffff);
	MMIO_OUT16(mmio, GLAMO_REG_CMDQ_BASE_ADDRH,
		   (queue_offset >> 16) & 0x7f);
	MMIO_OUT16(mmio, GLAMO_REG_CMDQ_LEN, cq_len);

	MMIO_OUT16(mmio, GLAMO_REG_CMDQ_WRITE_ADDRH, 0);
	MMIO_OUT16(mmio, GLAMO_REG_CMDQ_WRITE_ADDRL, 0);
	MMIO_OUT16(mmio, GLAMO_REG_CMDQ_READ_ADDRH, 0);
	MMIO_OUT16(mmio, GLAMO_REG_CMDQ_READ_ADDRL, 0);
	MMIO_OUT16(mmio, GLAMO_REG_CMDQ_CONTROL,
			 1 << 12 |
			 5 << 8 |
			 8 << 4);
	GLAMOEngineWaitReal(pScreen, GLAMO_ENGINE_ALL, FALSE);
}

static Bool
GLAMOCMDQInit(ScreenPtr pScreen,
	      Bool force)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	GLAMOCardInfo(pScreenPriv);
	char *mmio = glamoc->reg_base;
	int cq_len = CQ_LEN;

	if (!force && glamos->use_exa && glamos->exa_cmd_queue)
		return TRUE;
	if (!force && !glamos->use_exa && glamos->cmd_queue)
		return TRUE;

	glamos->ring_len = (cq_len + 1) * 1024;

	if (glamos->use_exa) {
		glamos->exa_cmd_queue =
			exaOffscreenAlloc(pScreen, glamos->ring_len + 4,
					  glamos->exa.pixmapOffsetAlign,
					  TRUE, NULL, NULL);
		if (!glamos->exa_cmd_queue)
			return FALSE;
		glamos->ring_addr =
			(CARD16 *) (pScreenPriv->screen->memory_base +
					glamos->exa_cmd_queue->offset);
	} else {
		glamos->cmd_queue =
			KdOffscreenAlloc(pScreen, glamos->ring_len + 4,
					 glamos->kaa.offsetAlign,
					 TRUE, NULL, NULL);
		if (!glamos->cmd_queue)
			return FALSE;
		glamos->ring_addr =
			(CARD16 *) (pScreenPriv->screen->memory_base +
						glamos->cmd_queue->offset);
	}

	GLAMOEngineEnable(glamos->screen->pScreen, GLAMO_ENGINE_CMDQ);
	GLAMOCMDQResetCP(glamos->screen->pScreen);
	return TRUE;
}

void
GLAMOCMDQCacheSetup(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);

	GLAMOCMDQInit(pScreen, TRUE);

	if (glamos->cmd_queue_cache)
		return;

	glamos->cmd_queue_cache = GLAMOCreateCMDQCache(glamos);
	if (glamos->cmd_queue_cache == FALSE)
		FatalError("Failed to allocate cmd queue cache buffer.\n");
}

void
GLAMOCMQCacheTeardown(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);

	GLAMOEngineWait(pScreen, GLAMO_ENGINE_ALL);

	xfree(glamos->cmd_queue_cache->address);
	xfree(glamos->cmd_queue_cache);
	glamos->cmd_queue_cache = NULL;
}
