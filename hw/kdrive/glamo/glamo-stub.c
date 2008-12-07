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
#include "glamo.h"
#include "klinux.h"


extern int use_evdev;

void
InitCard(char *name)
{
	KdCardAttr attr;

	attr.io = 0;
	attr.address[0] = 0x8000000;
	attr.naddr = 1;
	KdCardInfoAdd(&GLAMOFuncs, &attr, 0);
}

void
InitOutput(ScreenInfo *pScreenInfo, int argc, char **argv)
{
	char *verbosity_str=NULL;
	int verbosity=0;
	verbosity_str = getenv("VERBOSITY");
	if (!verbosity_str) {
		verbosity = 5;
	} else {
		verbosity = atoi(verbosity_str);
	}
	LogSetParameter(XLOG_VERBOSITY, verbosity) ;
	LogMessage(X_INFO, "verbosity set to %d\n", verbosity);
	KdInitOutput(pScreenInfo, argc, argv);
}

void
InitInput(int argc, char **argv)
{
    KdOsAddInputDrivers();
    KdInitInput();
}

void
ddxUseMsg (void)
{
	KdUseMsg();
}

int
ddxProcessArgument(int argc, char **argv, int i)
{
	int	ret;

		ret = KdProcessArgument(argc, argv, i);

	return ret;
}
