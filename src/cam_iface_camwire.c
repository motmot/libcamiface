/* $Id: cam_iface_camwire.c 1332 2006-08-05 10:18:11Z astraw $ */
#include <libdc1394/dc1394_control.h>

#include "camwire/camwire.h"
#include "camwire/camwirebus.h"
/*#include "camwirebus_internal_1394.h"*/ /* naughty, naughty... */

#include "cam_iface.h"

#include <time.h> /* for nanosleep */
#include <string.h> /* for memcpy */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> /* for sleep */
#include <errno.h>

#define SAFE_FRAME_PERIOD	1.5
#define BUF_LOW_MARK		0.1
#define BUF_HIGH_MARK		0.9

typedef struct 
{
    enum {stopped, running} activity;
    enum {continuous, single} acqtype;
    enum {internal, external} trigger;
    enum {falling, rising} polarity;
    float shutter;
    float framerate;
    int width, height, depth;
    int left, top;
    int num_buffers;
    long framenumber;
    int save_num_images;
    float save_delay;
}
Status_t;

/* internal structures for camwire implementation */

struct _cam_iface_backend_extras {
  int max_width;       // maximum buffer width
  int max_height;      // maximum buffer height
  int roi_width;
  int roi_height;
  int buffer_size;     // bytes per frame
};
typedef struct _cam_iface_backend_extras cam_iface_backend_extras;

/* globals -- allocate space */
int cam_iface_error;
#define CAM_IFACE_MAX_ERROR_LEN 255
char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN];

#define CAM_IFACE_ERROR_FORMAT(m)				\
  snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,	\
	   "%s (%d): %s\n",__FILE__,__LINE__,(m));

#define ASSERT_CAM_CONTEXT_DEFINED(ccntxt)				\
  if (!(ccntxt)) {							\
    cam_iface_error = -1;						\
    snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,		\
	     "%s (%d): no CamContext specified (NULL argument)\n",	\
	     __FILE__,__LINE__);					\
    return;								\
  }									

Camwire_handle *handle_array = NULL;
int num_cameras;

// forward definition:
Camwire_state *get_shadow_state(const Camwire_handle c_handle);

/*
  ----------------------------------------------------------------------
  Sleeps for multiple times one frame period, where multiple is a float.
*/
static void wait_frametime(const Camwire_handle c_handle,
			   const float multiple)
{
    float framerate, frameperiod;
    struct timespec nap;
    
    camwire_get_framerate(c_handle, &framerate);
    frameperiod = multiple/framerate;
    nap.tv_sec = frameperiod;
    nap.tv_nsec = (frameperiod - nap.tv_sec)*1e9;
    nanosleep(&nap, NULL);
}


/*
  ----------------------------------------------------------------------
  Checks the number of pending filled buffers and flushes some of them
  if there are too many.  This helps to ensure that frame numbers are
  accurately updated when frames are dropped.  Otherwise buffer
  overflows may result in the user not knowing if frames have been lost.
*/
static int manage_buffer_level(const Camwire_handle c_handle)
{
  int total_frames, current_level, num_to_flush, err;

  err = 0;
    camwire_get_num_framebuffers(c_handle, &total_frames);
    if (total_frames < 3)  return err;
    camwire_get_framebuffer_lag(c_handle, &current_level);
    current_level++; 	/* Buffer lag does not count current frame.*/

    /* It seems that the DMA buffers never fill up completely, hence the
       extra -1 in the if statement below: */
    if (current_level >= total_frames - 1)
    { 	/* Hit the ceiling.*/
	num_to_flush = total_frames;
	camwire_flush_framebuffers(c_handle, num_to_flush, NULL, NULL);
	err = 1;
    }
    else
    {
	if (current_level + 0.5 >= BUF_HIGH_MARK*total_frames)
	{
	    num_to_flush = current_level - BUF_LOW_MARK*total_frames;
	    camwire_flush_framebuffers(c_handle, num_to_flush, NULL, NULL);
	}
    }
    return err;
}

const char *cam_iface_get_driver_name() {
  return "camwire";
}

void cam_iface_clear_error() {
  cam_iface_error = 0;
}

