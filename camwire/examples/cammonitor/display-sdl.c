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


    Title: Simple SDL display server

    Description:
    This module uses SDL to display images.

***********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL/SDL.h"

#include "display-codes.h"
#include "display-sdl.h"


#define MAX_IMAGE_FILE_HEADER	50


/* The display info points to this struct: */
typedef struct display_info
{
    SDL_Surface *screen;
    SDL_Surface *frame;
}
display_info;


/* Local functions: */
static int pixel_coding_supported(const Camwire_pixel coding);
static SDL_Surface * create_display_surface(const int width,
					  const int height,
					  const int depth);
static SDL_Surface * create_frame_surface(const int width,
					  const int height,
					  const int depth);
static void set_linear_palette(SDL_Surface *surface);

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
Display_info display_sdl_create(const int width,
				const int height,
				const Camwire_pixel coding)
{
    Display_info info;
    int depth;

    /* First check supported pixel codings and get depth:*/
    if (!pixel_coding_supported(coding))
    {
	fprintf(stderr, "Requested pixel coding is not supported in "
		"display_sdl_create()\n");
	return(NULL);
    }
    camwire_pixel_depth(coding, &depth);
    
    /* Create our handle: */
    info =
	(struct display_info *) malloc(sizeof(struct display_info));
    if (info == NULL)  return(NULL);

    /* Initialize SDL: */
    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
	fprintf(stderr,
		"SDL_Init() failed in display_sdl_create():\n"
		"%s\n", SDL_GetError());
	free(info);
	return(NULL);
    }

    /* Set up the screen display and video mode:*/
    info->screen = create_display_surface(width, height, depth);
    if (info->screen == NULL)
    {
	SDL_Quit();
	free(info);
	return(NULL);
    }

    /* Create an empty frame surface.  display_sdl_frame() will assign
       the frame buffer pointer to it: */
    info->frame = create_frame_surface(width, height, depth);
    if(info->frame == NULL)
    {
	SDL_Quit(); 	/* Also frees info->screen.*/
	free(info);
	return(NULL);
    }

    return(info);
}

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
void display_sdl_destroy(Display_info info)
{
    SDL_FreeSurface(info->frame);
    SDL_Quit(); 	/* Also frees info->screen.*/
    free(info);
}

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
int display_sdl_resize(Display_info info, const int width,
		       const int height)
{
    int old_width, old_height, old_depth;
    int new_mode;

    /* Remember some current parameters:*/
    old_width = info->frame->w;
    old_height = info->frame->h;
    old_depth = info->frame->format->BitsPerPixel;

    /* New video mode: */
    new_mode = (width != old_width || height != old_height);
    if (new_mode)
    {
	info->screen = create_display_surface(width, height, old_depth);
	if (info->screen == NULL)
	{
	    info->screen = create_display_surface(old_width, old_height,
						  old_depth);
	    return(DISPLAY_FAILURE);
	}
    }

    /* New frame surface: */
    if (new_mode)
    {
	SDL_FreeSurface(info->frame);
	info->frame = create_frame_surface(width, height, old_depth);
	if (info->frame == NULL)
	{  /* Bother.  Undo as best we can.*/
	    info->screen = create_display_surface(old_width, old_height,
						  old_depth);
	    info->frame =
		create_frame_surface(old_width, old_height, old_depth);
	    return(DISPLAY_FAILURE);
	}
    }

    return(DISPLAY_SUCCESS);
}

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
int display_sdl_coding(Display_info info, const Camwire_pixel coding)
{
    int old_width, old_height, old_depth;
    int depth;
    Uint32 flags;

    if (!pixel_coding_supported(coding))  return(DISPLAY_FAILURE);

    /* Remember some current parameters:*/
    old_width = info->frame->w;
    old_height = info->frame->h;
    old_depth = info->frame->format->BitsPerPixel;
    flags = info->screen->flags;

    /* New frame surface: */
    camwire_pixel_depth(coding, &depth);
    if (depth != old_depth)
    {
	SDL_FreeSurface(info->frame);
	info->frame =
	    create_frame_surface(old_width, old_height, depth);
	if (info->frame == NULL)
	{  /* Undo.*/
	    info->frame =
		create_frame_surface(old_width, old_height, old_depth);
	    return(DISPLAY_FAILURE);
	}
    }

    return(DISPLAY_SUCCESS);
}

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
int display_sdl_frame(Display_info info, const void *buffer)
{
    SDL_Event event;

    /* Check for user quit event: */
    while (SDL_PollEvent(&event))
    {
	if (event.type == SDL_QUIT)
	{  /* Display window has been closed.*/
	    return(DISPLAY_QUIT_REQ);
	}
	else if (event.type == SDL_KEYDOWN)
	{
	    if (event.key.keysym.sym == SDLK_q)
	    {  /* User typed `Q'.*/
		return(DISPLAY_QUIT_REQ);
	    }
	}
    }

    /* Display it:*/
    info->frame->pixels = (void *) buffer;
    SDL_BlitSurface(info->frame, NULL, info->screen, NULL);
    SDL_Flip(info->screen);
    /* Note: X11 does not support hardware acceleration.  All SDL_Flip()
       does is to update the screen. */

    return(DISPLAY_SUCCESS);
}

