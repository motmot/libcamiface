/* Backend for libdc1394 v2.0 */
#include "cam_iface.h"

#include <dc1394/control.h>
#include <dc1394/utils.h>

//#define CAM_IFACE_CALL_UNLISTEN
#ifdef CAM_IFACE_CALL_UNLISTEN
#include <dc1394/kernel-video1394.h> // for VIDEO1394_IOC_UNLISTEN_CHANNEL
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/select.h>
#include <errno.h>

#undef CAM_IFACE_DC1394_SLOWDEBUG
#define INVALID_FILENO 0
#define DELAY 50000

struct CCdc1394; // forward declaration

// keep functable in sync across backends
typedef struct {
  cam_iface_constructor_func_t construct;
  void (*destruct)(struct CamContext*);

  void (*CCdc1394)(struct CCdc1394*,int,int,int);
  void (*close)(struct CCdc1394*);
  void (*start_camera)(struct CCdc1394*);
  void (*stop_camera)(struct CCdc1394*);
  void (*get_num_camera_properties)(struct CCdc1394*,int*);
  void (*get_camera_property_info)(struct CCdc1394*,
				   int,
				   CameraPropertyInfo*);
  void (*get_camera_property)(struct CCdc1394*,int,long*,int*);
  void (*set_camera_property)(struct CCdc1394*,int,long,int);
  void (*grab_next_frame_blocking)(struct CCdc1394*,
				   unsigned char*,
				   float);
  void (*grab_next_frame_blocking_with_stride)(struct CCdc1394*,
					       unsigned char*,
					       intptr_t,
					       float);
  void (*point_next_frame_blocking)(struct CCdc1394*,unsigned char**,float);
  void (*unpoint_frame)(struct CCdc1394*);
  void (*get_last_timestamp)(struct CCdc1394*,double*);
  void (*get_last_framenumber)(struct CCdc1394*,long*);
  void (*get_num_trigger_modes)(struct CCdc1394*,int*);
  void (*get_trigger_mode_string)(struct CCdc1394*,int,char*,int);
  void (*get_trigger_mode_number)(struct CCdc1394*,int*);
  void (*set_trigger_mode_number)(struct CCdc1394*,int);
  void (*get_frame_offset)(struct CCdc1394*,int*,int*);
  void (*set_frame_offset)(struct CCdc1394*,int,int);
  void (*get_frame_size)(struct CCdc1394*,int*,int*);
  void (*set_frame_size)(struct CCdc1394*,int,int);
  void (*get_max_frame_size)(struct CCdc1394*,int*,int*);
  void (*get_buffer_size)(struct CCdc1394*,int*);
  void (*get_framerate)(struct CCdc1394*,float*);
  void (*set_framerate)(struct CCdc1394*,float);
  void (*get_num_framebuffers)(struct CCdc1394*,int*);
  void (*set_num_framebuffers)(struct CCdc1394*,int);
} CCdc1394_functable;

typedef struct CCdc1394 {
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
} CCdc1394;

// forward declarations
CCdc1394* CCdc1394_construct( int device_number, int NumImageBuffers,
			      int mode_number);
void delete_CCdc1394(struct CCdc1394*);

void CCdc1394_CCdc1394(struct CCdc1394*,int,int,int);
void CCdc1394_close(struct CCdc1394*);
void CCdc1394_start_camera(struct CCdc1394*);
void CCdc1394_stop_camera(struct CCdc1394*);
void CCdc1394_get_num_camera_properties(struct CCdc1394*,int*);
void CCdc1394_get_camera_property_info(struct CCdc1394*,
			      int,
			      CameraPropertyInfo*);
void CCdc1394_get_camera_property(struct CCdc1394*,int,long*,int*);
void CCdc1394_set_camera_property(struct CCdc1394*,int,long,int);
void CCdc1394_grab_next_frame_blocking(struct CCdc1394*,
			      unsigned char*,
			      float);
void CCdc1394_grab_next_frame_blocking_with_stride(struct CCdc1394*,
					  unsigned char*,
					  intptr_t,
					  float);
void CCdc1394_point_next_frame_blocking(struct CCdc1394*,unsigned char**,float);
void CCdc1394_unpoint_frame(struct CCdc1394*);
void CCdc1394_get_last_timestamp(struct CCdc1394*,double*);
void CCdc1394_get_last_framenumber(struct CCdc1394*,long*);
void CCdc1394_get_num_trigger_modes(struct CCdc1394*,int*);
void CCdc1394_get_trigger_mode_string(struct CCdc1394*,int,char*,int);
void CCdc1394_get_trigger_mode_number(struct CCdc1394*,int*);
void CCdc1394_set_trigger_mode_number(struct CCdc1394*,int);
void CCdc1394_get_frame_offset(struct CCdc1394*,int*,int*);
void CCdc1394_set_frame_offset(struct CCdc1394*,int,int);
void CCdc1394_get_frame_size(struct CCdc1394*,int*,int*);
void CCdc1394_set_frame_size(struct CCdc1394*,int,int);
void CCdc1394_get_max_frame_size(struct CCdc1394*,int*,int*);
void CCdc1394_get_buffer_size(struct CCdc1394*,int*);
void CCdc1394_get_framerate(struct CCdc1394*,float*);
void CCdc1394_set_framerate(struct CCdc1394*,float);
void CCdc1394_get_num_framebuffers(struct CCdc1394*,int*);
void CCdc1394_set_num_framebuffers(struct CCdc1394*,int);

CCdc1394_functable CCdc1394_vmt = {
  (cam_iface_constructor_func_t)CCdc1394_construct,
  (void (*)(CamContext*))delete_CCdc1394,
  CCdc1394_CCdc1394,
  CCdc1394_close,
  CCdc1394_start_camera,
  CCdc1394_stop_camera,
  CCdc1394_get_num_camera_properties,
  CCdc1394_get_camera_property_info,
  CCdc1394_get_camera_property,
  CCdc1394_set_camera_property,
  CCdc1394_grab_next_frame_blocking,
  CCdc1394_grab_next_frame_blocking_with_stride,
  CCdc1394_point_next_frame_blocking,
  CCdc1394_unpoint_frame,
  CCdc1394_get_last_timestamp,
  CCdc1394_get_last_framenumber,
  CCdc1394_get_num_trigger_modes,
  CCdc1394_get_trigger_mode_string,
  CCdc1394_get_trigger_mode_number,
  CCdc1394_set_trigger_mode_number,
  CCdc1394_get_frame_offset,
  CCdc1394_set_frame_offset,
  CCdc1394_get_frame_size,
  CCdc1394_set_frame_size,
  CCdc1394_get_max_frame_size,
  CCdc1394_get_buffer_size,
  CCdc1394_get_framerate,
  CCdc1394_set_framerate,
  CCdc1394_get_num_framebuffers,
  CCdc1394_set_num_framebuffers
};

/* typedefs */
struct cam_iface_dc1394_mode {
  dc1394video_mode_t video_mode;
  dc1394framerate_t framerate;
  dc1394color_coding_t color_coding;
};
typedef struct cam_iface_dc1394_mode cam_iface_dc1394_mode_t;

struct cam_iface_dc1394_modes {
  int num_modes;
  cam_iface_dc1394_mode_t *modes;
};
typedef struct cam_iface_dc1394_modes cam_iface_dc1394_modes_t;

struct cam_iface_dc1394_feature_list {
  int num_features;
  int *dc1394_feature_ids;
};
typedef struct cam_iface_dc1394_feature_list cam_iface_dc1394_feature_list_t;

