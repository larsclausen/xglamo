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
 * authors:
 *   Dodji SEKETELI <dodji@openedhand.com>
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif


#ifdef XV

#include "glamo.h"
#include "glamo-cmdq.h"
#include "glamo-draw.h"
#include "glamo-regs.h"
#include "glamo-log.h"
#include "kaa.h"

#include <X11/extensions/Xv.h>
#include "fourcc.h"

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

#define SYS_PITCH_ALIGN(w) (((w) + 3) & ~3)
#define VID_PITCH_ALIGN(w) (((w) + 1) & ~1)

#define IMAGE_MAX_WIDTH		640
#define IMAGE_MAX_HEIGHT	640

static void
GLAMOStopVideo(KdScreenInfo *screen, pointer data, Bool exit)
{
	/*ScreenPtr pScreen = screen->pScreen;
	GLAMOPortPrivPtr pPortPriv = (GLAMOPortPrivPtr)data;*/


	GLAMO_LOG("enter\n");


	/*
	for (i = 0; i < GLAMO_VIDEO_NUM_BUFS; i++)
		if (pPortPriv->off_screen[i]) {
			KdOffscreenFree (pScreen, pPortPriv->off_screen[i]);
			pPortPriv->off_screen[i] = 0;
		}
	*/


	GLAMO_LOG("leave\n");

}

static int
GLAMOSetPortAttribute(KdScreenInfo *screen, Atom attribute, int value,
                      pointer data)
{
	return BadMatch;
}

static int
GLAMOGetPortAttribute(KdScreenInfo *screen, Atom attribute, int *value,
                      pointer data)
{
	return BadMatch;
}

static void
GLAMOQueryBestSize(KdScreenInfo *screen,
		   Bool motion,
		   short vid_w,
		   short vid_h,
		   short drw_w,
		   short drw_h,
		   unsigned int *p_w,
		   unsigned int *p_h,
		   pointer data)
{
	*p_w = drw_w;
	*p_h = drw_h;
}

static void
GLAMOVideoSave(ScreenPtr pScreen, KdOffscreenArea *area)
{
	/*KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	GLAMOPortPrivPtr pPortPriv = glamos->pAdaptor->pPortPrivates[0].ptr;
	int i;*/

	GLAMO_LOG("mark\n");
}

static Bool
GetYUVFrameByteSize (int fourcc_code,
		     unsigned short width,
		     unsigned short height,
		     unsigned int *size)
{
	if (!size)
		return FALSE;
	switch (fourcc_code) {
		case FOURCC_YV12:
		case FOURCC_I420:
			*size = width*height * 3 / 2 ;
			break;
		default:
			return FALSE;
	}
	return TRUE;

}

static Bool
GetYUVFrameAddresses (unsigned int frame_addr,
		      int fourcc_code,
		      unsigned short frame_width,
		      unsigned short frame_height,
		      unsigned short x,
		      unsigned short y,
		      unsigned int *y_addr,
		      unsigned int *u_addr,
		      unsigned int *v_addr)
{
	Bool is_ok = FALSE;

	if (!u_addr || !v_addr) {
		GLAMO_LOG_ERROR("failed sanity check\n");
		goto out;
	}


	GLAMO_LOG("enter: frame(%dx%d), frame_addr:%#x\n"
		  "position:(%d,%d)\n",
		  frame_width, frame_height,
		  frame_addr, x, y);


	switch (fourcc_code) {
		case FOURCC_YV12:
			*y_addr = frame_addr + x +  y * frame_width;
			*v_addr = frame_addr + frame_width*frame_height+
			          x/2 + (y/2) *(frame_width/2);
			*u_addr = frame_addr + frame_width*frame_height +
			          frame_width*frame_height/4 +
				  x/2 + (y/2)*(frame_width/2);
			is_ok = TRUE;
			break;
		case FOURCC_I420:
			*y_addr = frame_addr + x +  y*frame_width;
			*u_addr = frame_addr + frame_width*frame_height+
			          x/2 + (y/2)*frame_width/2;
			*v_addr = frame_addr + frame_width*frame_height +
			          frame_width*frame_height/4 +
				  x/2 + (y/2)*frame_height/2;
			is_ok = TRUE;
			break;
		default:
			is_ok = FALSE;
			break;
	}

	GLAMO_LOG("y_addr:%#x, u_addr:%#x, v_addr:%#x\n",
		  *y_addr, *u_addr, *v_addr);

out:

	GLAMO_LOG("leave. is_ok:%d\n",
		  is_ok);

	return is_ok;
}

