/* $Id: $ */
#include "cam_iface.h"

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/videodev.h>
#include <linux/videodev2.h>

struct CCv4l2; // forward declaration

// keep functable in sync across backends
typedef struct {
  cam_iface_constructor_func_t construct;
  void (*destruct)(struct CamContext*);

  void (*CCv4l2)(struct CCv4l2*,int,int,int);
  void (*close)(struct CCv4l2*);
  void (*start_camera)(struct CCv4l2*);
  void (*stop_camera)(struct CCv4l2*);
  void (*get_num_camera_properties)(struct CCv4l2*,int*);
  void (*get_camera_property_info)(struct CCv4l2*,
				   int,
				   CameraPropertyInfo*);
  void (*get_camera_property)(struct CCv4l2*,int,long*,int*);
  void (*set_camera_property)(struct CCv4l2*,int,long,int);
  void (*grab_next_frame_blocking)(struct CCv4l2*,
				   unsigned char*,
				   float);
  void (*grab_next_frame_blocking_with_stride)(struct CCv4l2*,
					       unsigned char*,
					       intptr_t,
					       float);
  void (*point_next_frame_blocking)(struct CCv4l2*,unsigned char**,float);
  void (*unpoint_frame)(struct CCv4l2*);
  void (*get_last_timestamp)(struct CCv4l2*,double*);
  void (*get_last_framenumber)(struct CCv4l2*,long*);
  void (*get_num_trigger_modes)(struct CCv4l2*,int*);
  void (*get_trigger_mode_string)(struct CCv4l2*,int,char*,int);
  void (*get_trigger_mode_number)(struct CCv4l2*,int*);
  void (*set_trigger_mode_number)(struct CCv4l2*,int);
  void (*get_frame_offset)(struct CCv4l2*,int*,int*);
  void (*set_frame_offset)(struct CCv4l2*,int,int);
  void (*get_frame_size)(struct CCv4l2*,int*,int*);
  void (*set_frame_size)(struct CCv4l2*,int,int);
  void (*get_max_frame_size)(struct CCv4l2*,int*,int*);
  void (*get_buffer_size)(struct CCv4l2*,int*);
  void (*get_framerate)(struct CCv4l2*,float*);
  void (*set_framerate)(struct CCv4l2*,float);
  void (*get_num_framebuffers)(struct CCv4l2*,int*);
  void (*set_num_framebuffers)(struct CCv4l2*,int);
} CCv4l2_functable;

typedef struct CCv4l2 {
  CamContext inherited;
  int is_v2;
  int deviceHandle;
  struct video_capability capability;
  struct v4l2_capability cap;
  int started;
  int num_buffers;
  int width;
  int height;
  long last_framenumber;
  double last_timestamp;
} CCv4l2;

// forward declarations
CCv4l2* CCv4l2_construct( int device_number, int NumImageBuffers,
			      int mode_number);
void delete_CCv4l2(struct CCv4l2*);

void CCv4l2_CCv4l2(struct CCv4l2*,int,int,int);
void CCv4l2_close(struct CCv4l2*);
void CCv4l2_start_camera(struct CCv4l2*);
void CCv4l2_stop_camera(struct CCv4l2*);
void CCv4l2_get_num_camera_properties(struct CCv4l2*,int*);
void CCv4l2_get_camera_property_info(struct CCv4l2*,
			      int,
			      CameraPropertyInfo*);
void CCv4l2_get_camera_property(struct CCv4l2*,int,long*,int*);
void CCv4l2_set_camera_property(struct CCv4l2*,int,long,int);
void CCv4l2_grab_next_frame_blocking(struct CCv4l2*,
			      unsigned char*,
			      float);
void CCv4l2_grab_next_frame_blocking_with_stride(struct CCv4l2*,
					  unsigned char*,
					  intptr_t,
					  float);
