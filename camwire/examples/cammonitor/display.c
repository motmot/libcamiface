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


    Title: Simple display server

    Description:
    This module switches the display interface to Xvideo or SDL to
    display images.

***********************************************************************/

#include <stdlib.h>

#include "display-sdl.h"
#include "display-xv.h"
#include "display.h"
#include "display-codes.h"


/* The display handle points to this struct: */
typedef struct display_handle
{
    Display_type type;
    Display_info info;
}
display_handle;


/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
Display_handle display_create(const Display_type type,
			      const int width,
			      const int height,
			      const Camwire_pixel coding)
{
    Display_handle handle;

    handle =
	(struct display_handle *) malloc(sizeof(struct display_handle));
    if (handle == NULL)  return(NULL);
    handle->type = type;
    handle->info = NULL;
    if (type == DISPLAY_SDL)
    {
	handle->info = display_sdl_create(width, height, coding);
    }
    else if (type == DISPLAY_XV)
    {
	handle->info = display_xv_create(width, height, coding);
    }
    if (type != DISPLAY_NONE && handle->info == NULL)
    {
	free(handle);
	return(NULL);
    }
    return(handle);
}

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
void display_destroy(Display_handle handle)
{
    if (handle == NULL)  return;
    if (handle->type == DISPLAY_SDL)
    {
	display_sdl_destroy(handle->info);
    }
    else if (handle->type == DISPLAY_XV)
    {
	display_xv_destroy(handle->info);
    }
    free(handle);
}

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
int display_resize(Display_handle handle, const int width,
		   const int height)
{
    int display_result;
    
    if (handle == NULL)  return(DISPLAY_FAILURE);
    display_result = DISPLAY_FAILURE;
    if (handle->type == DISPLAY_SDL)
    {
	display_result =
	    display_sdl_resize(handle->info, width, height);
    }
    else if (handle->type == DISPLAY_XV)
    {
	display_result =
	    display_xv_resize(handle->info, width, height);
    }
    return(display_result);
}

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
int display_coding(Display_handle handle, const Camwire_pixel coding)
{
    int display_result;
    
    if (handle == NULL)  return(DISPLAY_FAILURE);
    display_result = DISPLAY_FAILURE;
    if (handle->type == DISPLAY_SDL)
    {
	display_result =
	    display_sdl_coding(handle->info, coding);
    }
    else if (handle->type == DISPLAY_XV)
    {
	display_result =
	    display_xv_coding(handle->info, coding);
    }
    return(display_result);
}

/*
  ----------------------------------------------------------------------
  See display.h for documentation on this function.
*/
int display_frame(Display_handle handle, const void *buffer)
{
    int display_result;
    
    if (handle == NULL)  return(DISPLAY_FAILURE);
    display_result = DISPLAY_FAILURE;
    if (handle->type == DISPLAY_SDL)
    {
	display_result = display_sdl_frame(handle->info, buffer);
    }
    else if (handle->type == DISPLAY_XV)
    {
	display_result = display_xv_frame(handle->info, buffer);
    }
    return(display_result);
}
