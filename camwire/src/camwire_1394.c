/******************************************************************************

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


    Title: Camwire main module

    Description:
    This module is about using a single named camera through its
    handle. The handle should be all a user need know about for complete
    access to all camera functions.  Finding cameras and assigning
    handles to them is done in the Camwire bus module.

    This implementation is for IEEE 1394 digital cameras complying with
    the 1394 Trade Association IIDC (DCAM) specification.  I have tested
    it on Basler cameras which comply with DCAM version 1.20 and a Pyro
    Webcam which complies with version 1.04.

******************************************************************************/


#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>

#include "libdc1394/dc1394_control.h"
#include "camwire/camwirebus.h"
#include "camwire/camwire.h"
#include "camwirebus_internal_1394.h"
#include "camwiremacros_1394.h"


/* A bug in dma.c of the Linux ieee1394 module causes a segfault when
   video1394 tries to allocate a large DMA buffer.  This has been fixed
   in later versions (after mid-2004), in which case the following
   #define and the code below where it is used should be removed: */
#define IEEE1394_KALLOC_LIMIT	26.5e6

/* Other constants: */
#define FILE_ERROR_MAX_CHARS	500
#define ENVIRONMENT_VAR_CONF		"CAMWIRE_CONF"
#define CONFFILE_PATH_MAX_CHARS		200
#define CONFFILE_EXTENSION		".conf"
#define CONFFILE_EXTENSION_MAX_CHARS	10
#define CONFFILE_NAME_MAX_CHARS	(CONFFILE_PATH_MAX_CHARS + \
                                         1 + CAMWIRE_ID_MAX_CHARS + \
                                         CONFFILE_EXTENSION_MAX_CHARS)


/* Internal camera state parameters.  If the current_set->shadow flag is
   set then, wherever possible, settings are read from the current_set
   member, else they are read directly from the camera hardware.  Each
   camwire handle structure contains a userdata pointer which is set to
   this structure: */
typedef struct camwire_user_data
{
    int camera_connected;   /* Flag.*/
    long int frame_number;  /* 8 months @ 100fps before 31-bit overflow.*/
    dc1394_cameracapture *control;
    Camwire_conf *config_cache;
    Camwire_state *current_set;
}
camwire_user_data_struct;


/* Local prototypes:*/
static int create(const Camwire_handle c_handle, const Camwire_state *set);
static int connect(const Camwire_handle c_handle, Camwire_conf *cfg,
		   const Camwire_state *set);
static int set_non_dma_registers(const Camwire_handle c_handle,
				 const Camwire_state *set);
static void disconnect(const Camwire_handle c_handle);
static int reconnect(const Camwire_handle c_handle, Camwire_conf *cfg,
		     const Camwire_state *set);
static void free_internals(const Camwire_handle c_handle);
inline static
dc1394_cameracapture *get_capturecontrol(const Camwire_handle c_handle);
inline static Camwire_state *get_shadow_state(const Camwire_handle c_handle);
static unsigned int get_1394_format(const Camwire_handle c_handle);
static unsigned int get_1394_mode(const Camwire_handle c_handle);
static int get_current_settings(const Camwire_handle c_handle,
				Camwire_state *set);
static int sleep_frametime(const Camwire_handle c_handle,
			   const double multiple);
static volatile void blink(void);
static Camwire_pixel convert_colorid2pixelcoding(const int color_id);
static Camwire_pixel convert_mode2pixelcoding(const unsigned int mode);
static double convert_numpackets2framerate(const Camwire_handle c_handle,
					   const int num_packets);
static double convert_index2framerate(const int frame_rate_index);
static int convert_framerate2index(const double frame_rate);
static int get_numpackets(const Camwire_handle c_handle, int *num_p);
static int convert_packetsize2numpackets(const Camwire_handle c_handle,
					 const int packet_size,
					 const int width,
					 const int height,
					 const Camwire_pixel coding);
static int convert_numpackets2packetsize(const Camwire_handle c_handle,
					 const int num_packets,
					 const int width,
					 const int height,
					 const Camwire_pixel coding);
static int convert_framerate2numpackets(const Camwire_handle c_handle,
					const double frame_rate);
static
unsigned int convert_pixelcoding2colorid(const Camwire_pixel coding,
					 const quadlet_t bit_field);
static int generate_default_settings(const Camwire_handle c_handle,
				     Camwire_state *set);
static int read_conf_file(FILE *conffile, Camwire_conf *cfg);
static FILE * find_conf_file(const Camwire_id *id);
static FILE * open_named_conf_file(const char *path, const char *filename);
static int generate_default_config(const Camwire_handle c_handle,
				   Camwire_conf *cfg);

/*
static void debug_print_status(const User_handle is);
*/


/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
/* This can be implemented as a constructor of a Camwire class. */