void CCv4l2_point_next_frame_blocking(struct CCv4l2*,unsigned char**,float);
void CCv4l2_unpoint_frame(struct CCv4l2*);
void CCv4l2_get_last_timestamp(struct CCv4l2*,double*);
void CCv4l2_get_last_framenumber(struct CCv4l2*,long*);
void CCv4l2_get_num_trigger_modes(struct CCv4l2*,int*);
void CCv4l2_get_trigger_mode_string(struct CCv4l2*,int,char*,int);
void CCv4l2_get_trigger_mode_number(struct CCv4l2*,int*);
void CCv4l2_set_trigger_mode_number(struct CCv4l2*,int);
void CCv4l2_get_frame_offset(struct CCv4l2*,int*,int*);
void CCv4l2_set_frame_offset(struct CCv4l2*,int,int);
void CCv4l2_get_frame_size(struct CCv4l2*,int*,int*);
void CCv4l2_set_frame_size(struct CCv4l2*,int,int);
void CCv4l2_get_max_frame_size(struct CCv4l2*,int*,int*);
void CCv4l2_get_buffer_size(struct CCv4l2*,int*);
void CCv4l2_get_framerate(struct CCv4l2*,float*);
void CCv4l2_set_framerate(struct CCv4l2*,float);
void CCv4l2_get_num_framebuffers(struct CCv4l2*,int*);
void CCv4l2_set_num_framebuffers(struct CCv4l2*,int);

CCv4l2_functable CCv4l2_vmt = {
  (cam_iface_constructor_func_t)CCv4l2_construct,
  (void (*)(CamContext*))delete_CCv4l2,
  CCv4l2_CCv4l2,
  CCv4l2_close,
  CCv4l2_start_camera,
  CCv4l2_stop_camera,
  CCv4l2_get_num_camera_properties,
  CCv4l2_get_camera_property_info,
  CCv4l2_get_camera_property,
  CCv4l2_set_camera_property,
  CCv4l2_grab_next_frame_blocking,
  CCv4l2_grab_next_frame_blocking_with_stride,
  CCv4l2_point_next_frame_blocking,
  CCv4l2_unpoint_frame,
  CCv4l2_get_last_timestamp,
  CCv4l2_get_last_framenumber,
  CCv4l2_get_num_trigger_modes,
  CCv4l2_get_trigger_mode_string,
  CCv4l2_get_trigger_mode_number,
  CCv4l2_set_trigger_mode_number,
  CCv4l2_get_frame_offset,
  CCv4l2_set_frame_offset,
  CCv4l2_get_frame_size,
  CCv4l2_set_frame_size,
  CCv4l2_get_max_frame_size,
  CCv4l2_get_buffer_size,
  CCv4l2_get_framerate,
  CCv4l2_set_framerate,
  CCv4l2_get_num_framebuffers,
  CCv4l2_set_num_framebuffers
};

/* globals -- allocate space */
__thread int cam_iface_error = 0;
#define CAM_IFACE_MAX_ERROR_LEN 255
__thread char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN]  = {0x00}; //...
#define CAM_IFACE_MAX_NUM_CAMERAS 255
int num_cameras=0;
int camera_nums[CAM_IFACE_MAX_NUM_CAMERAS];

#define cam_iface_snprintf(...) snprintf(__VA_ARGS__)

#define CAM_IFACE_ERROR_FORMAT(m) \
cam_iface_snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,"%s (%d): %s\n",__FILE__,__LINE__,(m));