/*
  ----------------------------------------------------------------------
  Return 1 if pixel depth is supported, otherwise 0.
*/
static int pixel_coding_supported(const Camwire_pixel coding)
{
    int depth;
    
    camwire_pixel_depth(coding, &depth);
    if (depth == 8 || depth == 24)  return(1);
    else			    return(0);
}

/*
  ----------------------------------------------------------------------
  Create the SDL screen display surface.  Returns NULL on error.
*/
static SDL_Surface * create_display_surface(const int width,
					    const int height,
					    const int depth)
{
    Uint32 video_flags;
    SDL_Surface *screen;

    /* Set the video mode: */
    /* Note: X11 does not directly support video hardware acceleration,
       but may do so using OpenGL.
    video_flags = SDL_HWSURFACE | SDL_HWACCEL | SDL_DOUBLEBUF;
    */
    video_flags = SDL_SWSURFACE;
    screen = SDL_SetVideoMode(width, height, depth, video_flags);
    if (screen == NULL)
    {
	fprintf(stderr, "SDL_SetVideoMode() failed in "
		"create_display_surface():\n%s\n", SDL_GetError());
	return(NULL);
    }
    /*
    if ((screen->flags & SDL_HWSURFACE) == 0)
    {
	fprintf(stderr, "FYI: Could not use video hardware directly. SDL "
		"screen surface is in user memory.\n");
    }
    */

    /* Create a palette if necessary: */
    if (depth == 8)  set_linear_palette(screen);

    return(screen);
}

/*
  ----------------------------------------------------------------------
  Create an empty SDL frame surface.  Returns NULL on error.
*/
static SDL_Surface * create_frame_surface(const int width,
					  const int height,
					  const int depth)
{
    int pitch;
    Uint32 rmask, gmask, bmask, amask;
    SDL_Surface *frame;
    
    /* Set up the frame surface parameters: */
    pitch = width*depth/8;
    if (depth == 8)
    {
	rmask = 0;
	gmask = 0;
	bmask = 0;
	amask = 0;
    }
    else 	//if (depth == 24)
    {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0;
#endif
    }
    
    /* Create the frame surface: */
    frame = SDL_CreateRGBSurfaceFrom(NULL,
				       width, height, depth, pitch,
				       rmask, gmask, bmask, amask);
    if (frame == NULL)
    {
	fprintf(stderr, "SDL_CreateRGBSurfaceFrom() failed in "
		"create_frame_surface():\n%s\n", SDL_GetError());
	return(NULL);
    }

    /* Create a palette if necessary: */
    if (depth == 8)  set_linear_palette(frame);

    return(frame);
}

/*
  ----------------------------------------------------------------------
  Fill in the 8-bit colour map:
*/
static void set_linear_palette(SDL_Surface *surface)
{
    SDL_Color map[256];
    int i;

    for (i = 0; i < 256; i++)  map[i].r = map[i].g = map[i].b = i;
    SDL_SetColors(surface, map, 0, 256);
}