int camwire_create(const Camwire_handle c_handle)
{
    Camwire_state settings;
    
    /* Get factory default start-up settings: */
    ERROR_IF_NULL(c_handle);
    if (camwire_get_state(c_handle, &settings) != CAMWIRE_SUCCESS)
    {
	DPRINTF("camwire_get_state() failed in camwire_create().");
	return(CAMWIRE_FAILURE);
    }
    /* CAMWIRE_SUCCESS & CAMWIRE_FAILURE are defined in camwire.h.*/

    return(create(c_handle, &settings));
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
/* This can be implemented as a constructor of a Camwire class. */

int camwire_create_from_struct(const Camwire_handle c_handle,
			       const Camwire_state *set)
{
    ERROR_IF_NULL(c_handle);
    return(create(c_handle, set));
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
/* This can be implemented as the destructor of a Camwire class. */

void camwire_destroy(const Camwire_handle c_handle)
{
    if (c_handle != NULL)
    {
	camwire_set_run_stop(c_handle, 0);
	sleep_frametime(c_handle, 1.5);
	dc1394_init_camera(camwire_bus_get_port(c_handle),
			   camwire_bus_get_node(c_handle));
	disconnect(c_handle);
	free_internals(c_handle);
    }
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
/* Camwire_state is defined in camwire.h.*/

int camwire_get_state(const Camwire_handle c_handle, Camwire_state *set)
{
    User_handle internal_status;

    ERROR_IF_NULL(c_handle);
    internal_status = camwire_bus_get_userdata(c_handle);
    if (internal_status == NULL  || !internal_status->camera_connected)
    {  /* Camera does not exit.*/
	ERROR_IF_CAMWIRE_FAIL(
	    generate_default_settings(c_handle, set));
    }
    else
    {  /* Camera exists.*/
	ERROR_IF_CAMWIRE_FAIL(
	    get_current_settings(c_handle, set));
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_set_state(const Camwire_handle c_handle, const Camwire_state *set)
{
    Camwire_state current_settings;
    Camwire_conf config;

    ERROR_IF_NULL(c_handle);
    ERROR_IF_CAMWIRE_FAIL(
	get_current_settings(c_handle, &current_settings));
    
    if (set->num_frame_buffers != current_settings.num_frame_buffers ||
	set->width != current_settings.width ||
	set->height != current_settings.height ||
	set->coding != current_settings.coding ||
	set->frame_rate != current_settings.frame_rate)
    {
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_config(c_handle, &config));
	
	/* Set new state by re-initializing the camera: */
	ERROR_IF_CAMWIRE_FAIL(
	    reconnect(c_handle, &config, set));
    }
    else
    {
	/* Frame offset is a special case which neither requires
	   reconnect() nor is set in set_non_dma_registers():*/
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_set_frame_offset(c_handle, set->left, set->top));

	/* Set all the others: */
	ERROR_IF_CAMWIRE_FAIL(
	    set_non_dma_registers(c_handle, set));
    }
   
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_read_state_from_file(const char *filename, Camwire_state *set)
{
    FILE *infile;
    char error_message[FILE_ERROR_MAX_CHARS+1];
    int scan_result;
    
    infile = fopen(filename, "r");
    if (infile == NULL)
    {
	snprintf(error_message, FILE_ERROR_MAX_CHARS,
		 "Camwire could not open the file %s for reading.", filename);
	DPRINTF(error_message);
	return(CAMWIRE_FAILURE);
    }
    
    scan_result = fscanf(infile,
	   "Camwire settings:\n"
	   "  num_frame_buffers: %d\n"
	   "  blue_gain:         %f\n"
	   "  red_gain:          %f\n"
	   "  left:              %d\n"
	   "  top:               %d\n"
	   "  width:             %d\n"
	   "  height:            %d\n"
	   "  coding:            %d\n"
	   "  frame_rate:        %f\n"
	   "  shutter:           %f\n"
	   "  external_trigger:  %d\n"
	   "  trigger_polarity:  %d\n"
	   "  single_shot:       %d\n"
	   "  running:           %d\n"
	   "  shadow:            %d\n",
	   &set->num_frame_buffers,
	   &set->blue_gain,
	   &set->red_gain,
	   &set->left,
	   &set->top,
	   &set->width,
	   &set->height,
	   (int *) &set->coding,
	   &set->frame_rate,
	   &set->shutter,
	   &set->external_trigger,
	   &set->trigger_polarity,
	   &set->single_shot,
	   &set->running,
	   &set->shadow);

    fclose(infile);
    if (scan_result == EOF || scan_result != 15)
    {
	DPRINTF("fscanf() failed in camwire_read_state_from_file().");
	return(CAMWIRE_FAILURE);
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_write_state_to_file(const char *filename, const Camwire_state *set)
{
    FILE *outfile;
    char error_message[FILE_ERROR_MAX_CHARS+1];
    int print_result;

    outfile = fopen(filename, "w");
    if (outfile == NULL)
    {
	snprintf(error_message, FILE_ERROR_MAX_CHARS,
		 "Camwire could not open the file %s for writing.", filename);
	DPRINTF(error_message);
	return(CAMWIRE_FAILURE);
    }

    print_result = fprintf(outfile, "Camwire settings:\n"
	       "  num_frame_buffers: %d\n"
	       "  blue_gain:         %g\n"
	       "  red_gain:          %g\n"
	       "  left:              %d\n"
	       "  top:               %d\n"
	       "  width:             %d\n"
	       "  height:            %d\n"
	       "  coding:            %d\n"
	       "  frame_rate:        %g\n"
	       "  shutter:           %g\n"
	       "  external_trigger:  %d\n"
	       "  trigger_polarity:  %d\n"
	       "  single_shot:       %d\n"
	       "  running:           %d\n"
	       "  shadow:            %d\n",
	       set->num_frame_buffers,
	       set->blue_gain,
	       set->red_gain,
	       set->left,
	       set->top,
	       set->width,
	       set->height,
	       set->coding,
	       set->frame_rate,
	       set->shutter,
	       set->external_trigger,
	       set->trigger_polarity,
	       set->single_shot,
	       set->running,
	       set->shadow);
    
    fclose(outfile);
    if (print_result < 1)
    {
	DPRINTF("fprintf() failed in camwire_write_state_to_file().");
	return(CAMWIRE_FAILURE);
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
/* Camwire_id is defined in camwire.h.*/

int camwire_get_identifier(const Camwire_handle c_handle,
			   Camwire_id *identifier)
{
    dc1394_camerainfo info;

    ERROR_IF_NULL(c_handle);
    ERROR_IF_DC1394_FAIL(
	dc1394_get_camera_info(camwire_bus_get_port(c_handle),
			       camwire_bus_get_node(c_handle),
			       &info));
    strncpy(identifier->vendor, info.vendor, CAMWIRE_ID_MAX_CHARS);
    /* CAMWIRE_ID_MAX_CHARS is defined in camwire.h.*/
    identifier->vendor[CAMWIRE_ID_MAX_CHARS] = '\0';
    strncpy(identifier->model, info.model, CAMWIRE_ID_MAX_CHARS);
    identifier->model[CAMWIRE_ID_MAX_CHARS] = '\0';
    snprintf(identifier->chip, CAMWIRE_ID_MAX_CHARS, "%llXh", info.euid_64);
    identifier->chip[CAMWIRE_ID_MAX_CHARS] = '\0';
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_get_stateshadow(const Camwire_handle c_handle, int *shadow)
{
    Camwire_state *shadow_state;
    
    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    *shadow = shadow_state->shadow;
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_set_stateshadow(const Camwire_handle c_handle, const int shadow)
{
    Camwire_state *shadow_state;
    
    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    shadow_state->shadow = shadow;
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_get_num_framebuffers(const Camwire_handle c_handle,
				 int *num_frame_buffers)
{
    dc1394_cameracapture *capture_control;
    Camwire_state *shadow_state;
    
    ERROR_IF_NULL(c_handle);
    capture_control = get_capturecontrol(c_handle);
    ERROR_IF_NULL(capture_control);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    *num_frame_buffers = capture_control->num_dma_buffers;
    shadow_state->num_frame_buffers = *num_frame_buffers;
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_set_num_framebuffers(const Camwire_handle c_handle,
				 const int num_frame_buffers)
{
    Camwire_state settings;
    Camwire_conf config;
    int temp_num_bufs;

    ERROR_IF_NULL(c_handle);
    ERROR_IF_CAMWIRE_FAIL(
	get_current_settings(c_handle, &settings));

    /* Ensure that video1394 lower limit is met: */
    if (num_frame_buffers < 2)  temp_num_bufs = 2;
    else                        temp_num_bufs = num_frame_buffers;

    /* Only proceed if number of buffers has changed: */
    if (settings.num_frame_buffers != temp_num_bufs)
    {
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_config(c_handle, &config));
	
	/* Set new number of buffers by re-initializing the camera: */
	settings.num_frame_buffers = temp_num_bufs;
	ERROR_IF_CAMWIRE_FAIL(
	    reconnect(c_handle, &config, &settings));
    }
    
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.  Deprecated.
*/
int camwire_get_framebuffer_lag(const Camwire_handle c_handle, int *buffer_lag)
{
    dc1394_cameracapture *capture_control;

    ERROR_IF_NULL(c_handle);
    capture_control = get_capturecontrol(c_handle);
    ERROR_IF_NULL(capture_control);
    if (capture_control->drop_frames)
    {
	*buffer_lag = 0;
    }
    else
    {
	*buffer_lag = capture_control->num_dma_buffers_behind;
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_flush_framebuffers(const Camwire_handle c_handle,
			       const int num_to_flush,
			       int *num_flushed,
			       int *buffer_lag)
{
    int nb, b;
    void *buffer;
    int flush_count;

    ERROR_IF_NULL(c_handle);
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_num_framebuffers(c_handle, &nb));

    /* Believe it or not, this is what is needed to stop video1394
       failing on the next non-polling capture: */
    for (b = 0; b < 10/nb; b++)  blink();
    
    for (flush_count = 0; flush_count < num_to_flush; flush_count++)
    {
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_point_next_frame_poll(c_handle, &buffer, buffer_lag));
	if (buffer == NULL)  break;
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_unpoint_frame(c_handle));
    }

    if (num_flushed != NULL)  *num_flushed = flush_count;

    /* Best-guess buffer lag if no frames got flushed: */
    if (buffer_lag != NULL && num_to_flush < 1)
    {
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_framebuffer_lag(c_handle, buffer_lag));
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_get_frame_offset(const Camwire_handle c_handle, int *left,
			     int *top)
{
    Camwire_state *shadow_state;
    unsigned int format, mode;

    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	*left = shadow_state->left;
	*top = shadow_state->top;
    }
    else
    {
	format = get_1394_format(c_handle);
	ERROR_IF_ZERO(format);
	if (format == FORMAT_VGA_NONCOMPRESSED)
	{ 	/* Format 0.*/
	    *left = *top = 0;
	}
	else if (format == FORMAT_SCALABLE_IMAGE_SIZE)
	{ 	/* Format 7.*/
	    mode = get_1394_mode(c_handle);
	    ERROR_IF_ZERO(mode);
	    ERROR_IF_DC1394_FAIL(
		dc1394_query_format7_image_position(
		    camwire_bus_get_port(c_handle),
		    camwire_bus_get_node(c_handle),
		    mode,
		    (unsigned int *) left,
		    (unsigned int *) top));
	}
	else
	{
	    DPRINTF("Unsupported camera format in "
		    "camwire_get_frame_offset().");
	    return(CAMWIRE_FAILURE);
	}
	shadow_state->left = *left;
	shadow_state->top = *top;
    }
    
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_set_frame_offset(const Camwire_handle c_handle,
			     const int left, const int top)
{
    unsigned int format, mode;
    unsigned int max_width, max_height;
    unsigned int hor_pixel_unit, ver_pixel_unit;
    int hor_limit, ver_limit;
    unsigned int width, height;
    int new_left, new_top;
    Camwire_state *shadow_state;

    ERROR_IF_NULL(c_handle);
    format = get_1394_format(c_handle);
    ERROR_IF_ZERO(format);
    if (format == FORMAT_VGA_NONCOMPRESSED)
    { 	/* Format 0.*/
	DPRINTF("Attempt to set frame offset in Format 0 in "
		"camwire_set_frame_offset().");
	return(CAMWIRE_FAILURE); 	/* Can't in Format 0.*/
    }
    else if (format == FORMAT_SCALABLE_IMAGE_SIZE)
    { 	/* Format 7.*/

	/* Get maximum width and height from the camera and adjust input
	   arguments if necessary, taking frame dimensions into
	   account: */
	mode = get_1394_mode(c_handle);
	ERROR_IF_ZERO(mode);
	ERROR_IF_DC1394_FAIL(
	    dc1394_query_format7_max_image_size(
		camwire_bus_get_port(c_handle),
		camwire_bus_get_node(c_handle),
		mode,
		&max_width, &max_height));
	if (max_width == 0 || max_height == 0)
	{
	    DPRINTF("dc1394_query_format7_max_image_size() returned a "
		    "zero size in camwire_set_frame_offset().");
	    return(CAMWIRE_FAILURE);
	}
	ERROR_IF_DC1394_FAIL(
	    dc1394_query_format7_image_size(
		camwire_bus_get_port(c_handle),
		camwire_bus_get_node(c_handle),
		mode,
		&width, &height));
	if (width > max_width || height > max_height)
	{
	    DPRINTF("dc1394_query_format7_image_size() returned a size which "
		    "exceeded the maximum in camwire_set_frame_offset().");
	    return(CAMWIRE_FAILURE);
	}
	ERROR_IF_DC1394_FAIL(
	    dc1394_query_format7_unit_position(
		camwire_bus_get_port(c_handle),
		camwire_bus_get_node(c_handle),
		mode,
		&hor_pixel_unit,
		&ver_pixel_unit));
	if (hor_pixel_unit == 0 || ver_pixel_unit == 0)
	{
	    DPRINTF("dc1394_query_format7_unit_position() returned a zero "
		    "unit size in camwire_set_frame_offset().");
	    return(CAMWIRE_FAILURE);
	}

	new_left = left/hor_pixel_unit;
	new_top  = top /ver_pixel_unit;
	hor_limit = (max_width  - width )/hor_pixel_unit;
	ver_limit = (max_height - height)/ver_pixel_unit;
	if (new_left > hor_limit)  new_left = hor_limit;
	if (new_top  > ver_limit)  new_top  = ver_limit;
	new_left *= hor_pixel_unit;
	new_top  *= ver_pixel_unit;

	/* Write the new offsets to the camera: */
	ERROR_IF_DC1394_FAIL(
	    dc1394_set_format7_image_position(
		camwire_bus_get_port(c_handle),
		camwire_bus_get_node(c_handle),
		mode,
		new_left, new_top));
	shadow_state = get_shadow_state(c_handle);
	ERROR_IF_NULL(shadow_state);
	shadow_state->left = new_left;
	shadow_state->top = new_top;
    }
    else
    {
	DPRINTF("Unsupported camera format in camwire_set_frame_offset().");
	return(CAMWIRE_FAILURE);
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_get_frame_size(const Camwire_handle c_handle, int *width,
			   int *height)
{
    Camwire_state *shadow_state;
    unsigned int format, mode;
    dc1394_cameracapture *capture_control;

    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	*width = shadow_state->width;
	*height = shadow_state->height;
    }
    else
    {
	format = get_1394_format(c_handle);
	ERROR_IF_ZERO(format);
	if (format == FORMAT_VGA_NONCOMPRESSED)
	{ 	/* Format 0.*/
	    capture_control = get_capturecontrol(c_handle);
	    ERROR_IF_NULL(capture_control);
	    if (capture_control->frame_width == 0 ||
		capture_control->frame_height == 0)
	    {
		DPRINTF("dc1394_cameracapture contains a zero frame size in "
			"camwire_get_frame_size().");
		return(CAMWIRE_FAILURE);
	    }
	    *width = capture_control->frame_width;
	    *height = capture_control->frame_height;
	}
	else if (format == FORMAT_SCALABLE_IMAGE_SIZE)
	{ 	/* Format 7.*/
	    mode = get_1394_mode(c_handle);
	    ERROR_IF_ZERO(mode);
	    ERROR_IF_DC1394_FAIL(
		dc1394_query_format7_image_size(
		    camwire_bus_get_port(c_handle),
		    camwire_bus_get_node(c_handle),
		    mode,
		    (unsigned int *) width,
		    (unsigned int *) height));
	}
	else
	{
	    DPRINTF("Unsupported camera format in camwire_get_frame_size().");
	    return(CAMWIRE_FAILURE);
	}
	shadow_state->width = *width;
	shadow_state->height = *height;
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
/* The frame size is updated by disconnecting and reconnecting the
   camera.  I have not been able to do it less brutally.  It seems that
   the video1394 driver does not expect the frame size to change even if
   enough memory has been allocated for larger frames. */

int camwire_set_frame_size(const Camwire_handle c_handle, const int width,
			   const int height)
{
    Camwire_conf config;
    Camwire_state settings;
    unsigned int max_width, max_height;
    unsigned int hor_pixel_unit, ver_pixel_unit;
    int hor_limit, ver_limit;
    int new_width, new_height;

    ERROR_IF_NULL(c_handle);
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_config(c_handle, &config));
    ERROR_IF_ZERO(config.format);
    if (config.format == FORMAT_VGA_NONCOMPRESSED)
    { 	/* Format 0.*/
	DPRINTF("Attempt to change frame size in Format 0 in "
		"camwire_set_frame_size().");
	return(CAMWIRE_FAILURE); 	/* Can't in Format 0.*/
    }
    else if (config.format == FORMAT_SCALABLE_IMAGE_SIZE)
    { 	/* Format 7.*/
	ERROR_IF_CAMWIRE_FAIL(
	    get_current_settings(c_handle, &settings));

	if (width == settings.width && height == settings.height)
	{
	    return(CAMWIRE_SUCCESS); 	/* Nothing has changed.*/
	}

	/* Width and height: */
	if (width != settings.width || height != settings.height)
	{
	    /* Get maximum width, maximum height, and unit pixel sizes
	       from the camera: */
	    ERROR_IF_ZERO(config.mode);
	    ERROR_IF_DC1394_FAIL(
		dc1394_query_format7_max_image_size(
		    camwire_bus_get_port(c_handle),
		    camwire_bus_get_node(c_handle),
		    config.mode,
		    &max_width,
		    &max_height));
	    if (max_width  == 0 || max_height == 0 ||
		settings.left >= (int) max_width ||
		settings.top  >= (int) max_height)
	    {
		DPRINTF("Frame size in current settings is zero or exceeds "
			"maximum in camwire_set_frame_size().");
		return(CAMWIRE_FAILURE);
	    }
	    ERROR_IF_DC1394_FAIL(
		dc1394_query_format7_unit_size(
		    camwire_bus_get_port(c_handle),
		    camwire_bus_get_node(c_handle),
		    config.mode,
		    &hor_pixel_unit,
		    &ver_pixel_unit));
	    if (hor_pixel_unit == 0 || ver_pixel_unit == 0)
	    {
		DPRINTF("dc1394_query_format7_unit_size() returned a zero "
			"unit size in camwire_set_frame_size().");
		return(CAMWIRE_FAILURE);
	    }

	    /* Adjust input arguments if necessary, taking frame offsets
	       and unit pixel sizes into account: */
	    new_width  = width /hor_pixel_unit;
	    new_height = height/ver_pixel_unit;
	    if (new_width  < 1)  new_width = 1;
	    if (new_height < 1)  new_height = 1;
	    hor_limit = (max_width - settings.left)/hor_pixel_unit;
	    ver_limit = (max_height - settings.top)/ver_pixel_unit;
	    if (new_width  > hor_limit)  new_width  = hor_limit;
	    if (new_height > ver_limit)  new_height = ver_limit;
	    new_width  *= hor_pixel_unit;
	    new_height *= ver_pixel_unit;
	}
	else
	{
	    new_width  = width;
	    new_height = height;
	}

	/* Check that minimum number of pixels is maintained: */
	if (new_width*new_height < config.min_pixels)
	{
	    DPRINTF("camwire_get_config() returns a minimum number of "
		    "pixels which exceeds what is requested in "
		    "camwire_set_frame_size().");
	    return(CAMWIRE_FAILURE);
	}

        /* Only proceed if size has changed after all: */
	if (new_width != settings.width || new_height != settings.height)
	{
	    settings.width  = new_width;
	    settings.height = new_height;

	    /* Set the new dimensions by re-initializing the camera: */
	    ERROR_IF_CAMWIRE_FAIL(
		reconnect(c_handle, &config, &settings));
	}
    }
    else
    {
	DPRINTF("Unsupported camera format in camwire_set_frame_size().");
	return(CAMWIRE_FAILURE);
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_get_pixel_coding(const Camwire_handle c_handle,
			     Camwire_pixel *coding)
{
    Camwire_state *shadow_state;
    unsigned int format, mode, color_id;
    dc1394_cameracapture *capture_control;

    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	*coding = shadow_state->coding;
    }
    else
    {
	format = get_1394_format(c_handle);
	ERROR_IF_ZERO(format);
	mode = get_1394_mode(c_handle);
	ERROR_IF_ZERO(mode);
	if (format == FORMAT_VGA_NONCOMPRESSED)
	{ 	/* Format 0.*/
	    capture_control = get_capturecontrol(c_handle);
	    ERROR_IF_NULL(capture_control);
	    *coding = convert_mode2pixelcoding(mode);
	}
	else if (format == FORMAT_SCALABLE_IMAGE_SIZE)
	{ 	/* Format 7.*/
	    ERROR_IF_DC1394_FAIL(
		dc1394_query_format7_color_coding_id(
		    camwire_bus_get_port(c_handle),
		    camwire_bus_get_node(c_handle),
		    mode,
		    &color_id));
	    *coding = convert_colorid2pixelcoding(color_id);
	}
	else
	{
	    DPRINTF("Unsupported camera format in "
		    "camwire_get_pixel_coding().");
	    return(CAMWIRE_FAILURE);
	}
	shadow_state->coding = *coding;
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
/* The pixel colour coding is updated by disconnecting and reconnecting
   the camera.  I have not been able to do it less brutally.  It seems
   that the video1394 driver does not expect the frame size to change
   even if enough memory has been allocated for larger frames. */

int camwire_set_pixel_coding(const Camwire_handle c_handle,
			     const Camwire_pixel coding)
{
    Camwire_conf config;
    Camwire_pixel old_coding;
    quadlet_t bit_field; 	/* Unsigned 32-bit int (libraw1394).*/
    unsigned int color_id;
    int old_depth, new_depth;
    Camwire_state settings;
    Camwire_state *shadow_state;
    
    ERROR_IF_NULL(c_handle);
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_config(c_handle, &config));
    ERROR_IF_ZERO(config.format);
    if (config.format == FORMAT_VGA_NONCOMPRESSED)
    { 	/* Format 0.*/
	DPRINTF("Attempt to set pixel coding in Format 0 in "
		"camwire_set_pixel_coding().");
	return(CAMWIRE_FAILURE); 	/* Can't in Format 0.*/
    }
    else if (config.format == FORMAT_SCALABLE_IMAGE_SIZE)
    { 	/* Format 7.*/
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_pixel_coding(c_handle, &old_coding));

        /* Only proceed if pixel colour coding has changed: */
	if (coding != old_coding)
	{
	    /* Check if new pixel coding is supported by camera: */
	    ERROR_IF_DC1394_FAIL(
		dc1394_query_format7_color_coding(
		    camwire_bus_get_port(c_handle),
		    camwire_bus_get_node(c_handle),
		    config.mode,
		    &bit_field));
	    if (bit_field == 0)
	    {
		DPRINTF("dc1394_query_format7_color_coding() returned a null "
			"bitfield in camwire_set_pixel_coding().");
		return(CAMWIRE_FAILURE);
	    }
	    color_id = convert_pixelcoding2colorid(coding, bit_field);
	    if (color_id == 0)
	    {
		DPRINTF("Pixel colour coding is invalid or not supported  by "
			"the camera in camwire_set_pixel_coding().");
		return(CAMWIRE_FAILURE);
	    }

	    /* Set the new coding: */
	    ERROR_IF_CAMWIRE_FAIL(
		camwire_pixel_depth(old_coding, &old_depth));
	    ERROR_IF_CAMWIRE_FAIL(
		camwire_pixel_depth(coding, &new_depth));
	    if (new_depth == old_depth)
	    {
		/* Set the new coding directly: */
		ERROR_IF_DC1394_FAIL(
		    dc1394_set_format7_color_coding_id(
			camwire_bus_get_port(c_handle),
			camwire_bus_get_node(c_handle),
			config.mode,
			color_id));
		shadow_state = get_shadow_state(c_handle);
		ERROR_IF_NULL(shadow_state);
		shadow_state->coding = coding;
	    }
	    else
	    {
		/* Re-initialize the camera with the new coding: */
		ERROR_IF_CAMWIRE_FAIL(
		    get_current_settings(c_handle, &settings));
		settings.coding = coding;
		ERROR_IF_CAMWIRE_FAIL(
		    reconnect(c_handle, &config, &settings));
	    }
	}
    }
    else
    {
	DPRINTF("Unsupported camera format in camwire_set_pixel_coding().");
	return(CAMWIRE_FAILURE);
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_pixel_depth(const Camwire_pixel coding, int *depth)
{
    switch (coding)
    {
	case CAMWIRE_PIXEL_MONO8:
	    /* case CAMWIRE_PIXEL_RAW8: */
	    *depth = 8;
	    break;
	case CAMWIRE_PIXEL_YUV411:
	    *depth = 12;
	    break;
	case CAMWIRE_PIXEL_YUV422:
	case CAMWIRE_PIXEL_MONO16:
	    /* case CAMWIRE_PIXEL_MONO16S: */
	    /* case CAMWIRE_PIXEL_RAW16: */
	    *depth = 16;
	    break;
	case CAMWIRE_PIXEL_YUV444:
	case CAMWIRE_PIXEL_RGB8:
	    *depth = 24;
	    break;
	case CAMWIRE_PIXEL_RGB16:
	    /* case CAMWIRE_PIXEL_RGB16S: */
	    *depth = 48;
	    break;
	default:
	    *depth = 0;
	    return(CAMWIRE_FAILURE);  /* Invalid or unknown coding.*/
	    break;
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
/* In Format 0, the frame rate is stored in the camera as an index,
   which we translate into a frame rate in frames per second.

   In format 7, the camera's frame rate index is ignored.  One has to
   calculate the frame rate from the number of packets required to send
   one frame.  Since exactly one packet is sent every 125 microseconds,
   the frame rate is 1/(num_packets*125us).  The camera calculates
   num_packets and we read it from a register called
   PACKET_PER_FRAME_INQ.
*/

int camwire_get_framerate(const Camwire_handle c_handle,
			  float *frame_rate)
{
    Camwire_state *shadow_state;
    unsigned int format, frame_rate_index;
    int num_packets;

    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	*frame_rate = shadow_state->frame_rate;
    }
    else
    {
	format = get_1394_format(c_handle);
	ERROR_IF_ZERO(format);
	if (format == FORMAT_VGA_NONCOMPRESSED)
	{ 	/* Format 0.*/
	    ERROR_IF_DC1394_FAIL(
		dc1394_get_video_framerate(
		    camwire_bus_get_port(c_handle),
		    camwire_bus_get_node(c_handle),
		    &frame_rate_index));
	    
	    *frame_rate = convert_index2framerate(frame_rate_index);
	    if (*frame_rate < 0.0)
	    {
		DPRINTF("convert_index2framerate() failed in "
			"camwire_get_framerate().");
		return(CAMWIRE_FAILURE); 	/* Invalid index.*/
	    }
	}
	else if (format == FORMAT_SCALABLE_IMAGE_SIZE)
	{ 	/* Format 7.*/
	    ERROR_IF_CAMWIRE_FAIL(
		get_numpackets(c_handle, &num_packets));
	    *frame_rate = convert_numpackets2framerate(c_handle, num_packets);
	}
	else
	{
	    DPRINTF("Unsupported camera format in camwire_get_framerate().");
	    return(CAMWIRE_FAILURE);
	}
	shadow_state->frame_rate = *frame_rate;
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
/* In Format 0, the frame rate is written to a camera register as an
   index, which we get by quantizing the frame rate to the nearest legal
   value.

   In Format 7, the frame rate register is ignored.  The frame rate is
   controlled indirectly by setting the number of packets required per
   frame.  This is slightly involved because the number of packets
   cannot be written to the camera as such (although we may be able to
   read its current value).  We instead have to set the packet size,
   which in turn depends on the frame size.

   Even worse, changing the packet size causes the total frame size to
   change because of varying amounts of padding.  I have not been able
   to find a way to change the frame rate in Format 7 without
   re-initializing the camera interface to the video1394 driver.
*/

int camwire_set_framerate(const Camwire_handle c_handle,
			  const float frame_rate)
{
    int frame_rate_index;
    Camwire_state settings;
    int old_num_packets, new_num_packets;
    int old_packet_size, new_packet_size;
    Camwire_conf config;
    double actual_frame_rate;
    Camwire_state *shadow_state;

    ERROR_IF_NULL(c_handle);
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_config(c_handle, &config));
    ERROR_IF_ZERO(config.format);
    if (config.format == FORMAT_VGA_NONCOMPRESSED)
    { 	/* Format 0.*/
	frame_rate_index = convert_framerate2index(frame_rate);
	ERROR_IF_DC1394_FAIL(
	    dc1394_set_video_framerate(
		camwire_bus_get_port(c_handle),
		camwire_bus_get_node(c_handle),
		(unsigned int) frame_rate_index));
	actual_frame_rate = convert_index2framerate(frame_rate_index);
	shadow_state = get_shadow_state(c_handle);
	ERROR_IF_NULL(shadow_state);
	shadow_state->frame_rate = actual_frame_rate;
    }
    else if (config.format == FORMAT_SCALABLE_IMAGE_SIZE)
    { 	/* Format 7.*/
	if (frame_rate <= 0.0)
	{
	    DPRINTF("frame_rate argument is not positive in "
		    "camwire_set_framerate().");
	    return(CAMWIRE_FAILURE);
	}
    
	ERROR_IF_CAMWIRE_FAIL(
	    get_current_settings(c_handle, &settings));

	old_num_packets =
	    convert_framerate2numpackets(c_handle, settings.frame_rate);
	old_packet_size = convert_numpackets2packetsize(c_handle,
							old_num_packets,
							settings.width,
							settings.height,
							settings.coding);
	new_num_packets = convert_framerate2numpackets(c_handle, frame_rate);
	new_packet_size = convert_numpackets2packetsize(c_handle,
							new_num_packets,
							settings.width,
							settings.height,
							settings.coding);

	/* Only proceed if frame rate has actually changed: */
	if (old_packet_size != new_packet_size)
	{
	    settings.frame_rate = frame_rate;

	    /* Set the new frame rate by re-initializing the camera: */
	    ERROR_IF_CAMWIRE_FAIL(
		reconnect(c_handle, &config, &settings));
	}
    }
    else
    {
	DPRINTF("Unsupported camera format in camwire_set_framerate().");
	return(CAMWIRE_FAILURE);
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_get_shutter(const Camwire_handle c_handle, float *shutter)
{
    Camwire_state *shadow_state;
    dc1394bool_t has_shutter;
    unsigned int shutter_reg;
    Camwire_conf config;

    ERROR_IF_NULL(c_handle);
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_SHUTTER,
				  &has_shutter));
    if (has_shutter != DC1394_TRUE)
    {
	DPRINTF("dc1394_is_feature_present() reports no shutter in"
		"camwire_get_shutter().");
	return(CAMWIRE_FAILURE);
    }
    
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	*shutter = shadow_state->shutter;
    }
    else
    {
	ERROR_IF_DC1394_FAIL(
	    dc1394_get_shutter(camwire_bus_get_port(c_handle),
			       camwire_bus_get_node(c_handle),
			       &shutter_reg));
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_config(c_handle, &config));
	*shutter =
	    config.exposure_offset + shutter_reg*config.exposure_quantum;
	shadow_state->shutter = *shutter;
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_set_shutter(const Camwire_handle c_handle, const float shutter)
{
    Camwire_state *shadow_state;
    dc1394bool_t has_shutter;
    Camwire_conf config;
    unsigned int shutter_reg, shutter_min, shutter_max;

    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_SHUTTER,
				  &has_shutter));
    if (has_shutter != DC1394_TRUE)
    {
	shadow_state->shutter = shutter;
	DPRINTF("dc1394_is_feature_present() reports no shutter in"
		"camwire_set_shutter().");
	return(CAMWIRE_FAILURE);
    }
    
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_config(c_handle, &config));

    shutter_reg = (unsigned int)
	((shutter - config.exposure_offset)/config.exposure_quantum + 0.5);

    /* Limit shutter_reg to the allowed range: */
    ERROR_IF_DC1394_FAIL(
	dc1394_get_min_value(camwire_bus_get_port(c_handle),
			     camwire_bus_get_node(c_handle),
			     FEATURE_SHUTTER,
			     &shutter_min));
    if (shutter_reg < shutter_min)  shutter_reg = shutter_min;

    ERROR_IF_DC1394_FAIL(
	dc1394_get_max_value(camwire_bus_get_port(c_handle),
			     camwire_bus_get_node(c_handle),
			     FEATURE_SHUTTER,
			     &shutter_max));
    if (shutter_reg > shutter_max)  shutter_reg = shutter_max;

    ERROR_IF_DC1394_FAIL(
	dc1394_set_shutter(camwire_bus_get_port(c_handle),
			   camwire_bus_get_node(c_handle),
			   shutter_reg));
    shadow_state->shutter =
	config.exposure_offset + shutter_reg*config.exposure_quantum;

    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_get_trigger_source(const Camwire_handle c_handle, int *external)
{
    Camwire_state *shadow_state;
    dc1394bool_t has_trigger, trigger_on;

    ERROR_IF_NULL(c_handle);
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_TRIGGER,
				  &has_trigger));
    if (has_trigger != DC1394_TRUE)
    {
	DPRINTF("dc1394_is_feature_present() reports no trigger in"
		"camwire_get_trigger_source().");
	return(CAMWIRE_FAILURE);
    }
    
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	*external = shadow_state->external_trigger;
    }
    else
    {
	ERROR_IF_DC1394_FAIL(
	    dc1394_get_trigger_on_off(camwire_bus_get_port(c_handle),
				      camwire_bus_get_node(c_handle),
				      &trigger_on));
	if (trigger_on == DC1394_TRUE)  *external = 1;
	else                            *external = 0;
	shadow_state->external_trigger = *external;
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_set_trigger_source(const Camwire_handle c_handle,
			       const int external)
{
    dc1394bool_t has_trigger, on_off;
    Camwire_state *shadow_state;

    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_TRIGGER,
				  &has_trigger));
    if (has_trigger != DC1394_TRUE)
    {
	shadow_state->external_trigger = external;
	DPRINTF("dc1394_is_feature_present() reports no trigger in"
		"camwire_set_trigger_source().");
	return(CAMWIRE_FAILURE);
    }
    if (external != 0)  on_off = DC1394_TRUE;
    else                on_off = DC1394_FALSE;
    ERROR_IF_DC1394_FAIL(
	dc1394_set_trigger_on_off(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  on_off));
    shadow_state->external_trigger = external;
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_get_trigger_polarity(const Camwire_handle c_handle, int *rising)
{
    Camwire_state *shadow_state;
    dc1394bool_t has_trigger, has_polarity, polarity;

    ERROR_IF_NULL(c_handle);
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_TRIGGER,
				  &has_trigger));
    if (has_trigger != DC1394_TRUE)
    {
	DPRINTF("dc1394_is_feature_present() reports no trigger in"
		"camwire_get_trigger_polarity().");
	return(CAMWIRE_FAILURE);
    }
    ERROR_IF_DC1394_FAIL(
	dc1394_trigger_has_polarity(camwire_bus_get_port(c_handle),
				    camwire_bus_get_node(c_handle),
				    &has_polarity));
    if (has_polarity != DC1394_TRUE)
    {
	DPRINTF("dc1394_trigger_has_polarity() reports no polarity "
		"in camwire_get_trigger_polarity().");
	return(CAMWIRE_FAILURE);
    }
    
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	*rising = shadow_state->trigger_polarity;
    }
    else
    {
	ERROR_IF_DC1394_FAIL(
	    dc1394_get_trigger_polarity(camwire_bus_get_port(c_handle),
					camwire_bus_get_node(c_handle),
					&polarity));
	if (polarity == DC1394_TRUE)  *rising = 1;
	else                          *rising = 0;
	shadow_state->trigger_polarity = *rising;
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_set_trigger_polarity(const Camwire_handle c_handle,
				 const int rising)
{
    dc1394bool_t has_trigger, has_polarity, polarity;
    Camwire_state *shadow_state;

    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_TRIGGER,
				  &has_trigger));
    if (has_trigger != DC1394_TRUE)
    {
	shadow_state->trigger_polarity = rising;
	DPRINTF("dc1394_is_feature_present() reports no trigger in"
		"camwire_set_trigger_polarity().");
	return(CAMWIRE_FAILURE);
    }
    ERROR_IF_DC1394_FAIL(
	dc1394_trigger_has_polarity(camwire_bus_get_port(c_handle),
				    camwire_bus_get_node(c_handle),
				    &has_polarity));
    if (has_polarity != DC1394_TRUE)
    {
	shadow_state->trigger_polarity = rising;
	DPRINTF("dc1394_trigger_has_polarity() reports no polarity in "
		"camwire_set_trigger_polarity().");
	return(CAMWIRE_FAILURE);
    }
    if (rising != 0)  polarity = DC1394_TRUE;
    else              polarity = DC1394_FALSE;
    ERROR_IF_DC1394_FAIL(
	dc1394_set_trigger_polarity(camwire_bus_get_port(c_handle),
				    camwire_bus_get_node(c_handle),
				    polarity));
    shadow_state->trigger_polarity = rising;
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_get_white_balance(const Camwire_handle c_handle, float *blue_gain,
			      float *red_gain)
{
    Camwire_state *shadow_state;
    dc1394bool_t has_white_balance;
    unsigned int blue, red;
    Camwire_conf config;

    ERROR_IF_NULL(c_handle);
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_WHITE_BALANCE,
				  &has_white_balance));
    if (has_white_balance != DC1394_TRUE)
    {
	DPRINTF("dc1394_is_feature_present() reports no white balance in "
		"camwire_get_white_balance().");
	return(CAMWIRE_FAILURE);
    }
    
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	*blue_gain = shadow_state->blue_gain;
	*red_gain = shadow_state->red_gain;
    }
    else
    {
	*blue_gain = *red_gain = 1;  /* Default.*/
	ERROR_IF_DC1394_FAIL(
	    dc1394_get_white_balance(camwire_bus_get_port(c_handle),
				     camwire_bus_get_node(c_handle),
				     &blue, &red));
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_config(c_handle, &config));
	*blue_gain = blue/config.blue_gain_norm;
	*red_gain = red/config.red_gain_norm;
	shadow_state->blue_gain = *blue_gain;
	shadow_state->red_gain = *red_gain;
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_set_white_balance(const Camwire_handle c_handle,
			      const float blue_gain, const float red_gain)
{
    dc1394bool_t has_white_balance;
    unsigned int blue, red;
    Camwire_conf config;
    dc1394_feature_info capability;
    Camwire_state *shadow_state;

    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_WHITE_BALANCE,
				  &has_white_balance));
    if (has_white_balance != DC1394_TRUE)
    {
	shadow_state->blue_gain = blue_gain;
	shadow_state->red_gain = red_gain;
	DPRINTF("dc1394_is_feature_present() reports no white balance in "
		"camwire_set_white_balance().");
	return(CAMWIRE_FAILURE);
    }

    /* Check limits: */
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_config(c_handle, &config));
    blue = blue_gain*config.blue_gain_norm + 0.5;
    red = red_gain*config.red_gain_norm + 0.5;
    capability.feature_id = FEATURE_WHITE_BALANCE;
    ERROR_IF_DC1394_FAIL(
	dc1394_get_camera_feature(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  &capability));
    if (capability.min < capability.max)
    {
	if ((int) blue < capability.min)  blue = capability.min;
	if ((int) red  < capability.min)  red  = capability.min;
	if ((int) blue > capability.max)  blue = capability.max;
	if ((int) red  > capability.max)  red  = capability.max;
    }

    /* Update the camera with new gains: */
    ERROR_IF_DC1394_FAIL(
	dc1394_set_white_balance(camwire_bus_get_port(c_handle),
				 camwire_bus_get_node(c_handle),
				 blue, red));
    shadow_state->blue_gain = blue/config.blue_gain_norm;
    shadow_state->red_gain = red/config.red_gain_norm;
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
/* We have a concept of single-shot/continuous which is orthogonal to
   run/stop.  The closest equivalents in libdc1394 are the
   ISO_EN/Continuous_shot and One_shot/Multi_shot registers accessed
   through dc1394_start/stop_iso_transmission() and
   dc1394_set/unset_one_shot().

   Our orthogonal concept is implemented with the functions
   camwire_get/set_single_shot() and camwire_get/set_run_stop().
   
   We are forced to maintain our own idea of the single-shot/continuous
   status in shadow_state->single_shot.  libdc1394 can only tell us the
   state of the ISO_EN/Continuous_Shot register which is used to
   describe both continuous and run statuses, and the One_Shot register
   which asynchronously auto-clears itself after transmitting a frame.
   We use every opportunity to re-align our internal status with the
   hardware status.
*/

int camwire_get_single_shot(const Camwire_handle c_handle, int *single_shot_on)
{
    dc1394bool_t iso_en, one_shot_set;
    Camwire_state *shadow_state;

    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	*single_shot_on = shadow_state->single_shot;
    }
    else
    {  /* Don't use shadow: ask the camera:*/
	ERROR_IF_DC1394_FAIL(
	    dc1394_get_iso_status(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  &iso_en));
	
	if (iso_en == DC1394_TRUE)
	{ 	/* Running in continuous mode.*/
	    shadow_state->running = 1;
	    *single_shot_on = 0;
	}
	else
	{ 	/* Running in single-shot mode or stopped.*/
	    ERROR_IF_DC1394_FAIL(
		dc1394_get_one_shot(camwire_bus_get_port(c_handle),
				    camwire_bus_get_node(c_handle),
				    &one_shot_set));
	    if (one_shot_set == DC1394_TRUE)
	    { 	/* Camera is running.*/
		shadow_state->running = 1;
		*single_shot_on = 1;
	    }
	    else
	    { 	/* Camera is stopped.*/
		shadow_state->running = 0;
		*single_shot_on = shadow_state->single_shot;
	    }
	}
	shadow_state->single_shot = *single_shot_on;
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_set_single_shot(const Camwire_handle c_handle,
			    const int single_shot_on)
{
    dc1394bool_t iso_en, one_shot_set;
    Camwire_state *shadow_state;

    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	if (shadow_state->running)
	{  /* We *think* the camera is running.*/
	    if (!shadow_state->single_shot && single_shot_on)
	    { 	/* Camera is running: change to single-shot:*/
		ERROR_IF_DC1394_FAIL(
		    dc1394_stop_iso_transmission(
			camwire_bus_get_port(c_handle),
			camwire_bus_get_node(c_handle)));
		ERROR_IF_DC1394_FAIL(
		    dc1394_set_one_shot(
			camwire_bus_get_port(c_handle),
			camwire_bus_get_node(c_handle)));
	    }
	    else if (shadow_state->single_shot && !single_shot_on)
	    { 	/* Don't know if camera is still runnning: let's find out:*/
		ERROR_IF_DC1394_FAIL(
		    dc1394_get_one_shot(camwire_bus_get_port(c_handle),
					camwire_bus_get_node(c_handle),
					&one_shot_set));
		if (one_shot_set == DC1394_TRUE)
		{  /* Camera is still runnning: change to continuous:*/
		    ERROR_IF_DC1394_FAIL(
			dc1394_unset_one_shot(camwire_bus_get_port(c_handle),
					      camwire_bus_get_node(c_handle)));
		    ERROR_IF_DC1394_FAIL(
			dc1394_start_iso_transmission(
			    camwire_bus_get_port(c_handle),
			    camwire_bus_get_node(c_handle)));
		}
		else
		{  /* Camera has finished single shot: update shadow state:*/
		    shadow_state->running = 0;
		}
	    }
	}
	/* else change only the internal state.*/
    }
    else
    {  /* Don't use shadow: ask the camera:*/
	ERROR_IF_DC1394_FAIL(
	    dc1394_get_iso_status(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  &iso_en));
	
	if (iso_en == DC1394_TRUE && single_shot_on)
	{ 	/* Camera is running: change to single-shot:*/
	    ERROR_IF_DC1394_FAIL(
		dc1394_stop_iso_transmission(camwire_bus_get_port(c_handle),
					     camwire_bus_get_node(c_handle)));
	    ERROR_IF_DC1394_FAIL(
		dc1394_set_one_shot(camwire_bus_get_port(c_handle),
				    camwire_bus_get_node(c_handle)));
	    shadow_state->running = 1;
	}
	else if (iso_en == DC1394_FALSE && !single_shot_on)
	{
	    ERROR_IF_DC1394_FAIL(
		dc1394_get_one_shot(camwire_bus_get_port(c_handle),
				    camwire_bus_get_node(c_handle),
				    &one_shot_set));
	    if (one_shot_set == DC1394_TRUE)
	    { 	/* Camera is still runnning: change to continuous:*/
		ERROR_IF_DC1394_FAIL(
		    dc1394_unset_one_shot(camwire_bus_get_port(c_handle),
					  camwire_bus_get_node(c_handle)));
		ERROR_IF_DC1394_FAIL(
		    dc1394_start_iso_transmission(
			camwire_bus_get_port(c_handle),
			camwire_bus_get_node(c_handle)));
		shadow_state->running = 1;
	    }
	    /* else change only the internal state.*/
	}
    }
    shadow_state->single_shot = single_shot_on;

    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_get_run_stop(const Camwire_handle c_handle, int *runsts)
{
    dc1394bool_t iso_en, one_shot_set;
    Camwire_state *shadow_state;

    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	*runsts = shadow_state->running;
	
	/* One_Shot register self-clears after transmission:*/
	if (shadow_state->running && shadow_state->single_shot)
	{  /* Don't know if camera is still runnning: let's find out:*/
	    ERROR_IF_DC1394_FAIL(
		dc1394_get_one_shot(camwire_bus_get_port(c_handle),
				    camwire_bus_get_node(c_handle),
				    &one_shot_set));
	    if (one_shot_set == DC1394_FALSE)
	    {  /* Camera has finished single shot: update shadow state:*/
		shadow_state->running = *runsts = 0;
	    }
	}
    }
    else
    {  /* Don't use shadow: ask the camera:*/
	ERROR_IF_DC1394_FAIL(
	    dc1394_get_iso_status(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  &iso_en));
	if (iso_en == DC1394_TRUE) 
	{ 	/* Camera is running in continuous mode:*/
	    shadow_state->single_shot = 0;
	    *runsts = 1;
	}
	else
	{
	    ERROR_IF_DC1394_FAIL(
		dc1394_get_one_shot(camwire_bus_get_port(c_handle),
				    camwire_bus_get_node(c_handle),
				    &one_shot_set));
	    if (one_shot_set == DC1394_TRUE) 
	    { 	/* Camera is running in single-shot mode:*/
		shadow_state->single_shot = 1;
		*runsts = 1;
	    }
	    else
	    {
		*runsts = 0;
	    }
	}
	shadow_state->running = *runsts;
    }
    
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_set_run_stop(const Camwire_handle c_handle, const int runsts)
{
    dc1394bool_t iso_en, one_shot_set;
    Camwire_state *shadow_state;

    ERROR_IF_NULL(c_handle);
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	if (shadow_state->single_shot)
	{  /* Single-shot.*/
	    if (shadow_state->running && !runsts)
	    { 	/* Stop camera (even if it has already stopped):*/
		ERROR_IF_DC1394_FAIL(
		    dc1394_unset_one_shot(camwire_bus_get_port(c_handle),
					  camwire_bus_get_node(c_handle)));
	    }
	    else if (runsts)
	    {  /* Run in single-shot mode (even if we think it is already):*/
		ERROR_IF_DC1394_FAIL(
		    dc1394_set_one_shot(camwire_bus_get_port(c_handle),
					camwire_bus_get_node(c_handle)));
	    }
	    /* else do nothing.*/
	}
	else
	{  /* Continuous.*/
	    if (shadow_state->running && !runsts)
	    {  /* Stop camera:*/
		ERROR_IF_DC1394_FAIL(
		    dc1394_stop_iso_transmission(
			camwire_bus_get_port(c_handle),
			camwire_bus_get_node(c_handle)));
	    }
	    else if (!shadow_state->running && runsts)
	    { 	/* Run camera:*/
		ERROR_IF_DC1394_FAIL(
		    dc1394_start_iso_transmission(
			camwire_bus_get_port(c_handle),
			camwire_bus_get_node(c_handle)));
	    }
	    /* else do nothing.*/
	}
    }
    else
    {  /* Don't use shadow: ask the camera:*/
	ERROR_IF_DC1394_FAIL(
	    dc1394_get_iso_status(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  &iso_en));
	if (iso_en == DC1394_TRUE) 
	{  /* Camera is running in continuous mode:*/
	    shadow_state->single_shot = 0;
	    if (!runsts) 
	    {  /* Stop camera:*/
		ERROR_IF_DC1394_FAIL(
		    dc1394_stop_iso_transmission(
			camwire_bus_get_port(c_handle),
			camwire_bus_get_node(c_handle)));
	    }
	    /* else do nothing.*/
	}
	else
	{
	    ERROR_IF_DC1394_FAIL(
		dc1394_get_one_shot(camwire_bus_get_port(c_handle),
				    camwire_bus_get_node(c_handle),
				    &one_shot_set));
	    if (one_shot_set == DC1394_TRUE) 
	    {  /* Camera is running in single-shot mode:*/
		shadow_state->single_shot = 1;
		if (!runsts)
		{ 	/* Stop camera:*/
		    ERROR_IF_DC1394_FAIL(
			dc1394_unset_one_shot(camwire_bus_get_port(c_handle),
					      camwire_bus_get_node(c_handle)));
		}
		/* else do nothing.*/
	    }
	    else if (runsts)
	    {  /* Camera is stopped:*/
		if (!shadow_state->single_shot) 
		{ 	/* Run in continuous mode:*/
		    ERROR_IF_DC1394_FAIL(
			dc1394_start_iso_transmission(
			    camwire_bus_get_port(c_handle),
			    camwire_bus_get_node(c_handle)));
		}
		else
		{  /* Run in single-shot mode:*/
		    ERROR_IF_DC1394_FAIL(
			dc1394_set_one_shot(camwire_bus_get_port(c_handle),
					    camwire_bus_get_node(c_handle)));
		}
	    }
	}
    }
    shadow_state->running = runsts;
    
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_copy_next_frame(const Camwire_handle c_handle, void *buffer,
			    int *buffer_lag)
{
    dc1394_cameracapture *capture_control;
    void *buf_ptr;
    int width, height, depth;
    Camwire_pixel coding;

    ERROR_IF_NULL(c_handle);
    capture_control = get_capturecontrol(c_handle);
    ERROR_IF_NULL(capture_control);

    ERROR_IF_CAMWIRE_FAIL(
	camwire_point_next_frame(c_handle, &buf_ptr, buffer_lag));

    /* Copy the minimum number of bytes to avoid segfaults if the user's
       frame buffer is only just big enough to take one frame.  Note
       that capture_control->dma_frame_size should not be used in
       memcpy() because it includes padding up to a page alignment, and
       capture_control->quadlets_per_frame should also not be used
       because it may be rounded up to the nearest integer number of
       packet sizes in Format 7: */
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_frame_size(c_handle, &width, &height));
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_pixel_coding(c_handle, &coding));
    ERROR_IF_CAMWIRE_FAIL(
	camwire_pixel_depth(coding, &depth));
    memcpy(buffer, buf_ptr, (size_t) width*height*depth/8);

    camwire_unpoint_frame(c_handle);

    return(CAMWIRE_SUCCESS);
}
int camwire_copy_next_frame_with_stride(const Camwire_handle c_handle, void *buffer,
					int *buffer_lag, intptr_t stride0)
{
    dc1394_cameracapture *capture_control;
    void *buf_ptr;
    int width, height, depth;
    int row;
    int mywidth_bytes = width*depth/8;
    Camwire_pixel coding;

    ERROR_IF_NULL(c_handle);
    capture_control = get_capturecontrol(c_handle);
    ERROR_IF_NULL(capture_control);

    ERROR_IF_CAMWIRE_FAIL(
	camwire_point_next_frame(c_handle, &buf_ptr, buffer_lag));

    /* Copy the minimum number of bytes to avoid segfaults if the user's
       frame buffer is only just big enough to take one frame.  Note
       that capture_control->dma_frame_size should not be used in
       memcpy() because it includes padding up to a page alignment, and
       capture_control->quadlets_per_frame should also not be used
       because it may be rounded up to the nearest integer number of
       packet sizes in Format 7: */
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_frame_size(c_handle, &width, &height));
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_pixel_coding(c_handle, &coding));
    ERROR_IF_CAMWIRE_FAIL(
	camwire_pixel_depth(coding, &depth));
    /*memcpy(buffer, buf_ptr, (size_t) width*height*depth/8);*/
    mywidth_bytes = width*depth/8;
    for (row=0; row<height; ++row) {
      memcpy( buffer + row*stride0, buf_ptr + row*mywidth_bytes, mywidth_bytes);
    }

    camwire_unpoint_frame(c_handle);

    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_point_next_frame(const Camwire_handle c_handle, void **buf_ptr,
			     int *buffer_lag)
{
    User_handle internal_status;
    dc1394_cameracapture *capture_control;
    int dc1394_return, missed_frames;

    ERROR_IF_NULL(c_handle);
    internal_status = camwire_bus_get_userdata(c_handle);
    ERROR_IF_NULL(internal_status);
    capture_control = internal_status->control;
    ERROR_IF_NULL(capture_control);

    dc1394_return = dc1394_dma_single_capture(capture_control);
    if (dc1394_return != DC1394_SUCCESS)
    {
	DPRINTF("dc1394_dma_single_capture() failed in "
		"camwire_point_next_frame().");
	return(CAMWIRE_FAILURE);
    }
    *buf_ptr = (void *) capture_control->capture_buffer;

    /* Increment the frame number if we have a frame: */
    if (capture_control->drop_frames)
    {
	missed_frames = capture_control->num_dma_buffers_behind;
    }
    else
    {
	missed_frames = 0;
    }
    internal_status->frame_number += 1 + missed_frames;

    if (buffer_lag != NULL)
    {
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_framebuffer_lag(c_handle, buffer_lag));
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_point_next_frame_poll(const Camwire_handle c_handle,
				  void **buf_ptr, int *buffer_lag)
{
    User_handle internal_status;
    dc1394_cameracapture *capture_control;
    int dc1394_return, missed_frames;

    ERROR_IF_NULL(c_handle);
    internal_status = camwire_bus_get_userdata(c_handle);
    ERROR_IF_NULL(internal_status);
    capture_control = internal_status->control;
    ERROR_IF_NULL(capture_control);

    dc1394_return = dc1394_dma_single_capture_poll(capture_control);
    if (dc1394_return == DC1394_FAILURE)  /* Not !DC1394_SUCCESS.*/
    {
	DPRINTF("dc1394_dma_single_capture_poll() failed in "
		"camwire_point_next_frame_poll().");
	return(CAMWIRE_FAILURE);
    }
    if (dc1394_return == DC1394_NO_FRAME)
    {
	*buf_ptr = NULL;
	capture_control->num_dma_buffers_behind = 0;
    }
    else
    {
	*buf_ptr = (void *) capture_control->capture_buffer;

	/* Increment the frame number if we have a frame: */
	if (capture_control->drop_frames)
	{
	    missed_frames = capture_control->num_dma_buffers_behind;
	}
	else
	{
	    missed_frames = 0;
	}
	internal_status->frame_number += 1 + missed_frames;
    }

    if (buffer_lag != NULL)
    {
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_framebuffer_lag(c_handle, buffer_lag));
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_unpoint_frame(const Camwire_handle c_handle)
{
    dc1394_cameracapture *capture_control;

    ERROR_IF_NULL(c_handle);
    capture_control = get_capturecontrol(c_handle);
    ERROR_IF_NULL(capture_control);
    ERROR_IF_DC1394_FAIL(
	dc1394_dma_done_with_buffer(capture_control));
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_get_framenumber(const Camwire_handle c_handle, long *framenumber)
{
    User_handle internal_status;

    ERROR_IF_NULL(c_handle);
    internal_status = camwire_bus_get_userdata(c_handle);
    ERROR_IF_NULL(internal_status);
    *framenumber = internal_status->frame_number;
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
/* The estimated timestamp lag (the difference between external trigger
   and DMA buffer filled) has a tolerance of slightly more than half the
   bus cycle period (125 microsecond for IEEE 1394 running at 400 Mb/s)
   because start of transmission has to wait for the next bus cycle. */

int camwire_get_timestamp(const Camwire_handle c_handle,
			  struct timespec *timestamp)
{
    dc1394_cameracapture *capture_control;
    double filltime, timelag, estimate;
    int num_packets;
    float shutter;
    int width, height;
    Camwire_conf config;

    ERROR_IF_NULL(c_handle);
    capture_control = get_capturecontrol(c_handle);
    ERROR_IF_NULL(capture_control);

    filltime = capture_control->filltime.tv_sec +
	capture_control->filltime.tv_usec*1.0e-6;
    
    ERROR_IF_CAMWIRE_FAIL(
	get_numpackets(c_handle, &num_packets));
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_shutter(c_handle, &shutter));
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_frame_size(c_handle, &width, &height));
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_config(c_handle, &config));
    timelag =
	config.trig_setup_time +
	shutter +
	config.line_transfer_time*height +
	config.transmit_setup_time +
	num_packets*config.bus_period;
    
    estimate = filltime - timelag;
    timestamp->tv_sec = estimate;
    timestamp->tv_nsec = (estimate - timestamp->tv_sec)*1.0e9;
    return(CAMWIRE_SUCCESS);
}

/*
  ----------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
/* Camwire_conf is defined in camwire.h.*/

int camwire_get_config(const Camwire_handle c_handle,
				Camwire_conf *cfg)
{
    User_handle internal_status;
    Camwire_id identifier;
    FILE *conffile;

    ERROR_IF_NULL(c_handle);
    internal_status = camwire_bus_get_userdata(c_handle);
    if (internal_status != NULL &&
	internal_status->config_cache == NULL)
    {
	DPRINTF("Null pointer to configuration cache in  "
		"camwire_get_config().");
	return(CAMWIRE_FAILURE);
    }
    
    /* Use cached config if it is available: */
    if (internal_status != NULL &&
	internal_status->config_cache->format != 0)
    {
	memcpy(cfg, internal_status->config_cache,
	       sizeof(Camwire_conf));
    }
    else
    { 	/* Read a conf file and cache it.*/
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_identifier(c_handle, &identifier));
	conffile = find_conf_file(&identifier);
	if (conffile != NULL)
	{
	    ERROR_IF_CAMWIRE_FAIL(
		read_conf_file(conffile, cfg));
	    fclose(conffile);
	    if (internal_status != NULL)
	    {
		memcpy(internal_status->config_cache, cfg,
		       sizeof(Camwire_conf));
	    }
	}
	else
	{
	    fprintf(stderr,
    "\n"
    "Camwire could not find a hardware configuration file.\n"
    "Generating a default configuration...\n");
	    ERROR_IF_CAMWIRE_FAIL(
		generate_default_config(c_handle, cfg));
	    printf(
    "\n"
    "----------------------------------------------------------------\n"
            );
	    ERROR_IF_CAMWIRE_FAIL(
		camwire_print_config(cfg));
	    printf(
    "----------------------------------------------------------------\n"
    "\n"
    "This is a best guess of the camera and its bus's hardware\n"
    "configuration.  See README_conf in the Camwire distribution for\n"
    "details.\n\n"
    "To create a configuration file, copy the text between (but not\n"
    "including) the ----- lines into a file (and edit as needed).\n"
    "The filename must be identical to one of the camera's ID strings\n"
    "(as may be obtained from camwire_get_identifier()) appended by\n"
    "an extension of \".conf\".\n\n"
    "For the current camera suitable filenames are:\n"
    "%s.conf \t(chip)\n"
    "%s.conf \t(model)\n"
    "%s.conf \t(vendor)\n"
    "Camwire checks for filenames like these in this\n"
    "chip-model-vendor order.  It first looks for the three filenames\n"
    "in the current working directory and after that in a directory\n"
    "given by the CAMWIRE_CONF environment variable.\n\n",
            identifier.chip, identifier.model, identifier.vendor);
	    fflush(stdout);
	    return(CAMWIRE_FAILURE);
	}
    }
    
    return(CAMWIRE_SUCCESS);
}

/*
  ----------------------------------------------------------------------
  See camwire.h for documentation on this function.
*/
int camwire_print_config(const Camwire_conf *cfg)
{
    if (printf("Camwire IEEE 1394 IIDC DCAM hardware configuration:\n"
	       "  speed:               %d\n"
	       "  format:              %d\n"
	       "  mode:                %d\n"
	       "  max_packets:         %d\n"
	       "  min_pixels:          %d\n"
	       "  bus_period:          %g\n"
	       "  blue_gain_norm:      %g\n"
	       "  red_gain_norm:       %g\n"
	       "  trig_setup_time:     %g\n"
	       "  exposure_quantum:    %g\n"
	       "  exposure_offset:     %g\n"
	       "  line_transfer_time:  %g\n"
	       "  transmit_setup_time: %g\n"
	       "  drop_frames:         %d\n"
	       "  dma_device_name:     %s\n",
	       cfg->speed,
	       cfg->format,
	       cfg->mode,
	       cfg->max_packets,
	       cfg->min_pixels,
	       cfg->bus_period,
	       cfg->blue_gain_norm,
	       cfg->red_gain_norm,
	       cfg->trig_setup_time,
	       cfg->exposure_quantum,
	       cfg->exposure_offset,
	       cfg->line_transfer_time,
	       cfg->transmit_setup_time,
	       cfg->drop_frames,
	       cfg->dma_device_name) < 1)
    {
	return(CAMWIRE_FAILURE);
    }
    fflush(stdout);
    return(CAMWIRE_SUCCESS);
}

/*
  ----------------------------------------------------------------------
  Reads configuration from the given conf file into the given
  configuration structure.  Returns CAMWIRE_SUCCESS on success or
  CAMWIRE_FAILURE on failure.
*/
static int read_conf_file(FILE *conffile,
			    Camwire_conf *cfg)
{
    int scan_result;
    int buflength;
    
    scan_result =
	fscanf(conffile,
	       "Camwire IEEE 1394 IIDC DCAM hardware configuration:\n"
	       "  speed:               %d\n"
	       "  format:              %d\n"
	       "  mode:                %d\n"
	       "  max_packets:         %d\n"
	       "  min_pixels:          %d\n"
	       "  bus_period:          %f\n"
	       "  blue_gain_norm:      %f\n"
	       "  red_gain_norm:       %f\n"
	       "  trig_setup_time:     %f\n"
	       "  exposure_quantum:    %f\n"
	       "  exposure_offset:     %f\n"
	       "  line_transfer_time:  %f\n"
	       "  transmit_setup_time: %f\n"
	       "  drop_frames:         %d\n"
	       "  dma_device_name:     ",
	       &cfg->speed,
	       &cfg->format,
	       &cfg->mode,
	       &cfg->max_packets,
	       &cfg->min_pixels,
	       &cfg->bus_period,
	       &cfg->blue_gain_norm,
	       &cfg->red_gain_norm,
	       &cfg->trig_setup_time,
	       &cfg->exposure_quantum,
	       &cfg->exposure_offset,
	       &cfg->line_transfer_time,
	       &cfg->transmit_setup_time,
	       &cfg->drop_frames);
    if (scan_result == EOF || scan_result != 14)
    {
	DPRINTF("fscanf() failed reading configuration file in "
		"read_conf().");
	return(CAMWIRE_FAILURE);
    }

    if (fgets(cfg->dma_device_name, CAMWIRE_CONF_DMA_DEVICE_MAX_CHARS,
	      conffile) == NULL)
    {
	cfg->dma_device_name[0] = '\0';
    }
    else
    {
	buflength = strlen(cfg->dma_device_name);
	while (buflength > 0 &&
	       isspace(cfg->dma_device_name[buflength-1]))
	{
	    cfg->dma_device_name[buflength-1] = '\0';
	    buflength = strlen(cfg->dma_device_name);
	}
    }

    return(CAMWIRE_SUCCESS);
}

/*
  ----------------------------------------------------------------------
  Attempts to open a configuration file for reading.  Returns the stream
  pointer on success or NULL on failure.
*/
static FILE * find_conf_file(const Camwire_id *id)
{
    FILE *conffile;
    char *env_directory;

    conffile = open_named_conf_file(NULL, id->chip);
    if (conffile != NULL)  return(conffile);

    conffile = open_named_conf_file(NULL, id->model);
    if (conffile != NULL)  return(conffile);

    conffile = open_named_conf_file(NULL, id->vendor);
    if (conffile != NULL)  return(conffile);

    env_directory = getenv(ENVIRONMENT_VAR_CONF);
    if (env_directory != NULL)
    {
	conffile = open_named_conf_file(env_directory, id->chip);
	if (conffile != NULL)  return(conffile);

	conffile = open_named_conf_file(env_directory, id->model);
	if (conffile != NULL)  return(conffile);

	conffile = open_named_conf_file(env_directory, id->vendor);
	if (conffile != NULL)  return(conffile);
    }
    return(NULL);
}

/*
  ----------------------------------------------------------------------
  Attempts to open the named configuration file for reading after
  appending the configuration filename extension.  Returns the stream
  pointer on success or NULL on failure.
*/
static FILE * open_named_conf_file(const char *path,
				     const char *filename)
{
    char conffilename[CONFFILE_NAME_MAX_CHARS+1];

    conffilename[0] = '\0';
    if (path != NULL)
    {
	strncat(conffilename, path, CONFFILE_PATH_MAX_CHARS);
	if (conffilename[strlen(conffilename)-1] != '/')
	{
	    strncat(conffilename, "/", 1);
	}
    }
    strncat(conffilename, filename, CAMWIRE_ID_MAX_CHARS);
    strncat(conffilename, CONFFILE_EXTENSION,
	    CONFFILE_EXTENSION_MAX_CHARS);
    return(fopen(conffilename, "r"));
}

/*
  ----------------------------------------------------------------------
  Queries the camera for supported features and attempts to create a
  sensible default configuration.  Returns CAMWIRE_SUCCESS on success or
  CAMWIRE_FAILURE on failure.
*/
/* The IEEE 1394 IIDC digital camera (DCAM) specification gives camera
   manufacturers a choice of several predefined resolutions called
   "formats":

   Format 0 is for 160x120 up to 640x480 (VGA).
   Format 1 is for 800x600 up to 1024x768 (SVGA).
   Format 2 is for 1280x960 up to 1600x1200 (XVGA).
   Format 6 is "memory channels" or still images.
   Format 7 is for scalable image sizes.

   Format 0 and Format 7 have so far been implemented in Camwire, the
   others are not yet supported. */

static int generate_default_config(const Camwire_handle c_handle,
				   Camwire_conf *cfg)
{
    quadlet_t bit_field;
    dc1394bool_t has_white_balance;
    dc1394_feature_info capability;
    unsigned int blue, red;

    /* Initialize the camera to factory settings: */
    /* ERROR_IF_NULL(c_handle); */
    ERROR_IF_DC1394_FAIL(
	dc1394_init_camera(camwire_bus_get_port(c_handle),
			   camwire_bus_get_node(c_handle)));
  
    /* Determine the highest supported format: */
    ERROR_IF_DC1394_FAIL(
	dc1394_query_supported_formats(camwire_bus_get_port(c_handle),
				       camwire_bus_get_node(c_handle),
				       &bit_field));
    if ((bit_field & 0xFF000000) == 0)
    {
	DPRINTF("dc1394_query_supported_formats() returned null format "
		"bitfield in generate_default_config().");
	return(CAMWIRE_FAILURE);
    }
    if ((bit_field & 0x01000000) != 0)
    {
	cfg->format = FORMAT_SCALABLE_IMAGE_SIZE; 	/* Format 7.*/
    }
    else if ((bit_field & 0x80000000) != 0)
    {
	cfg->format = FORMAT_VGA_NONCOMPRESSED; 	/* Format 0.*/
    }
    else
    {
	DPRINTF("Camera's available formats are not supported in "
		"generate_default_config().");
	return(CAMWIRE_FAILURE);
    }

    /* Determine the highest supported mode in this format: */
    ERROR_IF_DC1394_FAIL(
	dc1394_query_supported_modes(camwire_bus_get_port(c_handle),
				     camwire_bus_get_node(c_handle),
				     cfg->format,
				     &bit_field));
    if ((bit_field & 0xFF000000) == 0)
    {
	DPRINTF("dc1394_query_supported_modes() returned null mode "
		"bitfield in generate_default_config().");
	return(CAMWIRE_FAILURE);
    }
    if (cfg->format == FORMAT_VGA_NONCOMPRESSED)
    { 	/* Format 0.*/
	if ((bit_field & 0x02000000) != 0)
	{
	    cfg->mode = MODE_640x480_MONO16; 	/* Mode 6.*/
	}
	else if ((bit_field & 0x04000000) != 0)
	{
	    cfg->mode = MODE_640x480_MONO; 	/* Mode 5.*/
	}
	else if ((bit_field & 0x08000000) != 0)
	{
	    cfg->mode = MODE_640x480_RGB; 	/* Mode 4.*/
	}
	else if ((bit_field & 0x10000000) != 0)
	{
	    cfg->mode = MODE_640x480_YUV422; 	/* Mode 3.*/
	}
	else if ((bit_field & 0x20000000) != 0)
	{
	    cfg->mode = MODE_640x480_YUV411; 	/* Mode 2.*/
	}
	else if ((bit_field & 0x40000000) != 0)
	{
	    cfg->mode = MODE_320x240_YUV422; 	/* Mode 1.*/
	}
	else if ((bit_field & 0x80000000) != 0)
	{
	    cfg->mode = MODE_160x120_YUV444; 	/* Mode 0.*/
	}
	else
	{
	    DPRINTF("Camera's available modes are not supported in "
		    "generate_default_config().");
	    return(CAMWIRE_FAILURE);
	}
    }
    else if (cfg->format == FORMAT_SCALABLE_IMAGE_SIZE)
    { 	/* Format 7.*/
	if ((bit_field & 0x01000000) != 0)
	{
	    cfg->mode = MODE_FORMAT7_7; 	/* Mode 7.*/
	}
	else if ((bit_field & 0x02000000) != 0)
	{
	    cfg->mode = MODE_FORMAT7_6; 	/* Mode 6.*/
	}
	else if ((bit_field & 0x04000000) != 0)
	{
	    cfg->mode = MODE_FORMAT7_5; 	/* Mode 5.*/
	}
	else if ((bit_field & 0x08000000) != 0)
	{
	    cfg->mode = MODE_FORMAT7_4; 	/* Mode 4.*/
	}
	else if ((bit_field & 0x10000000) != 0)
	{
	    cfg->mode = MODE_FORMAT7_3; 	/* Mode 3.*/
	}
	else if ((bit_field & 0x20000000) != 0)
	{
	    cfg->mode = MODE_FORMAT7_2; 	/* Mode 2.*/
	}
	else if ((bit_field & 0x40000000) != 0)
	{
	    cfg->mode = MODE_FORMAT7_1; 	/* Mode 1.*/
	}
	else if ((bit_field & 0x80000000) != 0)
	{
	    cfg->mode = MODE_FORMAT7_0; 	/* Mode 0.*/
	}
	else
	{
	    DPRINTF("Camera's available modes are not supported in "
		    "generate_default_config().");
	    return(CAMWIRE_FAILURE);
	}
    }
    else
    {
	DPRINTF("Internal error in generate_default_config().");
	return(CAMWIRE_FAILURE);
    }

    /* Some default values (may be overwritten below): */
    cfg->max_packets = 4095;
    cfg->min_pixels = 4096;
    cfg->blue_gain_norm = 64;
    cfg->red_gain_norm = 64;
    cfg->trig_setup_time = 0.0;
    cfg->exposure_quantum = 20e-6;
    cfg->exposure_offset = 0.0;
    cfg->line_transfer_time = 0.0;
    cfg->transmit_setup_time = 0.0;
    
    cfg->speed = SPEED_400;
    switch (cfg->speed)
    {
	case SPEED_100:
	    cfg->bus_period = 500e-6;
	    break;
	case SPEED_200:
	    cfg->bus_period = 250e-6;
	    break;
	case SPEED_400:
	    cfg->bus_period = 125e-6;
	    break;
	default:
	    DPRINTF("Bus speed is not a legal enum value in "
		    "generate_default_config().");
	    return(CAMWIRE_FAILURE);
    }
    cfg->drop_frames = 0;
    cfg->dma_device_name[0] = '\0'; 	/* Use default.*/

    /* Try to guess sensible values from camera's registers: */
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_WHITE_BALANCE,
				  &has_white_balance));
    if (has_white_balance == DC1394_TRUE)
    {
	capability.feature_id = FEATURE_WHITE_BALANCE;
	ERROR_IF_DC1394_FAIL(
	    dc1394_get_camera_feature(camwire_bus_get_port(c_handle),
				      camwire_bus_get_node(c_handle),
				      &capability));
	if (capability.readout_capable == DC1394_TRUE)
	{
	    ERROR_IF_DC1394_FAIL(
		dc1394_get_white_balance(camwire_bus_get_port(c_handle),
					 camwire_bus_get_node(c_handle),
					 &blue, &red));
	    if (blue != 0 && red != 0)
	    {
		cfg->blue_gain_norm = blue;  /* Default gain of 1.0.*/
		cfg->red_gain_norm = red;
	    }
	}
    }

    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  Returns the frame rate corresponding to the given number of packets
  per frame.
*/
static double convert_numpackets2framerate(const Camwire_handle c_handle,
				    const int num_packets)
{
    Camwire_conf config;
    int actual;
    
    if (camwire_get_config(c_handle, &config) != CAMWIRE_SUCCESS)
    {
	DPRINTF("camwire_get_config() failed in "
		"convert_numpackets2framerate().");
    }

#ifdef CAMWIRE_DEBUG
    if (config.format == 0)
    {
	DPRINTF("camwire_get_config() returned null format in "
		"convert_numpackets2framerate()");
    }
    else if (config.format != FORMAT_SCALABLE_IMAGE_SIZE)
    { 	/* Not Format 7.*/
	DPRINTF("Camera is not in Format 7 in "
		"convert_numpackets2framerate().");
    }
#endif
    
    actual = num_packets;
    if (actual < 1)  actual = 1;
    if (actual > config.max_packets)  actual = config.max_packets;
    return(1.0/(config.bus_period*actual));
}

/*
  ----------------------------------------------------------------------
  Queries the camera for supported features and attempts to create
  sensible default settings.  Note that the camera itself is initialized
  to factory settings in the process.  Returns CAMWIRE_SUCCESS on
  success or CAMWIRE_FAILURE on failure.
*/
static int generate_default_settings(const Camwire_handle c_handle,
				     Camwire_state *set)
{
    Camwire_conf config;
    quadlet_t bit_field;
    int dc1394_return;
    int num_packets;
    unsigned int color_id, shutter_reg;
    dc1394bool_t has_shutter, has_trigger, has_polarity, polarity;
    double max_shutter;
    
    /* Initialize the camera to factory settings: */
    /* ERROR_IF_NULL(c_handle); */
    dc1394_return = dc1394_init_camera(camwire_bus_get_port(c_handle),
				       camwire_bus_get_node(c_handle));
    if (dc1394_return != DC1394_SUCCESS)
    {
	DPRINTF("dc1394_init_camera() failed in "
		"generate_default_settings().");
	return(CAMWIRE_FAILURE);
    }

    /* Get the format and mode: */
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_config(c_handle, &config));

    /* Format and mode-specific frame dimensions: */
    if (config.format == FORMAT_VGA_NONCOMPRESSED)
    { 	/* Format 0:*/
	set->left = 0; 			/* Pixels.*/
	set->top = 0; 			/* Pixels.*/
	switch (config.mode)
	{
	    case MODE_160x120_YUV444:
		set->width = 160; 		/* Pixels.*/
		set->height = 120; 		/* Pixels.*/
		break;
	    case MODE_320x240_YUV422:
		set->width = 320; 		/* Pixels.*/
		set->height = 240; 		/* Pixels.*/
		break;
	    default:
		set->width = 640; 		/* Pixels.*/
		set->height = 480; 		/* Pixels.*/
		break;
	}
	set->coding = convert_mode2pixelcoding(config.mode);
    }
    /*
    else if (config.format == FORMAT_SVGA_NONCOMPRESSED_1)
    { 	// Format 1.
	set->left = 0; 			// Pixels.
	set->top = 0; 			// Pixels.
	switch (config.mode)
	{
	    case MODE_800x600_YUV422:
	    case MODE_800x600_RGB:
	    case MODE_800x600_MONO:
	    case MODE_800x600_MONO16:
		set->width = 800;
		set->height = 600;
		break;
	    default:
		set->width = 1024;
		set->height = 768;
		break;
	}
	set->coding = convert_mode2pixelcoding(config.mode);
    }
    */
    /*
    else if (config.format == FORMAT_SVGA_NONCOMPRESSED_2)
    { 	// Format 2.
	set->left = 0; 			// Pixels.
	set->top = 0; 			// Pixels.
	switch (config.mode)
	{
	    case MODE_1280x960_YUV422:
	    case MODE_1280x960_RGB:
	    case MODE_1280x960_MONO:
	    case MODE_1280x960_MONO16:
		set->width = 1280;
		set->height = 960;
		break;
	    default:
		set->width = 1600;
		set->height = 1200;
		break;
	}
	set->coding = convert_mode2pixelcoding(config.mode);
    }
    */
    else if (config.format == FORMAT_SCALABLE_IMAGE_SIZE)
    { 	/* Format 7:*/
	ERROR_IF_DC1394_FAIL(
	    dc1394_query_format7_image_position(camwire_bus_get_port(c_handle),
						camwire_bus_get_node(c_handle),
						config.mode,
						&set->left,
						&set->top));
	ERROR_IF_DC1394_FAIL(
	    dc1394_query_format7_image_size(camwire_bus_get_port(c_handle),
					    camwire_bus_get_node(c_handle),
					    config.mode,
					    &set->width,
					    &set->height));
	ERROR_IF_DC1394_FAIL(
	    dc1394_query_format7_color_coding_id(
		camwire_bus_get_port(c_handle),
		camwire_bus_get_node(c_handle),
		config.mode,
		&color_id));
	set->coding = convert_colorid2pixelcoding(color_id);
    }
    else
    {
	DPRINTF("Camera's format is not supported in "
		"generate_default_settings().");
	return(CAMWIRE_FAILURE);
    }
    
    /* Determine the maximum supported framerate in this mode and
       format: */
    if (config.format == FORMAT_VGA_NONCOMPRESSED /*||
	config.format == FORMAT_SVGA_NONCOMPRESSED_1 ||
	config.format == FORMAT_SVGA_NONCOMPRESSED_2*/ )
    {
	ERROR_IF_DC1394_FAIL(
	    dc1394_query_supported_framerates(camwire_bus_get_port(c_handle),
					      camwire_bus_get_node(c_handle),
					      config.format, config.mode,
					      &bit_field));
	if ((bit_field & 0xFF000000) == 0)
	{
	    DPRINTF("dc1394_query_supported_framerates() returned null "
		    "framerate field in generate_default_settings().");
	    return(CAMWIRE_FAILURE);
	}
	if ((bit_field & 0x01000000) != 0)
	{
	    set->frame_rate = 240;
	}
	else if ((bit_field & 0x02000000) != 0)
	{
	    set->frame_rate = 120;
	}
	else if ((bit_field & 0x04000000) != 0)
	{
	    set->frame_rate = 60;
	}
	else if ((bit_field & 0x08000000) != 0)
	{
	    set->frame_rate = 30;
	}
	else if ((bit_field & 0x10000000) != 0)
	{
	    set->frame_rate = 15;
	}
	else if ((bit_field & 0x20000000) != 0)
	{
	    set->frame_rate = 7.5;
	}
	else if ((bit_field & 0x40000000) != 0)
	{
	    set->frame_rate = 3.75;
	}
	else if ((bit_field & 0x80000000) != 0)
	{
	    set->frame_rate = 1.875;
	}
	else
	{
	    DPRINTF("Camera's available framerates are not supported in "
		    "generate_default_settings().");
	    return(CAMWIRE_FAILURE);
	}
    }
    else if (config.format == FORMAT_SCALABLE_IMAGE_SIZE)
    {
	set->frame_rate = 15.0; 		/* Fps, default.*/
	dc1394_return = dc1394_query_format7_packet_per_frame(
	    camwire_bus_get_port(c_handle),
	    camwire_bus_get_node(c_handle),
	    config.mode,
	    &num_packets);
	if (dc1394_return != DC1394_SUCCESS)
	{
	    DPRINTF("dc1394_query_format7_packet_per_frame() failed in "
		    "generate_default_settings().");
	}
	else if (num_packets == 0)
	{
	    DPRINTF("dc1394_query_format7_packet_per_frame() returned null "
		    "number of packets in generate_default_settings().");
	}
	else
	{
	    set->frame_rate =
		convert_numpackets2framerate(c_handle, num_packets);
	}
    }
    else
    {
	DPRINTF("Camera's format is not supported in "
		"generate_default_settings().");
	return(CAMWIRE_FAILURE);
    }

    /* Get the shutter speed and check that it fits into one frame
       period:*/
    set->shutter = 0.5/set->frame_rate; /* Seconds, default.*/
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_SHUTTER,
				  &has_shutter));
    if (has_shutter == DC1394_TRUE)
    {
	ERROR_IF_DC1394_FAIL(
	    dc1394_get_shutter(camwire_bus_get_port(c_handle),
			       camwire_bus_get_node(c_handle),
			       &shutter_reg));
	set->shutter =
	    config.exposure_offset + shutter_reg*config.exposure_quantum;
	
	max_shutter = config.exposure_quantum *
	    ((unsigned int) (1.0/(set->frame_rate*config.exposure_quantum)));
	if (set->shutter > max_shutter)  set->shutter = max_shutter;
    }
    else
    {
	DPRINTF("dc1394_is_feature_present() reports no shutter in"
		"generate_default_settings().");
    }
    
    /* Format and mode-independent settings: */
    set->external_trigger = 0; 		/* Flag.*/
    set->trigger_polarity = 1; 		/* Flag, default.*/
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_TRIGGER,
				  &has_trigger));
    if (has_trigger == DC1394_TRUE)
    {
	ERROR_IF_DC1394_FAIL(
	    dc1394_trigger_has_polarity(camwire_bus_get_port(c_handle),
					camwire_bus_get_node(c_handle),
					&has_polarity));
	if (has_polarity == DC1394_TRUE)
	{
	    ERROR_IF_DC1394_FAIL(
		dc1394_get_trigger_polarity(camwire_bus_get_port(c_handle),
					    camwire_bus_get_node(c_handle),
					    &polarity));
	    if (polarity == DC1394_TRUE)  set->trigger_polarity = 1;
	    else                          set->trigger_polarity = 0;
	}
    }
    set->blue_gain = 1.0;		/* Units.*/
    set->red_gain = 1.0;		/* Units.*/
    set->num_frame_buffers = 10; 	/* Frames.*/
    set->single_shot = 0; 		/* Flag.*/
    set->running = 0; 			/* Flag.*/
    set->shadow = 1; 			/* Flag.*/

    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  Returns the pixel coding given the libdc1394 mode in Format 0.
*/
static Camwire_pixel convert_mode2pixelcoding(const unsigned int mode)
{
    switch (mode)
    {
	case MODE_160x120_YUV444:
	    return(CAMWIRE_PIXEL_YUV444);  /* 24 bits/pixel.*/
	    break;
	case MODE_320x240_YUV422:
	    return(CAMWIRE_PIXEL_YUV422);  /* 16 bits/pixel.*/
	    break;
	case MODE_640x480_YUV411:
	    return(CAMWIRE_PIXEL_YUV411);  /* 12 bits/pixel.*/
	    break;
	case MODE_640x480_YUV422:
	    return(CAMWIRE_PIXEL_YUV422);  /* 16 bits/pixel.*/
	    break;
	case MODE_640x480_RGB:
	    return(CAMWIRE_PIXEL_RGB8);  /* 24 bits/pixel.*/
	    break;
	case MODE_640x480_MONO:
	    return(CAMWIRE_PIXEL_MONO8);  /* 8 bits/pixel.*/
	    break;
	case MODE_640x480_MONO16:
	    return(CAMWIRE_PIXEL_MONO16);  /* 16 bits/pixel.*/
	    break;
	default:
	    return(CAMWIRE_PIXEL_INVALID);  /* Unknown.*/
	    break;
    }
}

/*
  -----------------------------------------------------------------------------
   Returns the pixel coding given the libdc1394 colour coding ID in
   Format 7.
*/
static Camwire_pixel convert_colorid2pixelcoding(const int color_id)
{
    switch (color_id)
    {
	case COLOR_FORMAT7_MONO8:
	    return(CAMWIRE_PIXEL_MONO8);  /* 8 bits/pixel.*/
	    break;
	case COLOR_FORMAT7_YUV411:
	    return(CAMWIRE_PIXEL_YUV411);  /* 12 bits/pixel.*/
	    break;
	case COLOR_FORMAT7_YUV422:
	    return(CAMWIRE_PIXEL_YUV422);  /* 16 bits/pixel.*/
	    break;
	case COLOR_FORMAT7_YUV444:
	    return(CAMWIRE_PIXEL_YUV444);  /* 24 bits/pixel.*/
	    break;
	case COLOR_FORMAT7_RGB8:
	    return(CAMWIRE_PIXEL_RGB8);  /* 24 bits/pixel.*/
	    break;
	case COLOR_FORMAT7_MONO16:
	    return(CAMWIRE_PIXEL_MONO16);  /* 16 bits/pixel.*/
	    break;
	case COLOR_FORMAT7_RGB16:
	    return(CAMWIRE_PIXEL_RGB16);  /* 48 bits/pixel.*/
	    break;
	    /*
	case COLOR_FORMAT7_MONO16S:
	    return(CAMWIRE_PIXEL_MONO16S);  // 16 bits/pixel.
	    break;
	case COLOR_FORMAT7_RGB16S:
	    return(CAMWIRE_PIXEL_RGB16S);  // 48 bits/pixel.
	    break;
	case COLOR_FORMAT7_RAW8:
	    return(CAMWIRE_PIXEL_RAW8);  // 8 bits/pixel.
	    break;
	case COLOR_FORMAT7_RAW16:
	    return(CAMWIRE_PIXEL_RAW16);  // 16 bits/pixel.
	    break;
	    */
	default:
	    return(CAMWIRE_PIXEL_INVALID);  /* Not supported.*/
	    break;
    }
}

/*
  -----------------------------------------------------------------------------
  Does the actual work of camwire_create() and
  camwire_create_from_struct(), after they have initialized the camera
  to factory settings and sorted out where the start-up settings come
  from.
*/

static int create(const Camwire_handle c_handle, const Camwire_state *set)
{
    User_handle internal_status;
    Camwire_conf config;

    /* Allocate zero-filled space for internal status, and register a
       pointer to it in the camera handle: */
    internal_status = (struct camwire_user_data *)
	calloc(1, sizeof(struct camwire_user_data));
    /* struct camwire_user_data is defined above.*/
    /* Note that internal_status is zero-filled by calloc().  Other
       functions may use this fact (and the existence of a pointer to
       it) to check if it has been initialized or not. */
    ERROR_IF_NULL(internal_status); 	/* Allocation failure.*/
    if (!camwire_bus_set_userdata(c_handle, internal_status))
    {   /* Already in use.*/
	DPRINTF("camwire_bus_set_userdata() failed in create().");
	free(internal_status);
	return(CAMWIRE_FAILURE);
    }

    /* Allocate zero-filled space for the libdc1394 cameracapture
       structure: */
    internal_status->control =
	(dc1394_cameracapture *) calloc(1, sizeof(dc1394_cameracapture));
    if (internal_status->control == NULL)
    { 	/* Allocation failure.*/
	DPRINTF("calloc(dc1394_cameracapture) failed in create().");
	free_internals(c_handle);
	return(CAMWIRE_FAILURE);
    }

    /* Allocate zero-filled space for the config cache: */
    internal_status->config_cache =
	(Camwire_conf *) calloc(1, sizeof(Camwire_conf));
    if (internal_status->config_cache == NULL)
    { 	/* Allocation failure.*/
	DPRINTF("calloc(Camwire_conf) failed in create().");
	free_internals(c_handle);
	return(CAMWIRE_FAILURE);
    }

    /* Allocate space and make a copy of the initial settings: */
    internal_status->current_set =
	(Camwire_state *) malloc(sizeof(Camwire_state));
    if (internal_status->current_set == NULL)
    { 	/* Allocation failure.*/
	DPRINTF("malloc(Camwire_state) failed in create().");
	free_internals(c_handle);
	return(CAMWIRE_FAILURE);
    }
    memcpy(internal_status->current_set, set, sizeof(Camwire_state));

    /* Get 1394-specific hardware configuration: */
    if (camwire_get_config(c_handle, &config) != CAMWIRE_SUCCESS)
    {
	DPRINTF("camwire_get_config() failed in create().");
	free_internals(c_handle);
	return(CAMWIRE_FAILURE);
    }

    /* Make sure the video1394 device is not already listening on this
       channel (due to a provious process being killed without resetting
       the camera).  Hopefully this won't hurt anything: */
    dc1394_dma_unlisten(camwire_bus_get_port(c_handle),
			internal_status->control);

    /* Connect the camera to the bus and initialize it with our
       settings: */
    if (connect(c_handle, &config, set) != CAMWIRE_SUCCESS)
    {
	DPRINTF("connect() failed in create().");
	free_internals(c_handle);
	return(CAMWIRE_FAILURE);
    }

    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  Connects the camera to the bus and sets it to the given configuration
  and initial settings.  Returns CAMWIRE_SUCCESS on success or
  CAMWIRE_FAILURE on failure.  The function disconnect() must be called
  when done to free the allocated memory.
*/
static int connect(const Camwire_handle c_handle, Camwire_conf *cfg,
		   const Camwire_state *set)
{
    User_handle internal_status;
    int frame_rate_index;
    int num_packets, packet_size;
    char *dma_device_file;
    int depth;
    quadlet_t bit_field; 	/* Unsigned 32-bit int (libraw1394).*/
    unsigned int color_id;
    double actual_frame_rate;
    Camwire_pixel actual_coding;
    Camwire_state *shadow_state;
    
    internal_status = camwire_bus_get_userdata(c_handle);
    ERROR_IF_NULL(internal_status);
    ERROR_IF_NULL(internal_status->control);

    if (cfg->dma_device_name[0] == '\0')
    {
	dma_device_file = NULL;
    }
    else
    {
	dma_device_file = cfg->dma_device_name;
    }
    
    /* If dc1394_dma_release_camera() is called without a preceding
       successful call to dc1394_dma_setup[_format7]_capture(),
       libdc1394 gets into a tangled state.  That is why we have to keep
       track with the camera_connected flag, and check it in
       disconnect(): */
    internal_status->camera_connected = 0;

    ERROR_IF_ZERO(cfg->format);
    if (cfg->format == FORMAT_VGA_NONCOMPRESSED)
    { 	/* Format 0.*/
	frame_rate_index = convert_framerate2index(set->frame_rate);

	ERROR_IF_DC1394_FAIL(
	    dc1394_dma_setup_capture(camwire_bus_get_port(c_handle),
				     camwire_bus_get_node(c_handle),
				     /* Set channel = nodeid:*/
				     camwire_bus_get_node(c_handle),
				     cfg->format,
				     cfg->mode,
				     cfg->speed,
				     frame_rate_index,
				     set->num_frame_buffers,
				     cfg->drop_frames,
				     dma_device_file,
				     internal_status->control));
	actual_coding = convert_mode2pixelcoding(cfg->mode);
	actual_frame_rate = convert_index2framerate(frame_rate_index);
    }
    else if (cfg->format == FORMAT_SCALABLE_IMAGE_SIZE)
    { 	/* Format 7.*/
	/* Prevent a segfault due to kalloc() bug in dma.c of the
	   linux1394 system.  This ought to be removed for later
	   versions: */
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_pixel_depth(set->coding, &depth));
	/*
	if ((long) set->num_frame_buffers *
	    set->width*set->height*depth > 8*IEEE1394_KALLOC_LIMIT)
	{
	    DPRINTF("Trying to allocate too large a DMA buffer in connect().");
	    return(CAMWIRE_FAILURE);
	}
	*/

	/* Set up the color_coding_id before calling
	   dc1394_dma_setup_format7_capture(), otherwise the wrong DMA
	   buffer size may be allocated: */
	ERROR_IF_DC1394_FAIL(
	    dc1394_query_format7_color_coding(
		camwire_bus_get_port(c_handle),
		camwire_bus_get_node(c_handle),
		cfg->mode,
		&bit_field));
	if (bit_field == 0)
	{
	    DPRINTF("dc1394_query_format7_color_coding() returned a null "
		    "bitfield in connect().");
	    return(CAMWIRE_FAILURE);
	}
	color_id = convert_pixelcoding2colorid(set->coding, bit_field);
	if (color_id == 0)
	{
	    DPRINTF("Pixel colour coding is invalid or not supported  by the "
		    "camera in connect().");
	    return(CAMWIRE_FAILURE);
	}
	ERROR_IF_DC1394_FAIL(
	    dc1394_set_format7_color_coding_id(
		camwire_bus_get_port(c_handle),
		camwire_bus_get_node(c_handle),
		cfg->mode,
		color_id));
	actual_coding = convert_colorid2pixelcoding(color_id);

	/* Calculate the packet size from the wanted frame rate: */
	num_packets =
	    convert_framerate2numpackets(c_handle, set->frame_rate);
	packet_size = convert_numpackets2packetsize(c_handle,
						    num_packets,
						    set->width,
						    set->height,
						    set->coding);

	/* Set up the camera and DMA buffers: */
	ERROR_IF_DC1394_FAIL(
	    dc1394_dma_setup_format7_capture(
		camwire_bus_get_port(c_handle),
		camwire_bus_get_node(c_handle),
		/* Set channel = nodeid: */
		camwire_bus_get_node(c_handle),
		cfg->mode,
		cfg->speed,
		packet_size,
		set->left, set->top,
		set->width, set->height,
		set->num_frame_buffers,
		cfg->drop_frames,
		dma_device_file,
		internal_status->control));
	num_packets = convert_packetsize2numpackets(c_handle,
						    packet_size,
						    set->width,
						    set->height,
						    actual_coding);
	actual_frame_rate =
	    convert_numpackets2framerate(c_handle, num_packets);
    }
    else
    {
	DPRINTF("Unsupported camera format in connect().");
	return(CAMWIRE_FAILURE);
    }
    internal_status->camera_connected = 1;

    /* Update shadow state not done by camwire_set_...() calls below: */
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    shadow_state->left = set->left;
    shadow_state->top = set->top;
    shadow_state->width = set->width;
    shadow_state->height = set->height;
    shadow_state->coding = actual_coding;
    shadow_state->frame_rate = actual_frame_rate;

    /* Initialize camera registers not already done by
       dc1394_dma_setup_...() and update shadow state of these: */
    ERROR_IF_CAMWIRE_FAIL(
	set_non_dma_registers(c_handle, set));
    return(CAMWIRE_SUCCESS);
}


/*
  -----------------------------------------------------------------------------
  Initialize camera registers not already done by dc1394_dma_setup_...()
  and update their shadow state.  Note that the order of register writes
  may be significant for some cameras after power-up or reset/initilize.
*/
static int set_non_dma_registers(const Camwire_handle c_handle,
				 const Camwire_state *set)
{
    dc1394bool_t has_trigger, has_polarity, has_shutter, has_white_balance;
    Camwire_state *shadow_state;
    
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);

    /* Update the state shadow flag: */
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_set_stateshadow(c_handle, set->shadow));

    /* Set up default shadow states for settings which may not be
       supported by the camera.  These may be overwritten below.  The
       rest of the shadow states will be updated within each
       camwire_set_...() call: */
    shadow_state->trigger_polarity = set->trigger_polarity;
    shadow_state->external_trigger = set->external_trigger;
    shadow_state->shutter = set->shutter;
    shadow_state->blue_gain = set->blue_gain;
    shadow_state->red_gain = set->red_gain;

    /* Trigger source and polarity: */
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_TRIGGER,
				  &has_trigger));
    if (has_trigger == DC1394_TRUE)
    {
	ERROR_IF_DC1394_FAIL(
	    dc1394_set_trigger_mode(camwire_bus_get_port(c_handle),
				    camwire_bus_get_node(c_handle),
				    TRIGGER_MODE_0)); 	/* Edge triggered.*/
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_set_trigger_source(c_handle, set->external_trigger));
	ERROR_IF_DC1394_FAIL(
	    dc1394_trigger_has_polarity(camwire_bus_get_port(c_handle),
					camwire_bus_get_node(c_handle),
					&has_polarity));
	if (has_polarity == DC1394_TRUE)
	{
	    ERROR_IF_CAMWIRE_FAIL(
		camwire_set_trigger_polarity(c_handle, set->trigger_polarity));
	}
    }

    /* Shutter: */
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_SHUTTER,
				  &has_shutter));
    if (has_shutter == DC1394_TRUE)
    {
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_set_shutter(c_handle, set->shutter));
    }

    /* White balance: */
    ERROR_IF_DC1394_FAIL(
	dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				  camwire_bus_get_node(c_handle),
				  FEATURE_WHITE_BALANCE,
				  &has_white_balance));
    if (has_white_balance == DC1394_TRUE)
    {
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_set_white_balance(c_handle, set->blue_gain,
				      set->red_gain));
    }

    /* The rest: */
    ERROR_IF_CAMWIRE_FAIL(
	camwire_set_single_shot(c_handle, set->single_shot));
    ERROR_IF_CAMWIRE_FAIL(
	camwire_set_run_stop(c_handle, set->running));

    /* The list of settings updated above does not include
       camwire_set_frame_size(), camwire_set_pixel_coding(), or
       camwire_set_framerate() because these are already set in
       dc1394_dma_setup_..() and because they could cause infinite
       recursion since they themselves contain calls to (re)connect()
       which call this function.  camwire_set_frame_offset() is a bit
       different in that it is set up with dc1394_dma_setup_..() but
       does not require a reconnect() when it changes. */

    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  Disconnects the camera from the bus and frees memory allocated in
  connect().  The camera should be stopped before calling this
  function.
