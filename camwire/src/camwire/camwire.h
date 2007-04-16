#ifndef CAMWIRE_H
#define CAMWIRE_H
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


    Title: Header for camwire_1394.c

    Description:
    This module is about using a single named camera through its
    handle. The handle should be all a user need know about for complete
    access to all camera functions.  Finding cameras and assigning
    handles to them is done in the Camwire bus module.

******************************************************************************/


#include "camwirebus.h"  /* For Camwire_handle.*/
#include "unistd.h" /* for intptr_t */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif


/* If consistently used, it should be possible to change these return
   codes without breaking anything: */
#define CAMWIRE_SUCCESS 0
#define CAMWIRE_FAILURE 1

/* Limits for Camwire_id and Camwire_conf: */
#define CAMWIRE_ID_MAX_CHARS	100
#define CAMWIRE_CONF_DMA_DEVICE_MAX_CHARS	100

typedef struct 
{
    char vendor[CAMWIRE_ID_MAX_CHARS+1];
    char model[CAMWIRE_ID_MAX_CHARS+1];
    char chip[CAMWIRE_ID_MAX_CHARS+1];
}
Camwire_id;
/* Type for a unique camera identifier comprising null-terminated vendor
   name, model name, and chip number strings, such as used by
   camwire_get_identifier() below. */

typedef enum
{
    CAMWIRE_PIXEL_INVALID,
    CAMWIRE_PIXEL_MONO8,	/*  8 bits average.*/
    CAMWIRE_PIXEL_YUV411,	/* 12 bits average.*/
    CAMWIRE_PIXEL_YUV422,	/* 16 bits average.*/
    CAMWIRE_PIXEL_YUV444,	/* 24 bits average.*/
    CAMWIRE_PIXEL_RGB8,		/* 24 bits average.*/
    CAMWIRE_PIXEL_MONO16,	/* 16 bits average.*/
    CAMWIRE_PIXEL_RGB16,	/* 48 bits average.*/
    CAMWIRE_PIXEL_MONO16S,	/* 16 bits average.*/
    CAMWIRE_PIXEL_RGB16S,	/* 48 bits average.*/
    CAMWIRE_PIXEL_RAW8,		/*  8 bits average.*/
    CAMWIRE_PIXEL_RAW16		/* 16 bits average.*/
}
Camwire_pixel;
/* Type for selecting the pixel encoding and pixel depth, as used by
   camwire_get/set_pixel_coding() and camwire_pixel_depth() below.
*/

typedef struct 
{
    int num_frame_buffers;
    float blue_gain, red_gain;
    int left, top, width, height;
    Camwire_pixel coding;
    float frame_rate, shutter;
    int external_trigger, trigger_polarity;
    int single_shot, running;
    int shadow;
}
Camwire_state;
/* Type for holding camera settings, such as those returned from
   camwire_get_state() or passed to
   camwire_create_from_struct().  Camwire uses this struct internally
   for shadowing current settings but these are not available to the
   user via this type.  Use the individual access functions instead.

   Each member corresponds to the arguments of the camwire_get/set_...()
   functions:

   num_frame_buffers:	The number of frames to store, not counting any
                        buffers internal to the camera.  Minimum 2,
                        maximum according to how much free memory (RAM)
			you have.

   blue_gain, red_gain:	Blue (U-value) and red (V-value) white balance
                        levels for colour cameras, between 0.0 and 1.0.

   left, top:		Top left-hand corner of region-of-interest in
                        sensor pixel coordinates.  The top left-hand
			corner pixel of the sensor is at (0,0), and the
			bottom right-hand corner pixel is at
			(sensorwidth-1, sensorheight-1).  Only
			applicable to scalable image sizes.

  width, height:	Width (X dimension) and height (Y dimension) in
                        pixels of the region-of-interest (scalable
			sizes) or the whole image (fixed sizes).

  coding:               One of the Camwire_pixel enumeration (see
                        above).

  frame_rate:		The rate at which the camera transmits frames to
                        the computer, in frames per second.  The actual
			frame rate may be determined by an external
			trigger frequency or image processing speed.

  shutter:		The integration time, in seconds.

  external_trigger:	Flag set to non-zero to trigger frame
                        acquisition on a hardware trigger signal (if
			available), otherwise the camera sends frames at
			the rate programmed with frame_rate above.

  trigger_polarity:	Flag set to non-zero for triggering on a rising
                        edge of the extenal trigger signal, otherwise it
			triggers on the falling edge.  Ignored if
			external triggering is not used.

  single_shot:		Flag set to non-zero to put camera in
                        single-shot mode (in which only one frame is
			transmitted), otherwise frame transmission is
			continuous when the camera is running.

  running:		Flag set to non-zero to start frame transmission
                        (possibly pending an external trigger if used),
			otherwise the camera is stopped and does not
			transmit frames to the computer.

  shadow:		Flag set to non-zero to cause, wherever
                        possible, camera settings to be read from this
			struct, otherwise they are read directly from
			the camera hardware (which takes longer).
*/