/**
 * copy a portion of the YUV frame "src_frame" to a destination in memory.
 * The portion to copy is a rectangle located at (src_x,src_y),
 * of size (rect_width,rect_height).
 *
 * @src_frame pointer to the start of the YUV frame to consider
 * @frame_width width of the YUV frame
 * @frame_height height of the YUV frame
 * @src_x
 * @src_y
 * @rect_width
 * @rect_height
 */
static Bool
CopyYUVPlanarFrameRect (const unsigned char *src_frame,
			int fourcc_code,
			unsigned short frame_width,
			unsigned short frame_height,
			unsigned short src_x,
			unsigned short src_y,
			unsigned short rect_width,
			unsigned short rect_height,
			char *destination)
{
	char *y_copy_src, *u_copy_src, *v_copy_src,
		*y_copy_dst, *u_copy_dst, *v_copy_dst;
	unsigned line;
	Bool is_ok = FALSE;


	GLAMO_LOG("enter. src_frame:%#x, code:%#x\n"
		  "frame(%d,%d)-(%dx%d), crop(%dx%d)\n"
		  "dest:%#x",
		  (unsigned)src_frame, (unsigned)fourcc_code,
		  src_x, src_y, frame_width, frame_height,
		  rect_width, rect_height, (unsigned)destination);


	switch (fourcc_code) {
		case FOURCC_YV12:
		case FOURCC_I420:
			/*planar yuv formats of the 4:2:0 family*/
			y_copy_src = (char*) src_frame + src_x +
					frame_width*src_y;
			u_copy_src = (char*) src_frame +
					frame_width*frame_height +
					src_x/2 + (frame_width/2)*(src_y/2);
			v_copy_src = (char*) src_frame +
				frame_width*frame_height*5/4 + src_x/2 +
				(frame_width/2)*src_y/2;
			y_copy_dst = destination;
			u_copy_dst = destination + rect_width*rect_height;
			v_copy_dst = destination + rect_width*rect_height*5/4;

			GLAMO_LOG("y_copy_src:%#x, "
				  "u_copy_src:%#x, "
				  "v_copy_src:%#x\n"
				  "y_copy_dst:%#x, "
				  "u_copy_dst:%#x, "
				  "v_copy_dst:%#x\n",
				  (unsigned)y_copy_src,
				  (unsigned)u_copy_src,
				  (unsigned)v_copy_src,
				  (unsigned)y_copy_dst,
				  (unsigned)u_copy_dst,
				  (unsigned)v_copy_dst);

			for (line = 0; line < rect_height; line++) {

				GLAMO_LOG("============\n"
					  "line:%d\n"
					  "============\n",
					  line);
				GLAMO_LOG("y_copy_src:%#x, "
					  "y_copy_dst:%#x, \n",
					  (unsigned)y_copy_src,
					  (unsigned)y_copy_dst);


				memcpy(y_copy_dst,
				       y_copy_src,
				       rect_width);
				y_copy_src += frame_width;
				y_copy_dst += rect_width;

				/*
				 * one line out of two has chrominance (u,v)
				 * sampling.
				 */
				if (!(line&1)) {

					GLAMO_LOG("u_copy_src:%#x, "
						  "u_copy_dst:%#x\n",
						  (unsigned)u_copy_src,
						  (unsigned)u_copy_dst);

					memcpy(u_copy_dst,
					       u_copy_src,
					       rect_width/2);

					GLAMO_LOG("v_copy_src:%#x, "
						  "v_copy_dst:%#x\n",
						  (unsigned)v_copy_src,
						  (unsigned)v_copy_dst);

					memcpy(v_copy_dst,
					       v_copy_src,
					       rect_width/2);
					u_copy_src += frame_width/2;
					u_copy_dst += rect_width/2;
					v_copy_src += frame_width/2;
					v_copy_dst += rect_width/2;
				}
			}
			break;
		default:
			/*
			 * glamo 3362 only supports YUV 4:2:0 planar formats.
			 */
			is_ok = FALSE;
			goto out;
	}
	is_ok = TRUE;
out:

	GLAMO_LOG("leave.is_ok:%d\n", is_ok);

	return is_ok;
}