*/
static void disconnect(const Camwire_handle c_handle)
{
    User_handle internal_status;
    
    internal_status = camwire_bus_get_userdata(c_handle);
    if (internal_status != NULL)
    {
	if (internal_status->control != NULL &&
	    internal_status->camera_connected)
	{
	    dc1394_dma_unlisten(camwire_bus_get_port(c_handle),
				internal_status->control);
	    dc1394_dma_release_camera(camwire_bus_get_port(c_handle),
				      internal_status->control);
	}
	internal_status->camera_connected = 0;
    }
}

/*
  -----------------------------------------------------------------------------
  Disconnects the camera from and connects it to the bus.  Any changes
  in the cfg and set arguments take effect.  This function is used maily
  to re-initialize the video1394 driver interface for things like
  flushing the frame buffers or changing the frame dimensions or frame
  rate.  If the camera is running, it is stopped and the process sleeps
  for one frame time before disconnecting.  Returns CAMWIRE_SUCCESS on
  success or CAMWIRE_FAILURE on failure.
*/
static int reconnect(const Camwire_handle c_handle, Camwire_conf *cfg,
		     const Camwire_state *set)
{
    if (set->running)
    {
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_set_run_stop(c_handle, 0));
	ERROR_IF_CAMWIRE_FAIL(
	    sleep_frametime(c_handle, 1.5));
    }
    disconnect(c_handle);
    ERROR_IF_CAMWIRE_FAIL(
	connect(c_handle, cfg, set));
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  Frees the memory allocated in create().  Should only ever be called
  from create() and camwire_destroy().  Assumes a valid c_handle.