typedef struct
{
    int speed;
    int format;
    int mode;
    int max_packets;
    int min_pixels;
    float bus_period;
    float blue_gain_norm;
    float red_gain_norm;
    float trig_setup_time;
    float exposure_quantum;
    float exposure_offset;
    float line_transfer_time;
    float transmit_setup_time;
    int drop_frames;
    char dma_device_name[CAMWIRE_CONF_DMA_DEVICE_MAX_CHARS+1];
}
Camwire_conf;
/* Type for holding IEEE 1394 and IIDC DCAM hardware configuration data
   that the casual user probably does not want to know about.  See
   README_conf for a detailed description of each member. */


int camwire_create(const Camwire_handle c_handle);
/* Sets the camera to default initialization settings and connects it to
   the bus.  This function is equivalent to camwire_get_state()
   followed by camwire_create_from_struct().  The handle c_handle is
   obtained from camwire_bus_create().  Returns CAMWIRE_SUCCESS on
   success or CAMWIRE_FAILURE on failure or if the camera had previously
   been created.  The function camwire_destroy() must be called when
   done to free the allocated memory.  */

int camwire_create_from_struct(const Camwire_handle c_handle,
			       const Camwire_state *set);
/* Sets the camera to the given initialization settings and connects it
   to the bus.  The Camwire_state structure is returned unchanged.  The
   handle c_handle is obtained from camwire_bus_create().  Returns
   CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure or if the
   camera had previously been created.  The function camwire_destroy()
   must be called when done to free the allocated memory.  */

void camwire_destroy(const Camwire_handle c_handle);
/* Disconnects the camera from the bus and frees memory allocated in
   camwire_create() or camwire_create_from_struct().  All camera
   settings are lost.  Calling this function with a null handle has no
   effect. */

int camwire_get_state(const Camwire_handle c_handle, Camwire_state *set);
/* Gets the camera's current settings (running/stopped, trigger source,
   frame rate, frame size, etc).  If the camera has not been created,
   the camera is physically reset to factory default settings and those
   are probed and returned (without creating the camera).  If the camera
   has been created and the state shadow flag is set, the shadow state
   settings are returned, else the camera is queried for current
   settings.  If the camera does not support some settings their
   returned values are undefined.  Returns CAMWIRE_SUCCESS on success or
   CAMWIRE_FAILURE on failure.*/

int camwire_set_state(const Camwire_handle c_handle, const Camwire_state *set);
/* Sets the camera to the given settings (running/stopped, trigger
   source, frame rate, frame size, etc).  The camera must already have
   been created.  If the camera does not support some settings, they are
   ignored.  Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on
   failure.*/

int camwire_read_state_from_file(const char *filename, Camwire_state *set);
/* Reads camera initialization settings from the given filename.
   Physical cameras are unchanged (there is no camera handle in the
   argument list).  This function can be called at any time.  Returns
   CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure.*/

int camwire_write_state_to_file(const char *filename,
				const Camwire_state *set);
/* Writes camera initialization settings to the given filename.  Any
   existing file of that name is overwritten.  This function can be
   called at any time.  Returns CAMWIRE_SUCCESS on success or
   CAMWIRE_FAILURE on failure.*/
    
