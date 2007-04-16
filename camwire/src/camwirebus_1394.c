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

   
    Title: Camwire bus module

    Description:
    This Camwire bus module is about finding all visible cameras and
    providing handles to them.  The handles should be all a user need
    know about for complete access to all camera functions.  Functions
    to control individual cameras through their handles are defined in
    the Camwire main module.

***********************************************************************/


#include <stdlib.h>

#include "libdc1394/dc1394_control.h"
#include "camwire/camwirebus.h"

#include "camwirebus_internal_1394.h"


/* Below is the type definition of the camwire handle structure.  The
   camwire handle itself is typedefed in camwirebus.h as a pointer to
   this structure.

   The user need not know this structure nor should anyone use its
   members directly.  The camwire_bus_get_... functions defined in this
   module provide efficient maintainable access.

   The port and node members are specific to IEEE1394 and are needed by
   many libdc1394 functions.  They are handles to ports (IEEE 1394 OHCI
   cards or chips) and nodes (IEEE 1394 devices connected to a port).

   The userdata member provides a way for a user (in our case this is
   the Camwire main module) to associate private data with the camwire
   handle, such as status information.  A pointer to the user data
   structure can be stored in userdata by the camwire_bus_set_userdata()
   function, without this module needing to know the user data structure
   definition.  */
typedef struct camwire_bus_handle
{
    Port_handle port;
    Node_handle node;
    User_handle userdata;
}
camwire_bus_handle_struct;
/* Port_handle, Node_handle, & User_handle are defined in
   camwirebus_internal_1394.h.*/


/* Local status: */
static int num_cams = 0;
static Camwire_handle *handle_array = NULL;


/*Local prototypes: */
static int count_the_cameras(const int num_ports);
static int count_the_ports(void);
static int count_the_nodes(const raw1394handle_t p_handle);


/*
  ----------------------------------------------------------------------
  See camwirebus.h for documentation on this function.
*/
/* This can be implemented as the constructor of a Camwire_bus class. */
/* Camwire_handle is defined in camwirebus.h.*/

Camwire_handle * camwire_bus_create(int *num_handles)
{
    int p, n;
    int camcount, num_ports, num_nodes;
    int cycle_master;
    raw1394handle_t p_handle = NULL;
    nodeid_t *nodelist = NULL;

    /* Check whether the bus has not already been created: */
    if (handle_array != NULL && num_cams >= 0)
    {
	*num_handles = num_cams;
	return(handle_array);
    }

    /* Set default (error) values: */
    *num_handles = -1;
    num_cams = 0;

    /* Count the number of visible cameras and IEEE 1394 ports (cards or
       chips): */
    num_ports = count_the_ports();
    if (num_ports < 0)  return(NULL); 	/* Error accessing a port.*/

    camcount = count_the_cameras(num_ports);
    if (camcount < 0)  return(NULL); 	/* Error accessing a port.*/
    if (num_ports == 0)  return(NULL); 	/* Found no ports.*/

    if (camcount == 0) 	/* Nothing wrong, just found no cameras.*/
    {
	*num_handles = 0;
	return(NULL);
    }

    /* Allocate memory for camwire handles: */
    handle_array = (Camwire_handle *) calloc((size_t) camcount,
					 sizeof(Camwire_handle));
    if (handle_array == NULL)  return(NULL); 	/* Allocation failure.*/

    /* Fill in the camera addressing map: */
    for (p = 0; p < num_ports; p++)
    {
	p_handle = dc1394_create_handle(p);
	/*
	if (p_handle == NULL)
	// Internal error: should have been caught in
	// count_the_cameras() above.
	{
	    camwire_bus_destroy();
	    return(NULL);
	}
	*/

        /* Find out who the cycle master is:*/
	cycle_master = raw1394_get_nodecount(p_handle) - 1;
	
	nodelist = dc1394_get_camera_nodes(p_handle, &num_nodes, 0);
	/*
	if ((num_nodes < 0) || ((num_nodes > 0) && (nodelist == NULL)))
	// Internal error: should have been caught in
	// count_the_cameras() above.
	{
	    dc1394_destroy_handle(p_handle);
	    camwire_bus_destroy();
	    return(NULL);
	}
	*/
	for (n = 0; n < num_nodes; n++)
	{
	    if (nodelist[n] == cycle_master)
	    { 	/* Cameras are not cycle master capable. */
		if (n == 0) 	dc1394_destroy_handle(p_handle);
		camwire_bus_destroy();
		return(NULL);
	    }
	    handle_array[num_cams] =
		(struct camwire_bus_handle *)
		malloc(sizeof(struct camwire_bus_handle));
	    if (handle_array[num_cams] == NULL)
	    { 	/* Allocation failure.*/
		if (n == 0) 	dc1394_destroy_handle(p_handle);
		camwire_bus_destroy();
		return(NULL);
	    }
	    handle_array[num_cams]->port = p_handle;
	    handle_array[num_cams]->node = nodelist[n];
	    handle_array[num_cams]->userdata = NULL;
	    num_cams++;
	}
	free(nodelist); 	/* Because libdc1394 doesn't.*/
    }

    /* Sanity check: */
    /*
      if (num_cams != camcount) 	// Internal error.
    {
	camwire_bus_destroy();
	return(NULL);
    }
    */

    /* Returns: */
    *num_handles = num_cams;
    return(handle_array);
}

