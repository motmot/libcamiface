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


    Title: Test Camwire bus module

    Description:
    Exercises as much of the Camwire bus module as possible.  Use
    "echo $?" in bash to show which test failed.

***********************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "libdc1394/dc1394_control.h"
#include "camwire/camwirebus.h"

#include "camwirebus_internal_1394.h"


/* Comment out the next line if no camera is available (but keep in mind
   that much less testing is then done): */
#define HAVE_CAMERA

/* Macro to make error exit code more concise: */
static int testnumber = 0;
#define TESTIF(t,e)	{testnumber++; if(t) {{e;} return(testnumber);}}

int main(void)
{
    Camwire_handle *handle_array, *handle_array2;
    int num_cameras, num_cameras2;
    int sameport;
    int c;
    Camwire_handle c_handle;
    Camwire_handle last_c_handle = 0;
    Port_handle p_handle;
    Port_handle last_p_handle = 0;
    Node_handle n_handle;
    Node_handle last_n_handle = 0;
    User_handle userdata, userdata2;
    int retval;
    dc1394bool_t is_camera;
    struct timespec small_pause = {0, 1000000};

    handle_array = camwire_bus_create(&num_cameras);
#ifdef HAVE_CAMERA
    TESTIF(num_cameras == 0, camwire_bus_destroy());
#endif
    TESTIF(num_cameras < 0, camwire_bus_destroy());
    TESTIF(handle_array == 0, camwire_bus_destroy());

    for (c = 0; c < num_cameras; c++)
    {
	/* Confirm camwire handles are not all identical: */
	c_handle = handle_array[c];
	if (c > 0)
	{
	    TESTIF(c_handle == last_c_handle, camwire_bus_destroy());
	}
	last_c_handle = c_handle;

	/* Confirm non-null port handle: */
	p_handle = camwire_bus_get_port(c_handle);
	TESTIF(p_handle == 0, camwire_bus_destroy());

	/* Confirm node handles are not all identical: */
	if (c > 0)  sameport = (last_p_handle == p_handle);
	else        sameport = 0;
	last_p_handle = p_handle;
	n_handle = camwire_bus_get_node(c_handle);
	if (sameport)
	{
	    TESTIF(n_handle == last_n_handle, camwire_bus_destroy());
	}
	last_n_handle = n_handle;

	/* Confirm that handle is an IEEE 1394 camera: */
	retval = dc1394_is_camera(p_handle, n_handle, &is_camera);
	TESTIF(retval != DC1394_SUCCESS, camwire_bus_destroy());
	TESTIF(is_camera != DC1394_TRUE, camwire_bus_destroy());

	/* Confirm user data registration: */
	userdata2 = camwire_bus_get_userdata(c_handle);
	TESTIF(userdata2 != 0, camwire_bus_destroy());
	userdata = (User_handle) malloc(sizeof(int));
	TESTIF(userdata == 0, camwire_bus_destroy());
	retval = camwire_bus_set_userdata(c_handle, userdata);
	TESTIF(retval != 1, free(userdata); camwire_bus_destroy());
	userdata2 = camwire_bus_get_userdata(c_handle);
	TESTIF(userdata2 != userdata,
	       free(userdata); camwire_bus_destroy());
	retval = camwire_bus_set_userdata(c_handle, userdata);
	TESTIF(retval != 0, free(userdata); camwire_bus_destroy());
	free(userdata);
	retval = camwire_bus_set_userdata(c_handle, 0);
	TESTIF(retval != 1, camwire_bus_destroy());
	userdata2 = camwire_bus_get_userdata(c_handle);
	TESTIF(userdata2 != 0, camwire_bus_destroy());
    }

    camwire_bus_destroy();

    /* Confirm that we can start again: */
    handle_array2 = camwire_bus_create(&num_cameras2);
#ifdef HAVE_CAMERA
    TESTIF(num_cameras2 == 0, camwire_bus_destroy());
#endif
    TESTIF(num_cameras2 < 0, camwire_bus_destroy());
    TESTIF(handle_array2 == 0, camwire_bus_destroy());
    TESTIF(num_cameras2 != num_cameras, camwire_bus_destroy());

    /* Confirm that no re-creating is done: */
    handle_array = camwire_bus_create(&num_cameras);
#ifdef HAVE_CAMERA
    TESTIF(num_cameras == 0, camwire_bus_destroy());
#endif
    TESTIF(num_cameras < 0, camwire_bus_destroy());
    TESTIF(handle_array == 0, camwire_bus_destroy());
    TESTIF(handle_array2 != handle_array, camwire_bus_destroy());
    TESTIF(num_cameras2 != num_cameras, camwire_bus_destroy());

    /* Try a reset while handles are assigned: */
    camwire_bus_reset();
    nanosleep(&small_pause, 0);

    /* Confirm that resetting twice is harmless (no crash): */
    camwire_bus_reset();
    nanosleep(&small_pause, 0);

    /* Confirm that destroying twice is harmless (no crash): */
    camwire_bus_destroy();
    camwire_bus_destroy();
    return(0);
}