int camwire_get_config(const Camwire_handle c_handle, Camwire_conf *cfg);
/* Gets the camera and its bus's static configuration settings for
   initialization from a configuration file.  They are bus-specific
   hardware parameters that the casual user need not know or care about.
   The structure Camwire_conf is defined above.  If a configuration file
   does not exist, an error message is printed which includes a
   best-guess default configuration.  This can be copied into a new
   configuration file (and edited as needed).  The filename must be
   identical to one of the camera's ID strings (as may be obtained from
   camwire_get_identifier()) appended by an extension of ".conf".  The
   function checks for existing filenames containing the unique chip
   string, or the model name string, or the vendor name string, in this
   order.  It first looks for the three filenames in the current working
   directory and after that in a directory given by the CAMWIRE_CONF
   environment variable.  The configuration file is cached the first
   time it is read by the current camera with handle c_handle.  There
   should be no need to call this function from a user program for
   normal camera operations.  It is provided here in case it is needed
   for debugging.  Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE
   on failure.*/

int camwire_print_config(const Camwire_conf *cfg);
/* Prints the static configuration settings as obtained from
   camwire_get_config() to the standard output.  The print format is the
   same as that expected by camwire_get_config() when it reads
   configuration files.  Returns CAMWIRE_SUCCESS on success or
   CAMWIRE_FAILURE on failure.*/

int camwire_get_identifier(const Camwire_handle c_handle,
			   Camwire_id *identifier);
/* Fills in the given camwire identifier structure (type defined above).
   The identifier is uniquely and permanently associated with the camera
   hardware, such as might be obtained from configuration ROM data.
   Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure. */

int camwire_get_stateshadow(const Camwire_handle c_handle, int *shadow);
/* Gets the state shadow flag: 1 to get camera settings from an internal
   shadow structure or 0 to read them directly from the camera hardware.
   Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure.*/

int camwire_set_stateshadow(const Camwire_handle c_handle, const int shadow);
/* Sets the state shadow flag: 1 to get camera settings from an internal
   shadow structure or 0 to read them directly from the camera hardware.
   Setting state shadowing may result in faster camera responses and may
   increase robustness because the camera firmware is not interrupted by
   unnecessary status requests.  State shadowing may however be slightly
   risky because bugs in Camwire or in the camera firmware could cause
   the shadow state to differ from the actual state.  The state shadow
   flag of a created camera can be modified at any time because the
   shadow state is internally maintained irrespective of the state
   shadow flag status.  Returns CAMWIRE_SUCCESS on success or
   CAMWIRE_FAILURE on failure.*/

int camwire_get_num_framebuffers(const Camwire_handle c_handle,
				 int *num_frame_buffers);
/* Gets the number of frame reception buffers allocated for the camera
   bus in the host computer.  Returns CAMWIRE_SUCCESS on success or
   CAMWIRE_FAILURE on failure. */

int camwire_set_num_framebuffers(const Camwire_handle c_handle,
				 const int num_frame_buffers);
/* Sets the number of frame reception buffers allocated for the camera
   bus in the host computer.  Returns CAMWIRE_SUCCESS on success or
   CAMWIRE_FAILURE on failure. */

int camwire_get_framebuffer_lag(const Camwire_handle c_handle,
				int *buffer_lag);
/* Warning: The use of this function is deprecated because it creates
   the impression that it returns a currently fresh measure of the
   buffer lag.  Rather use the buffer_lag returned by calls to
   camwire_copy_next_frame(), camwire_point_next_frame(),
   camwire_point_next_frame_poll(), and calls to
   camwire_flush_framebuffers() with argument num_to_flush >= 1.
   Ideally it should get the number of bus frame buffers which have been
   filled by the camera but not yet accessed or, in other words, the
   number of frames by which we are behind.  In the current
   implementation this number is only updated by the calls listed above
   and should otherwise be considered stale.  Returns CAMWIRE_SUCCESS on
   success or CAMWIRE_FAILURE on failure. */

int camwire_flush_framebuffers(const Camwire_handle c_handle,
			       const int num_to_flush,
			       int *num_flushed,
			       int *buffer_lag);