/* globals -- allocate space */
dc1394_t * libdc1394_instance=NULL;
int cam_iface_error = 0;
#define CAM_IFACE_MAX_ERROR_LEN 255
char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN]  = {0x00}; //...

uint32_t num_cameras = 0;
dc1394camera_t **cameras = NULL;
cam_iface_dc1394_modes_t *modes_by_device_number=NULL;
cam_iface_dc1394_feature_list_t *features_by_device_number=NULL;

#define CAM_IFACE_ERROR_FORMAT(m)					\
  snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,		\
	   "%s (%d): %s\n",__FILE__,__LINE__,(m));

#define CAM_IFACE_CHECK_DEVICE_NUMBER(m)				\
  if ( ((m)<0) | ((m)>=num_cameras) ) {					\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("invalid device_number");			\
    return;								\
  }

#define CIDC1394CHK(err) {						\
    dc1394error_t m = (err);						\
    if (m!=DC1394_SUCCESS) {						\
      cam_iface_error = -1;						\
      snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,		\
	       "%s (%d): libdc1394 err %d: %s\n",__FILE__,__LINE__,	\
	       m,							\
	       dc1394_error_get_string(m));				\
      return;								\
    }									\
  }

#define CIDC1394CHKV(err) {						\
    dc1394error_t m = (err);						\
    if (m!=DC1394_SUCCESS) {						\
      cam_iface_error = -1;						\
      snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,		\
	       "%s (%d): libdc1394 err %d: %s\n",__FILE__,__LINE__,	\
	       m,							\
	       dc1394_error_get_string(m));				\
      return NULL;							\
    }									\
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

int cam_iface_is_video_mode_scalable(dc1394video_mode_t video_mode)
{
  return ((video_mode>=DC1394_VIDEO_MODE_FORMAT7_MIN)&&(video_mode<=DC1394_VIDEO_MODE_FORMAT7_MAX));
}

/* internal structures for dc1394 implementation */