int cam_iface_have_error() {
  return cam_iface_error;
}

const char * cam_iface_get_error_string() {
  return cam_iface_error_string;
}

const char* cam_iface_get_api_version() {
  return CAM_IFACE_API_VERSION;
}

void cam_iface_startup() {
    int retry;
    int bad_bus;

    /* Initialize the camera bus:*/
    handle_array = camwire_bus_create(&num_cameras);
    if (num_cameras < 0 || handle_array == NULL)
    {
	retry = 1;
	bad_bus = 1;
	while (retry <= 5 && bad_bus)
	{
	    camwire_bus_reset();
	    sleep(1);
	    handle_array = camwire_bus_create(&num_cameras);
	    if (num_cameras >= 0 && handle_array != NULL)  bad_bus = 0;
	    retry++;
	}
	if (bad_bus)
	{
	  cam_iface_error = -1;
	  CAM_IFACE_ERROR_FORMAT("Could not initialize the camera bus.\n"\
	    "Make sure ieee1394,video1394,ohci1394,raw1394 modules are\n"\
	    "installed, that /dev/raw1394 is properly configured, and\n"\
	    "that you actually have a camera plugged in.");
	  return;
	}
	else
	{
	}
    }
}

void cam_iface_shutdown() {
  camwire_bus_destroy();
}


int cam_iface_get_num_cameras() {
  return num_cameras;
}

void cam_iface_get_camera_info(int index, Camwire_id *out_camid) {
  if (camwire_get_identifier(handle_array[index], out_camid) !=
      CAMWIRE_SUCCESS)
  {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("Could not get the identifier");
  }
}


void cam_iface_get_num_modes(int device_number, int *num_modes) {
  *num_modes = 1;
}

void cam_iface_get_mode_string(int device_number,
			       int mode_number,
			       char* mode_string,
			       int mode_string_maxlen) {
  snprintf(mode_string,mode_string_maxlen,"(set in camwire configuration file)");
}

CamContext * new_CamContext( int device_number, int NumImageBuffers,
			     int mode_number) {
  CamContext *ccntxt = NULL;
  Camwire_handle c_handle = NULL;
  Camwire_pixel camwire_coding;
  cam_iface_backend_extras* backend_extras;
  dc1394bool_t iso_en;

  ccntxt = malloc(sizeof(CamContext));
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("malloc failed");
    return NULL;
  }

  /* initialize */
  ccntxt->cam = (void *)NULL;
  
  if (device_number >= num_cameras) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("requested too high camera number");
    return NULL;
  }
  
  if (mode_number != 0) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("mode_number must be 0 for camwire backend");
    return NULL;
  }

  ccntxt->device_number = device_number;
  c_handle = handle_array[device_number];

  /* Get factory default start-up settings: */
  if (c_handle == NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("c_handle is NULL");
    return NULL;
  }

  if (c_handle==NULL) {
    fprintf(stderr,"c_handle NULL\n");
  } else {
    if (DC1394_SUCCESS != dc1394_get_iso_status((raw1394handle_t)camwire_bus_get_port(c_handle),
						(nodeid_t)camwire_bus_get_node(c_handle),
						&iso_en)) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("error getting ISO status");
      return NULL;
    }
  }

  if (camwire_create(c_handle) != CAMWIRE_SUCCESS) {
    int errsv = errno; // save error value from ioctl, which hopefully libdc1394 or camwire didn't change
    if (errsv) {
      fprintf(stderr,"errno %d: %s (%s, %d)\n",errsv,strerror(errsv),__FILE__,__LINE__);
    }
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("Could not initialize camera.\n"\
      "Make sure ieee1394,video1394,ohci1394,raw1394 modules are\n"\
      "installed, that /dev/raw1394 and /dev/video1394/0 are setup,\n"\
      "and that the camwire .conf file is available.\n"\
      "(Then try again.)");
    return NULL;
  }

  ccntxt->cam = (void *)c_handle;

  ccntxt->backend_extras = malloc(sizeof(cam_iface_backend_extras));
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("malloc failed");
    return NULL;
  }
  backend_extras = (cam_iface_backend_extras*)ccntxt->backend_extras;

  camwire_get_frame_size(ccntxt->cam, &backend_extras->max_width, 
			 &backend_extras->max_height);
  camwire_get_frame_size(ccntxt->cam, &backend_extras->roi_width,
			 &backend_extras->roi_height);

  camwire_get_pixel_coding(ccntxt->cam, &camwire_coding);
  switch (camwire_coding) {
  case CAMWIRE_PIXEL_MONO8: ccntxt->coding = CAM_IFACE_MONO8; break;
  case CAMWIRE_PIXEL_YUV411: ccntxt->coding = CAM_IFACE_YUV411; break;
  case CAMWIRE_PIXEL_YUV422: ccntxt->coding = CAM_IFACE_YUV422; break;
  default: ccntxt->coding = CAM_IFACE_UNKNOWN; break;
  }
  camwire_pixel_depth(camwire_coding, &ccntxt->depth);

  backend_extras->buffer_size = (  backend_extras->roi_height
				  *backend_extras->roi_width
				   *ccntxt->depth/8);

  if (camwire_set_num_framebuffers(ccntxt->cam, NumImageBuffers) !=
      CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error setting num framebuffers");
    return NULL;
  }

  return ccntxt;
}