/* Tries to flush num_to_flush buffered frames.  If num_flushed is not
   the null pointer, the actual number of frames flushed is returned in
   *num_flashed.  *num_flushed will be less than num_to_flush if there
   were fewer buffered frames available to flush than requested.
   *num_flushed will never be more than num_to_flush.  It is safe to
   make num_to_flush a larger number than the total number of buffered
   frames.  If buffer_lag is not the null pointer, the number of bus
   frame buffers by which we are behind after flushing is returned in
   *buffer_lag.  This is the same number as can be obtained from
   camwire_get_framebuffer_lag().  *buffer_lag is only valid if
   num_to_flush was 1 or more.  The historically older frames are
   flushed first.  Frames currently being transmitted by the camera are
   not affected.  Any frames accessed by camwire_point_next_frame() or
   camwire_point_next_frame_poll() should first be released with
   camwire_unpoint_frame() before flushing.  If you want to be sure of
   completely emptying all bus frame buffers, you should stop the
   camera, wait for more than one frame transmission period and then
   flush.  Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on
   failure. */

int camwire_get_frame_offset(const Camwire_handle c_handle, int *left,
			 int *top);
/* Gets the frame offsets (left, top) in units of pixels.  Returns
   CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure. */

int camwire_set_frame_offset(const Camwire_handle c_handle,
			     const int left, const int top);
/* Sets the frame offsets (left, top) in units of pixels to the nearest
   values valid for the camera.  The resulting offsets are constrained
   by the maximum available frame dimensions and the current frame
   dimensions.  The offsets can be checked afterwards with
   camwire_get_frame_offset().  Returns CAMWIRE_SUCCESS on success or
   CAMWIRE_FAILURE on failure. */

int camwire_get_frame_size(const Camwire_handle c_handle, int *width,
			   int *height);
/* Gets the frame size (width, height) in units of pixels.  Returns
   CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure. */

int camwire_set_frame_size(const Camwire_handle c_handle, const int width,
			   const int height);
/* Sets the frame size (width, height) in units of pixels to the nearest
   values valid for the camera.  The sizes available may be constrained
   by the maximum available frame size and the current frame offsets.
   The actual size set can be checked afterwards with
   camwire_get_frame_size().  For some camera buses like IEEE 1394, the
   frame rate may also change, especially with small frame sizes.  Check
   afterwards with camwire_get_framerate().  Returns CAMWIRE_SUCCESS on
   success or CAMWIRE_FAILURE on failure. */

int camwire_get_pixel_coding(const Camwire_handle c_handle,
			     Camwire_pixel *coding);
/* Gets the pixel coding, as one of the Camwire_pixel enumeration
   members above.  Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE
   on failure. */

int camwire_set_pixel_coding(const Camwire_handle c_handle,
			     const Camwire_pixel coding);
/* Sets the pixel coding, given one of the Camwire_pixel enumeration
   members above.  For some camera buses like IEEE 1394, the frame rate
   may also change, especially with small frame sizes.  Check afterwards
   with camwire_get_framerate().  Returns CAMWIRE_SUCCESS on success or
   CAMWIRE_FAILURE on failure. */

int camwire_pixel_depth(const Camwire_pixel coding, int *depth);
/* Translates the given Camwire pixel colour coding into the
   corresponding pixel depth in bits per pixel.  Returns CAMWIRE_SUCCESS
   on success or CAMWIRE_FAILURE on failure. */

int camwire_get_framerate(const Camwire_handle c_handle, float *framerate);
/* Gets the video frame rate in frames per second.  See the description
   of the meaning of 'frame rate' under camwire_set_framerate().
   Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure. */

int camwire_set_framerate(const Camwire_handle c_handle,
			  const float framerate);
/* Sets the video frame rate in frames per second to the nearest value
   valid for the camera.  The frame rates available may be constrained
   by the frame dimensions.  The actual frame rate set can be checked
   afterwards with camwire_get_framerate().  The meaning of 'frame rate'
   changes depending on the single-shot and trigger source settings.
   The frame rate determines how long frame transmission from camera to
   computer takes and how much bus bandwidth is used.  With internal
   trigger, non-single-shot (continuous), it is indeed the number of
   frames per second.  With external trigger, non-single-shot, it should
   be set faster than the external trigger frequency to avoid dropped
   frames.  With single-shot, it should be set faster than the frequency
   of single-shot requests.  Returns CAMWIRE_SUCCESS on success or
   CAMWIRE_FAILURE on failure. */

int camwire_get_shutter(const Camwire_handle c_handle, float *shutter);
/* Gets the camera's shutter speed (exposure time in seconds).  Returns
   CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure. */

