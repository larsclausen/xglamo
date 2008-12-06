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
 *
 * Author: Dodji Seketeli <dodji@openedhand.com>
 */
#ifndef _GLAMO_LOG_H_
#define _GLAMO_LOG_H_

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif

#include <assert.h>
#include "os.h"

#ifdef NDEBUG
/*we are not in debug mode*/
#define GLAMO_LOG
#define GLAMO_LOG_ERROR

#else /*NDEBUG*/
#define ERROR_LOG_LEVEL 3
#define INFO_LOG_LEVEL 4

#ifndef GLAMO_LOG
#define GLAMO_LOG(...) \
LogMessageVerb(X_NOTICE, INFO_LOG_LEVEL, "in %s:%d:%s: ",\
               __FILE__, __LINE__, __func__) ; \
LogMessageVerb(X_NOTICE, INFO_LOG_LEVEL, __VA_ARGS__)
#endif /*GLAMO_LOG*/

#ifndef GLAMO_LOG_ERROR
#define GLAMO_LOG_ERROR(...) \
LogMessageVerb(X_NOTICE, ERROR_LOG_LEVEL, "Error:in %s:%d:%s: ",\
               __FILE__, __LINE__, __func__) ; \
LogMessageVerb(X_NOTICE, ERROR_LOG_LEVEL, __VA_ARGS__)
#endif /*GLAMO_LOG_ERROR*/


#endif /*NDEBUG*/

#ifndef GLAMO_RETURN_IF_FAIL
#define GLAMO_RETURN_IF_FAIL(cond) \
if (!(cond)) {\
	GLAMO_LOG_ERROR("contion failed:%s\n",#cond);\
	return; \
}
#endif /*GLAMO_RETURN_IF_FAIL*/

#ifndef GLAMO_RETURN_VAL_IF_FAIL
#define GLAMO_RETURN_VAL_IF_FAIL(cond, val) \
if (!(cond)) {\
	GLAMO_LOG_ERROR("contion failed:%s\n",#cond);\
	return val; \
}
#endif /*GLAMO_RETURN_VAL_IF_FAIL*/

#endif /*_GLAMO_LOG_H_*/