const char *cam_iface_get_driver_name() {
  return "dc1394";
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

dc1394bool_t _available_feature_filter( int feature_id, dc1394bool_t was_available ) {
  if (!was_available) {
    return was_available;
  }
  switch (feature_id) {
  case DC1394_FEATURE_TRIGGER: return 0; break;
  default: return 1;
  }
}

const char* cam_iface_get_api_version() {
  return CAM_IFACE_API_VERSION;
}

#define MODE_CASE(m) case m:\
    result = #m;	      \
  break;

void fprint_dc1394format7mode_t(FILE* fd, dc1394format7mode_t* mode) {
  if (!(mode->present)) {
    fprintf(fd,"present: FALSE\n");
    return;
  }

  fprintf(fd,"present: TRUE\n");
  fprintf(fd,"size_x: %d\n",mode->size_x);
  fprintf(fd,"size_y: %d\n",mode->size_y);
  fprintf(fd,"max_size_x: %d\n",mode->max_size_x);
  fprintf(fd,"max_size_y: %d\n",mode->max_size_y);
  fprintf(fd,"(more info display not implemented...)\n");
};

const char* get_dc1394_mode_string(dc1394video_mode_t mode) {
  const char* result;
  switch (mode) {
  MODE_CASE(DC1394_VIDEO_MODE_160x120_YUV444)
  MODE_CASE(DC1394_VIDEO_MODE_320x240_YUV422)
  MODE_CASE(DC1394_VIDEO_MODE_640x480_YUV411)
  MODE_CASE(DC1394_VIDEO_MODE_640x480_YUV422)
  MODE_CASE(DC1394_VIDEO_MODE_640x480_RGB8)
  MODE_CASE(DC1394_VIDEO_MODE_640x480_MONO8)
  MODE_CASE(DC1394_VIDEO_MODE_640x480_MONO16)
  MODE_CASE(DC1394_VIDEO_MODE_800x600_YUV422)
  MODE_CASE(DC1394_VIDEO_MODE_800x600_RGB8)
  MODE_CASE(DC1394_VIDEO_MODE_800x600_MONO8)
  MODE_CASE(DC1394_VIDEO_MODE_1024x768_YUV422)
  MODE_CASE(DC1394_VIDEO_MODE_1024x768_RGB8)
  MODE_CASE(DC1394_VIDEO_MODE_1024x768_MONO8)
  MODE_CASE(DC1394_VIDEO_MODE_800x600_MONO16)
  MODE_CASE(DC1394_VIDEO_MODE_1024x768_MONO16)
  MODE_CASE(DC1394_VIDEO_MODE_1280x960_YUV422)
  MODE_CASE(DC1394_VIDEO_MODE_1280x960_RGB8)
  MODE_CASE(DC1394_VIDEO_MODE_1280x960_MONO8)
  MODE_CASE(DC1394_VIDEO_MODE_1600x1200_YUV422)
  MODE_CASE(DC1394_VIDEO_MODE_1600x1200_RGB8)
  MODE_CASE(DC1394_VIDEO_MODE_1600x1200_MONO8)
  MODE_CASE(DC1394_VIDEO_MODE_1280x960_MONO16)
  MODE_CASE(DC1394_VIDEO_MODE_1600x1200_MONO16)
  MODE_CASE(DC1394_VIDEO_MODE_EXIF)
  MODE_CASE(DC1394_VIDEO_MODE_FORMAT7_0)
  MODE_CASE(DC1394_VIDEO_MODE_FORMAT7_1)
  MODE_CASE(DC1394_VIDEO_MODE_FORMAT7_2)
  MODE_CASE(DC1394_VIDEO_MODE_FORMAT7_3)
  MODE_CASE(DC1394_VIDEO_MODE_FORMAT7_4)
  MODE_CASE(DC1394_VIDEO_MODE_FORMAT7_5)
  MODE_CASE(DC1394_VIDEO_MODE_FORMAT7_6)
  MODE_CASE(DC1394_VIDEO_MODE_FORMAT7_7)
  default:
    result = "UNKNOWN";
  }
  return result;

}

void cam_iface_startup() {
  int device_number,i,j,current_mode,feature_number;
  dc1394video_modes_t video_modes;
  dc1394framerates_t framerates;
  dc1394featureset_t features;
  dc1394feature_info_t    *feature_info;
  dc1394color_codings_t color_codings;
  dc1394camera_list_t * list;

  libdc1394_instance = dc1394_new ();

  // Disable printouts of error messages - we are pretty good about
  // checking return codes, so this should be fine.
  CIDC1394CHK(dc1394_log_register_handler(DC1394_LOG_ERROR,NULL,NULL));

  CIDC1394CHK(dc1394_camera_enumerate (libdc1394_instance, &list));

  num_cameras = list->num;

  cameras = malloc( num_cameras*sizeof(dc1394camera_t));
  if (cameras == NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error allocating memory");
    return;
  }

  modes_by_device_number = malloc( num_cameras*sizeof(cam_iface_dc1394_modes_t));
  if (modes_by_device_number == NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error allocating memory");
    return;
  }

  features_by_device_number = malloc( num_cameras*sizeof(cam_iface_dc1394_feature_list_t));
  if (features_by_device_number == NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error allocating memory");
    return;
  }

  // initialize cameras
  for (device_number=0;device_number<num_cameras;device_number++) {
    cameras[device_number] = dc1394_camera_new( libdc1394_instance, list->ids[device_number].guid );
    if (!cameras[device_number]) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("Failed to initialize camera.");
      return;
    }
  }

  // initialize new structures
  for (device_number=0;device_number<num_cameras;device_number++) {
    modes_by_device_number[device_number].num_modes = 0;
    modes_by_device_number[device_number].modes = NULL;
    features_by_device_number[device_number].num_features = 0;
    features_by_device_number[device_number].dc1394_feature_ids = NULL;
  }

  // fill new structures
  for (device_number=0;device_number<num_cameras;device_number++) {
    // features: pass 1 count num available
    CIDC1394CHK(dc1394_feature_get_all(cameras[device_number], &features));
    for (i=0; i<DC1394_FEATURE_NUM; i++) {
      feature_info = &(features.feature[i]);

      if (_available_feature_filter( feature_info->id, feature_info->available)) {
	features_by_device_number[device_number].num_features++;
      }

#ifdef DISABLE_TRIGGER_CODE
      if (feature_info->id == DC1394_FEATURE_TRIGGER) {
	continue;
      }
#endif

      if (feature_info->id == DC1394_FEATURE_TRIGGER) {
	/*
	printf("feature_info->trigger_modes.num = %d\n",feature_info->trigger_modes.num);
	for (j=0; j<(feature_info->trigger_modes.num); j++) {
	  printf("AVAIL feature_info->trigger_mode = %d\n",feature_info->trigger_modes.modes[j]);
	}
	printf("feature_info->trigger_mode = %d\n",feature_info->trigger_mode);
	printf("feature_info->trigger_polarity = %d\n",feature_info->trigger_polarity);
	printf("feature_info->trigger_sources.num = %d\n",feature_info->trigger_sources.num);
	for (j=0; j<(feature_info->trigger_sources.num); j++) {
	  printf("AVAIL feature_info->trigger_source = %d\n",feature_info->trigger_sources.sources[j]);
	}
	printf("feature_info->trigger_source = %d\n",feature_info->trigger_source);
	*/

	if (feature_info->trigger_sources.num) {
	  CIDC1394CHK(dc1394_external_trigger_set_source(cameras[device_number], DC1394_TRIGGER_SOURCE_0));
	  //printf("set source\n");
	}
      }

    }

    // modes:
    CIDC1394CHK(dc1394_video_get_supported_modes(cameras[device_number],
						 &video_modes));

    // enumerate total number of modes ("mode" = dc1394 mode + dc1394 framerate)
    for (i=video_modes.num-1;i>=0;i--) {
      // framerates don't work for format 7
      //fprintf(stderr,"mode: %s ",get_dc1394_mode_string(video_modes.modes[i]));
      if (cam_iface_is_video_mode_scalable(video_modes.modes[i])) {
	// format7
	//dc1394_format7_get_mode_info(cameras[device_number], video_modes.modes[i],&sf7mode);
	//fprint_dc1394format7mode_t(stderr,&sf7mode);
	CIDC1394CHK(dc1394_format7_get_color_codings(cameras[device_number],
						     video_modes.modes[i],
						     &color_codings));
	for (j=0;j<color_codings.num;j++) {
	  modes_by_device_number[device_number].num_modes++; // a single mode entry
	}
      } else {
	CIDC1394CHK(dc1394_video_get_supported_framerates(cameras[device_number],
							  video_modes.modes[i],
							  &framerates));
	for (j=framerates.num-1;j>=0;j--) {
	  modes_by_device_number[device_number].num_modes++;
	  //fprintf(stderr,"non-format7: num_modes++\n");
	}
      }
    }

    // features: pass 2 - allocate memory
    features_by_device_number[device_number].dc1394_feature_ids = malloc(features_by_device_number[device_number].num_features *sizeof(int));
    if (features_by_device_number[device_number].dc1394_feature_ids==NULL) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("error allocating memory");
      return;
    }
    feature_number = 0;
    for (i=0; i<DC1394_FEATURE_NUM; i++) {
      feature_info = &(features.feature[i]);
      if (_available_feature_filter( feature_info->id, feature_info->available)) {
	features_by_device_number[device_number].dc1394_feature_ids[feature_number] = feature_info->id;
	feature_number++;
      }
    }

    // modes:

    // allocate memory for each device to store possible modes
    modes_by_device_number[device_number].modes = malloc(modes_by_device_number[device_number].num_modes * sizeof(cam_iface_dc1394_mode_t));
    if (modes_by_device_number[device_number].modes==NULL) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("error allocating memory");
      return;
    }

    // now fill mode list
    current_mode = 0;
    // enumerate total number of modes ("mode" = dc1394 mode + dc1394 framerate)
    for (i=video_modes.num-1;i>=0;i--) {
      if (cam_iface_is_video_mode_scalable(video_modes.modes[i])) {
	// format7
	CIDC1394CHK(dc1394_format7_get_color_codings(cameras[device_number],
						     video_modes.modes[i],
						     &color_codings));
	for (j=0;j<color_codings.num;j++) {
	  modes_by_device_number[device_number].modes[current_mode].video_mode = video_modes.modes[i];
	  modes_by_device_number[device_number].modes[current_mode].framerate = -1; // format7 - no framerate
	  modes_by_device_number[device_number].modes[current_mode].color_coding = color_codings.codings[j];
	  current_mode++;
	}
      } else {
	CIDC1394CHK(dc1394_video_get_supported_framerates(cameras[device_number],
							  video_modes.modes[i],
							  &framerates));
	for (j=framerates.num-1;j>=0;j--) {
	  modes_by_device_number[device_number].modes[current_mode].video_mode = video_modes.modes[i];
	  modes_by_device_number[device_number].modes[current_mode].framerate = framerates.framerates[j];
	  CIDC1394CHK(dc1394_get_color_coding_from_video_mode(cameras[device_number],
							      video_modes.modes[i],
							      &(modes_by_device_number[device_number].modes[current_mode].color_coding)));
	  current_mode++;
	}
      }
    }

  }
  dc1394_camera_free_list (list);
}

void cam_iface_shutdown() {
  int device_number;

  for (device_number=0;device_number<num_cameras;device_number++) {
    if (modes_by_device_number!=NULL) {
      if (modes_by_device_number[device_number].modes!=NULL) {
	free(modes_by_device_number[device_number].modes);
      }
    }
    if (features_by_device_number!=NULL) {
      if (features_by_device_number[device_number].dc1394_feature_ids!=NULL) {
	free(features_by_device_number[device_number].dc1394_feature_ids);
      }
    }
  }

  if (modes_by_device_number!=NULL) {
    free(modes_by_device_number);
  }

  if (features_by_device_number!=NULL) {
    free(features_by_device_number);
  }

  for (device_number=0;device_number<num_cameras;device_number++) {
    dc1394_camera_free(cameras[device_number]);
  }

  free(cameras);

  num_cameras=0;
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
  snprintf(out_camid->vendor, CAMWIRE_ID_MAX_CHARS, "%s", cameras[device_number]->vendor);
  snprintf(out_camid->model, CAMWIRE_ID_MAX_CHARS, "%s", cameras[device_number]->model);
  snprintf(out_camid->chip, CAMWIRE_ID_MAX_CHARS, "%llXh", (long long unsigned int)cameras[device_number]->guid);
}

void cam_iface_get_num_modes(int device_number, int *num_modes) {
  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  *num_modes = modes_by_device_number[device_number].num_modes;
}