void delete_CamContext(CamContext *ccntxt) {
  Camwire_handle c_handle;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  c_handle = ccntxt->cam;
  
  if (c_handle != NULL) {
    camwire_set_run_stop(c_handle, 0);
    wait_frametime(c_handle, SAFE_FRAME_PERIOD);
  }

  camwire_destroy(c_handle);
  camwire_bus_destroy();
  free(ccntxt->backend_extras);
  free(ccntxt);
}

void CamContext_start_camera( CamContext *ccntxt ) {
  Camwire_handle c_handle;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  c_handle = ccntxt->cam;
  
  if (camwire_set_run_stop(c_handle, 1) != CAMWIRE_SUCCESS)
    {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("Could not set camera to running.");
      return;
    }
}

void CamContext_stop_camera( CamContext *ccntxt ) {
  Camwire_handle c_handle;
  int num_buffers;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  c_handle = ccntxt->cam;
  
  if (camwire_set_run_stop(c_handle, 0) != CAMWIRE_SUCCESS)
    {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("Could not set camera to stopped.");
      return;
    }

  wait_frametime(c_handle, SAFE_FRAME_PERIOD);
  camwire_get_num_framebuffers(c_handle, &num_buffers);
  camwire_flush_framebuffers(c_handle, num_buffers, NULL, NULL);

}

typedef enum {
  cic_shutter=0,
  cic_brightness,
  cic_gain
} cam_iface_camwire_property_t;
#define cam_iface_camwire_property_min               cic_shutter
#define cam_iface_camwire_property_max               cic_gain
#define cam_iface_camwire_property_num (cam_iface_camwire_property_max-cam_iface_camwire_property_min+1)

void CamContext_get_num_camera_properties(CamContext *ccntxt, 
					  int* num_properties) {
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  *num_properties=cam_iface_camwire_property_num;
}

