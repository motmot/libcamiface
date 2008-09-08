/* blank backend */
#include "cam_iface.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct CCblank; // forward declaration

// keep functable in sync across backends
typedef struct {
  cam_iface_constructor_func_t construct;
  void (*destruct)(struct CamContext*);

  void (*CCblank)(struct CCblank*,int,int,int);
  void (*close)(struct CCblank*);
  void (*start_camera)(struct CCblank*);
  void (*stop_camera)(struct CCblank*);
  void (*get_num_camera_properties)(struct CCblank*,int*);
  void (*get_camera_property_info)(struct CCblank*,
				   int,
				   CameraPropertyInfo*);
  void (*get_camera_property)(struct CCblank*,int,long*,int*);
  void (*set_camera_property)(struct CCblank*,int,long,int);
  void (*grab_next_frame_blocking)(struct CCblank*,
				   unsigned char*,
				   float);
  void (*grab_next_frame_blocking_with_stride)(struct CCblank*,
					       unsigned char*,
					       intptr_t,
					       float);
  void (*point_next_frame_blocking)(struct CCblank*,unsigned char**,float);
  void (*unpoint_frame)(struct CCblank*);
  void (*get_last_timestamp)(struct CCblank*,double*);
  void (*get_last_framenumber)(struct CCblank*,unsigned long*);
  void (*get_num_trigger_modes)(struct CCblank*,int*);
  void (*get_trigger_mode_string)(struct CCblank*,int,char*,int);
  void (*get_trigger_mode_number)(struct CCblank*,int*);
  void (*set_trigger_mode_number)(struct CCblank*,int);
  void (*get_frame_offset)(struct CCblank*,int*,int*);
  void (*set_frame_offset)(struct CCblank*,int,int);
  void (*get_frame_size)(struct CCblank*,int*,int*);
  void (*set_frame_size)(struct CCblank*,int,int);
  void (*get_max_frame_size)(struct CCblank*,int*,int*);
  void (*get_buffer_size)(struct CCblank*,int*);
  void (*get_framerate)(struct CCblank*,float*);
  void (*set_framerate)(struct CCblank*,float);
  void (*get_num_framebuffers)(struct CCblank*,int*);
  void (*set_num_framebuffers)(struct CCblank*,int);
} CCblank_functable;

typedef struct CCblank {
  CamContext inherited;

  int cam_iface_mode_number; // different than DC1934 mode number

  int max_width;       // maximum buffer width
  int max_height;      // maximum buffer height
  int roi_width;
  int roi_height;
  int buffer_size;     // bytes per frame
  long nframe_hack;

  int num_dma_buffers;
  uint64_t last_timestamp;

  // for select()
  int fileno;
  fd_set fdset;
  int nfds;
  int capture_is_set;
} CCblank;

// forward declarations
CCblank* CCblank_construct( int device_number, int NumImageBuffers,
			      int mode_number);
void delete_CCblank(struct CCblank*);

void CCblank_CCblank(struct CCblank*,int,int,int);
void CCblank_close(struct CCblank*);
void CCblank_start_camera(struct CCblank*);
void CCblank_stop_camera(struct CCblank*);
void CCblank_get_num_camera_properties(struct CCblank*,int*);
void CCblank_get_camera_property_info(struct CCblank*,
			      int,
			      CameraPropertyInfo*);
void CCblank_get_camera_property(struct CCblank*,int,long*,int*);
void CCblank_set_camera_property(struct CCblank*,int,long,int);
void CCblank_grab_next_frame_blocking(struct CCblank*,
			      unsigned char*,
			      float);
void CCblank_grab_next_frame_blocking_with_stride(struct CCblank*,
					  unsigned char*,
					  intptr_t,
					  float);