int _get_size_for_video_mode(int device_number,
			     dc1394video_mode_t video_mode,
			     uint32_t *h_size,uint32_t *v_size,
			     int *scalable){
  switch (video_mode) {
  case DC1394_VIDEO_MODE_160x120_YUV444: *h_size=160; *v_size=120; break;
  case DC1394_VIDEO_MODE_320x240_YUV422: *h_size=320; *v_size=240; break;
  case DC1394_VIDEO_MODE_640x480_YUV411: *h_size=640; *v_size=480; break;
  case DC1394_VIDEO_MODE_640x480_YUV422: *h_size=640; *v_size=480; break;
  case DC1394_VIDEO_MODE_640x480_RGB8: *h_size=640; *v_size=480; break;
  case DC1394_VIDEO_MODE_640x480_MONO8: *h_size=640; *v_size=480; break;
  case DC1394_VIDEO_MODE_640x480_MONO16: *h_size=640; *v_size=480; break;
  case DC1394_VIDEO_MODE_800x600_YUV422: *h_size=800; *v_size=600; break;
  case DC1394_VIDEO_MODE_800x600_RGB8: *h_size=800; *v_size=600; break;
  case DC1394_VIDEO_MODE_800x600_MONO8: *h_size=800; *v_size=600; break;
  case DC1394_VIDEO_MODE_1024x768_YUV422: *h_size=1024; *v_size=600; break;
  case DC1394_VIDEO_MODE_1024x768_RGB8: *h_size=1024; *v_size=768; break;
  case DC1394_VIDEO_MODE_1024x768_MONO8: *h_size=1024; *v_size=768; break;
  case DC1394_VIDEO_MODE_800x600_MONO16: *h_size=800; *v_size=600; break;
  case DC1394_VIDEO_MODE_1024x768_MONO16: *h_size=1024; *v_size=768; break;
  case DC1394_VIDEO_MODE_1280x960_YUV422: *h_size=1280; *v_size=960; break;
  case DC1394_VIDEO_MODE_1280x960_RGB8: *h_size=1280; *v_size=960; break;
  case DC1394_VIDEO_MODE_1280x960_MONO8: *h_size=1280; *v_size=960; break;
  case DC1394_VIDEO_MODE_1600x1200_YUV422: *h_size=1600; *v_size=1200; break;
  case DC1394_VIDEO_MODE_1600x1200_RGB8: *h_size=1600; *v_size=1200; break;
  case DC1394_VIDEO_MODE_1600x1200_MONO8: *h_size=1600; *v_size=1200; break;
  case DC1394_VIDEO_MODE_1280x960_MONO16: *h_size=1280; *v_size=960; break;
  case DC1394_VIDEO_MODE_1600x1200_MONO16: *h_size=1600; *v_size=1200; break;
  default: *h_size=0; *v_size=0; break;
  }

  if (dc1394_is_video_mode_scalable(video_mode)) {
    if (DC1394_SUCCESS!=dc1394_format7_get_max_image_size(cameras[device_number],
							  video_mode,
							  h_size,v_size)) {
      return 0;
    }
    *scalable = 1;
  } else {
    *scalable = 0;
  }
  return 1;
}

void cam_iface_get_mode_string(int device_number,
			       int mode_number,
			       char* mode_string,
			       int mode_string_maxlen) {
  dc1394video_mode_t video_mode;
  dc1394framerate_t framerate;
  char *coding_string, *framerate_string;
  const char *dc1394_mode_string;
  int scalable;
  uint32_t h_size,v_size;

  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  if ((mode_number<0)|
      (mode_number>=modes_by_device_number[device_number].num_modes)) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("invalid mode_number");
    return;
  }
  video_mode = modes_by_device_number[device_number].modes[mode_number].video_mode;
  framerate = modes_by_device_number[device_number].modes[mode_number].framerate;

  if (!_get_size_for_video_mode(device_number,video_mode,&h_size,&v_size,&scalable)){
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("Could not get size for video mode");
    return;
  }

  switch (modes_by_device_number[device_number].modes[mode_number].color_coding) {
  case DC1394_COLOR_CODING_MONO8:  coding_string = "MONO8";  break;
  case DC1394_COLOR_CODING_YUV411: coding_string = "YUV411"; break;
  case DC1394_COLOR_CODING_YUV422: coding_string = "YUV422"; break;
  case DC1394_COLOR_CODING_YUV444: coding_string = "YUV444"; break;
  case DC1394_COLOR_CODING_RGB8:   coding_string = "RGB8";   break;
  case DC1394_COLOR_CODING_MONO16: coding_string = "MONO16"; break;
  case DC1394_COLOR_CODING_RGB16:  coding_string = "RGB16";  break;
  case DC1394_COLOR_CODING_MONO16S:coding_string = "MONO16S";break;
  case DC1394_COLOR_CODING_RGB16S: coding_string = "RGB16S"; break;
  case DC1394_COLOR_CODING_RAW8:   coding_string = "RAW8";   break;
  case DC1394_COLOR_CODING_RAW16:  coding_string = "RAW16";  break;
  default: coding_string = "unknown color coding"; break;
  }

  if (cam_iface_is_video_mode_scalable(video_mode)) {
    framerate_string="(user selectable framerate)";
  } else {
    switch (framerate){
    case DC1394_FRAMERATE_1_875: framerate_string="1.875 fps"; break;
    case DC1394_FRAMERATE_3_75:  framerate_string="3.75 fps"; break;
    case DC1394_FRAMERATE_7_5:   framerate_string="7.5 fps"; break;
    case DC1394_FRAMERATE_15:    framerate_string="15 fps"; break;
    case DC1394_FRAMERATE_30:    framerate_string="30 fps"; break;
    case DC1394_FRAMERATE_60:    framerate_string="60 fps"; break;
    case DC1394_FRAMERATE_120:   framerate_string="120 fps"; break;
    case DC1394_FRAMERATE_240:   framerate_string="240 fps"; break;
    default: framerate_string="unknown framerate"; break;
    }
  }

  dc1394_mode_string = get_dc1394_mode_string(video_mode);
  if (dc1394_is_video_mode_scalable(video_mode)) {
    snprintf(mode_string,mode_string_maxlen,"%d x %d %s %s %s",
	     h_size,v_size,dc1394_mode_string,coding_string,framerate_string);
  } else {
    snprintf(mode_string,mode_string_maxlen,"%d x %d %s %s",
	     h_size,v_size,dc1394_mode_string,framerate_string);
  }
}

cam_iface_constructor_func_t cam_iface_get_constructor_func(int device_number) {
  return (CamContext* (*)(int, int, int))CCdc1394_construct;
}

CCdc1394* CCdc1394_construct( int device_number, int NumImageBuffers,
			      int mode_number) {
  CCdc1394* this=NULL;

  this = malloc(sizeof(CCdc1394));
  if (this==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error allocating memory");
  } else {
    CCdc1394_CCdc1394( this,
		       device_number, NumImageBuffers,
		       mode_number);
    if (cam_iface_error) {
      free(this);
      return NULL;
    }
  }
  return this;
}

void delete_CCdc1394( CCdc1394 *this ) {
  CCdc1394_close(this);
  this->inherited.vmt = NULL;
  free(this);
  this = NULL;
}

