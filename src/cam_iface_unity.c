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
  void (*clear_error)(void);
  const char* (*get_error_string)(void);
  void (*startup)(void);
  void (*shutdown)(void);
  int (*get_num_cameras)(void);
  void (*get_num_modes)(int,int*);
  void (*get_camera_info)(int,Camwire_id*);
  void (*get_mode_string)(int,int,char*,int);
  cam_iface_constructor_func_t (*get_constructor_func)(int);
};

/* globals -- allocate space */
int num_cameras;
int cam_iface_error = 0;
#define CAM_IFACE_MAX_ERROR_LEN 255
char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN]  = {0x00}; //...

#define NUM_BACKENDS 2
char *backend_names[NUM_BACKENDS] = {"dc1394","prosilica_gige"};
struct backend_info_t backend_info[NUM_BACKENDS];
static int backends_started = 0;

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
  int i;
  int err;
  struct backend_info_t* this_backend_info;

  if (cam_iface_error) {
    return cam_iface_error;
  }

  if (backends_started) {
    err = 0;
    for (i=0; i<NUM_BACKENDS; i++) {
      this_backend_info = &(backend_info[i]);
      if (this_backend_info->started) {
	err = this_backend_info->have_error();
	if (err) return err;
      }
    }
  }
  return 0;
}

void cam_iface_clear_error() {
  int i;
  struct backend_info_t* this_backend_info;

  cam_iface_error=0;

  if (backends_started) {
    for (i=0; i<NUM_BACKENDS; i++) {
      this_backend_info = &(backend_info[i]);
      if (this_backend_info->started) {
	this_backend_info->clear_error();
      }
    }
  }
}

const char * cam_iface_get_error_string() {
  struct backend_info_t* this_backend_info;
  int i,err;

  if (cam_iface_error) {
    return cam_iface_error_string;
  }

  if (backends_started) {
    err = 0;
    for (i=0; i<NUM_BACKENDS; i++) {
      this_backend_info = &(backend_info[i]);
      if (this_backend_info->started) {
	err = this_backend_info->have_error();
	if (err) return this_backend_info->get_error_string();
      }
    }
  }
  return "";
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
    snprintf(full_backend_name,256,UNITY_BACKEND_PREFIX "cam_iface_%s" UNITY_BACKEND_SUFFIX ,backend_names[i]);

    // RTLD_GLOBAL needed for embedded Python to work. (For examples, see pythoncall.c
    // and pymplug.c.)

    libhandle = dlopen(full_backend_name, RTLD_NOW | RTLD_GLOBAL );
    if (libhandle==NULL) {
      /*
      cam_iface_error = -1;
      snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,
	       "%s (%d): dlopen() error when attempting to open backend '%s' (at %s)\n",__FILE__,__LINE__,
	       backend_names[i],full_backend_name);
      */
      this_backend_info->cam_start_idx = num_cameras;
      this_backend_info->cam_stop_idx = num_cameras;
      continue;
      //return;
    }

    LOAD_DLSYM(this_backend_info->have_error,"cam_iface_have_error");
    LOAD_DLSYM(this_backend_info->clear_error,"cam_iface_clear_error");
    LOAD_DLSYM(this_backend_info->get_error_string,"cam_iface_get_error_string");
    LOAD_DLSYM(this_backend_info->startup,"cam_iface_startup");
    LOAD_DLSYM(this_backend_info->shutdown,"cam_iface_shutdown");
    LOAD_DLSYM(this_backend_info->get_num_cameras,"cam_iface_get_num_cameras");
    LOAD_DLSYM(this_backend_info->get_num_modes,"cam_iface_get_num_modes");
    LOAD_DLSYM(this_backend_info->get_camera_info,"cam_iface_get_camera_info");

    LOAD_DLSYM(this_backend_info->get_mode_string,"cam_iface_get_mode_string");
    LOAD_DLSYM(this_backend_info->get_constructor_func,"cam_iface_get_constructor_func");

    this_backend_info->startup();
    CHECK_CI_ERR();
    this_backend_info->started = 1;

    next_num_cameras = num_cameras + this_backend_info->get_num_cameras();
    this_backend_info->cam_start_idx = num_cameras;
    this_backend_info->cam_stop_idx = next_num_cameras;
    num_cameras = next_num_cameras;
  }
  backends_started = 1;
}

void cam_iface_shutdown(void) {
  int i;
  struct backend_info_t* this_backend_info;
  for (i=0; i<NUM_BACKENDS; i++) {
    this_backend_info = &(backend_info[i]);
    if (this_backend_info->started) {
      this_backend_info->shutdown();
      this_backend_info->started = 0;
    }
  }
}

CamContext* CCunity_construct( int device_number, int NumImageBuffers,
			       int mode_number) {
  int i;
  struct backend_info_t* this_backend_info;
  CamContext* result;
  cam_iface_constructor_func_t construct;

  for (i=0; i<NUM_BACKENDS; i++) {
    this_backend_info = &(backend_info[i]);
    if ( (this_backend_info->cam_start_idx <= device_number) &&
	 (device_number < this_backend_info->cam_stop_idx) ) {

      construct = this_backend_info->get_constructor_func( device_number-this_backend_info->cam_start_idx );
      result = construct( device_number-this_backend_info->cam_start_idx,
			  NumImageBuffers, mode_number );
      CHECK_CI_ERR();
    }
  }
  return result;
}

cam_iface_constructor_func_t cam_iface_get_constructor_func(int device_number) {
  return CCunity_construct;
}

void cam_iface_get_mode_string(int device_number,
			       int mode_number,
			       char* mode_string, //output parameter
			       int mode_string_maxlen) {
  int i;
  struct backend_info_t* this_backend_info;
  for (i=0; i<NUM_BACKENDS; i++) {
    this_backend_info = &(backend_info[i]);
    if ( (this_backend_info->cam_start_idx <= device_number) &&
	 (device_number < this_backend_info->cam_stop_idx) ) {
      this_backend_info->get_mode_string(device_number-this_backend_info->cam_start_idx,
					 mode_number, mode_string, mode_string_maxlen);
      CHECK_CI_ERR();
    }
  }
}


void cam_iface_get_num_modes(int device_number,int* num_modes) {
  int i;
  struct backend_info_t* this_backend_info;
  for (i=0; i<NUM_BACKENDS; i++) {
    this_backend_info = &(backend_info[i]);
    if ( (this_backend_info->cam_start_idx <= device_number) &&
	 (device_number < this_backend_info->cam_stop_idx) ) {
      this_backend_info->get_num_modes(device_number-this_backend_info->cam_start_idx,num_modes);
      CHECK_CI_ERR();
    }
  }
}

void cam_iface_get_camera_info(int device_number,
			       Camwire_id *out_camid) { //output parameter
  int i;
  struct backend_info_t* this_backend_info;
  for (i=0; i<NUM_BACKENDS; i++) {
    this_backend_info = &(backend_info[i]);
    if ( (this_backend_info->cam_start_idx <= device_number) &&
	 (device_number < this_backend_info->cam_stop_idx) ) {
      this_backend_info->get_camera_info(device_number-this_backend_info->cam_start_idx,
					 out_camid);
      CHECK_CI_ERR();
    }
  }
}