*/
static void free_internals(const Camwire_handle c_handle)
{
    User_handle internal_status;

    internal_status = camwire_bus_get_userdata(c_handle);
    if (internal_status != NULL)
    {
	free(internal_status->config_cache);
	free(internal_status->control);
	free(internal_status);
	camwire_bus_set_userdata(c_handle, NULL);
    }
}

/*
  -----------------------------------------------------------------------------
  Returns a pointer to the dc1394_cameracapture structure for the given
  camwire handle, or NULL on error.  Needed by many libdc1394
  functions.
*/
inline static
dc1394_cameracapture *get_capturecontrol(const Camwire_handle c_handle)
{
    User_handle internal_status;

    internal_status = camwire_bus_get_userdata(c_handle);
    if (internal_status == NULL)  return(NULL);
    return(internal_status->control);
}

/*
  -----------------------------------------------------------------------------
  Returns a pointer to the Camwire_state structure for the given camwire
  handle, or NULL on error.  Needed by many camwire_get/set_...()
  functions.
*/
inline static Camwire_state *get_shadow_state(const Camwire_handle c_handle)
{
    User_handle internal_status;

    internal_status = camwire_bus_get_userdata(c_handle);
    if (internal_status == NULL)  return(NULL);
    return(internal_status->current_set);
}