void CCdc1394_CCdc1394( CCdc1394 *this,
			int device_number, int NumImageBuffers,
			int mode_number) {
  dc1394video_mode_t video_mode, test_video_mode;
  dc1394framerate_t framerate;
  uint32_t h_size,v_size,tmp_depth;
  dc1394color_coding_t coding;
  int scalable;

  // call parent
  CamContext_CamContext((CamContext*)this,device_number,NumImageBuffers,mode_number);
  this->inherited.vmt = (CamContext_functable*)&CCdc1394_vmt;

#ifdef CAM_IFACE_DC1394_SLOWDEBUG
  char mode_string[255];

  fprintf(stderr,"attempting to start camera %d with mode_number %d\n",device_number,mode_number);
  cam_iface_get_mode_string(device_number,mode_number,mode_string,255);
  if (cam_iface_error) {
    return;
  }
  fprintf(stderr,"mode string: %s\n",mode_string);
#endif

  if ((device_number < 0)|(device_number >= num_cameras)) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("requested invalid camera number");
    return;
  }
  if ((mode_number<0)|
      (mode_number>=modes_by_device_number[device_number].num_modes)) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("invalid mode_number");
    return;
  }
  video_mode = modes_by_device_number[device_number].modes[mode_number].video_mode;
  framerate = modes_by_device_number[device_number].modes[mode_number].framerate;
  coding = modes_by_device_number[device_number].modes[mode_number].color_coding;

  /* initialize */
  this->inherited.cam = (void *)NULL;
  this->inherited.backend_extras = (void *)NULL;
  if (!this) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("malloc failed");
    return;
  }
  this->cam_iface_mode_number = mode_number; // different than DC1934 mode number
  this->nframe_hack=0;
  this->fileno = INVALID_FILENO;
  this->nfds = 0;
  FD_ZERO(&(this->fdset));

  CIDC1394CHK(dc1394_video_set_transmission(cameras[device_number],
					    DC1394_OFF));
  this->capture_is_set=0;

  CIDC1394CHK(dc1394_video_set_iso_speed(cameras[device_number], DC1394_ISO_SPEED_400));

  this->inherited.device_number = device_number;

  if (!_get_size_for_video_mode(device_number,video_mode,&h_size,&v_size,&scalable)){
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("Could not get size for video mode");
    return;
  }

  this->max_width = h_size;
  this->max_height = v_size;

  CIDC1394CHK(dc1394_video_set_mode(cameras[device_number],video_mode));
  CIDC1394CHK(dc1394_video_get_mode(cameras[device_number],&test_video_mode));

  if (test_video_mode != video_mode) {
    fprintf(stderr,"ERROR while setting video modes\n");
    fprintf(stderr,"  video_mode (desired): %s\n",get_dc1394_mode_string(video_mode));
    fprintf(stderr,"  video_mode (actual): %s\n", get_dc1394_mode_string(test_video_mode));
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("Video mode set failed. (Camera may report modes it can't use.)");
    return;
  }

  if (!dc1394_is_video_mode_scalable(video_mode)) {
    CIDC1394CHK(dc1394_video_set_framerate(cameras[device_number], framerate));
  } else {
    // format7
    CIDC1394CHK(dc1394_format7_set_color_coding(cameras[device_number],
						 video_mode,
						 coding));
  }

  switch (coding) {
  case DC1394_COLOR_CODING_MONO8:
    this->inherited.coding=CAM_IFACE_MONO8;
    this->inherited.depth = 8;
    break;
  case DC1394_COLOR_CODING_RAW8:
    this->inherited.coding=CAM_IFACE_RAW8;
    this->inherited.depth = 8;
    break;
  case DC1394_COLOR_CODING_YUV411:
    this->inherited.coding = CAM_IFACE_YUV411;
    this->inherited.depth = 12;
    break;
  case DC1394_COLOR_CODING_YUV422:
    this->inherited.coding = CAM_IFACE_YUV422;
    this->inherited.depth = 16;
    break;
  case DC1394_COLOR_CODING_MONO16:
    this->inherited.coding = CAM_IFACE_MONO16;
    this->inherited.depth = 16;
    break;
  case DC1394_COLOR_CODING_YUV444:
  case DC1394_COLOR_CODING_RGB8:
  case DC1394_COLOR_CODING_RGB16:
  case DC1394_COLOR_CODING_MONO16S:
  case DC1394_COLOR_CODING_RGB16S:
  case DC1394_COLOR_CODING_RAW16:
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("Currently unsupported color coding");
    return;
    break;
  default:
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("Unknown color coding");
    return;
    break;
  }

  // This doesn't give 12 for YUV411, so we force our values above.
  //CIDC1394CHK(dc1394_video_get_data_depth(cameras[device_number], &tmp_depth));
  //this->inherited.depth = tmp_depth;

  this->roi_width = this->max_width;
  this->roi_height = this->max_height;

  // if format 7
  if (scalable) {
    if (!cam_iface_is_video_mode_scalable(video_mode)) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("Format 7 did not make scalable video mode");
      return;
    }
    CIDC1394CHK(dc1394_format7_set_roi(cameras[device_number], video_mode,
					coding,
					DC1394_USE_MAX_AVAIL, // use max packet size
					0, 0, // left, top
					this->roi_width,
					this->roi_height));  // width, height
  }

  this->num_dma_buffers=NumImageBuffers;
  this->buffer_size=(this->roi_width)*(this->roi_height)*this->inherited.depth/8;

#ifdef CAM_IFACE_DEBUG
  fprintf(stdout,"new cam context 1\n");
  fflush(stdout);
  usleep(5000000);
#endif

  return;
}

void CCdc1394_close(CCdc1394 *this) {
  CHECK_CC(this);
  dc1394camera_t *camera;
  camera = cameras[this->inherited.device_number];

  /* make sure the camera stops sending data */
  CIDC1394CHK(dc1394_video_set_transmission(camera,
					    DC1394_OFF));

  if (this->capture_is_set>0) {
    CIDC1394CHK(dc1394_capture_stop(camera));
  }
}

void CCdc1394_start_camera( CCdc1394 *this ) {
  int DROP_FRAMES;
  dc1394camera_t *camera;
  dc1394error_t err;

  DROP_FRAMES=0;
  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];

#ifdef CAM_IFACE_DEBUG
  fprintf(stdout,"start_camera 1\n");
  fflush(stdout);
  usleep(5000000);
#endif

  dc1394switch_t pwr;
  CIDC1394CHK(dc1394_video_get_transmission(camera, &pwr));
  if (pwr==DC1394_ON) {
    // Stop the camera if it's already started. (XXX Should check if
    // capture parameters are OK, and only restart if they're not.)
    CCdc1394_stop_camera(this);
  }
  err = dc1394_capture_setup(camera,
			     this->num_dma_buffers,
			     DC1394_CAPTURE_FLAGS_DEFAULT);

  if (err==DC1394_IOCTL_FAILURE) {
    cam_iface_error = -1;
    snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,		\
	     "%s (%d): libdc1394 err %d: %s. (Did you request too many DMA buffers?)\n",__FILE__,__LINE__, \
	     err,							\
	     dc1394_error_get_string(err));
    return;
  }

  CIDC1394CHK(err);

  this->capture_is_set=1;

  /*have the camera start sending data*/
  CIDC1394CHK(dc1394_video_set_transmission(camera,
					    DC1394_ON));
#ifdef CAM_IFACE_DEBUG
  fprintf(stdout,"start_camera 3\n");
  fflush(stdout);
  usleep(5000000);
#endif

  this->fileno = dc1394_capture_get_fileno(camera);
  this->nfds = (this->fileno+1);

}

void CCdc1394_stop_camera( CCdc1394 *this ) {
  dc1394camera_t *camera;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];

  if ((this->fileno) != INVALID_FILENO) {
    FD_CLR(this->fileno, &(this->fdset));

    this->nfds = 0;
  }

  /* have the camera stop sending data */
  CIDC1394CHK(dc1394_video_set_transmission(camera,
					    DC1394_OFF));

  if (this->capture_is_set>0) {
    CIDC1394CHK(dc1394_capture_stop(camera));
  }
  this->capture_is_set=0;