void CCblank_point_next_frame_blocking(struct CCblank*,unsigned char**,float);
void CCblank_unpoint_frame(struct CCblank*);
void CCblank_get_last_timestamp(struct CCblank*,double*);
void CCblank_get_last_framenumber(struct CCblank*,unsigned long*);
void CCblank_get_num_trigger_modes(struct CCblank*,int*);
void CCblank_get_trigger_mode_string(struct CCblank*,int,char*,int);
void CCblank_get_trigger_mode_number(struct CCblank*,int*);
void CCblank_set_trigger_mode_number(struct CCblank*,int);
void CCblank_get_frame_offset(struct CCblank*,int*,int*);
void CCblank_set_frame_offset(struct CCblank*,int,int);
void CCblank_get_frame_size(struct CCblank*,int*,int*);
void CCblank_set_frame_size(struct CCblank*,int,int);
void CCblank_get_max_frame_size(struct CCblank*,int*,int*);
void CCblank_get_buffer_size(struct CCblank*,int*);
void CCblank_get_framerate(struct CCblank*,float*);
void CCblank_set_framerate(struct CCblank*,float);
void CCblank_get_num_framebuffers(struct CCblank*,int*);
void CCblank_set_num_framebuffers(struct CCblank*,int);

CCblank_functable CCblank_vmt = {
  (cam_iface_constructor_func_t)CCblank_construct,
  (void (*)(CamContext*))delete_CCblank,
  CCblank_CCblank,
  CCblank_close,
  CCblank_start_camera,
  CCblank_stop_camera,
  CCblank_get_num_camera_properties,
  CCblank_get_camera_property_info,
  CCblank_get_camera_property,
  CCblank_set_camera_property,
  CCblank_grab_next_frame_blocking,
  CCblank_grab_next_frame_blocking_with_stride,
  CCblank_point_next_frame_blocking,
  CCblank_unpoint_frame,
  CCblank_get_last_timestamp,
  CCblank_get_last_framenumber,
  CCblank_get_num_trigger_modes,
  CCblank_get_trigger_mode_string,
  CCblank_get_trigger_mode_number,
  CCblank_set_trigger_mode_number,
  CCblank_get_frame_offset,
  CCblank_set_frame_offset,
  CCblank_get_frame_size,
  CCblank_set_frame_size,
  CCblank_get_max_frame_size,
  CCblank_get_buffer_size,
  CCblank_get_framerate,
  CCblank_set_framerate,
  CCblank_get_num_framebuffers,
  CCblank_set_num_framebuffers
};

/* globals -- allocate space */
__thread int cam_iface_error = 0;
#define CAM_IFACE_MAX_ERROR_LEN 255
__thread char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN]  = {0x00}; //...

uint32_t num_cameras = 0;

#define CAM_IFACE_ERROR_FORMAT(m)					\
  snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,		\
	   "%s (%d): %s\n",__FILE__,__LINE__,(m));

#define CAM_IFACE_CHECK_DEVICE_NUMBER(m)				\
  if ( ((m)<0) | ((m)>=num_cameras) ) {					\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("invalid device_number");			\
    return;								\
  }

