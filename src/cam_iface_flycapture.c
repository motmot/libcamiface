// heavily modified version of cam_iface_camwire.c, 
// for Point Grey Research FlyCapture API in Windows

#include "PGRFlyCapture.h"

#include "cam_iface.h"

#include <time.h> /* for nanosleep */
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_SAVE_DELAY	0.0
#define DEFAULT_SAVE_NUM_IMAGES	1
#define BUF_LOW_MARK		(1.0/3.0)
#define BUF_HIGH_MARK		(2.0/3.0)
#define MAX_CAMS			16

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

/* globals -- allocate space */
int cam_iface_error;
#define CAM_IFACE_MAX_ERROR_LEN 255
char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN];

#define CAM_IFACE_ERROR_FORMAT(m) \
sprintf(cam_iface_error_string,"%s (%d): %s\n",__FILE__,__LINE__,(m));

FlyCaptureInfo handle_array[MAX_CAMS];
unsigned int num_cameras;

double flycapture_last_timestamp; // updated each time an image is grabbed


const char *cam_iface_get_driver_name() {
  return "flycapture";
}

void cam_iface_clear_error() {
  cam_iface_error = 0;
}

int cam_iface_have_error() {
  if (cam_iface_error != 0) {
    return 1;
  }
  return 0;
}

const char * cam_iface_get_error_string() {
  return cam_iface_error_string;
}

extern void cam_iface_startup() {
    int retry;
    int bad_bus;

    /* Initialize the camera bus:*/
	num_cameras = MAX_CAMS; // value gets changed by following function call
	cam_iface_error = flycaptureBusEnumerateCameras(handle_array, &num_cameras);
    
/*	if (num_cameras < 0 || handle_array == NULL)
    {
	retry = 1;
	bad_bus = 1;
	while (retry <= 1 && bad_bus)
	{
	    camwire_bus_reset();
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
    }*/
}

extern void cam_iface_shutdown() {
//  camwire_bus_destroy();
}


int cam_iface_get_num_cameras() {
  return num_cameras;
}

void cam_iface_get_camera_info(int index, Camwire_id *out_camid) {
	printf("ERROR: cam_iface_get_camera_info not implemented!\n");	
/*  if (camwire_get_identifier(handle_array[index], out_camid) !=
      CAMWIRE_SUCCESS)
  {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("Could not get the identifier");
  }*/
}

CamContext * new_CamContext( int device_number, int NumImageBuffers) {
  CamContext *out_cr = NULL;

  out_cr = (CamContext*)malloc(sizeof(CamContext));
  if (!out_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("malloc failed");
    return NULL;
  }

  /* initialize */
  out_cr->cam = (void *)NULL;
  out_cr->ppBuffers = (void *)NULL;
  
  if (device_number > num_cameras) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("requested too high camera number");
    return NULL;
  }

  cam_iface_error = flycaptureCreateContext((FlyCaptureContext*)&(out_cr->cam));
  cam_iface_error = flycaptureInitialize( (out_cr->cam), device_number );

  // FlyCapture doesn't provide many parameters until image is actually grabbed
  // Unfortunately, it's possible that some programs might try to use these
  out_cr->NumImageBuffers = 0;
  out_cr->ImageBufferSize = 0;
  out_cr->max_width = 0;
  out_cr->max_height = 0;
  out_cr->depth = 0;
  out_cr->buffer_size = 0;

  return out_cr;
}

void delete_CamContext(CamContext *in_cr) {
  FlyCaptureContext c_handle;

  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  c_handle = in_cr->cam;
  
  if (c_handle != NULL) {
  	flycaptureStop(c_handle);
  }

  flycaptureDestroyContext(c_handle);
  free(in_cr);
}

void CamContext_start_camera( CamContext *in_cr ) {
  FlyCaptureContext context;

  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  context = in_cr->cam;
  flycaptureStart(context, FLYCAPTURE_VIDEOMODE_ANY, FLYCAPTURE_FRAMERATE_ANY);

  // we'll cheat now and grab an image, so we can get width and height info
  FlyCaptureImage image;
  image.iCols = 0; image.iRows = 0; // initialize structure to sane values 
  flycaptureGrabImage2( context, &image);
  in_cr->max_width = image.iCols;
  in_cr->max_height = image.iRows;
}

void CamContext_stop_camera( CamContext *in_cr ) {
  FlyCaptureContext context;

  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  context = in_cr->cam;
  flycaptureStop(context);
}

// helper function to get the equivalent FlyCaptureProperty
FlyCaptureProperty CamContext_convertToFCProperty(CameraProperty camProperty)
{
	switch(camProperty) {
		case BRIGHTNESS: 
			return FLYCAPTURE_BRIGHTNESS;
		case SHUTTER: 
			return FLYCAPTURE_SHUTTER; 
		case GAIN: 
			return FLYCAPTURE_GAIN; 
		default:
			cam_iface_error = -1;
			CAM_IFACE_ERROR_FORMAT("invalid property type");
			return FLYCAPTURE_BRIGHTNESS;
	}
}

void CamContext_get_camera_property(CamContext *in_cr,
				    enum CameraProperty cameraProperty,
				    long* ValueA,
				    long* ValueB,
				    int* Auto ) {
	int retval;
	FlyCaptureProperty fcProperty;
  	FlyCaptureContext context;

  	context = in_cr->cam;
	fcProperty = CamContext_convertToFCProperty(cameraProperty);
	retval = flycaptureGetCameraProperty(context, fcProperty, 
		ValueA, ValueB, (bool*)Auto);
}

void CamContext_get_camera_property_range(CamContext *in_cr, 
					  enum CameraProperty cameraProperty,
					  int* Present,
					  long* Min,
					  long* Max,
					  long* Default,
					  int* Auto,
					  int* Manual) {
	int retval;
	FlyCaptureProperty fcProperty;
  	FlyCaptureContext context;

  	context = in_cr->cam;
	fcProperty = CamContext_convertToFCProperty(cameraProperty);
	retval = flycaptureGetCameraPropertyRange(context, fcProperty, 
		(bool*)Present, Min, Max, Default, (bool*)Auto, (bool*)Manual);
}

void CamContext_set_camera_property(CamContext *in_cr, 
				    enum CameraProperty cameraProperty,
				    long ValueA,
				    long ValueB,
				    int Auto ) {
	int retval;
	FlyCaptureProperty fcProperty;
  	FlyCaptureContext context;

  	context = in_cr->cam;
	fcProperty = CamContext_convertToFCProperty(cameraProperty);
	retval = flycaptureSetCameraProperty(context, fcProperty, 
		ValueA, ValueB, Auto);
}

void CamContext_grab_next_frame_blocking( CamContext *in_cr, unsigned char *out_bytes ) {
  FlyCaptureImage tempImage;
  FlyCaptureContext c_handle;
  long timestampSeconds, timestampMicroSeconds;

  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  c_handle = in_cr->cam;

  //NOTE: not sure about when buffer memory is allocated and destroyed
  flycaptureGrabImage2((FlyCaptureContext)c_handle, &tempImage);
  out_bytes = tempImage.pData;
 
  // update global timestamp value
  flycapture_last_timestamp = (double)tempImage.timeStamp.ulSeconds
  		+ (double)(tempImage.timeStamp.ulMicroSeconds)/1000.0;
}

void CamContext_get_last_timestamp( CamContext *in_cr, double* timestamp ) {
  *timestamp = flycapture_last_timestamp;
}