void CamContext_get_camera_property_info(CamContext *ccntxt,
					 int property_number,
					 CameraPropertyInfo *info) {
  unsigned int feature;
  unsigned int tmp;
  int retval;
  dc1394bool_t tmp2;
  Camwire_handle c_handle;
  Camwire_conf config;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  
  c_handle = ccntxt->cam;

  switch (property_number) {
  case cic_shutter: info->name = "shutter"; feature = FEATURE_SHUTTER; break;
  case cic_gain: info->name = "gain"; feature = FEATURE_GAIN; break;
  case cic_brightness: info->name = "brightness"; feature = FEATURE_BRIGHTNESS; break;
  default:     
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("unknown property_number");
    return;
  }
  
  retval = dc1394_is_feature_present(
				     (raw1394handle_t)camwire_bus_get_port(c_handle), 
				     (nodeid_t)camwire_bus_get_node(c_handle),
				     feature, &tmp);
  
  if (DC1394_SUCCESS != retval) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error getting property_number present status");
    return;
  }
  info->is_present = (int)tmp;

  retval = dc1394_get_min_value((raw1394handle_t)camwire_bus_get_port(c_handle),
				(nodeid_t)camwire_bus_get_node(c_handle),
				feature, &tmp);
  
  if (DC1394_SUCCESS != retval) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error getting property_number min");
    return;
  }

  info->min_value = (long)tmp;

  
  retval = dc1394_get_max_value((raw1394handle_t)camwire_bus_get_port(c_handle),
				(nodeid_t)camwire_bus_get_node(c_handle),
				feature, &tmp);
  
  if (DC1394_SUCCESS != retval) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error getting property_number max");
    return;
  }

  info->max_value = (long)tmp;

  retval = dc1394_has_auto_mode((raw1394handle_t)camwire_bus_get_port(c_handle),
				(nodeid_t)camwire_bus_get_node(c_handle),
				feature, &tmp2);
  
  if (DC1394_SUCCESS != retval) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error getting property_number auto");
    return;
  }

  info->has_auto_mode = (int)tmp2;

  retval = dc1394_has_manual_mode((raw1394handle_t)camwire_bus_get_port(c_handle),
				  (nodeid_t)camwire_bus_get_node(c_handle),
				  feature, &tmp2);
  
  if (DC1394_SUCCESS != retval) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error getting property_number manual");
    return;
  }
  
  info->has_manual_mode = (int)tmp2;

  info->is_scaled_quantity = 0;
  info->scaled_unit_name = NULL;
  info->scale_gain = 1.0;
  info->scale_offset = 0.0;

  if (property_number==cic_shutter) {
    if (camwire_get_config(c_handle, &config) != CAMWIRE_SUCCESS) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("Couldn't get camera config.");
      return;
    }
    info->is_scaled_quantity = 1;
    info->scaled_unit_name = "msec";
    info->scale_offset = config.exposure_offset*1000.0; // use msec not sec
    info->scale_gain = config.exposure_quantum*1000.0;  // use msec not sec
  }
}

void CamContext_get_camera_property(CamContext *ccntxt,
				    int property_number,
				    long* Value,
				    int* Auto ) {
  unsigned int tmp, tmp2;
  unsigned int feature;
  int (*get_function)( raw1394handle_t, nodeid_t, unsigned int* );
  Camwire_handle c_handle;
  int retval;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  c_handle = ccntxt->cam;
  
  switch (property_number) {
  case cic_shutter: 
    get_function = &dc1394_get_shutter; 
    feature = FEATURE_SHUTTER; 
    break;
  case cic_gain: 
    get_function = &dc1394_get_gain; 
    feature = FEATURE_GAIN; 
    break;
  case cic_brightness: 
    get_function = &dc1394_get_brightness;
    feature = FEATURE_BRIGHTNESS; break;
  default:     
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("unknown property_number");
    return;
  }

  // naughty to use internal camwire structures...
  retval = (*get_function)( (raw1394handle_t)camwire_bus_get_port(c_handle),
			    (nodeid_t)camwire_bus_get_node(c_handle),
			    &tmp);
  if (DC1394_SUCCESS != retval) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error getting property_number");
    return;
  }
  *Value = (long)tmp;
  
  retval = dc1394_is_feature_auto(
				  (raw1394handle_t)camwire_bus_get_port(c_handle),
				  (nodeid_t)camwire_bus_get_node(c_handle),
				  feature, &tmp2);
  if (DC1394_SUCCESS != retval) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error getting auto status of property_number");
    return;
  }
  *Auto = (long)tmp2;
  
  return;
}

// WARNING: Keep in sync with camwire version
//    from camwire_1394.c
typedef struct camwire_user_data
{
    int camera_connected;   /* Flag.*/
    long int frame_number;  /* 8 months @ 100fps before 31-bit overflow.*/
    dc1394_cameracapture *control;
    Camwire_conf *config_cache;
    Camwire_state *current_set;
}
camwire_user_data_struct;
//    from camwirebus_internal_1394.h
typedef struct camwire_user_data 	*User_handle;
//    from camwire_1394.c
Camwire_state *get_shadow_state(const Camwire_handle c_handle)
{
    User_handle internal_status;

    internal_status = (User_handle)camwire_bus_get_userdata(c_handle);
    if (internal_status == NULL)  return(NULL);
    return( internal_status->current_set);
}