#define CHECK_CC(m)							\
  if (!(m)) {								\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");	\
    return;								\
  }

#define CAM_IFACE_THROW_ERROR(m)			\
  {							\
    cam_iface_error = -1;				\
    CAM_IFACE_ERROR_FORMAT((m));			\
    return;						\
  }

#define NOT_IMPLEMENTED							\
  cam_iface_error = -1;							\
  CAM_IFACE_ERROR_FORMAT("Function not implemented.");			\
  return;

double ci_v4l2_floattime() {
  struct timeval t;
  if (gettimeofday(&t, (struct timezone *)NULL) == 0)
    return (double)t.tv_sec + t.tv_usec*0.000001;
  else
    return 0.0;
}

// This function from cvcap_v4l.cpp, Intel OpenCV library, licensed
// under a BSD-like license.

// IOCTL handling for V4L2
static int xioctl( int fd, int request, void *arg)
{

  int r;


  do r = ioctl (fd, request, arg);
  while (-1 == r && EINTR == errno);

  return r;

}

const char *cam_iface_get_driver_name() {
  return "v4l2";
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
#define civ4l2_strlen 100
  int i;
  int fd;
  char device[civ4l2_strlen];

  num_cameras = 0;
  for (i=0;i<CAM_IFACE_MAX_NUM_CAMERAS;i++) {
    snprintf(device, civ4l2_strlen, "/dev/video%1d", i);
    fd = open(device, O_RDONLY);
    if (fd != -1) {
      printf("opened camera %d\n",i);
      camera_nums[num_cameras] = i;
      num_cameras++;
      close(fd);
    }
  }
}

void cam_iface_shutdown() {
}


int cam_iface_get_num_cameras() {
  return num_cameras;
}

void cam_iface_get_camera_info(int index, Camwire_id *out_camid) {
  if (out_camid==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("return structure NULL");
    return;
  }
  snprintf(out_camid->vendor, CAMWIRE_ID_MAX_CHARS, "<unknown vendor>");
  snprintf(out_camid->model, CAMWIRE_ID_MAX_CHARS, "<unknown model>");
  snprintf(out_camid->chip, CAMWIRE_ID_MAX_CHARS, "<unknown chip>");
}


void cam_iface_get_num_modes(int device_number, int *num_modes) {
  *num_modes = 1;
}

void cam_iface_get_mode_string(int device_number,
			       int mode_number,
			       char* mode_string,
			       int mode_string_maxlen) {
  cam_iface_snprintf(mode_string,mode_string_maxlen,"(v4l2 mode string)");
}

cam_iface_constructor_func_t cam_iface_get_constructor_func(int device_number) {
  return (CamContext* (*)(int, int, int))CCv4l2_construct;
}

CCv4l2 * CCv4l2_construct( int device_number, int NumImageBuffers,
			       int mode_number) {
  CCv4l2 *this = NULL;

  this = malloc(sizeof(CCv4l2));
  if (this==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error allocating memory");
  } else {
    CCv4l2_CCv4l2( this,
		   device_number, NumImageBuffers,
		   mode_number);
    if (cam_iface_error) {
      free(this);
      return NULL;
    }
  }
  return this;
}

void delete_CCv4l2(struct CCv4l2 *this) {
  CCv4l2_close(this);
  free(this);
}

void CCv4l2_close( struct CCv4l2 *this) {
  CHECK_CC(this);
}

void CCv4l2_CCv4l2( struct CCv4l2 *this, int device_number, int NumImageBuffers,
		    int mode_number) {
  char device[civ4l2_strlen];

  // call parent
  CamContext_CamContext((CamContext*)this,device_number,NumImageBuffers,mode_number);
  this->inherited.vmt = (CamContext_functable*)&CCv4l2_vmt;

  if (device_number >= num_cameras) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("requested too high camera number");
    return;
  }

  snprintf(device, civ4l2_strlen, "/dev/video%1d", camera_nums[device_number]);
  this->deviceHandle = open(device, O_RDWR | O_NONBLOCK, 0);
  if (this->deviceHandle == 0) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("could not open device for read/write access");
    return;
  }

  this->is_v2 = 1;
  if (xioctl(this->deviceHandle, VIDIOC_QUERYCAP, &this->cap) < 0) {
    this->is_v2 = 0;
    close( this->deviceHandle );

    this->deviceHandle = open(device, O_RDWR );
    if (this->deviceHandle == 0) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("could not open device for read/write access");
      return;
    }
    if (ioctl(this->deviceHandle, VIDIOCGCAP, &this->capability) < 0) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("could not query device for capabilities");
      return;
    }
    if ((this->capability.type & VID_TYPE_CAPTURE) == 0) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("device is not a video capture device");
      return;
    }
    printf("channels %d\n",this->capability.channels);
  }

  this->num_buffers = NumImageBuffers;
  this->started = 0;
  this->width = 640;
  this->height = 480;
  this->last_framenumber = -1;
  this->last_timestamp = -1.0;
  printf("setting width = %d\n",this->width);

  /* initialize */
  this->inherited.cam = (void *)NULL;
  this->inherited.backend_extras = (void *)NULL;
  this->inherited.coding = CAM_IFACE_MONO8;
  this->inherited.depth = 8;
  this->inherited.device_number = device_number;

}

void CCv4l2_start_camera( struct CCv4l2 *this ) {
  CHECK_CC(this);
  this->started = 1;
}

void CCv4l2_stop_camera( struct CCv4l2 *this ) {
  CHECK_CC(this);
  this->started = 0;
}

