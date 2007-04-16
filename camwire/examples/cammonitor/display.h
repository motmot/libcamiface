#ifndef DISPLAY_H
#define DISPLAY_H
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


    Title: Simple display interface server

    Description:
    This module uses a display interface to show images on the screen.

***********************************************************************/

#include "camwire/camwire.h"  /* For Camwire_pixel type.*/

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct display_handle *Display_handle;

typedef enum {DISPLAY_NONE, DISPLAY_SDL, DISPLAY_XV} Display_type;

Display_handle display_create(const Display_type type,
			      const int width,
			      const int height,
			      const Camwire_pixel coding);
/* Sets up a display interface window of the given type and size (in
   pixels) and pixel colour coding.  Returns a handle to the display
   interface on success or NULL on failure.  The function
   display_destroy() must be called when done to free the allocated
   memory. */

void display_destroy(Display_handle handle);
/* Frees the memory allocated by display_create(). */

int display_resize(Display_handle handle, const int width,
		   const int height);
/* Notifies the display interface of changes in frame size (in pixels).
   The resulting display window may change in size as a result.  Returns
   DISPLAY_SUCCESS on success or DISPLAY_FAILURE on failure. */

int display_coding(Display_handle handle, const Camwire_pixel coding);
/* Requests a new pixel colour coding in the display interface.  Returns
   DISPLAY_SUCCESS on success or DISPLAY_FAILURE on failure. */

int display_frame(Display_handle handle, const void *buffer);
/* Uses a display interface to display the given frame.  Returns
   DISPLAY_SUCCESS on success, DISPLAY_FAILURE on failure, or
   DISPLAY_QUIT_REQ when the user has activated a quit request event in
   the display interface. */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
}
#endif

#endif
