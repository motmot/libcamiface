/***********************************************************************

    Copyright (c) Industrial Research Limited 2004

    This file is part of Camwire, a generic camera interface.

    Camwire is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation; either version 2.1 of the
    License, or (at your option) any later version.

    Camwire is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Camwire; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
    USA


    Title: Simple Xvideo display server
    Based on the libdc1394 example dc1394_multiview.c by Dan Dennedy

    Description:
    This module uses Xvideo to display images.  It only works well when
    only one process is displaying at a time.  When more than one
    process uses it simultaneously, the displayed images are mixed up.

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>
#include <X11/keysym.h>

#include "display-codes.h"
#include "display-xv.h"


/* ok the following constant should be by right included thru in
 * Xvlib.h, obtained from xvinfo */
/* 16 bits/pixel, 1 plane, YUV packed:*/
#ifndef XV_YUY2
#define XV_YUY2 0x32595559L
#endif
/* 12 bits/pixel, 3 planes, YUV planar:*/
#ifndef XV_YV12
#define XV_YV12 0x32315659L
#endif
/* 16 bits/pixel, 1 plane, YUV packed:*/
#ifndef XV_UYVY
#define XV_UYVY 0x59565955L
#endif
/* 12 bits/pixel, 3 planes, YUV planar:*/
#ifndef XV_I420
#define XV_I420 0x30323449L
#endif


/* The display info points to this struct: */
typedef struct display_info
{
    Display *display;
    Window window;
    long xformat;
    GC gc;
    XvPortID base_id;
    void *frame_buffer;
    size_t display_width, display_height;
    size_t device_width, device_height, device_depth;
}
display_info;


/* Prototypes */
static int set_frame_format(Display_info info, const int cam_width,
			    const int cam_height, const int cam_depth);
static inline void iyu12yuy2 (const unsigned char *src,
			      unsigned char *dest,
			      const size_t NumPixels);
static inline void rgb2yuy2 (const unsigned char *RGB,
			     unsigned char *YUV,
			     const size_t NumPixels);
static inline void mono2yuy2(const unsigned char *src,
			     unsigned char *dest,
			     const size_t NumPixels);
static void convert_frames(Display_info info, void *fb,
			   const void *capture_buffer);
static void display_frames(Display_info info, void *fb);
static int QueryXv(Display_info info);


/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
Display_info display_xv_create(const int cam_width,
			       const int cam_height,
			       const Camwire_pixel coding)
{
    int cam_depth;
    Display_info info;
    XGCValues xgcv;
    unsigned long background = 0x010203L;

    info =
	(struct display_info *) calloc(1, sizeof(struct display_info));
    if (info == NULL)  return(NULL);
    
    /* Force the resize later: */
    info->device_width = info->device_height =
	info->device_depth = 0;
    camwire_pixel_depth(coding, &cam_depth);
    if (!set_frame_format(info, cam_width, cam_height, cam_depth))
    {
	return(NULL);
    }

    /* make the window */
    info->display = XOpenDisplay(getenv("DISPLAY"));
    if (info->display == NULL)
    {
	fprintf(stderr, "Could not open display \"%s\"\n",
	        getenv("DISPLAY"));
	display_xv_destroy(info);
	return(NULL);
    }
    
    if (QueryXv(info) < 0)
    {
	display_xv_destroy(info);
	return(NULL);
    }
    
    info->display_width = info->device_width;
    info->display_height = info->device_height;
    
    info->window =
	XCreateSimpleWindow(info->display,
			    DefaultRootWindow(info->display),
			    0, 0,
			    info->display_width,
			    info->display_height,
			    0,
			    WhitePixel(info->display,
				       DefaultScreen(info->display)),
			    background);
    
    XSelectInput(info->display, info->window,
		 StructureNotifyMask | KeyPressMask);
    XMapWindow(info->display, info->window);
    /* connection = ConnectionNumber(info->display); */
    (void) ConnectionNumber(info->display);
    
    info->gc = XCreateGC(info->display, info->window, 0, &xgcv);
    return(info);
}

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
void display_xv_destroy(Display_info info)
{
    if ((void*) info->window !=  0)
    {
	XUnmapWindow(info->display, info->window);
    }
    info->window = 0;
    if (info->display != NULL) 	XFlush(info->display);
    free(info->frame_buffer );
    free(info);
}

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
int display_xv_resize(Display_info info, const int cam_width,
		      const int cam_height)
{
    size_t newwidth, newheight;
    
    newwidth = cam_width*info->display_width/info->device_width;
    newheight = cam_height*info->display_height/info->device_height;
    if (!set_frame_format(info, cam_width, cam_height,
			  info->device_depth))
    {
	return(DISPLAY_FAILURE);
    }
    info->display_width = newwidth;
    info->display_height = newheight;
    XResizeWindow(info->display, info->window,
		  info->display_width,info->display_height);
    return(DISPLAY_SUCCESS);
}

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
int display_xv_coding(Display_info info, const Camwire_pixel coding)
{
    int cam_depth;
    
    camwire_pixel_depth(coding, &cam_depth);
    if (!set_frame_format(info, info->device_width, info->device_height,
			  cam_depth))
    {
	return(DISPLAY_FAILURE);
    }
    return(DISPLAY_SUCCESS);
}

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
int display_xv_frame(Display_info info, const void *capture_buffer)
{
    XEvent xev;
    
    convert_frames(info, info->frame_buffer, capture_buffer);
    display_frames(info, info->frame_buffer);
    /*XFlush(info->display);*/
    
    while(XPending(info->display) > 0)
    {
	XNextEvent(info->display, &xev);
	switch (xev.type)
	{
	    case ConfigureNotify:
		info->display_width = xev.xconfigure.width;
		info->display_height = xev.xconfigure.height;
		display_frames(info, info->frame_buffer);
		break;
	    case KeyPress:
		switch (XKeycodeToKeysym(info->display,
					 xev.xkey.keycode, 0))
		{
		    case XK_q:
		    case XK_Q: 	/* Quit.*/
			/*display_xv_destroy();*/
			return(DISPLAY_QUIT_REQ);
			break;
		    case XK_comma:
		    case XK_less:
			info->display_width = info->display_width/2;
			info->display_height =
			    info->display_height/2;
			XResizeWindow(info->display,
				      info->window,
				      info->display_width,
				      info->display_height);
			display_frames(info, info->frame_buffer);
			break;
		    case XK_period:
		    case XK_greater:
			info->display_width = 2*info->display_width;
			info->display_height =
			    2*info->display_height;
			XResizeWindow(info->display,
				      info->window,
				      info->display_width,
				      info->display_height);
			display_frames(info, info->frame_buffer);
			break;
		}
		break;
	} /* switch*/
    } /* XPending*/
    return(DISPLAY_SUCCESS);
}