static Bool
GLAMOVideoUploadFrameToOffscreen (KdScreenInfo *screen,
				  unsigned char *yuv_frame,
				  int fourcc_code,
				  unsigned yuv_frame_width,
				  unsigned yuv_frame_height,
				  short src_x, short src_y,
				  short src_w, short src_h,
				  GLAMOPortPrivPtr portPriv,
				  unsigned int *out_offscreen_frame)
{
	unsigned size = 0;
	Bool is_ok = FALSE;
	ScreenPtr pScreen = screen->pScreen;
	char *offscreen_frame = NULL;


	GLAMO_LOG("enter. frame(%dx%d), crop(%dx%d)\n",
		  yuv_frame_width, yuv_frame_height,
		  src_w, src_h);


	if (!GetYUVFrameByteSize (fourcc_code, src_w, src_h, &size)) {
		GLAMO_LOG_ERROR("failed to get frame size\n");
		goto out;
	}

	if (!portPriv->off_screen_yuv_buf
	    || size < portPriv->off_screen_yuv_buf->size) {
		if (portPriv->off_screen_yuv_buf) {
			KdOffscreenFree(pScreen,
					portPriv->off_screen_yuv_buf);
		}
		portPriv->off_screen_yuv_buf =
			KdOffscreenAlloc(pScreen, size, VID_PITCH_ALIGN(2),
					 TRUE, GLAMOVideoSave, portPriv);
		if (!portPriv->off_screen_yuv_buf) {
			GLAMO_LOG_ERROR("failed to allocate "
					"offscreen memory\n");
			goto out;
		}

		GLAMO_LOG("allocated %d bytes of offscreen memory\n", size);

	}
	offscreen_frame = (char*)screen->memory_base +
				portPriv->off_screen_yuv_buf->offset;

	if (out_offscreen_frame)
		*out_offscreen_frame = portPriv->off_screen_yuv_buf->offset;

	if (!CopyYUVPlanarFrameRect (yuv_frame, fourcc_code,
				     yuv_frame_width, yuv_frame_height,
				     src_x, src_y, src_w, src_h,
				     offscreen_frame)) {
		GLAMO_LOG_ERROR("failed to copy yuv "
				"frame to offscreen memory\n");
		goto out;
	}

	is_ok = TRUE;
out:


	GLAMO_LOG("leave:%d\n", is_ok);

	return is_ok;

}