void CCv4l2_get_num_camera_properties(struct CCv4l2 *this,
					  int* num_properties) {
  CHECK_CC(this);
  fprintf(stderr,"WARNING: CCv4l2_get_num_camera_properties() not implemented, returning 0\n");
  *num_properties = 0;
}

void CCv4l2_get_camera_property_info(struct CCv4l2 *this,
					 int property_number,
					 CameraPropertyInfo *info) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCv4l2_get_camera_property(struct CCv4l2 *this,
				    int property_number,
				    long* Value,
				    int* Auto ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCv4l2_set_camera_property(struct CCv4l2 *this,
				    int property_number,
				    long Value,
				    int Auto ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCv4l2_grab_next_frame_blocking_with_stride( struct CCv4l2 *this, unsigned char *out_bytes, intptr_t stride0, float timeout) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCv4l2_grab_next_frame_blocking( struct CCv4l2 *this, unsigned char *out_bytes, float timeout) {
  CHECK_CC(this);
  long fno=0;
  bzero(out_bytes,(this->width) * (this->height) * (this->inherited.depth)/8 );
  NOT_IMPLEMENTED;
  this->last_timestamp = ci_v4l2_floattime();
  this->last_framenumber++;
}

void CCv4l2_point_next_frame_blocking( struct CCv4l2 *this, unsigned char **buf_ptr, float timeout){
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCv4l2_unpoint_frame( struct CCv4l2 *this){
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCv4l2_get_last_timestamp( struct CCv4l2 *this, double* timestamp ) {
  CHECK_CC(this);
  *timestamp = this->last_timestamp;
}

void CCv4l2_get_last_framenumber( struct CCv4l2 *this, long* framenumber ){
  CHECK_CC(this);
  *framenumber = this->last_framenumber;
}

void CCv4l2_get_num_trigger_modes( struct CCv4l2 *this,
				       int *num_exposure_modes ) {
  CHECK_CC(this);
  *num_exposure_modes = 1;
}

void CCv4l2_get_trigger_mode_string( struct CCv4l2 *this,
				     int exposure_mode_number,
				     char* exposure_mode_string, //output parameter
				     int exposure_mode_string_maxlen) {
  if (!this) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CCv4l2 specified (NULL argument)");
    return;
  }

  switch (exposure_mode_number) {
  case 0:
    cam_iface_snprintf(exposure_mode_string,exposure_mode_string_maxlen,"default trigger mode");
    break;
  default:
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("exposure_mode_number invalid");
    return;
  }
}

void CCv4l2_get_trigger_mode_number( struct CCv4l2 *this,
					 int *exposure_mode_number ) {
  CHECK_CC(this);
  *exposure_mode_number = 0;
}

void CCv4l2_set_trigger_mode_number( struct CCv4l2 *this,
					 int exposure_mode_number ) {
  CHECK_CC(this);
  if (exposure_mode_number != 0) {
    CAM_IFACE_THROW_ERROR("exposure_mode_number invalid");
  }
}

void CCv4l2_get_frame_offset( struct CCv4l2 *this,
				  int *left, int *top ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCv4l2_set_frame_offset( struct CCv4l2 *this,
				  int left, int top ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}


void CCv4l2_get_frame_size( struct CCv4l2 *this,
			    int *width, int *height ) {
  CHECK_CC(this);
  printf("getting width = %d\n",this->width);

  *width = this->width;
  *height = this->height;
}

void CCv4l2_set_frame_size( struct CCv4l2 *this,
				int width, int height ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCv4l2_get_buffer_size( struct CCv4l2 *this,
					int *size){
  CHECK_CC(this);
  *size = (this->width) * (this->height) * (this->inherited.depth)/8 ;
}

void CCv4l2_get_framerate( struct CCv4l2 *this,
			       float *framerate ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCv4l2_set_framerate( struct CCv4l2 *this,
			       float framerate ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCv4l2_get_max_frame_size( struct CCv4l2 *this,
				    int *width, int *height ){
  CHECK_CC(this);
  *width = this->width;
  *height = this->height;
}

void CCv4l2_get_num_framebuffers( struct CCv4l2 *this,
				      int *num_framebuffers ) {
  CHECK_CC(this);
  *num_framebuffers =  this->num_buffers;
}

void CCv4l2_set_num_framebuffers( struct CCv4l2 *this,
				      int num_framebuffers ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}
