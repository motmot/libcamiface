/* $Id: $ */
#include "cam_iface.h"

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/videodev2.h>

/* globals -- allocate space */
int cam_iface_error = 0;
#define CAM_IFACE_MAX_ERROR_LEN 255
char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN]  = {0x00}; //...

typedef struct cam_iface_v4l2_backend_extras cam_iface_v4l2_backend_extras;
struct cam_iface_v4l2_backend_extras {
  int started;
  int num_buffers;
  int width;
  int height;
  long last_framenumber;
  double last_timestamp;
};

#ifdef _WIN32
#if _MSC_VER == 1310
#define cam_iface_snprintf(dst, len, fmt, ...) _snprintf((char*)dst, (size_t)len, (const char*)fmt, __VA_ARGS__)
#else
#define cam_iface_snprintf(dst, len, fmt, ...) _snprintf_s((char*)dst, (size_t)len, (size_t)len, (const char*)fmt, __VA_ARGS__)
#endif
#else
#define cam_iface_snprintf(...) snprintf(__VA_ARGS__)
#endif

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

int num_cameras=0;

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

extern void cam_iface_startup() {
#define civ4l2_strlen 100
  int i;
  int fd;
  char device[civ4l2_strlen];

  num_cameras = 0;
  for (i=0;i<100;i++) {
    snprintf(device, civ4l2_strlen, "/dev/video%1d", i);
    fd = open(device, O_RDONLY);
    if (fd != -1) {
      num_cameras++;
      close(fd);
    }
  }
}

extern void cam_iface_shutdown() {
}


int cam_iface_get_num_cameras() {
  return num_cameras;
}

void cam_iface_get_camera_info(int index, Camwire_id *out_camid) {
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

CamContext * new_CamContext( int device_number, int NumImageBuffers,
			     int mode_number) {
  CamContext *ccntxt = NULL;
  cam_iface_v4l2_backend_extras* backend_extras = NULL;

  backend_extras = (cam_iface_v4l2_backend_extras*)malloc(sizeof(cam_iface_v4l2_backend_extras));
  if (!backend_extras) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("malloc failed");
    return NULL;
  }

  backend_extras->num_buffers = NumImageBuffers;
  backend_extras->started = 0;
  backend_extras->width = 640;
  backend_extras->height = 480;
  backend_extras->last_framenumber = -1;
  backend_extras->last_timestamp = -1.0;

  ccntxt = (CamContext*)malloc(sizeof(CamContext));
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("malloc failed");
    return NULL;
  }

  /* initialize */
  ccntxt->cam = (void *)NULL;
  ccntxt->backend_extras = (void *)backend_extras;
  ccntxt->coding = CAM_IFACE_MONO8;
  ccntxt->depth = 8;
  ccntxt->device_number = device_number;

  if (device_number >= num_cameras) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("requested too high camera number");
    return NULL;
  }

  return ccntxt;
}

void delete_CamContext(CamContext *ccntxt) {
  CHECK_CC(ccntxt);
  cam_iface_v4l2_backend_extras* backend_extras = (cam_iface_v4l2_backend_extras*)(ccntxt->backend_extras);
  free(backend_extras);
  free(ccntxt);
}

void CamContext_start_camera( CamContext *ccntxt ) {
  CHECK_CC(ccntxt);
  cam_iface_v4l2_backend_extras* backend_extras = (cam_iface_v4l2_backend_extras*)(ccntxt->backend_extras);
  backend_extras->started = 1;
}

void CamContext_stop_camera( CamContext *ccntxt ) {
  CHECK_CC(ccntxt);
  cam_iface_v4l2_backend_extras* backend_extras = (cam_iface_v4l2_backend_extras*)(ccntxt->backend_extras);
  backend_extras->started = 0;
}

void CamContext_get_num_camera_properties(CamContext *ccntxt,
					  int* num_properties) {
  CHECK_CC(ccntxt);
  fprintf(stderr,"WARNING: CamContext_get_num_camera_properties() not implemented, returning 0\n");
  *num_properties = 0;
}

void CamContext_get_camera_property_info(CamContext *ccntxt,
					 int property_number,
					 CameraPropertyInfo *info) {
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}

void CamContext_get_camera_property(CamContext *ccntxt,
				    int property_number,
				    long* Value,
				    int* Auto ) {
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}

void CamContext_set_camera_property(CamContext *ccntxt,
				    int property_number,
				    long Value,
				    int Auto ) {
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}