/* helper functions */



static int set_frame_format(Display_info info, const int cam_width,
			    const int cam_height, const int cam_depth)
{
/*
    switch (mode) {

	case 0:
	mode = MODE_160x120_YUV444;
	    device_width = 160;
	    device_height = 120;
	    device_depth = 24;
	    xformat = XV_YUY2;
	    break;

	case 2:
	    mode = MODE_640x480_YUV411;
	    device_width = 640;
	    device_height = 480;
	    device_depth = 12;
	    xformat = XV_YUY2;
	    break;
	case 3:
	    mode = MODE_640x480_YUV422;
	    device_width = 640;
	    device_height = 480;
	    device_depth = 16;
	    xformat = XV_YUY2;
	    break;
	case 4:
	    mode = MODE_640x480_RGB;
	    device_width = 640;
	    device_height = 480;
	    device_depth = 24;
	    xformat = XV_YUY2;
	    break;
	case 5: 
	    mode = MODE_640x480_MONO;
	    device_width = 640;
	    device_height = 480;
	    device_depth = 8;
	    xformat = XV_YUY2;
	    break;
	case 1: 
	default:
	    mode = MODE_320x240_YUV422;
	    device_width = 320;
	    device_height = 240;
	    device_depth = 16;
	    xformat = XV_UYVY;
	    break;
    }
*/

    switch(cam_depth)
    {
	case 8:
	case 12:
	case 24:
	    info->xformat = XV_YUY2;
	    break;
	case 16:
	    //xformat = XV_YUY2; 	// Pick one that works.
	    info->xformat = XV_UYVY;
	    break;
	default:
	    fprintf(stderr,
		    "Sorry, display does not support device depth %d\n",
	            cam_depth);
	    return(DISPLAY_FAILURE);
	    break;
    }

    /* (Re-)allocate memory for display buffer (only if size has
       changed): */
    if ((size_t) cam_width*cam_height !=
	info->device_width*info->device_height)
    {
	switch (info->xformat)
	{
	    // 12 bits/pixel:
	    case XV_YV12:
	    case XV_I420:
		if (info->frame_buffer != NULL)
		{
		    free(info->frame_buffer);
		}
		info->frame_buffer =
		    malloc((size_t) cam_width*cam_height*3/2);
		break;
		// 16 bits/pixel:
	    case XV_YUY2:
	    case XV_UYVY:
		if (info->frame_buffer != NULL)
		{
		    free(info->frame_buffer);
		}
		info->frame_buffer =
		    malloc((size_t) cam_width*cam_height*2);
		break;
	    default:
		fprintf(stderr,
			"Internal error: Unknown Xv format %ld\n",
			info->xformat);
		return(DISPLAY_FAILURE);
	}
    }
	
    info->device_width = cam_width;
    info->device_height = cam_height;
    info->device_depth = cam_depth;
    return(DISPLAY_SUCCESS);
}


/* image format conversion functions */

