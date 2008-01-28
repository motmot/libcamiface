#include "cam_iface.h"
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>

struct backend_info_t {
  char* name;
  int started;
  int cam_start_idx;
  int cam_stop_idx;
  int (*have_error)(void);
  void (*startup)(void);
  void (*shutdown)(void);
  char *(*get_error_string)(void);
  int (*get_num_cameras)(void);
};

/* globals -- allocate space */
int num_cameras;
int cam_iface_error = 0;
#define CAM_IFACE_MAX_ERROR_LEN 255
char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN]  = {0x00}; //...

#define NUM_BACKENDS 3
char *backend_names[NUM_BACKENDS] = {"pynet","dc1394","prosilica_gige"};
struct backend_info_t backend_info[NUM_BACKENDS];

#define CAM_IFACE_ERROR_FORMAT(m)					\
  snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,		\
	   "%s (%d): %s\n",__FILE__,__LINE__,(m));

#define CHECK_CI_ERR()						\
  if (this_backend_info->have_error()) {			\
    cam_iface_error = this_backend_info->have_error();		\
    snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,	\
	     "%s",this_backend_info->get_error_string());	\
    return;							\
  }


const char* cam_iface_get_api_version() {
  return CAM_IFACE_API_VERSION;
}

int cam_iface_have_error() {
  return cam_iface_error;
}

const char * cam_iface_get_error_string() {
  return cam_iface_error_string;
}

#define LOAD_DLSYM(var,name) {					\
    (var) = dlsym(libhandle,(name));				\
    if ((var)==NULL) {						\
      cam_iface_error = -1;					\
      CAM_IFACE_ERROR_FORMAT("dlsym() error");			\
      return;							\
    }								\
  }

const char *cam_iface_get_driver_name(void) {
  return "unity";
}

int cam_iface_get_num_cameras(void) {
  return num_cameras;
}

void cam_iface_startup(void) {
  int* libhandle;
  struct backend_info_t* this_backend_info;
  char *full_backend_name;
  int i, next_num_cameras;

  num_cameras = 0;

  for (i=0; i<NUM_BACKENDS; i++) {
    this_backend_info = &(backend_info[i]);
    this_backend_info->name = backend_names[i];
    this_backend_info->started = 0;

    full_backend_name = (char*)malloc( 256*sizeof(char) );
    snprintf(full_backend_name,256,"./lib/libcam_iface_%s.so",backend_names[i]);

    // RTLD_GLOBAL needed for embedded Python to work. (For examples, see pythoncall.c
    // and pymplug.c.)

    libhandle = dlopen(full_backend_name, RTLD_NOW | RTLD_GLOBAL );
    if (libhandle==NULL) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("dlopen() error");
      return;
    }

    LOAD_DLSYM(this_backend_info->have_error,"cam_iface_have_error");
    LOAD_DLSYM(this_backend_info->startup,"cam_iface_startup");
    LOAD_DLSYM(this_backend_info->get_error_string,"cam_iface_get_error_string");
    //LOAD_DLSYM(this_backend_info->get_num_modes,"cam_iface_get_num_modes");
    //LOAD_DLSYM(this_backend_info->get_mode_string,"cam_iface_get_mode_string");
    LOAD_DLSYM(this_backend_info->get_num_cameras,"cam_iface_get_num_cameras");
    //LOAD_DLSYM(this_backend_info->get_camera_info,"cam_iface_get_camera_info");

    fprintf(stderr,"opening backend: %s\n",this_backend_info->name);
    this_backend_info->startup();
    CHECK_CI_ERR();
    this_backend_info->started = 1;

    fprintf(stderr,"OK\n");
    next_num_cameras = num_cameras + this_backend_info->get_num_cameras();
    this_backend_info->cam_start_idx = num_cameras;
    this_backend_info->cam_stop_idx = next_num_cameras;
    num_cameras = next_num_cameras;
  }

}

void cam_iface_shutdown(void) {
  int i;
  struct backend_info_t* this_backend_info;
  for (i=0; i<NUM_BACKENDS; i++) {
    this_backend_info = &(backend_info[i]);
    if (this_backend_info->started) {
      this_backend_info->shutdown();
    }
  }
}

void CamContext_CamContext(CamContext *ccntxt,
			   int device_number,int NumImageBuffers,
			   int mode_number) {
  for (i=0; i<NUM_BACKENDS; i++) {
    this_backend_info = &(backend_info[i]);
    if ( (this_backend_info->start_idx <= device_number) &&
	 (device_number < this_backend_info->stop_idx) ) {
      this_backend_info->CamContext_CamContext(ccntxt,
					       device_number-this_backend_info->start_idx,
					       NumImageBuffers,mode_number);
      CHECK_CI_ERR();
    }
  }
}