/*
  -----------------------------------------------------------------------------
  Returns the IEEE 1394 image format, or 0 on error.
*/
static unsigned int get_1394_format(const Camwire_handle c_handle)
{
    Camwire_conf config;

    if (camwire_get_config(c_handle, &config) != CAMWIRE_SUCCESS)
    {
	return(0);
    }
    return(config.format);
}

/*
  -----------------------------------------------------------------------------
  Returns the IEEE 1394 mode (a subdivision of the format), or 0 on
  error.
*/
static unsigned int get_1394_mode(const Camwire_handle c_handle)
{
    Camwire_conf config;

    if (camwire_get_config(c_handle, &config) != CAMWIRE_SUCCESS)
    {
	return(0);
    }
    return(config.mode);
}

/*
  -----------------------------------------------------------------------------
  Gets the camera's current settings from the state shadow or as
  physically read from the camera.
*/
static int get_current_settings(const Camwire_handle c_handle,
				Camwire_state *set)
{
    Camwire_state *shadow_state;
    dc1394bool_t has_white_balance, has_shutter, has_trigger, has_polarity;
    
    shadow_state = get_shadow_state(c_handle);
    ERROR_IF_NULL(shadow_state);
    if (shadow_state->shadow)
    {
	memcpy(set, shadow_state, sizeof(Camwire_state));
    }
    else
    {
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_stateshadow(c_handle, &set->shadow));
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_run_stop(c_handle, &set->running));
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_single_shot(c_handle, &set->single_shot));

	/* White balance may not exist: */
	ERROR_IF_DC1394_FAIL(
	    dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				      camwire_bus_get_node(c_handle),
				      FEATURE_WHITE_BALANCE,
				      &has_white_balance));
	if (has_white_balance == DC1394_TRUE)
	{
	    ERROR_IF_CAMWIRE_FAIL(
		camwire_get_white_balance(c_handle, &set->blue_gain,
					  &set->red_gain));
	}
	else
	{
	    set->blue_gain = shadow_state->blue_gain;
	    set->red_gain = shadow_state->red_gain;
	}

	/* Shutter may not exist: */
	ERROR_IF_DC1394_FAIL(
	    dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				      camwire_bus_get_node(c_handle),
				      FEATURE_SHUTTER,
				      &has_shutter));
	if (has_shutter == DC1394_TRUE)
	{
	    ERROR_IF_CAMWIRE_FAIL(
		camwire_get_shutter(c_handle, &set->shutter));
	}
	else
	{
	    set->shutter = shadow_state->shutter;
	}
	
	/* Trigger source and polarity may not exist: */
	ERROR_IF_DC1394_FAIL(
	    dc1394_is_feature_present(camwire_bus_get_port(c_handle),
				      camwire_bus_get_node(c_handle),
				      FEATURE_TRIGGER,
				      &has_trigger));
	has_polarity = has_trigger;
	if (has_trigger == DC1394_TRUE)
	{
	    ERROR_IF_DC1394_FAIL(
		dc1394_trigger_has_polarity(camwire_bus_get_port(c_handle),
					    camwire_bus_get_node(c_handle),
					    &has_polarity));
	}
	if (has_polarity == DC1394_TRUE)
	{
	    ERROR_IF_CAMWIRE_FAIL(
		camwire_get_trigger_polarity(c_handle,
					     &set->trigger_polarity));
	}
	else
	{
	    set->trigger_polarity = shadow_state->trigger_polarity;
	}
	if (has_trigger == DC1394_TRUE)
	{
	    ERROR_IF_CAMWIRE_FAIL(
		camwire_get_trigger_source(c_handle, &set->external_trigger));
	}
	else
	{
	    set->external_trigger = shadow_state->external_trigger;
	}
	
	/* The rest always exist: */
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_framerate(c_handle, &set->frame_rate));
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_pixel_coding(c_handle, &set->coding));
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_frame_size(c_handle, &set->width, &set->height));
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_frame_offset(c_handle, &set->left, &set->top));
	ERROR_IF_CAMWIRE_FAIL(
	    camwire_get_num_framebuffers(c_handle, &set->num_frame_buffers));
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  Sleeps for multiple frame periods, where multiple is given by the
  second argument.  Typically used after stopping transmission to make
  sure any stray frames have been dispatched.  Returns CAMWIRE_SUCCESS
  on success or CAMWIRE_FAILURE on failure.