#ifdef CAM_IFACE_DEBUG
  fprintf(stdout,"stop_camera 1\n");
  fflush(stdout);
  usleep(5000000);
#endif
  /* release the dma buffers */
  /*
  CIDC1394CHK(dc1394_capture_stop(cameras[this->inherited.device_number]));
  */
}

void CCdc1394_get_num_camera_properties(CCdc1394 *this,
					int* num_properties) {
  dc1394featureset_t features;
  dc1394feature_info_t    *feature_info;
  int i;

  CHECK_CC(this);
  *num_properties = features_by_device_number[this->inherited.device_number].num_features;
}

void feature_has_auto_mode(dc1394feature_modes_t* modes, dc1394bool_t* result){
  *result = 0;
  int i;
  for (i=0; i<(modes->num); i++) {
    if ((modes->modes[i]) == DC1394_FEATURE_MODE_AUTO) {
      *result = 1;
      break;
    }
  }
}

void feature_has_manual_mode(dc1394feature_modes_t* modes, dc1394bool_t* result) {
  *result = 0;
  int i;
  for (i=0; i<(modes->num); i++) {
    if ((modes->modes[i]) == DC1394_FEATURE_MODE_MANUAL) {
      *result = 1;
      break;
    }
  }
}

void CCdc1394_get_camera_property_info(CCdc1394 *this,
				       int property_number,
				       CameraPropertyInfo *info) {
  dc1394camera_t *camera;
  int feature_id;
  dc1394feature_info_t feature_info;
  dc1394bool_t mybool;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];

  if (property_number < 0) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("invalid property_number");
    return;
  }

  if (property_number >= features_by_device_number[this->inherited.device_number].num_features) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("invalid property_number");
    return;
  }

  feature_id = features_by_device_number[this->inherited.device_number].dc1394_feature_ids[property_number];

  switch (feature_id) {
  case DC1394_FEATURE_BRIGHTNESS: info->name = "brightness"; break;
  case DC1394_FEATURE_EXPOSURE: info->name = "exposure"; break;
  case DC1394_FEATURE_SHARPNESS: info->name = "sharpness"; break;
  case DC1394_FEATURE_WHITE_BALANCE: info->name = "white balance"; break;
  case DC1394_FEATURE_HUE: info->name = "hue"; break;
  case DC1394_FEATURE_SATURATION: info->name = "saturation"; break;
  case DC1394_FEATURE_GAMMA: info->name = "gamma"; break;
  case DC1394_FEATURE_SHUTTER: info->name = "shutter"; break;
  case DC1394_FEATURE_GAIN: info->name = "gain"; break;
  case DC1394_FEATURE_IRIS: info->name = "iris"; break;
  case DC1394_FEATURE_FOCUS: info->name = "focus"; break;
  case DC1394_FEATURE_TEMPERATURE: info->name = "temperature"; break;
  case DC1394_FEATURE_TRIGGER: info->name = "trigger"; break;
  case DC1394_FEATURE_TRIGGER_DELAY: info->name = "trigger delay"; break;
  case DC1394_FEATURE_WHITE_SHADING: info->name = "white shading"; break;
  case DC1394_FEATURE_FRAME_RATE: info->name = "frame rate"; break;
  case DC1394_FEATURE_ZOOM: info->name = "zoom"; break;
  case DC1394_FEATURE_PAN: info->name = "pan"; break;
  case DC1394_FEATURE_TILT: info->name = "tilt"; break;
  case DC1394_FEATURE_OPTICAL_FILTER: info->name = "optical filter"; break;
  /* 12 reserved features */
  case DC1394_FEATURE_CAPTURE_SIZE: info->name = "capture size"; break;
  case DC1394_FEATURE_CAPTURE_QUALITY: info->name = "capture quality"; break;
  default:
    fprintf(stderr,"unknown feature id = %d\n",feature_id);
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("unknown feature_id");
    return;
  }

  info->is_present = 1;

  feature_info.id = feature_id;
  CIDC1394CHK(dc1394_feature_get(camera, &feature_info));
  info->min_value = feature_info.min;
  info->max_value = feature_info.max;

  feature_has_auto_mode(&(feature_info.modes), &mybool);
  info->has_auto_mode = mybool;
  feature_has_manual_mode(&(feature_info.modes), &mybool);
  info->has_manual_mode = mybool;

  info->is_scaled_quantity = 0;

  // Hacks for each known camera type should go here, or a more
  // general way.

  if ((strcmp(camera->vendor,"Basler")==0) &&
      (strcmp(camera->model,"A602f")==0) &&
      (feature_id==DC1394_FEATURE_SHUTTER)) {
    info->is_scaled_quantity = 1;
    info->scaled_unit_name = "msec";
    info->scale_offset = 0.0;
    info->scale_gain = 0.02;
  }
}

void CCdc1394_get_camera_property(CCdc1394 *this,
				  int property_number,
				  long* Value,
				  int* Auto ) {
  dc1394camera_t *camera;
  int feature_id;
  uint32_t this_val;
  dc1394feature_mode_t this_mode;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];

  if (property_number < 0) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("invalid property_number");
    return;
  }

  if (property_number >= features_by_device_number[this->inherited.device_number].num_features) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("invalid property_number");
    return;
  }

  feature_id = features_by_device_number[this->inherited.device_number].dc1394_feature_ids[property_number];

  if (feature_id == DC1394_FEATURE_WHITE_BALANCE ) {
    // we don't support multi-value features.
    *Value = 0;
    *Auto = 0;
    return;
  }

  CIDC1394CHK(dc1394_feature_get_value(camera, feature_id, &this_val));
  *Value = this_val;

  CIDC1394CHK(dc1394_feature_get_mode(camera, feature_id, &this_mode));
  switch (this_mode) {
  case DC1394_FEATURE_MODE_MANUAL: *Auto = 0; break;
  case DC1394_FEATURE_MODE_AUTO: *Auto = 1; break;
  default:
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("unknown mode");
    return;
  }
}

void CCdc1394_set_camera_property(CCdc1394 *this,
				  int property_number,
				  long Value,
				  int Auto ) {
  dc1394camera_t *camera;
  int feature_id;
  uint32_t this_val;
  dc1394feature_mode_t this_mode;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];

  if (property_number < 0) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("invalid property_number");
    return;
  }

  if (property_number >= features_by_device_number[this->inherited.device_number].num_features) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("invalid property_number");
    return;
  }

  feature_id = features_by_device_number[this->inherited.device_number].dc1394_feature_ids[property_number];

  if (feature_id == DC1394_FEATURE_WHITE_BALANCE ) {
    // we don't support multi-value features.
    fprintf(stderr,"not setting camera white balance (%s, line %d)",__FILE__,__LINE__);
    return;
  }

  this_val = Value; // cast
  CIDC1394CHK(dc1394_feature_set_value(camera, feature_id, this_val));

  if (Auto==0) {
    this_mode = DC1394_FEATURE_MODE_MANUAL;
  } else {
    this_mode = DC1394_FEATURE_MODE_AUTO;
  }
  CIDC1394CHK(dc1394_feature_set_mode(camera, feature_id, this_mode));
}