static void
GLAMODisplayYUVPlanarFrameRegion (ScreenPtr pScreen,
				  unsigned int yuv_frame_addr,
				  int fourcc_code,
				  short frame_width,
				  short frame_height,
				  unsigned int dst_addr,
				  short dst_pitch,
				  unsigned short scale_w,
				  unsigned short scale_h,
				  RegionPtr clipping_region,
				  BoxPtr dst_box)
{
	BoxPtr rect = NULL;
	int num_rects = 0;
	int i =0;
	int dst_width = 0, dst_height = 0;
	unsigned int y_addr = 0, u_addr = 0, v_addr = 0,
		     src_x, src_y, src_w, src_h, dest_addr;

	GLAMO_RETURN_IF_FAIL(clipping_region && dst_box);


	GLAMO_LOG("enter: frame addr:%#x, fourcc:%#x, \n"
		  "frame:(%dx%d), dst_addr:%#x, dst_pitch:%hd\n"
		  "scale:(%hd,%hd), dst_box(%d,%d)\n",
		  yuv_frame_addr, fourcc_code, frame_width, frame_height,
		  dst_addr, dst_pitch, scale_w, scale_h,
		  dst_box->x1, dst_box->y1);


	GLAMO_RETURN_IF_FAIL(clipping_region);
	rect = REGION_RECTS(clipping_region);
	num_rects = REGION_NUM_RECTS(clipping_region);

	GLAMO_LOG("num_rects to display:%d\n", num_rects);

	for (i = 0; i < num_rects; i++, rect++) {

		GLAMO_LOG("rect num:%d, (%d,%d,%d,%d)\n",
			  i,
			  rect->x1, rect->y1,
			  rect->x2, rect->y2);

		dst_width = abs(rect->x2 - rect->x1);
		dst_height = abs(rect->y2 - rect->y1);
		dest_addr = dst_addr + rect->x1*2 + rect->y1*dst_pitch;
		src_w = (dst_width * scale_w) >> 11;
		src_h = (dst_height * scale_h) >> 11;
		src_x = ((abs(rect->x1 - dst_box->x1) * scale_w) >> 11);
		src_y = ((abs(rect->y1 - dst_box->y1) * scale_h) >> 11);

		GLAMO_LOG("matching src rect:(%d,%d)-(%dx%d)\n",

			  src_x,src_y, src_w, src_h);

		if (!GetYUVFrameAddresses (yuv_frame_addr,
					   fourcc_code,
					   frame_width,
					   frame_height,
					   src_x,
					   src_y,
					   &y_addr,
					   &u_addr,
					   &v_addr)) {
			GLAMO_LOG_ERROR("failed to get yuv frame @\n");
			continue;
		}
		GLAMOISPDisplayYUVPlanarFrame(pScreen,
					      y_addr,
					      u_addr,
					      v_addr,
					      frame_width,
					      frame_width/2,
					      src_w, src_h,
					      dest_addr,
					      dst_pitch,
					      dst_width, dst_height,
					      scale_w, scale_h);
	}

	GLAMO_LOG("leave\n");

}

static void
GLAMODisplayFrame (KdScreenInfo *screen,
		   GLAMOPortPrivPtr portPriv)
{
	GLAMOVideoFrameDisplayInfo *info;
	unsigned int dest_w, dest_h, dest_frame_addr;
	unsigned short scale_w, scale_h;
	PixmapPtr dest_pixmap;

	GLAMO_LOG("enter\n");


	GLAMO_RETURN_IF_FAIL (screen
			      && screen->pScreen
			      && portPriv);

	info = &portPriv->frame_display_info;

	GLAMO_RETURN_IF_FAIL (info
			      && info->dest_drawable);

	if (info->dest_drawable->type == DRAWABLE_WINDOW) {
		dest_pixmap =
			(*screen->pScreen->GetWindowPixmap)
					((WindowPtr)info->dest_drawable);
	} else {
		dest_pixmap = (PixmapPtr)info->dest_drawable;
	}
	dest_frame_addr =
		(CARD8*)dest_pixmap->devPrivate.ptr - screen->memory_base;

	dest_w = abs(info->dest_box.x2 - info->dest_box.x1);
	dest_h = abs(info->dest_box.y2 - info->dest_box.y1);
	scale_w = (info->src_w << 11)/dest_w;
	scale_h = (info->src_h << 11)/dest_h;

	GLAMODisplayYUVPlanarFrameRegion(screen->pScreen,
					 info->offscreen_frame_addr,
					 info->id,
					 info->src_w, info->src_h,
					 dest_frame_addr,
					 dest_pixmap->devKind,
					 scale_w, scale_h,
					 &info->clipped_dest_region,
					 &info->dest_box);

	GLAMO_LOG("leave\n");

}