/* Convert 12 bits/pixel YUV411 into 16 bits/pixel YUY2: */
static inline void iyu12yuy2 (const unsigned char *src,
			      unsigned char *dest,
			      const size_t NumPixels)
{
    unsigned int i=0,j=0;
    int y0, y1, y2, y3, u, v;
    while (i < NumPixels*3/2)
    {
	u = src[i++];
	y0 = src[i++];
	y1 = src[i++];
	v = src[i++];
	y2 = src[i++];
	y3 = src[i++];
	
	dest[j++] = y0;
	dest[j++] = u;
	dest[j++] = y1;
	dest[j++] = v;
	
	dest[j++] = y2;
	dest[j++] = u;
	dest[j++] = y3;
	dest[j++] = v;
    }
}


/* macro by Bart Nabbe */
#define RGB2YUV(r, g, b, y, u, v)\
  y = (9798*r + 19235*g + 3736*b)  / 32768;\
  u = (-4784*r - 9437*g + 14221*b)  / 32768 + 128;\
  v = (20218*r - 16941*g - 3277*b) / 32768 + 128;\
  y = y < 0 ? 0 : y;\
  u = u < 0 ? 0 : u;\
  v = v < 0 ? 0 : v;\
  y = y > 255 ? 255 : y;\
  u = u > 255 ? 255 : u;\
  v = v > 255 ? 255 : v

/* Convert 24 bits/pixel RGB into 16 bits/pixel YUY2: */
static inline void rgb2yuy2 (const unsigned char *src,
			     unsigned char *dest,
			     const size_t NumPixels)
{
    unsigned int i = 0, j = 0;
    int y0, y1, u0, u1, v0, v1 ;
    int r, g, b;

    while (i < 3*NumPixels)
    {
	r = src[i++];
	g = src[i++];
	b = src[i++];
	RGB2YUV (r, g, b, y0, u0 , v0);
	r = src[i++];
	g = src[i++];
	b = src[i++];
	RGB2YUV (r, g, b, y1, u1 , v1);
	dest[j++] = y0;
	dest[j++] = (u0 + u1)/2;
	dest[j++] = y1;
	dest[j++] = (v0 + v1)/2;
    }
}

/* Convert 8 bits/pixel MONO into 16 bits/pixel YUY2: */
static inline void mono2yuy2(const unsigned char *src,
			     unsigned char *dest,
			     const size_t NumPixels)
{
    unsigned int i, j;

    j = 0;
    for (i = 0; i < NumPixels; i++)
    {
	dest[j++] = src[i];
	dest[j++] = 128;
    }
}


static void convert_frames(Display_info info, void *fb,
			   const void *capture_buffer)
{
    switch (info->device_depth)
    {
	case 8:
	    mono2yuy2(capture_buffer, fb,
		      info->device_width*info->device_height);
	    break;
	case 12:
	    iyu12yuy2(capture_buffer, fb,
		      info->device_width*info->device_height);
	    break;
	case 16:
	    memcpy(fb, capture_buffer,
		   info->device_width*info->device_height*2);
	    break;
	case 24:
	    rgb2yuy2(capture_buffer, fb,
		     info->device_width*info->device_height);
	    break;
    }
}


static void display_frames(Display_info info, void *fb)
{
    XvImage *xv_image;

    xv_image = XvCreateImage(info->display,
			     info->base_id,
			     (int) info->xformat,
			     (char *) fb,
			     (int) info->device_width,
			     (int) info->device_height);
    XvPutImage(info->display,
	       info->base_id,
	       info->window,
	       info->gc,
	       xv_image,
	       0, 0,
	       info->device_width, info->device_height,
	       0, 0,
	       info->display_width, info->display_height);
}


static int QueryXv(Display_info info)
{
    unsigned int i, num_adaptors;
    int adaptor;
    XvAdaptorInfo *xv_info;
    int j, num_xformats;
    XvImageFormatValues *xformats = 0;
    /*char xv_name[5];*/
    
    XvQueryAdaptors(info->display,
		    DefaultRootWindow(info->display),
		    &num_adaptors,
		    &xv_info);

    adaptor = -1;
    for (i = 0; i < num_adaptors; i++) {
	xformats = XvListImageFormats(info->display,
				      xv_info[i].base_id,
				      &num_xformats);
	for (j = 0; j < num_xformats; j++) {
	    if (xformats[j].id == info->xformat) {
		/*
		xv_name[4] = 0;
		memcpy(xv_name, &xformats[j].id, 4);
		fprintf(stderr, "using Xv format 0x%x %s %s\n",
		        xformats[j].id,
			xv_name,
			(xformats[j].format == XvPacked) ? "packed" :
		                                           "planar");
		*/
		if (adaptor < 0)  adaptor = i;
		break;
	    }
	}
	XFree(xformats);
	if (adaptor >= 0)  break;
    }
    if (adaptor >= 0)  info->base_id = xv_info[adaptor].base_id;

    XvFreeAdaptorInfo(xv_info);

    if (adaptor < 0) {
	fprintf(stderr, "Could not find a suitable Xv adaptor.\n");
    }
    return(adaptor);
}
