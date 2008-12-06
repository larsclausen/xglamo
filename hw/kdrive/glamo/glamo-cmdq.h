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

#ifndef _GLAMO_DMA_H_
#define _GLAMO_DMA_H_

#define CCE_DEBUG 1

#if !CCE_DEBUG

#define RING_LOCALS	CARD16 *__head; int __count
#define BEGIN_CMDQ(n)							\
do {									\
	if ((glamos->cmd_queue_cache->used + 2 * (n)) >			\
	    glamos->cmd_queue_cache->size) {				\
		GLAMOFlushCMDQCache(glamos, 1);				\
	}								\
	__head = (CARD16 *)((char *)glamos->cmd_queue_cache->address +	\
	    glamos->cmd_queue_cache->used);				\
	__count = 0;							\
} while (0)
#define END_CMDQ() do {							\
	glamos->cmd_queue_cache->used += __count * 2;			\
} while (0)

#else

#define RING_LOCALS	\
	CARD16 *__head; int __count, __total, __reg, __packet0count
#define BEGIN_CMDQ(n)							\
do {									\
	if ((glamos->cmd_queue_cache->used + 2 * (n)) >			\
	    glamos->cmd_queue_cache->size) {				\
		GLAMOFlushCMDQCache(glamos, 1);				\
	}								\
	__head = (CARD16 *)((char *)glamos->cmd_queue_cache->address +	\
	    glamos->cmd_queue_cache->used);				\
	__count = 0;							\
	__total = n;							\
	__reg = 0;								\
	__packet0count = 0;								\
} while (0)
#define END_CMDQ() do {							\
	if (__count != __total)						\
		FatalError("count != total (%d vs %d) at %s:%d\n",	 \
		     __count, __total, __FILE__, __LINE__);		\
	glamos->cmd_queue_cache->used += __count * 2;			\
} while (0)

#endif

#define OUT_PAIR(v1, v2)                                               \
do {                                                                   \
       __head[__count++] = (v1);                                       \
       __head[__count++] = (v2);                                       \
} while (0)

#define OUT_BURST_REG(reg, val) do {                                   \
       if (__reg != reg)                                               \
               FatalError("unexpected reg (0x%x vs 0x%x) at %s:%d\n",  \
                   reg, __reg, __FILE__, __LINE__);                    \
       if (__packet0count-- <= 0)                                      \
               FatalError("overrun of packet0 at %s:%d\n",             \
                   __FILE__, __LINE__);                                \
       __head[__count++] = (val);                                      \
       __reg += 2;                                                     \
} while (0)

#define OUT_REG(reg, val)                                              \
       OUT_PAIR(reg, val)

#define OUT_BURST(reg, n)                                              \
do {                                                                   \
       OUT_PAIR((1 << 15) | reg, n);                                   \
       __reg = reg;                                                    \
       __packet0count = n;                                             \
} while (0)


#define TIMEOUT_LOCALS struct timeval _target, _curtime

static inline Bool
tv_le(struct timeval *tv1, struct timeval *tv2)
{
	if (tv1->tv_sec < tv2->tv_sec ||
	    (tv1->tv_sec == tv2->tv_sec && tv1->tv_usec < tv2->tv_usec))
		return TRUE;
	else
		return FALSE;
}

#define WHILE_NOT_TIMEOUT(_timeout)					\
	gettimeofday(&_target, NULL);					\
	_target.tv_usec += ((_timeout) * 1000000);			\
	_target.tv_sec += _target.tv_usec / 1000000;			\
	_target.tv_usec = _target.tv_usec % 1000000;			\
	while (gettimeofday(&_curtime, NULL), tv_le(&_curtime, &_target))

#define TIMEDOUT()	(!tv_le(&_curtime, &_target))

MemBuf *
GLAMOCreateCMDQCache(GLAMOScreenInfo *glamos);

void
GLAMOFlushCMDQCache(GLAMOScreenInfo *glamos, Bool discard);

void
GLAMOCMDQCacheSetup(ScreenPtr pScreen);

void
GLAMOCMQCacheTeardown(ScreenPtr pScreen);

enum GLAMOEngine {
	GLAMO_ENGINE_CMDQ,
	GLAMO_ENGINE_ISP,
	GLAMO_ENGINE_2D,
	GLAMO_ENGINE_MPEG,
	GLAMO_ENGINE_ALL,
	NB_GLAMO_ENGINES /*should be the last entry*/
};

void
GLAMOEngineEnable(ScreenPtr pScreen, enum GLAMOEngine engine);

void
GLAMOEngineDisable(ScreenPtr pScreen, enum GLAMOEngine engine);

void
GLAMOEngineReset(ScreenPtr pScreen, enum GLAMOEngine engine);

int
GLAMOEngineBusy(ScreenPtr pScreen, enum GLAMOEngine engine);

void
GLAMOEngineWait(ScreenPtr pScreen, enum GLAMOEngine engine);

#endif /* _GLAMO_DMA_H_ */