static int
GLAMOPutImage(KdScreenInfo *screen, DrawablePtr dst_drawable,
	      short src_x, short src_y,
	      short drw_x, short drw_y,
	      short src_w, short src_h,
	      short drw_w, short drw_h,
	      int id,
	      unsigned char *buf,
	      short width,
	      short height,
	      Bool sync,
	      RegionPtr clipBoxes,
	      pointer data)
{
	GLAMOPortPrivPtr portPriv = (GLAMOPortPrivPtr)data;
	GLAMOVideoFrameDisplayInfo *info = NULL;
	RegionRec dst_reg;
    unsigned int offscreen_frame_addr = 0;


	GLAMO_LOG("enter. id:%#x, frame:(%dx%d) srccrop:(%d,%d)-(%dx%d)\n"
		  "dst geo:(%d,%d)-(%dx%d)\n",
		  id, width, height, src_x, src_y, src_w, src_h,
		  drw_x, drw_y, drw_w, drw_h);

	/*
	 * upload the YUV frame to offscreen vram so that GLAMO can
	 * later have access to it and blit it from offscreen vram to
	 * onscreen vram.
	 */
	if (!GLAMOVideoUploadFrameToOffscreen(screen, buf, id,
					      width, height,
					      src_x, src_y, src_w, src_h,
					      portPriv,
					      &offscreen_frame_addr)) {
		GLAMO_LOG("failed to upload frame to offscreen\n");
		return BadAlloc;
	}

	GLAMO_LOG("copied video frame to vram offset:%#x\n",
		  offscreen_frame_addr);
	GLAMO_LOG("y_pitch:%hd, crop(%hdx%hd)\n", src_w, src_w, src_h);


	/*
	 * Save the information necessary to display the video frame
	 * - that has been uploaded to offscreen vram - in portPriv.
	 * That way, we can later display that very same frame from
	 * within other driver calls like ReputImage.
	 */
	info = &portPriv->frame_display_info;
	memset(&dst_reg, 0, sizeof(dst_reg));

	info->offscreen_frame_addr = offscreen_frame_addr;
	info->id = id;
	info->src_w = src_w;
	info->src_h = src_h;
	info->dest_drawable = dst_drawable;
	info->drw_w = drw_w;
	info->drw_h = drw_h;
	info->dest_box.x1 = drw_x;
	info->dest_box.y1 =  drw_y;
	info->dest_box.x2 = drw_x + drw_w;
	info->dest_box.y2 = drw_y + drw_h;
	REGION_INIT(pScreen, &dst_reg, &info->dest_box, 0);
	REGION_INTERSECT(pScreen, &info->clipped_dest_region,
			 &dst_reg, clipBoxes);
	REGION_UNINIT(pScreen, &dst_reg);
	GLAMO_RETURN_VAL_IF_FAIL(REGION_NOTEMPTY(pScreen,
				                 &info->clipped_dest_region),
				 BadImplementation);

	/*
	 * Okay now that we did save all the information necessary to
	 * display the video frame in portPriv, let's display the frame.
	 */
	GLAMODisplayFrame(screen, portPriv);


	GLAMO_LOG("leave\n");

	return Success;
}

static int
GLAMOReputImage(KdScreenInfo *screen,
		DrawablePtr dest_drawable,
		short drw_x, short drw_y,
		RegionPtr clipBoxes,
		pointer data)
{
	GLAMOPortPrivPtr	portPriv = (GLAMOPortPrivPtr)data;
	RegionRec dst_reg;


	GLAMO_LOG("enter\n");


	GLAMO_RETURN_VAL_IF_FAIL(portPriv
				 && screen
				 && dest_drawable
				 && clipBoxes,
				 BadImplementation);
	GLAMOVideoFrameDisplayInfo *info = &portPriv->frame_display_info;
	memset(&dst_reg, 0, sizeof(dst_reg));

	info->dest_box.x1 = drw_x;
	info->dest_box.y1 = drw_y;
	info->dest_box.x2 = drw_x + info->drw_w;
	info->dest_box.y2 = drw_y + info->drw_h;
	REGION_INIT(pScreen, &dst_reg, &info->dest_box, 0);
	REGION_INTERSECT(pScreen, &info->clipped_dest_region,
			 &dst_reg, clipBoxes);
	REGION_UNINIT(pScreen, &dst_reg);
	GLAMO_RETURN_VAL_IF_FAIL(REGION_NOTEMPTY(pScreen,
				                 &info->clipped_dest_region),
				 BadImplementation);
	GLAMODisplayFrame(screen, portPriv);

	GLAMO_LOG("leave\n");

	return Success;
}

