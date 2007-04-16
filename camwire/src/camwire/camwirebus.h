#ifndef CAMWIREBUS_H
#define CAMWIREBUS_H
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


    Title: Header for camwirebus_....c

    Description: This Camwirebus module is about finding all visible
    cameras and providing handles to them.  The handles are all a user
    should need for implementation-independent access to all camera
    functions.  Functions to control individual cameras through their
    handles are defined in the main Camwire module.

***********************************************************************/

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct camwire_bus_handle *Camwire_handle;
/* This is the Camwire handle type. */

Camwire_handle * camwire_bus_create(int *num_handles);
/* Returns a pointer to an array of all available Camwire handles.  The
   cameras are not initialized.  The number of handles is returned in
   num_handles.  If no cameras are found, returns a null pointer and 0
   for num_handles.  On error, returns a null pointer and -1 for
   num_handles.  The function camwire_bus_destroy() must be called when
   done to free the allocated memory.  If camwire_bus_create() is called
   again (without an intervening camwire_bus_destroy()), it returns the
   previous array of Camwire handles and previous number of handles
   without doing anything else. */

void camwire_bus_destroy(void);
/* Frees memory and handles allocated by camwire_bus_create().
   Invalidates all Camwire handles. */

void camwire_bus_reset(void);
/* Requests a reset of each bus which has cameras attached.  All Camwire
   handles are invalidated.  Any existing handles should first be
   destroyed with camwire_bus_destroy().  The time needed for a bus
   reset is hardware-dependent and the calling program may have to wait
   a while before trying to access the bus again. */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
}
#endif

#endif