void CamContext_set_camera_property(CamContext *ccntxt, 
				    int property_number,
				    long Value,
				    int Auto ) {
  Camwire_handle c_handle;
  unsigned int feature;
  int (*set_function)( raw1394handle_t, nodeid_t, unsigned int );
  int retval;
  unsigned int shutter_reg;
  Camwire_state *shadow_state;

  /* Have to use libdc1394 functions here because camwire does all
     kinds of fancy stuff we don't want. */

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  c_handle = ccntxt->cam;
  
  switch (property_number) {
  case cic_shutter: 
    set_function = &dc1394_set_shutter; 
    feature = FEATURE_SHUTTER; 
    break;
  case cic_gain: 
    set_function = &dc1394_set_gain; 
    feature = FEATURE_GAIN; 
    break;
  case cic_brightness: 
    set_function = &dc1394_set_brightness;
    feature = FEATURE_BRIGHTNESS; break;
  default:     
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("unknown property_number");
    return;
  }

  retval = dc1394_auto_on_off( (raw1394handle_t)camwire_bus_get_port(c_handle),
			       (nodeid_t)camwire_bus_get_node(c_handle),
			       (unsigned int)feature,
			       (unsigned int)Auto);
  if (DC1394_SUCCESS != retval) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error setting property_number auto mode");
    return;
  }

  retval = (*set_function)( (raw1394handle_t)camwire_bus_get_port(c_handle),
			    (nodeid_t)camwire_bus_get_node(c_handle),
			    (unsigned int)Value);
  if (DC1394_SUCCESS != retval) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error setting property_number");
    return;
  }

  return;
}

void CamContext_grab_next_frame_blocking( CamContext *ccntxt, 
					  unsigned char *out_bytes, 
					  float timeout ) {
  Camwire_handle c_handle;
  int buffer_err;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  if (timeout >= 0) {
    CAM_IFACE_ERROR_FORMAT("timeout not implemented");
    return;
  }

  c_handle = ccntxt->cam;

  if (camwire_copy_next_frame(c_handle, out_bytes, NULL) != CAMWIRE_SUCCESS) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("Couldn't copy next frame.");
      return;
  }

  buffer_err = manage_buffer_level(c_handle);

  if (buffer_err) {
      cam_iface_error = CAM_IFACE_BUFFER_OVERFLOW_ERROR;
      CAM_IFACE_ERROR_FORMAT("Buffers overflowed");
      return;
  }

}

void CamContext_grab_next_frame_blocking_with_stride( CamContext *ccntxt, 
						      unsigned char *out_bytes, 
						      intptr_t stride0, 
						      float timeout ) {
  Camwire_handle c_handle;
  int buffer_err;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  if (timeout >= 0) {
    CAM_IFACE_ERROR_FORMAT("timeout not implemented");
    return;
  }

  c_handle = ccntxt->cam;

  if (camwire_copy_next_frame_with_stride(c_handle, out_bytes, NULL, stride0) != CAMWIRE_SUCCESS) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("Couldn't copy next frame.");
      return;
  }

  buffer_err = manage_buffer_level(c_handle);

  if (buffer_err) {
      cam_iface_error = CAM_IFACE_BUFFER_OVERFLOW_ERROR;
      CAM_IFACE_ERROR_FORMAT("Buffers overflowed");
      return;
  }

}

void CamContext_point_next_frame_blocking( CamContext *ccntxt, 
					   unsigned char **buf_ptr, 
					   float timeout){
  Camwire_handle c_handle;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  if (timeout >= 0) {
    CAM_IFACE_ERROR_FORMAT("timeout not implemented");
    return;
  }

  c_handle = ccntxt->cam;

  camwire_point_next_frame(c_handle, (void**)buf_ptr, NULL);

}

void CamContext_unpoint_frame( CamContext *ccntxt){
  Camwire_handle c_handle;
  int buffer_err;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  c_handle = ccntxt->cam;

  camwire_unpoint_frame(c_handle);
  buffer_err = manage_buffer_level(c_handle);
  if (buffer_err) {
      cam_iface_error = CAM_IFACE_BUFFER_OVERFLOW_ERROR;
      CAM_IFACE_ERROR_FORMAT("Buffers overflowed");
      return;
  }
}