static int
GLAMOQueryImageAttributes(KdScreenInfo *screen, int id, unsigned short *w,
			  unsigned short *h, int *pitches, int *offsets)
{
	int size, tmp;

	if (*w > IMAGE_MAX_WIDTH)
		*w = IMAGE_MAX_WIDTH;
	if (*h > IMAGE_MAX_HEIGHT)
		*h = IMAGE_MAX_HEIGHT;

	*w = (*w + 1) & ~1;
	if (offsets)
		offsets[0] = 0;

	switch (id)
	{
	case FOURCC_YV12:
	case FOURCC_I420:
		*h = (*h + 1) & ~1;

		tmp = SYS_PITCH_ALIGN(*w);
		size = tmp * *h;

		if (pitches)
			pitches[0] = tmp;
		if (offsets)
			offsets[1] = size;

		tmp = SYS_PITCH_ALIGN(*w / 2);
		size += tmp * *h / 2;
		if (pitches)
			pitches[1] = pitches[2] = tmp;
		if (offsets)
			offsets[2] = size;

		size += tmp * *h / 2;
		break;
	case FOURCC_UYVY:
	case FOURCC_YUY2:
	default:
		size = *w << 1;
		if (pitches)
			pitches[0] = size;
		size *= *h;
		break;
	}

	return size;
}


/* client libraries expect an encoding */
static KdVideoEncodingRec DummyEncoding[1] =
{
	{
		0,
		"XV_IMAGE",
		IMAGE_MAX_WIDTH, IMAGE_MAX_HEIGHT,
		{1, 1}
	}
};

#define NUM_FORMATS 1

static KdVideoFormatRec Formats[NUM_FORMATS] =
{
	{16, TrueColor}
};

#define NUM_ATTRIBUTES 0
static KdAttributeRec Attributes[NUM_ATTRIBUTES] =
{
};

static KdImageRec Images[] =
{
	XVIMAGE_YV12,
	XVIMAGE_I420,
};
#define NUM_IMAGES (sizeof(Images)/sizeof(Images[0]))

static KdVideoAdaptorPtr
GLAMOSetupImageVideo(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	KdVideoAdaptorPtr adapt;
	GLAMOPortPrivPtr portPriv;
	int i;

	GLAMO_LOG("enter\n");
	glamos->num_texture_ports = 1;

	adapt = xcalloc(1, sizeof(KdVideoAdaptorRec) +
			   glamos->num_texture_ports *
			   (sizeof(GLAMOPortPrivRec) + sizeof(DevUnion)));
	if (adapt == NULL)
		return NULL;

	adapt->type = XvWindowMask | XvInputMask | XvImageMask;
	adapt->flags = VIDEO_CLIP_TO_VIEWPORT;
	adapt->name = "GLAMO Texture Video";
	adapt->nEncodings = 1;
	adapt->pEncodings = DummyEncoding;
	adapt->nFormats = NUM_FORMATS;
	adapt->pFormats = Formats;
	adapt->nPorts = glamos->num_texture_ports;
	adapt->pPortPrivates = (DevUnion*)(&adapt[1]);

	portPriv = (GLAMOPortPrivPtr)
		(&adapt->pPortPrivates[glamos->num_texture_ports]);

	for (i = 0; i < glamos->num_texture_ports; i++)
		adapt->pPortPrivates[i].ptr = &portPriv[i];

	adapt->nAttributes = NUM_ATTRIBUTES;
	adapt->pAttributes = Attributes;
	adapt->pImages = Images;
	adapt->nImages = NUM_IMAGES;
	adapt->PutVideo = NULL;
	adapt->PutStill = NULL;
	adapt->GetVideo = NULL;
	adapt->GetStill = NULL;
	adapt->StopVideo = GLAMOStopVideo;
	adapt->SetPortAttribute = GLAMOSetPortAttribute;
	adapt->GetPortAttribute = GLAMOGetPortAttribute;
	adapt->QueryBestSize = GLAMOQueryBestSize;
	adapt->PutImage = GLAMOPutImage;
	adapt->ReputImage = GLAMOReputImage;
	adapt->QueryImageAttributes = GLAMOQueryImageAttributes;

	for (i = 0; i < glamos->num_texture_ports; i++) {
		REGION_INIT(pScreen, &portPriv[i].clip, NullBox, 0);
		portPriv[i].color_key = 0xffff;
	}

	glamos->pAdaptor = adapt;

	GLAMO_LOG("leave. adaptor:%#x\n", (unsigned)adapt);

	return adapt;
}