int camwire_set_shutter(const Camwire_handle c_handle, const float shutter);
/* Sets the given shutter speed (exposure time in seconds) to the
   nearest valid value for the camera.  The actual value can be checked
   afterwards with camwire_get_shutter().  Returns CAMWIRE_SUCCESS on
   success or CAMWIRE_FAILURE on failure. */

int camwire_get_trigger_source(const Camwire_handle c_handle, int *external);
/* Gets the camera's trigger source setting: 1 for external or 0 for
   internal.  Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on
   failure. */

int camwire_set_trigger_source(const Camwire_handle c_handle,
			       const int external);
/* Sets the camera's trigger source: 1 for external or 0 for internal.
   Fails if the camera does not have an external trigger.  Returns
   CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure. */

int camwire_get_trigger_polarity(const Camwire_handle c_handle, int *rising);
/* Gets the camera's trigger polarity setting: 1 for rising edge (active
   high) or 0 for falling edge (active low).  Returns CAMWIRE_SUCCESS on
   success or CAMWIRE_FAILURE on failure. */

int camwire_set_trigger_polarity(const Camwire_handle c_handle,
				 const int rising);
/* Sets the camera's trigger polarity: 1 for rising edge (active high)
   or 0 for falling edge (active low).  Fails if the trigger polarity is
   not settable.  Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE
   on failure. */

int camwire_get_white_balance(const Camwire_handle c_handle, float *blue_gain,
			      float *red_gain);
/* Gets the white balance gains (blue, red) for colour cameras.  Returns
   CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure. */

int camwire_set_white_balance(const Camwire_handle c_handle,
			      const float blue_gain, const float red_gain);
/* Sets the white balance gains (blue, red) for colour cameras to the
   nearest values valid for the camera.  The gains can be checked
   afterwards with camwire_get_white_balance().  Fails if the white
   balance is not settable.  Returns CAMWIRE_SUCCESS on success or
   CAMWIRE_FAILURE on failure. */

int camwire_get_single_shot(const Camwire_handle c_handle,
			    int *single_shot_on);
/* Gets the camera's acquisition type setting in single_shot_on: 1 for
   single-shot or 0 for continuous.  Returns CAMWIRE_SUCCESS on success
   or CAMWIRE_FAILURE on failure. */

int camwire_set_single_shot(const Camwire_handle c_handle,
			    const int single_shot_on);
/* Sets the camera's acquisition type in single_shot_on: 1 for
   single-shot or 0 for continuous.  To capture a single frame, make
   sure that the camera is stopped, set the acquisition type to
   single-shot, and set the camera running (with camwire_set_run_stop())
   at the moment the frame is needed.  If the camera is already running
   while changing acquisition type from continous to single-shot, then
   the single frame is acquired immediately (pending an external trigger
   if used).  Changing acquisition type from single-shot to continuous
   while the camera is running is an unreliable thing to do, because you
   can't be sure if the single frame acquisition has started and hence
   you don't know whether the camera is stopped or running.  Returns
   CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure. */

int camwire_get_run_stop(const Camwire_handle c_handle, int *runsts);
/* Gets the camera run status in runsts: 1 for running or 0 for stopped.
   Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure. */

int camwire_set_run_stop(const Camwire_handle c_handle, const int runsts);
/* Sets the camera run status in runsts: 1 for running or 0 for stopped.
   If a stopped camera is set to running while the acquisition type (as
   set by camwire_set_single_shot()) is single-shot, then only one frame
   is transmitted (pending an external trigger if used) after which the
   camera reverts to the stopped state.  Otherwise, frames are
   transmitted from camera to bus frame buffers at the programmed frame
   rate until the camera is stopped.  Returns CAMWIRE_SUCCESS on success
   or CAMWIRE_FAILURE on failure. */

int camwire_copy_next_frame(const Camwire_handle c_handle, void *buffer,
			    int *buffer_lag);
int camwire_copy_next_frame_with_stride(const Camwire_handle c_handle, void *buffer,
					int *buffer_lag, intptr_t stride0);