void CamContext_grab_next_frame_blocking_with_stride( CamContext *ccntxt, unsigned char *out_bytes, intptr_t stride0, float timeout) {
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}

void CamContext_grab_next_frame_blocking( CamContext *ccntxt, unsigned char *out_bytes, float timeout) {
  CHECK_CC(ccntxt);
  long fno=0;
  cam_iface_v4l2_backend_extras* backend_extras = (cam_iface_v4l2_backend_extras*)(ccntxt->backend_extras);
  bzero(out_bytes,(backend_extras->width) * (backend_extras->height) * (ccntxt->depth)/8 );
  backend_extras->last_timestamp = ci_v4l2_floattime();
  backend_extras->last_framenumber++;
}

void CamContext_point_next_frame_blocking( CamContext *ccntxt, unsigned char **buf_ptr, float timeout){
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}

void CamContext_unpoint_frame( CamContext *ccntxt){
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}

void CamContext_get_last_timestamp( CamContext *ccntxt, double* timestamp ) {
  CHECK_CC(ccntxt);
  cam_iface_v4l2_backend_extras* backend_extras = (cam_iface_v4l2_backend_extras*)(ccntxt->backend_extras);
  *timestamp = backend_extras->last_timestamp;
}

void CamContext_get_last_framenumber( CamContext *ccntxt, long* framenumber ){
  CHECK_CC(ccntxt);
  cam_iface_v4l2_backend_extras* backend_extras = (cam_iface_v4l2_backend_extras*)(ccntxt->backend_extras);
  *framenumber = backend_extras->last_framenumber;
}

void CamContext_get_num_trigger_modes( CamContext *ccntxt,
				       int *num_exposure_modes ) {
  CHECK_CC(ccntxt);
  *num_exposure_modes = 1;
}

void CamContext_get_trigger_mode_string( CamContext *ccntxt,
					 int exposure_mode_number,
					 char* exposure_mode_string, //output parameter
					 int exposure_mode_string_maxlen) {
  if (!ccntxt) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
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

void CamContext_get_trigger_mode_number( CamContext *ccntxt,
					 int *exposure_mode_number ) {
  CHECK_CC(ccntxt);
  *exposure_mode_number = 0;
}

void CamContext_set_trigger_mode_number( CamContext *ccntxt,
					 int exposure_mode_number ) {
  CHECK_CC(ccntxt);
  if (exposure_mode_number != 0) {
    CAM_IFACE_THROW_ERROR("exposure_mode_number invalid");
  }
}

void CamContext_get_frame_offset( CamContext *ccntxt,
				  int *left, int *top ) {
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}

void CamContext_set_frame_offset( CamContext *ccntxt,
				  int left, int top ) {
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}


void CamContext_get_frame_size( CamContext *ccntxt,
				int *width, int *height ) {
  CHECK_CC(ccntxt);
  cam_iface_v4l2_backend_extras* backend_extras = (cam_iface_v4l2_backend_extras*)(ccntxt->backend_extras);
  *width = backend_extras->width;
  *height = backend_extras->height;
}

void CamContext_set_frame_size( CamContext *ccntxt,
				int width, int height ) {
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}

void CamContext_get_buffer_size( CamContext *ccntxt,
					int *size){
  CHECK_CC(ccntxt);
  cam_iface_v4l2_backend_extras* backend_extras = (cam_iface_v4l2_backend_extras*)(ccntxt->backend_extras);
  *size = (backend_extras->width) * (backend_extras->height) * (ccntxt->depth)/8 ;
}

void CamContext_get_framerate( CamContext *ccntxt,
			       float *framerate ) {
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}

void CamContext_set_framerate( CamContext *ccntxt,
			       float framerate ) {
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}

void CamContext_get_max_frame_size( CamContext *ccntxt,
				    int *width, int *height ){
  CHECK_CC(ccntxt);
  cam_iface_v4l2_backend_extras* backend_extras = (cam_iface_v4l2_backend_extras*)(ccntxt->backend_extras);
  *width = backend_extras->width;
  *height = backend_extras->height;
}

void CamContext_get_num_framebuffers( CamContext *ccntxt,
				      int *num_framebuffers ) {
  CHECK_CC(ccntxt);
  cam_iface_v4l2_backend_extras* backend_extras = (cam_iface_v4l2_backend_extras*)(ccntxt->backend_extras);
  *num_framebuffers =  backend_extras->num_buffers;
}

void CamContext_set_num_framebuffers( CamContext *ccntxt,
				      int num_framebuffers ) {
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}