Bool
GLAMOInitVideo(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	GLAMOCardInfo(pScreenPriv);
	KdScreenInfo *screen = pScreenPriv->screen;
	KdVideoAdaptorPtr *oldAdaptors = NULL, *newAdaptors = NULL;
	KdVideoAdaptorPtr newAdaptor = NULL;
	int num_adaptors;
	Bool is_ok = FALSE;

	GLAMO_LOG("enter\n");

	glamos->pAdaptor = NULL;

	if (glamoc->reg_base == NULL) {
		GLAMO_LOG("glamoc->reg_base is null\n");
		goto out;
	}

	num_adaptors = KdXVListGenericAdaptors(screen, &oldAdaptors);

	newAdaptor = GLAMOSetupImageVideo(pScreen);

	if (newAdaptor)  {
		if (!num_adaptors) {
			num_adaptors = 1;
			newAdaptors = &newAdaptor;
		} else {
			newAdaptors = xalloc((num_adaptors + 1) *
			    sizeof(KdVideoAdaptorPtr *));
			if (!newAdaptors)
				goto out;

			memcpy(newAdaptors,
			       oldAdaptors,
			       num_adaptors * sizeof(KdVideoAdaptorPtr));
			newAdaptors[num_adaptors] = newAdaptor;
			num_adaptors++;
		}
	}

	GLAMOCMDQCacheSetup(pScreen);
	GLAMOISPEngineInit(pScreen);

	if (num_adaptors)
		KdXVScreenInit(pScreen, newAdaptors, num_adaptors);

	is_ok = TRUE;
out:
	GLAMO_LOG("leave. is_ok:%d, adaptors:%d\n",
		  is_ok, num_adaptors);
	/*
	if (newAdaptors) {
		xfree(newAdaptors);
	}
	*/
	return is_ok;
}

void
GLAMOFiniVideo(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	GLAMOScreenInfo(pScreenPriv);
	KdVideoAdaptorPtr adapt = glamos->pAdaptor;
	GLAMOPortPrivPtr portPriv;
	int i;

	GLAMO_LOG ("enter\n");

	if (!adapt)
		return;

	for (i = 0; i < glamos->num_texture_ports; i++) {
		portPriv = (GLAMOPortPrivPtr)(adapt->pPortPrivates[i].ptr);
		GLAMO_LOG("freeing clipping region...\n");
		REGION_UNINIT(pScreen, &portPriv->clip);
		GLAMO_LOG("freed clipping region\n");
		if (portPriv->off_screen_yuv_buf) {
			GLAMO_LOG("freeing offscreen yuv buf...\n");
			KdOffscreenFree(pScreen,
					portPriv->off_screen_yuv_buf);
			portPriv->off_screen_yuv_buf = NULL;
			GLAMO_LOG("freeed offscreen yuv buf\n");
		}
		REGION_UNINIT
			(pScreen,
			 &portPriv->frame_display_info.clipped_dest_region);
	}
	xfree(adapt);
	glamos->pAdaptor = NULL;
	GLAMO_LOG ("leave\n");
}

void
GLAMOVideoTeardown(ScreenPtr pScreen)
{
	GLAMOEngineReset(pScreen, GLAMO_ENGINE_ISP);
	GLAMOEngineDisable(pScreen, GLAMO_ENGINE_ISP);
}

#endif /*XV*/
