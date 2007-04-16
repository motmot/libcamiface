#ifndef CAMWIREBUS_INTERNAL_1394_H
#define CAMWIREBUS_INTERNAL_1394_H
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


    Title: Intermodule interface header for camwirebus.c

    Description:
    Header file to provide a private interface to the Camwire bus
    module, for exclusive use by Camwire.  This header should not be
    included in any camera user code.

    This interface could have been avoided if the interface between the
    Camwire bus and main modules was completely independent of IEEE
    1394.  For example, the port and node handles could be replaced with
    a more generic camera address.  That would be somewhat contrived
    because the Camwire main module would then be forced to make all of
    its many calls to libdc1394 through the Camwire bus module, when
    both modules make extensive use of the same IEEE 1394-specific
    libraries anyway.  libdc1394 makes so little distinction between its
    tightly interwoven bus operations and camera operations that it is
    not worth separating them at this level.

    The real separation between the Camwire bus and main modules is that
    the Camwire bus module deals with identifying and providing access
    to all the buses and their attached cameras, while the Camwire main
    module deals with controlling one camera through its handle.  Their
    public interfaces (through camerabus.h and camera.h) are
    implementation-independent in that they make no mention of IEEE
    1394.

***********************************************************************/


#include <libraw1394/raw1394.h>  /* raw1394handle_t & nodeid_t.*/

#include "camwire/camwirebus.h"  /* Camwire_handle.*/

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif


typedef raw1394handle_t 		 Port_handle;
typedef nodeid_t 			 Node_handle;
typedef struct camwire_user_data 	*User_handle;


inline Port_handle camwire_bus_get_port(const Camwire_handle c_handle);
/* Returns the raw1394handle_t port handle for the given camwire handle.
   Needed by many dc1394 functions in Camwire. */

inline Node_handle camwire_bus_get_node(const Camwire_handle c_handle);
/* Returns the nodeid_t node handle for the given camwire handle.
   Needed by many dc1394 functions in Camwire. */

inline User_handle camwire_bus_get_userdata(
    const Camwire_handle c_handle);
/* Returns a pointer to the user data structure for the given camwire
   handle.  Needed for internal status maintenance in Camwire. */

int camwire_bus_set_userdata(const Camwire_handle c_handle,
			User_handle user_data);
/* Registers a pointer to a user data structure for the given camwire
   handle.  Returns 1 on success and 0 if the handle does not exist or
   if the pointer is already in use.  Needed for internal status
   maintenance in Camwire. */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
}
#endif

#endif