void CamContext_get_last_timestamp( CamContext *ccntxt, double* timestamp ) {
  Camwire_handle c_handle;
  struct timespec tspec;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  c_handle = ccntxt->cam;

  if (camwire_get_timestamp(c_handle, &tspec) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_get_timestamp");
    return;
  }

  *timestamp = tspec.tv_sec + tspec.tv_nsec*1.0e-9;

  return;
}

void CamContext_get_last_framenumber( CamContext *ccntxt, long* framenumber ){
					   
  Camwire_handle c_handle;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  c_handle = ccntxt->cam;

  if (camwire_get_framenumber(c_handle, framenumber) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_get_framenumber");
    return;
  }

  return;
}

void CamContext_get_num_trigger_modes( CamContext *ccntxt, 
				       int *num_exposure_modes ) {
  Camwire_handle c_handle;
  dc1394bool_t has_trigger;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  c_handle = ccntxt->cam;

  if (DC1394_SUCCESS != dc1394_is_feature_present((raw1394handle_t)camwire_bus_get_port(c_handle),
						  (nodeid_t)camwire_bus_get_node(c_handle),
						  FEATURE_TRIGGER,
						  &has_trigger)) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error getting trigger availability");
    return;
  }
  if (has_trigger) {
    *num_exposure_modes = 2;
  } else {
    *num_exposure_modes = 1;
  }
}

void CamContext_get_trigger_mode_string( CamContext *ccntxt,
					 int exposure_mode_number,
					 char* exposure_mode_string, //output parameter
					 int exposure_mode_string_maxlen) {
  Camwire_handle c_handle;
  dc1394bool_t has_trigger;
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  c_handle = ccntxt->cam;
  if (DC1394_SUCCESS != dc1394_is_feature_present((raw1394handle_t)camwire_bus_get_port(c_handle),
						  (nodeid_t)camwire_bus_get_node(c_handle),
						  FEATURE_TRIGGER,
						  &has_trigger)) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error getting trigger availability");
    return;
  }

  if (!has_trigger) {
    if (exposure_mode_number!=0) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("exposure_mode_number invalid");
      return;
    }
    snprintf(exposure_mode_string,exposure_mode_string_maxlen,"(camera default)");
    return;
  }

  switch (exposure_mode_number) {
  case 0:
    snprintf(exposure_mode_string,exposure_mode_string_maxlen,"internal trigger, freerunning");
    break;
  case 1:
    snprintf(exposure_mode_string,exposure_mode_string_maxlen,"external trigger, rising edge");
    break;
  default:
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("exposure_mode_number invalid");
    return;
  }
}

void CamContext_get_trigger_mode_number( CamContext *ccntxt,
					 int *exposure_mode_number ) {
  Camwire_handle c_handle;
  int exttrig;
  dc1394bool_t has_trigger;

  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  
  c_handle = ccntxt->cam;

  if (DC1394_SUCCESS != dc1394_is_feature_present((raw1394handle_t)camwire_bus_get_port(c_handle),
						  (nodeid_t)camwire_bus_get_node(c_handle),
						  FEATURE_TRIGGER,
						  &has_trigger)) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error getting trigger availability");
    return;
  }
  
  if (!has_trigger) {
    return 0;
  }

  if (camwire_get_trigger_source(c_handle, &exttrig) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_get_trigger_source");
    return;
  }
  
  if (!exttrig) {
    *exposure_mode_number = 0;
  } else {
    *exposure_mode_number = 1;
  }

  return;
}

void CamContext_set_trigger_mode_number( CamContext *ccntxt,
					 int exposure_mode_number ) {
  Camwire_handle c_handle;
  int exttrig;
  dc1394bool_t has_trigger;
  
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  
  c_handle = ccntxt->cam;

  if (DC1394_SUCCESS != dc1394_is_feature_present((raw1394handle_t)camwire_bus_get_port(c_handle),
						  (nodeid_t)camwire_bus_get_node(c_handle),
						  FEATURE_TRIGGER,
						  &has_trigger)) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error getting trigger availability");
    return;
  }

  if (!has_trigger) {
    if (exposure_mode_number!=0) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("invalid trigger mode");
    }
    return;
  }

  switch (exposure_mode_number) {
  case 0:
    exttrig = 0;
    break;
  case 1:
    exttrig = 1;
    break;
  default:
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("exposure_mode_number invalid");
    return;
  }

  if (camwire_set_trigger_source(c_handle, exttrig) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_set_trigger_source");
    return;
  }

  return;
}