/*
  ----------------------------------------------------------------------
  See camwirebus.h for documentation on this function.
*/
/* This can be implemented as the destructor of a Camwire_bus class. */

void camwire_bus_destroy(void)
{
    int h;
    raw1394handle_t last_port = NULL; 	/* Initialized to suppress*/
                                        /*  compiler warning.*/

    for (h = 0; h < num_cams; h++)
    {
	/* Placeholder here for destroying node handles: not needed for
           IEEE 1394 nodes which just evaporate. */
	
	/* Destroy port handles, skipping duplicates: */
	if ((h == 0) || (handle_array[h]->port != last_port)) 
	{
	    dc1394_destroy_handle(handle_array[h]->port);
	    last_port = handle_array[h]->port;
	}

	free(handle_array[h]);
    }
    free(handle_array);
    handle_array = NULL;
    num_cams = 0;
}

/*
  ----------------------------------------------------------------------
  See camwirebus.h for documentation on this function.
*/
void camwire_bus_reset(void)
{
    int num_ports, num_nodes;
    int p, h;
    raw1394handle_t p_handle, last_port = NULL;

    if (handle_array == NULL && num_cams == 0)
    { 	/* No existing bus. */
	num_ports = count_the_ports();
	for (p = 0; p < num_ports; p++)
	{
	    p_handle = dc1394_create_handle(p);
	    if (p_handle)
	    {
		num_nodes = count_the_nodes(p_handle);
		if (num_nodes > 0)  raw1394_reset_bus(p_handle);
		dc1394_destroy_handle(p_handle);
	    }
	}
    }
    else if (handle_array)
    { 	/* A bus exists. */
	for (h = 0; h < num_cams; h++)
	{
	    if ((h == 0) || (handle_array[h]->port != last_port)) 
	    {
		p_handle = handle_array[h]->port;
		if (p_handle)
		{
		    num_nodes = count_the_nodes(p_handle);
		    if (num_nodes > 0)  raw1394_reset_bus(p_handle);
		    dc1394_destroy_handle(p_handle);
		}
		last_port = p_handle;
	    }
	    free(handle_array[h]);
	}
	free(handle_array);
	handle_array = NULL;
    }
    num_cams = 0;
}

/*
  ----------------------------------------------------------------------
  See camwirebus_internal_1394.h for documentation on this function.
*/
inline Port_handle camwire_bus_get_port(const Camwire_handle c_handle)
{
    return(c_handle->port);
}

/*
  ----------------------------------------------------------------------
  See camwirebus_internal_1394.h for documentation on this function.
*/
inline Node_handle camwire_bus_get_node(const Camwire_handle c_handle)
{
    return(c_handle->node);
}

/*
  ----------------------------------------------------------------------
  See camwirebus_internal_1394.h for documentation on this function.
*/
inline User_handle camwire_bus_get_userdata(
    const Camwire_handle c_handle)
{
    return(c_handle->userdata);
}

/*
  ----------------------------------------------------------------------
  See camwirebus_internal_1394.h for documentation on this function.
*/
int camwire_bus_set_userdata(const Camwire_handle c_handle,
			User_handle user_data)
{
    if (c_handle == NULL)  return(0);
    if (user_data != NULL && c_handle->userdata != NULL)  return(0);
    c_handle->userdata = user_data;
    return(1);
}


/*
  ----------------------------------------------------------------------
  Returns the total number of IEEE 1394 cameras on all available IEEE
  1394 ports, or -1 on error.  The number of visible ports is provided
  in n_ports, which may be the null pointer if not needed.
*/
static int count_the_cameras(const int num_ports)
{
    int camcount, p, num_nodes;
    raw1394handle_t p_handle;
    
    camcount = 0;
    for (p = 0; p < num_ports; p++)
    {
	p_handle = dc1394_create_handle(p);
	if (p_handle == NULL)
	{
	    camcount = -1;
	    break;
	}
	num_nodes = count_the_nodes(p_handle);
	if (num_nodes < 0)
	{
	    dc1394_destroy_handle(p_handle);
	    camcount = -1;
	    break;
	}
	camcount += num_nodes;
	dc1394_destroy_handle(p_handle);
    }
    return(camcount);
}


/*
  ----------------------------------------------------------------------
  Returns the number of available IEEE 1394 ports (cards or on-board
  chips), or -1 on error.
*/
static int count_the_ports(void)
{
    int num_ports;
    raw1394handle_t rawhandle;

    rawhandle = raw1394_new_handle();
    if (rawhandle == NULL)  return(-1);
    num_ports = raw1394_get_port_info(rawhandle, NULL, 0);
    raw1394_destroy_handle(rawhandle);
    return(num_ports);
}


/*
  ----------------------------------------------------------------------
  Returns the number of available nodes that have been identified as
  IEEE 1394 cameras on the given port, or -1 on error.
*/
static int count_the_nodes(const raw1394handle_t p_handle)
{
    int num_nodes;
    nodeid_t *nodeid;

    /* Use dc1394_get_camera_nodes() because raw1394_get_nodecount()
       does not check whether nodes are cameras: */
    nodeid = dc1394_get_camera_nodes(p_handle, &num_nodes, 0);
    free(nodeid); 	/* Because libdc1394 doesn't.*/
    return(num_nodes);
}