void CCdc1394_grab_next_frame_blocking_with_stride( CCdc1394 *this,
						    unsigned char *out_bytes,
						    intptr_t stride0, float timeout) {
  dc1394camera_t *camera;
  dc1394video_frame_t *frame;
  int row, depth, wb;
  uint32_t w,h;
#ifdef CAM_IFACE_DC1394_SLOWDEBUG
  uint32_t h_size,v_size;
  int scalable;
#endif
  struct timeval tv;
  int retval;
  int errsv;
  dc1394error_t err;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];

  // wait on our fileno
  FD_SET(this->fileno, &(this->fdset));

  if (timeout >= 0) {
    // wait for up to timeout seconds for something to become available on our fileno.
    tv.tv_sec = timeout;
    tv.tv_usec = ((timeout-(float)tv.tv_sec)*1.0e6);
    retval = select( this->nfds, &(this->fdset), NULL, NULL, &tv );
  } else {
    retval = select( this->nfds, &(this->fdset), NULL, NULL, NULL );
  }

  errsv = errno;

  if (retval < 0 ) {
    if (errsv==EINTR) {
      cam_iface_error = CAM_IFACE_FRAME_INTERRUPTED_SYSCALL;
      CAM_IFACE_ERROR_FORMAT("Interrupted syscall (EINTR)");
      return;
    } else {
      // some error that we want to deal with
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("select() error");
      return;
    }
  } else if (retval==0) {
    // timeout exceeded
    cam_iface_error = CAM_IFACE_FRAME_TIMEOUT;
    CAM_IFACE_ERROR_FORMAT("timeout exceeded");
    return;
  }

  // Poll here even thought user wants to block -- we blocked above
  // using select(), and this lets us handle EINTR.

  CIDC1394CHK(dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_POLL, &frame));

  if (frame==NULL) {
    // No error, but no frame: polling ioctl call returned with EINTR.
    cam_iface_error = CAM_IFACE_FRAME_INTERRUPTED_SYSCALL;
    CAM_IFACE_ERROR_FORMAT("Interrupted syscall (EINTR)");
    return;
  }

  (this->nframe_hack)+=1;

  w = frame->size[0];
  h = frame->size[1];
  depth=this->inherited.depth;

#ifdef CAM_IFACE_DC1394_SLOWDEBUG
  if (!_get_size_for_video_mode(this->inherited.device_number,
				camera->video_mode,&h_size,&v_size,&scalable)){
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("Could not get size for video mode");
    return;
  }
  fprintf(stderr,"h_size %d, v_size %d, scalable %d\n",h_size,v_size,scalable);
  if (h_size!=w) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("Width returned not accurate");
    return;
  }
  if (v_size!=h) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("Height returned not accurate");
    return;
  }
#endif

  wb=w*depth/8;

  if (wb>stride0) {
    cam_iface_error = -1;
    fprintf(stderr,"w %d, wb %d, stride0 %d, depth %d\n",w,wb,stride0,depth);
    CAM_IFACE_ERROR_FORMAT("the buffer provided is not large enough");
    return;
  }

  for (row=0;row<h;row++) {
    memcpy((void*)(out_bytes+row*stride0), /*dest*/
    	   (const void*)(frame->image + row*wb),/*src*/
    	   wb);/*size*/
  }

  this->last_timestamp=frame->timestamp; // get timestamp

  CIDC1394CHK(dc1394_capture_enqueue (camera, frame));
}

void CCdc1394_grab_next_frame_blocking( CCdc1394 *this, unsigned char *out_bytes, float timeout) {
  CHECK_CC(this);
  int stride0 = this->roi_width*(this->inherited.depth/8);
  CCdc1394_grab_next_frame_blocking_with_stride(this,out_bytes,stride0,timeout);
}

void CCdc1394_point_next_frame_blocking( CCdc1394 *this, unsigned char **buf_ptr, float timeout) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCdc1394_unpoint_frame( CCdc1394 *this){
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}

void CCdc1394_get_last_timestamp( CCdc1394 *this, double* timestamp ) {
  CHECK_CC(this);
  // convert from microseconds to seconds
  *timestamp = (double)(this->last_timestamp) * 1e-6;
}

void CCdc1394_get_last_framenumber( CCdc1394 *this, long* framenumber ){
  CHECK_CC(this);
  *framenumber=this->nframe_hack;
}

void CCdc1394_get_num_trigger_modes( CCdc1394 *this,
				       int *num_exposure_modes ) {
  dc1394camera_t *camera;
  dc1394bool_t has_trigger;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];
  CIDC1394CHK(dc1394_feature_is_present(camera,
					DC1394_FEATURE_TRIGGER,
					&has_trigger));
  if (has_trigger) {
    *num_exposure_modes = 2;
  } else {
    *num_exposure_modes = 1;
  }
}

void CCdc1394_get_trigger_mode_string( CCdc1394 *this,
				       int exposure_mode_number,
				       char* exposure_mode_string, //output parameter
				       int exposure_mode_string_maxlen) {
  dc1394camera_t *camera;
  dc1394bool_t has_trigger;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];
  CIDC1394CHK(dc1394_feature_is_present(camera,
					DC1394_FEATURE_TRIGGER,
					&has_trigger));
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

void CCdc1394_get_trigger_mode_number( CCdc1394 *this,
				       int *exposure_mode_number ) {
  dc1394camera_t *camera;
  dc1394bool_t has_trigger;
  dc1394switch_t pwr;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];
  CIDC1394CHK(dc1394_feature_is_present(camera,
					DC1394_FEATURE_TRIGGER,
					&has_trigger));
  if (!has_trigger) {
    *exposure_mode_number = 0;
    return;
  }

  CIDC1394CHK(dc1394_feature_get_power(camera,DC1394_FEATURE_TRIGGER,&pwr));

  if (!pwr) {
    *exposure_mode_number = 0;
  } else {
    *exposure_mode_number = 1;
  }

  return;
}

void CCdc1394_set_trigger_mode_number( CCdc1394 *this,
				       int exposure_mode_number ) {
  dc1394camera_t *camera;
  dc1394bool_t has_trigger;
  dc1394switch_t pwr;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];
  CIDC1394CHK(dc1394_feature_is_present(camera,
					DC1394_FEATURE_TRIGGER,
					&has_trigger));

  if (!has_trigger) {
    if (exposure_mode_number!=0) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("invalid trigger mode");
    }
    return;
  }

  switch (exposure_mode_number) {
  case 0: pwr = 0; break;
  case 1: pwr = 1; break;
  default:
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("exposure_mode_number invalid");
    return;
  }
  CIDC1394CHK(dc1394_feature_set_power(camera, DC1394_FEATURE_TRIGGER, pwr));
  return;
}

void CCdc1394_get_frame_offset( CCdc1394 *this,
				int *left, int *top ) {
  uint32_t l,t;
  dc1394camera_t *camera;
  dc1394video_mode_t video_mode;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];
  CIDC1394CHK(dc1394_video_get_mode(camera, &video_mode));
  if (cam_iface_is_video_mode_scalable(video_mode)) {
    CIDC1394CHK(dc1394_format7_get_image_position(cameras[this->inherited.device_number],
						  video_mode,
						  &l, &t));
    *left=l;
    *top=t;
  } else {
    *left=0;
    *top=0;
  }
}