/* Waits until a frame has been received and copies it into the given
   buffer.  Note that this function may never return to the calling
   program if a frame does not become available, for example if the
   camera is not running or an external trigger does not arrive.  If
   more than one frame are buffered when this function is called then
   the earliest frame is copied.  If buffer_lag is not the null pointer,
   the number of bus frame buffers by which we are behind is returned in
   *buffer_lag.  This is the same number as can be obtained from
   camwire_get_framebuffer_lag().  The frame's number and time stamp are
   available afterwards from camwire_get_framenumber() and
   camwire_get_timestamp().  If speed is important then
   camwire_point_next_frame() or camwire_point_next_frame_poll() might
   be faster.  Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on
   failure. */

int camwire_point_next_frame(const Camwire_handle c_handle, void **buf_ptr,
			     int *buffer_lag);
/* Sets the given buffer pointer buf_ptr to the next received frame
   buffer.  If a frame is ready it returns immediately, otherwise it
   waits until a frame has been received.  Note that this function may
   never return to the calling program if a frame does not become
   available, for example if the camera is not running or an external
   trigger does not arrive.  The frame buffer is locked, which prevents
   it from being overwritten by subsequent frames.  The calling program
   may freely read and write the buffer.  If more than one frames are
   buffered when this function is called then buf_ptr points to the
   earliest frame.  If buffer_lag is not the null pointer, the number of
   bus frame buffers by which we are behind is returned in *buffer_lag.
   This is the same number as can be obtained from
   camwire_get_framebuffer_lag().  The frame's number and time stamp are
   available afterwards from camwire_get_framenumber() and
   camwire_get_timestamp().  This function may be faster than
   camwire_copy_next_frame() because no copying is involved.  The
   downside is that the function camwire_unpoint_frame() must be called
   each time when done to release the frame buffer again.  Returns
   CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on failure. */

int camwire_point_next_frame_poll(const Camwire_handle c_handle,
				  void **buf_ptr, int *buffer_lag);
/* Sets the given buffer pointer buf_ptr to the next received frame
   buffer and returns immediately.  If no frame is ready it sets buf_ptr
   to the null pointer and returns 0 in *buffer_lag.  Although it may
   not always return a frame pointer, this function always returns to
   the calling program even if a frame does not become available.
   Otherwise its behaviour is similar to camwire_point_next_frame().
   Note that the values returned from camwire_get_framenumber() and
   camwire_get_timestamp() are not valid if no frame was returned.  Like
   camwire_point_next_frame(), the function camwire_unpoint_frame() must
   be called each time a frame is obtained to release the frame buffer
   again.  If no frame was obtained then camwire_unpoint_frame() should
   not be called.  Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE
   on failure. */

int camwire_unpoint_frame(const Camwire_handle c_handle);
/* Releases the bus frame buffer that was pointed to with the pointer
   access functions camwire_point_next_frame() or
   camwire_point_next_frame_poll(), so that it can be used again for
   receiving new image data.  Calls to pointer access functions and
   camwire_unpoint_frame() should be strictly interleaved, otherwise the
   frame buffer may never be released.  Returns CAMWIRE_SUCCESS on
   success or CAMWIRE_FAILURE on failure. */

int camwire_get_framenumber(const Camwire_handle c_handle, long *framenumber);
/* Gets the serial number of the last frame captured with
   camwire_copy_next_frame(), camwire_point_next_frame() or
   camwire_point_next_frame_poll().  The number is initialized to zero
   by camwire_create() or camwire_create_from_struct(), and the first
   frame is number 1.  All frames are numbered (as far as possible -
   buffer overflows may cause problems) even if they are not accessed by
   camwire_copy_next_frame(), camwire_point_next_frame() or
   camwire_point_next_frame_poll().  If the camera or its bus are
   configured to drop old frames, the number of missed frames may be
   calculated as one less than the difference between consecutive frame
   numbers.  Returns CAMWIRE_SUCCESS on success or CAMWIRE_FAILURE on
   failure. */

int camwire_get_timestamp(const Camwire_handle c_handle,
			  struct timespec *timestamp);
/* Gets the time stamp as a timespec structure (seconds and nanoeconds)
   of the last frame captured with camwire_copy_next_frame(),
   camwire_point_next_frame() or camwire_point_next_frame_poll().  The
   time stamp is the estimated time of the shutter trigger relative to
   the start of the Unix clock (0 January 1970) as reported by the local
   host processor.  Returns CAMWIRE_SUCCESS on success or
   CAMWIRE_FAILURE on failure. */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
}
#endif

#endif