#define CHECK_CC(m)							\
  if (!(m)) {								\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");	\
    return;								\
  }

#define CHECK_CCV(m)							\
  if (!(m)) {								\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");	\
    return NULL;							\
  }


#define NOT_IMPLEMENTED					\
  cam_iface_error = -1;					\
  CAM_IFACE_ERROR_FORMAT("not yet implemented");	\
  return;

const char *cam_iface_get_driver_name() {
  return "blank";
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
}

void cam_iface_shutdown() {
}

int cam_iface_get_num_cameras() {
  return num_cameras;
}

void cam_iface_get_camera_info(int device_number, Camwire_id *out_camid) {
  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);

  if (out_camid==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("return structure NULL");
    return;
  }
  snprintf(out_camid->vendor, CAMWIRE_ID_MAX_CHARS, "blank vendor");
  snprintf(out_camid->model, CAMWIRE_ID_MAX_CHARS, "blank model");
  snprintf(out_camid->chip, CAMWIRE_ID_MAX_CHARS, "blank chip");
}

void cam_iface_get_num_modes(int device_number, int *num_modes) {
  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  *num_modes = 0;
}
void cam_iface_get_mode_string(int device_number,
			       int mode_number,
			       char* mode_string,
			       int mode_string_maxlen) {

  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  snprintf(mode_string,mode_string_maxlen,"mode %d",mode_number);
}

cam_iface_constructor_func_t cam_iface_get_constructor_func(int device_number) {
  return (CamContext* (*)(int, int, int))CCblank_construct;
}

CCblank* CCblank_construct( int device_number, int NumImageBuffers,
			      int mode_number) {
  CCblank* this=NULL;

  this = malloc(sizeof(CCblank));
  if (this==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error allocating memory");
  } else {
    CCblank_CCblank( this,
		     device_number, NumImageBuffers,
		     mode_number);
    if (cam_iface_error) {
      free(this);
      return NULL;
    }
  }
  return this;
}

void delete_CCblank( CCblank *this ) {
  CCblank_close(this);
  this->inherited.vmt = NULL;
  free(this);
  this = NULL;
}

void CCblank_CCblank( CCblank *this,
			int device_number, int NumImageBuffers,
			int mode_number) {
  // call parent
  CamContext_CamContext((CamContext*)this,device_number,NumImageBuffers,mode_number);
  this->inherited.vmt = (CamContext_functable*)&CCblank_vmt;

  if ((device_number < 0)|(device_number >= num_cameras)) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("requested invalid camera number");
    return;
  }
  this->inherited.coding=CAM_IFACE_MONO8;
  this->inherited.depth = 8;

  return;
}

void CCblank_close(CCblank *this) {
  CHECK_CC(this);
}

void CCblank_start_camera( CCblank *this ) {
  CHECK_CC(this);
}

void CCblank_stop_camera( CCblank *this ) {
  CHECK_CC(this);
}

void CCblank_get_num_camera_properties(CCblank *this,
					int* num_properties) {
  CHECK_CC(this);
  *num_properties = 0;
}

void CCblank_get_camera_property_info(CCblank *this,
				      int property_number,
				      CameraPropertyInfo *info) {
  CHECK_CC(this);

  if (1) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("invalid property_number");
    return;
  }
}

void CCblank_get_camera_property(CCblank *this,
				  int property_number,
				  long* Value,
				  int* Auto ) {
  CHECK_CC(this);

  if (1) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("invalid property_number");
    return;
  }
}

void CCblank_set_camera_property(CCblank *this,
				  int property_number,
				  long Value,
				  int Auto ) {
  CHECK_CC(this);

  if (1) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("invalid property_number");
    return;
  }
}

void CCblank_grab_next_frame_blocking_with_stride( CCblank *this,
						   unsigned char *out_bytes,
						   intptr_t stride0, float timeout) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_grab_next_frame_blocking( CCblank *this, unsigned char *out_bytes, float timeout) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_point_next_frame_blocking( CCblank *this, unsigned char **buf_ptr, float timeout) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_unpoint_frame( CCblank *this){
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_get_last_timestamp( CCblank *this, double* timestamp ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_get_last_framenumber( CCblank *this, unsigned long* framenumber ){
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_get_num_trigger_modes( CCblank *this,
				    int *num_exposure_modes ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_get_trigger_mode_string( CCblank *this,
				      int exposure_mode_number,
				      char* exposure_mode_string, //output parameter
				      int exposure_mode_string_maxlen) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_get_trigger_mode_number( CCblank *this,
				      int *exposure_mode_number ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_set_trigger_mode_number( CCblank *this,
				      int exposure_mode_number ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_get_frame_offset( CCblank *this,
			       int *left, int *top ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_set_frame_offset( CCblank *this,
			       int left, int top ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}


void CCblank_get_frame_size( CCblank *this,
			     int *width, int *height ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_set_frame_size( CCblank *this,
			     int width, int height ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}


void CCblank_get_framerate( CCblank *this,
			    float *framerate ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_set_framerate( CCblank *this,
			    float framerate ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_get_max_frame_size( CCblank *this,
				 int *width, int *height ){
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_get_buffer_size( CCblank *this,
			      int *size) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_get_num_framebuffers( CCblank *this,
				    int *num_framebuffers ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCblank_set_num_framebuffers( CCblank *this,
				    int num_framebuffers ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}