void CCdc1394_set_frame_offset( CCdc1394 *this,
				int left, int top ) {
  dc1394camera_t *camera;
  dc1394video_mode_t video_mode;
  uint32_t h_unit_pos,  v_unit_pos;
  dc1394color_coding_t coding;
  int restart;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];
  video_mode = modes_by_device_number[this->inherited.device_number].modes[this->cam_iface_mode_number].video_mode;
  coding = modes_by_device_number[this->inherited.device_number].modes[this->cam_iface_mode_number].color_coding;

  if (!dc1394_is_video_mode_scalable(video_mode)) {
    CAM_IFACE_ERROR_FORMAT("cannot set frame offset - video mode not scalable");
    return;
  }

  CIDC1394CHK(dc1394_format7_get_unit_position(camera,
					       video_mode,
					       &h_unit_pos,
					       &v_unit_pos));
  left = left/h_unit_pos * h_unit_pos;
  top = left/v_unit_pos * v_unit_pos;

  restart = 0;
  if (this->capture_is_set>0) {
    CCdc1394_stop_camera( this );
    restart = 1;
  }

  CIDC1394CHK(dc1394_format7_set_roi(camera, video_mode, coding,
				     DC1394_USE_MAX_AVAIL, // use max packet size
				     left, top,
				     this->roi_width,
				     this->roi_height));
  if (restart) {
    CCdc1394_start_camera( this );
  }
}


void CCdc1394_get_frame_size( CCdc1394 *this,
			      int *width, int *height ) {
  CHECK_CC(this);
  *width=this->roi_width;
  *height=this->roi_height;
}

void CCdc1394_set_frame_size( CCdc1394 *this,
			      int width, int height ) {
  dc1394camera_t *camera;
  dc1394video_mode_t video_mode;
  int restart;
  uint32_t test_width, test_height;

  restart=0;
  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];
  CIDC1394CHK(dc1394_video_get_mode(camera, &video_mode));
  if (!dc1394_is_video_mode_scalable(video_mode)) {
    CAM_IFACE_ERROR_FORMAT("cannot set frame offset - video mode not scalable");
    return;
  }
  if (this->capture_is_set>0) {
    CCdc1394_stop_camera( this );
    restart = 1;
  }
  uint32_t h_unit,v_unit;
  CIDC1394CHK(dc1394_format7_get_unit_size(camera,
					   video_mode,
					   &h_unit, &v_unit));

  if ((width%h_unit != 0) || (height%v_unit != 0)) {
    if (restart) {
      CCdc1394_start_camera( this );
    }
    CAM_IFACE_ERROR_FORMAT("specified ROI size not attainable with this camera");
    return;
  }

  CIDC1394CHK(dc1394_format7_set_image_size(camera,
					    video_mode,
					    width, height));

  CIDC1394CHK(dc1394_format7_get_image_size(camera, video_mode,
					    &test_width, &test_height));


  if (restart) {
    CCdc1394_start_camera( this );
  }

  if (test_width != width) {
    CAM_IFACE_ERROR_FORMAT("width was not successfully set");
    return;
  }

  if (test_height != height) {
    CAM_IFACE_ERROR_FORMAT("height was not successfully set");
    return;
  }

  this->roi_width = test_width;
  this->roi_height = test_height;
  this->buffer_size=(this->roi_width)*(this->roi_height)*this->inherited.depth/8;
}


void CCdc1394_get_framerate( CCdc1394 *this,
			     float *framerate ) {

  dc1394camera_t *camera;
  uint32_t ppf;
  dc1394video_mode_t video_mode;
  unsigned int speed;
  float bus_period;

  dc1394framerate_t framerate_idx;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];
  CIDC1394CHK(dc1394_video_get_mode(camera, &video_mode));


  if (cam_iface_is_video_mode_scalable(video_mode)) {
    // Format 7
    CIDC1394CHK(dc1394_format7_get_packets_per_frame(camera,
						     video_mode,
						     &ppf));

    CIDC1394CHK(dc1394_video_get_iso_speed(camera,&speed));
    switch (speed) {
    case DC1394_ISO_SPEED_400: bus_period = 125e-6; break;
    default: NOT_IMPLEMENTED; break;
    }

    *framerate = (1.0/(bus_period*ppf));
  } else {
    framerate_idx = modes_by_device_number[this->inherited.device_number].modes[this->cam_iface_mode_number].framerate;
    switch (framerate_idx){
    case DC1394_FRAMERATE_1_875: *framerate=1.875; break;
    case DC1394_FRAMERATE_3_75:  *framerate=3.75; break;
    case DC1394_FRAMERATE_7_5:   *framerate=7.5; break;
    case DC1394_FRAMERATE_15:    *framerate=15.0; break;
    case DC1394_FRAMERATE_30:    *framerate=30.0; break;
    case DC1394_FRAMERATE_60:    *framerate=60.0; break;
    case DC1394_FRAMERATE_120:   *framerate=120.0; break;
    case DC1394_FRAMERATE_240:   *framerate=240.0; break;
    default:
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("invalid framerate index");
      return;
    }
  }
}

void CCdc1394_set_framerate( CCdc1394 *this,
			     float framerate ) {
  dc1394camera_t *camera;
  int num_packets, denominator;
  uint64_t total_bytes;
  uint32_t packet_size;
  dc1394video_mode_t video_mode;
  unsigned int speed;
  float bus_period;
  int restart = 0;
  unsigned int min_bytes, max_bytes;

  CHECK_CC(this);
  camera = cameras[this->inherited.device_number];
  CIDC1394CHK(dc1394_video_get_mode(camera, &video_mode));

  if (cam_iface_is_video_mode_scalable(video_mode)) {
    if (this->capture_is_set>0) {
      CCdc1394_stop_camera( this );
      restart = 1;
    }
    // Format 7

    // see http://damien.douxchamps.net/ieee1394/libdc1394/v2.x/faq/#How_can_I_work_out_the_packet_size_for_a_wanted_frame_rate

    CIDC1394CHK(dc1394_video_get_iso_speed(camera,&speed));
    switch (speed) {
    case DC1394_ISO_SPEED_400: bus_period = 125e-6; break;
    default: NOT_IMPLEMENTED; break;
    }

    CIDC1394CHK(dc1394_format7_get_packet_parameters(camera, video_mode, &min_bytes, &max_bytes));


    num_packets = (int) (1.0/(bus_period*framerate) + 0.5);
    denominator = num_packets;
    CIDC1394CHK(dc1394_format7_get_total_bytes(camera, video_mode, &total_bytes));
    packet_size = (total_bytes + denominator - 1)/denominator;

    packet_size = packet_size >= min_bytes ? packet_size : min_bytes;
    packet_size = packet_size <= max_bytes ? packet_size : max_bytes;

    // make packet_size integer multiple of min_bytes
    packet_size = ((packet_size + min_bytes - 1)/min_bytes)*min_bytes;

    CIDC1394CHK(dc1394_format7_set_packet_size(camera, video_mode, packet_size));
    CIDC1394CHK(dc1394_format7_get_packet_size(camera, video_mode, &packet_size));

    usleep(DELAY);

    if (restart) {
      CCdc1394_start_camera( this );
    }
  } else {
    NOT_IMPLEMENTED;
    // should call dc1394_video_set_framerate()
  }
}

void CCdc1394_get_max_frame_size( CCdc1394 *this,
				  int *width, int *height ){
  CHECK_CC(this);
  *width=this->max_width;
  *height=this->max_height;
}

CAM_IFACE_API void CCdc1394_get_buffer_size( CCdc1394 *this,
					     int *size) {
  CHECK_CC(this);
  *size=this->buffer_size;
}

void CCdc1394_get_num_framebuffers( CCdc1394 *this,
				    int *num_framebuffers ) {

  CHECK_CC(this);
  *num_framebuffers=this->num_dma_buffers;
}

void CCdc1394_set_num_framebuffers( CCdc1394 *this,
				    int num_framebuffers ) {
  CHECK_CC(this);
  NOT_IMPLEMENTED;
}