*/
static int sleep_frametime(const Camwire_handle c_handle,
			   const double multiple)
{
    float frame_rate;
    double sleep_period;
    struct timespec nap, left;
    
    ERROR_IF_CAMWIRE_FAIL(
	camwire_get_framerate(c_handle, &frame_rate));
    sleep_period = multiple/frame_rate;
    nap.tv_sec = (time_t) sleep_period; 	/* Trunc. to integer.*/
    nap.tv_nsec = (long) ((sleep_period - nap.tv_sec)*1e9);
    if (nanosleep(&nap, &left) != 0)
    {
	DPRINTF("nanosleep() failed in sleep_frametime().");
	return(CAMWIRE_FAILURE);
    }
    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  Sleeps for a short time.
*/
static volatile void blink(void)
{
    volatile int count;

    for (count = 0; count < 20000; count++)  ;
}

/*
  -----------------------------------------------------------------------------
  Returns the video frame rate for the given libdc1394 index, or -1.0 if
  it is not recognized.
*/
static double convert_index2framerate(const int frame_rate_index)
{
    switch (frame_rate_index) 
    {
	case FRAMERATE_1_875:  return( 1.875);
	case FRAMERATE_3_75:   return( 3.75);
	case FRAMERATE_7_5:    return( 7.5);
	case FRAMERATE_15:     return(15.0);
	case FRAMERATE_30:     return(30.0);
	case FRAMERATE_60:     return(60.0);
	default:               return(-1.0);
    }
}

/*
  -----------------------------------------------------------------------------
  Returns the nearest valid libdc1394 index for the given video frame
  rate.
*/
static int convert_framerate2index(const double frame_rate)
{
    double frame_rate2;

    frame_rate2 = frame_rate*frame_rate; 	/* For geometric mean.*/
    if      (frame_rate2 < 2*1.875*1.875)  return(FRAMERATE_1_875);
    else if (frame_rate2 < 2*3.75*3.75)    return(FRAMERATE_3_75);
    else if (frame_rate2 < 2*7.5*7.5)      return(FRAMERATE_7_5);
    else if (frame_rate2 < 2*15*15)        return(FRAMERATE_15);
    else if (frame_rate2 < 2*30*30)        return(FRAMERATE_30);
    else                                   return(FRAMERATE_60);
}

/*
  -----------------------------------------------------------------------------
  Returns the number of packets required to transmit a single frame, as
  obtained from the camera.
*/
static int get_numpackets(const Camwire_handle c_handle, int *num_p)
{
    unsigned int format;
    unsigned int frame_rate_index;
    unsigned int mode;
    
    format = get_1394_format(c_handle);
    ERROR_IF_ZERO(format);

    if (format == FORMAT_VGA_NONCOMPRESSED)
    { 	/* Format 0.*/
	ERROR_IF_DC1394_FAIL(
	    dc1394_get_video_framerate(camwire_bus_get_port(c_handle),
				       camwire_bus_get_node(c_handle),
				       &frame_rate_index));
	switch (frame_rate_index) 
	{
	    //  case FRAMERATE_1_875:
	    //	*num_p = 3840;
	    //	break;
	    case FRAMERATE_3_75:
		*num_p = 1920;
		break;
	    case FRAMERATE_7_5:
		*num_p = 960;
		break;
	    case FRAMERATE_15:
		*num_p = 480;
		break;
	    case FRAMERATE_30:
		*num_p = 240;
		break;
	    case FRAMERATE_60:
		*num_p = 120;
		break;
	    default:
		DPRINTF("dc1394_get_video_framerate() returned "
			"invalid index in get_numpackets().");
		return(CAMWIRE_FAILURE);
	}
    }
    else if (format == FORMAT_SCALABLE_IMAGE_SIZE)
    { 	/* Format 7.*/
	mode = get_1394_mode(c_handle);
	ERROR_IF_ZERO(mode);
	ERROR_IF_DC1394_FAIL(
	    dc1394_query_format7_packet_per_frame(
		camwire_bus_get_port(c_handle),
		camwire_bus_get_node(c_handle),
		mode,
		num_p));
	if (*num_p == 0)
	{
	    DPRINTF("dc1394_query_format7_packet_per_frame() returned null "
		    "number of packets in get_numpackets().");
	    return(CAMWIRE_FAILURE);
	}
    }
    else
    {
	DPRINTF("Unsupported camera format in camwire_get_numpackets().");
	return(CAMWIRE_FAILURE);
    }

    return(CAMWIRE_SUCCESS);
}

/*
  -----------------------------------------------------------------------------
  Returns the (positive, non-zero) number of video packets per frame for
  the given packet_size.
*/
static int convert_packetsize2numpackets(const Camwire_handle c_handle,
					 const int packet_size,
					 const int width,
					 const int height,
					 const Camwire_pixel coding)
{
    long denominator;
    int num_packets, depth;
    Camwire_conf config;
    
    // num_packets = ceil(framesize/packet_size):
    ERROR_IF_CAMWIRE_FAIL(
        camwire_pixel_depth(coding, &depth));
    denominator = (long) packet_size*8;
    if (denominator > 0)
    {
	num_packets =
	    ((long) width*height*depth + denominator - 1)/denominator;
	if (num_packets < 1)  num_packets = 1;
    }
    else
    {
	if (camwire_get_config(c_handle, &config) != CAMWIRE_SUCCESS)
	{
	    DPRINTF("camwire_get_config() failed in "
		    "convert_packetsize2numpackets().");
	}
	num_packets = config.max_packets;
    }
    return(num_packets);
}

/*
  -----------------------------------------------------------------------------
  Returns the nearest packet_size for the given number of video packets
  per frame.
*/
/* Required limitations imposed on intermediate values:
   num_packets is a positive integer
   num_packets <= 4095
   packet_size = unit_bytes * n where n is a positive integer
   packet_size <= max_bytes
   where unit_bytes and max_bytes are obtained from the PACKET_PARA_INQ
   register.

   Note that we don't use the function dc1394_query_total_bytes() which
   reads the TOTAL_BYTE_INQ camera registers, because different
   manufacturers apparently interpret it differently.
*/

static int convert_numpackets2packetsize(const Camwire_handle c_handle,
					 const int num_packets,
					 const int width,
					 const int height,
					 const Camwire_pixel coding)
{
    Camwire_conf config;
    int actual;
    unsigned int packet_size;
    unsigned int unit_bytes, max_bytes;
    int depth;
    long denominator;

    if (camwire_get_config(c_handle, &config) != CAMWIRE_SUCCESS)
    {
	DPRINTF("camwire_get_config() failed in "
		"convert_numpackets2packetsize().");
    }

#ifdef CAMWIRE_DEBUG
    if (config.format == 0)
    {
	DPRINTF("get_1394_format() failed in convert_numpackets2packetsize()");
    }
    else if (config.format != FORMAT_SCALABLE_IMAGE_SIZE)
    { 	/* Not Format 7.*/
	DPRINTF("Camera is not in Format 7 in "
		"convert_numpackets2packetsize().");
    }
#endif
    
    actual = num_packets;
    if (actual < 1)  actual = 1;
    if (actual > config.max_packets)  actual = config.max_packets;

    /* packet_size = ceil(framesize/num_packets): */
    ERROR_IF_CAMWIRE_FAIL(
	camwire_pixel_depth(coding, &depth));
    denominator = (long) actual*8;
    packet_size = ((long) width*height*depth + denominator - 1)/denominator;

    /* Set unit_bytes quantum and max_bytes packet size, even if we
       cannot yet access the camera: */
    unit_bytes = max_bytes = 0;
    dc1394_query_format7_packet_para(camwire_bus_get_port(c_handle),
				     camwire_bus_get_node(c_handle),
				     config.mode,
				     &unit_bytes, &max_bytes);
    if (unit_bytes < 4)  unit_bytes = 4; 	/* At least a quadlet.*/
    if (max_bytes < unit_bytes)  max_bytes = UINT_MAX;

    /* Quantize packet_size up to unit_bytes and check limits: */
    packet_size = ((packet_size + unit_bytes - 1)/unit_bytes)*unit_bytes;
    if (packet_size < unit_bytes)  packet_size = unit_bytes;
    if (packet_size > max_bytes)   packet_size = max_bytes;
    
    return(packet_size);
}

/*
  -----------------------------------------------------------------------------
  Returns the number of video packets per frame corresponding to the
  given frame rate.
*/
static int convert_framerate2numpackets(const Camwire_handle c_handle,
					const double frame_rate)
{
    Camwire_conf config;
    int num_packets;
    
    if (camwire_get_config(c_handle, &config) != CAMWIRE_SUCCESS)
    {
	DPRINTF("camwire_get_config() failed in "
		"convert_numpackets2framerate().");
    }

#ifdef CAMWIRE_DEBUG
    if (config.format == 0)
    {
	DPRINTF("get_1394_format() failed in convert_framerate2numpackets()");
    }
    else if (config.format != FORMAT_SCALABLE_IMAGE_SIZE)
    { 	/* Not Format 7.*/
	DPRINTF("Camera is not in Format 7 in "
		"convert_framerate2numpackets().");
    }
#endif

    if (frame_rate <= 0)  return(config.max_packets);
    num_packets = (int) (1.0/(config.bus_period*frame_rate) + 0.5);
    if (num_packets < 1)  num_packets = 1;
    if (num_packets > config.max_packets)  num_packets = config.max_packets;
    return(num_packets);
}

/*
  -----------------------------------------------------------------------------
  Returns the libdc1394 colour coding ID that supports the given pixel
  coding, or 0 on error.  The bit_field argument must be non-zero.

  libdc1394 presently only supports IIDC spec 1.30.  The signed and raw
  codings should be added to the list below as soon as libdc1394
  supports version 1.31.
*/
static
unsigned int convert_pixelcoding2colorid(const Camwire_pixel coding,
					 const quadlet_t bit_field)
{
    switch (coding)
    {
	case CAMWIRE_PIXEL_MONO8:  /* 8 bits/pixel.*/
	    if (bit_field & 0x80000000)  return(COLOR_FORMAT7_MONO8);
	    break;
	case CAMWIRE_PIXEL_YUV411:  /* 12 bits/pixel.*/
	    if (bit_field & 0x40000000)  return(COLOR_FORMAT7_YUV411);
	    break;
	case CAMWIRE_PIXEL_YUV422:  /* 16 bits/pixel.*/
	    if (bit_field & 0x20000000)  return(COLOR_FORMAT7_YUV422);
	    break;
	case CAMWIRE_PIXEL_YUV444:  /* 24 bits/pixel.*/
	    if (bit_field & 0x10000000)  return(COLOR_FORMAT7_YUV444);
	    break;
	case CAMWIRE_PIXEL_RGB8:  /* 24 bits/pixel.*/
	    if (bit_field & 0x08000000)  return(COLOR_FORMAT7_RGB8);
	    break;
	case CAMWIRE_PIXEL_MONO16:  /* 16 bits/pixel.*/
	    if (bit_field & 0x04000000)  return(COLOR_FORMAT7_MONO16);
	    break;
	case CAMWIRE_PIXEL_RGB16:  /* 48 bits/pixel.*/
	    if (bit_field & 0x02000000)  return(COLOR_FORMAT7_RGB16);
	    break;
	    /*
	case CAMWIRE_PIXEL_MONO16S:  // 16 bits/pixel.
	    if (bit_field & 0x01000000)  return(COLOR_FORMAT7_MONO16S);
	    break;
	case CAMWIRE_PIXEL_RGB16S:  // 48 bits/pixel.
	    if (bit_field & 0x00800000)  return(COLOR_FORMAT7_RGB16S);
	    break;
	case CAMWIRE_PIXEL_RAW8:  // 8 bits/pixel.
	    if (bit_field & 0x00400000)  return(COLOR_FORMAT7_RAW8);
	    break;
	case CAMWIRE_PIXEL_RAW16:  // 16 bits/pixel.
	    if (bit_field & 0x00200000)  return(COLOR_FORMAT7_RAW16);
	    break;
	    */
	default:
	    return(0);  /* No such coding.*/
	    break;
    }
    return(0);  /* Not supported by camera.*/
}

/*
  -----------------------------------------------------------------------------
  Prints most of the contents of c_handle->userdata.
*/
/*
static void debug_print_status(const User_handle is)
{
    dc1394_cameracapture *cc;
    Camwire_state *cs;
    
    printf("\ninternal_status:");
    if (is == NULL)
    {
	printf("  (null)\n");
	return;
    }
    printf("\n"
	   "  camera_connected:      %d\n"
	   "  frame_number:          %ld\n"
	   "  control:",
	   is->camera_connected,
	   is->frame_number);
    cc = is->control;
    if (cc == NULL)
    {
	printf("               (null)\n");
    }
    else
    {
        printf("\n"
	   "    node:                   %d\n"
	   "    channel:                %d\n"
	   "    frame_rate:             %d\n"
	   "    frame_width:            %d\n"
	   "    frame_height:           %d\n"
	   "    capture_buffer:         %s\n"
	   "    quadlets_per_frame:     %d\n"
	   "    quadlets_per_packet:    %d\n",
	   cc->node,
	   cc->channel,
	   cc->frame_rate,
	   cc->frame_width,
	   cc->frame_height,
	   cc->capture_buffer   ? "(non-null)" : "(null)",
	   cc->quadlets_per_frame,
	   cc->quadlets_per_packet);
        printf("    dma_ring_buffer:        %s\n"
	   "    dma_buffer_size:        %d\n"
	   "    dma_frame_size:         %d\n"
	   "    num_dma_buffers:        %d\n"
	   "    dma_last_buffer:        %d\n"
	   "    num_dma_buffers_behind: %d\n"
	   "    dma_device_file:        %s\n"
	   "    dma_fd:                 %d\n"
	   "    port:                   %d\n"
	   "    filltime.tv_sec:        %ld\n"
	   "            .tv_usec        %ld\n"
	   "    drop_frames:            %d\n",
	   cc->dma_ring_buffer  ? "(non-null)" : "(null)",
	   cc->dma_buffer_size,
	   cc->dma_frame_size,
	   cc->num_dma_buffers,
	   cc->dma_last_buffer,
	   cc->num_dma_buffers_behind,
	   cc->dma_device_file,
	   cc->dma_fd,
	   cc->port,
	   cc->filltime.tv_sec,
	   cc->filltime.tv_usec,
	   cc->drop_frames);
    }
    printf("  current_set:");
    cs = is->current_set;
    if (cs == NULL)
    {
	printf("               (null)\n");
    }
    else
    {
	printf("\n");
	fflush(stdout);
	camwire_write_state_to_file("/dev/stdout", cs);
    }
    
    return;
}
*/