void CamContext_get_frame_offset( CamContext *ccntxt, 
				  int *left, int *top ) {
  Camwire_handle c_handle;
  
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  
  c_handle = ccntxt->cam;
  
  if (camwire_get_frame_offset(c_handle, left, top) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_get_frame_offset");
    return;
  }

  return;

}

void CamContext_set_frame_offset( CamContext *ccntxt, 
				  int left, int top ) {
  Camwire_handle c_handle;
  
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  
  c_handle = ccntxt->cam;
  
  if (camwire_set_frame_offset(c_handle, left, top) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_set_frame_offset");
    return;
  }

  return;

}


void CamContext_get_frame_size( CamContext *ccntxt, 
				int *width, int *height ) {
  Camwire_handle c_handle;
  
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  
  c_handle = ccntxt->cam;
  
  if (camwire_get_frame_size(c_handle, width, height) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_get_frame_size");
    return;
  }

  return;

}

void CamContext_set_frame_size( CamContext *ccntxt, 
				int width, int height ) {
  Camwire_handle c_handle;
  Camwire_state set;
  cam_iface_backend_extras* backend_extras;
  
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  
  c_handle = ccntxt->cam;
  backend_extras = (cam_iface_backend_extras*)ccntxt->backend_extras;

  if (camwire_get_state(c_handle, &set) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_set_state");
    return;
  }
 
  if (camwire_set_frame_size(c_handle, width, height) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_set_frame_size");
    return;
  }

  backend_extras->roi_width = width;
  backend_extras->roi_height = height;
  backend_extras->buffer_size = (width*height*ccntxt->depth/8);
  return;

}

void CamContext_get_max_frame_size( CamContext *ccntxt, 
				    int *width, int *height ){
  cam_iface_backend_extras* backend_extras;
  
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  
  backend_extras = (cam_iface_backend_extras*)ccntxt->backend_extras;
  *width = backend_extras->max_width;
  *height = backend_extras->max_height;
}

void CamContext_get_buffer_size( CamContext *ccntxt,
				 int *size) {
  cam_iface_backend_extras* backend_extras;
  
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  
  backend_extras = (cam_iface_backend_extras*)ccntxt->backend_extras;
  *size = backend_extras->buffer_size;
}

void CamContext_get_framerate( CamContext *ccntxt, 
			       float *framerate ) {
  Camwire_handle c_handle;
  
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  
  c_handle = ccntxt->cam;
  
  if (camwire_get_framerate(c_handle, framerate) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_get_framerate");
    return;
  }

  return;

}

void CamContext_set_framerate( CamContext *ccntxt, 
			       float framerate ) {
  Camwire_handle c_handle;
  
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  
  c_handle = ccntxt->cam;
  
  if (camwire_set_framerate(c_handle, framerate) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_set_framerate");
    return;
  }

  return;

}

void CamContext_get_num_framebuffers( CamContext *ccntxt, 
				      int *num_framebuffers ) {
  Camwire_handle c_handle;

  ASSERT_CAM_CONTEXT_DEFINED(ccntxt);
  
  c_handle = ccntxt->cam;
  
  if (camwire_get_num_framebuffers(c_handle, num_framebuffers) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_get_num_framebuffers");
    return;
  }

  return;

}

void CamContext_set_num_framebuffers( CamContext *ccntxt, 
				      int num_framebuffers ) {
  Camwire_handle c_handle;

  ASSERT_CAM_CONTEXT_DEFINED(ccntxt);
  
  c_handle = ccntxt->cam;
  
  if (camwire_set_num_framebuffers(c_handle, num_framebuffers) != CAMWIRE_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error calling camwire_set_num_framebuffers");
    return;
  }

  return;
}
