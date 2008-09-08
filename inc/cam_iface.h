/* cam_iface.h -- Camera Interface to present generic interface to any driver

*/

#ifndef CAM_IFACE_H
#define CAM_IFACE_H

#define CAM_IFACE_API_VERSION "20080124a"

#ifdef _WIN32
#include <windows.h>
#include <process.h> // for intptr_t

#ifndef _INTPTR_T_DEFINED
typedef int		intptr_t;
#  define _INTPTR_T_DEFINED
#endif

#else
#include <unistd.h>
#endif

/* get intptr_t */
#include <stdint.h>

#ifdef CAM_IFACE_DLL
#ifdef CAM_IFACE_EXPORTS
#define CAM_IFACE_API __declspec(dllexport)
#else
#define CAM_IFACE_API __declspec(dllimport)
#endif /* CAM_IFACE_EXPORTS */
#else
#define CAM_IFACE_API extern
#endif /* CAM_IFACE_DLL */



#ifdef __cplusplus
extern "C" {
#endif

#ifndef CAMWIRE_H
/* This will be the same across backends, even though it is defined
   natively by camwire. */

/* Limits for Camwire_id: */
#define CAMWIRE_ID_MAX_CHARS	100

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
#endif /* CAMWIRE_H */

CAM_IFACE_API const char *cam_iface_get_driver_name(void);

CAM_IFACE_API void cam_iface_clear_error(void);
CAM_IFACE_API int cam_iface_have_error(void);
CAM_IFACE_API const char * cam_iface_get_error_string(void);

#define CAM_IFACE_BUFFER_OVERFLOW_ERROR -392081
#define CAM_IFACE_FRAME_DATA_MISSING_ERROR -392073
#define CAM_IFACE_FRAME_TIMEOUT -392074
#define CAM_IFACE_FRAME_DATA_LOST_ERROR -392075
#define CAM_IFACE_HARDWARE_FEATURE_NOT_AVAILABLE -392076
#define CAM_IFACE_OTHER_ERROR -392077
#define CAM_IFACE_FRAME_INTERRUPTED_SYSCALL -392078
#define CAM_IFACE_GENERIC_ERROR -1

CAM_IFACE_API const char* cam_iface_get_api_version(void);
CAM_IFACE_API void cam_iface_startup(void); /* call cam_iface_startup_with_version_check() */
CAM_IFACE_API void cam_iface_shutdown(void);

#define cam_iface_startup_with_version_check() {			\
  if (strcmp(CAM_IFACE_API_VERSION,cam_iface_get_api_version())) {	\
    fprintf(stderr,"cam_iface error: code uses API version %s, ",CAM_IFACE_API_VERSION); \
    fprintf(stderr,"but library implements %s. Exiting.\n",cam_iface_get_api_version()); \
    exit(1);								\
  }									\
  cam_iface_startup();							\
}

CAM_IFACE_API int cam_iface_get_num_cameras(void);
CAM_IFACE_API void cam_iface_get_camera_info(int device_number, Camwire_id *out_camid);

typedef struct CameraPropertyInfo CameraPropertyInfo;
struct CameraPropertyInfo {
  const char* name; /* name of property, e.g. "brightness" or "shutter" or "gain" */
  int is_present;   /* set to nonzero if property is available on camera */

  long min_value;
  long max_value;
  int has_auto_mode;
  int has_manual_mode;

  int is_scaled_quantity; /* set to nonzero if the following values are used */

  /* the following values are not used if is_scaled_quantity is zero. */
  const char* scaled_unit_name; /* e.g. "msec" */
  double scale_offset; /* Value ("val") sent to cam_iface will always be long, */
  double scale_gain;   /* but the display value will be val*scale_gain+scale_offset */
};

typedef enum CameraPixelCoding
{
  CAM_IFACE_UNKNOWN=0,
  CAM_IFACE_MONO8,
  CAM_IFACE_YUV411,
  CAM_IFACE_YUV422,
  CAM_IFACE_YUV444,
  CAM_IFACE_RGB8,
  CAM_IFACE_MONO16,
  CAM_IFACE_RGB16,
  CAM_IFACE_MONO16S,
  CAM_IFACE_RGB16S,
  CAM_IFACE_RAW8,
  CAM_IFACE_RAW16,
  CAM_IFACE_ARGB8
}
CameraPixelCoding;

/* Get the number of video modes possible (e.g. 640x480 x MONO8 or 1600x1200xYUV422) */
CAM_IFACE_API void cam_iface_get_num_modes(int device_number, int *num_modes);

/* Describe the video mode possible (returns string "640x480xMONO8") */
CAM_IFACE_API void cam_iface_get_mode_string(int device_number,
					     int mode_number,
					     char* mode_string, /* output parameter */
					     int mode_string_maxlen);


/*
 The CamContext object has a virtual method table. For all function
 calls except the constructor, call the CamContext_function_name()
 version. In other words, don't call the derived class's
 implementation directly.

 For a simplified version or what the design pattern is, see
 http://flinflon.brandonu.ca/dueck/1997/62285/virtualc.html
*/

struct CamContext; /* forward declaration */

/* constructor that mallocs memory: */
typedef struct CamContext* (*cam_iface_constructor_func_t)(int,int,int);
cam_iface_constructor_func_t cam_iface_get_constructor_func(int device_number);

/* keep functable in sync across backends */
typedef struct {
  /* constructor that mallocs memory: */
  cam_iface_constructor_func_t construct;
  /* destructor that frees memory: */
  void (*destruct)(struct CamContext*);

  /* constructor on already malloced memory: */
  void (*CamContext)(struct CamContext*,int,int,int);
  /* destructor on externally malloced memory: */
  void (*close)(struct CamContext*);

  void (*start_camera)(struct CamContext*);
  void (*stop_camera)(struct CamContext*);
  void (*get_num_camera_properties)(struct CamContext*,int*);
  void (*get_camera_property_info)(struct CamContext*,
				   int,
				   CameraPropertyInfo*);
  void (*get_camera_property)(struct CamContext*,int,long*,int*);
  void (*set_camera_property)(struct CamContext*,int,long,int);
  void (*grab_next_frame_blocking)(struct CamContext*,
				   unsigned char*,
				   float);
  void (*grab_next_frame_blocking_with_stride)(struct CamContext*,
					       unsigned char*,
					       intptr_t,
					       float);
  void (*point_next_frame_blocking)(struct CamContext*,unsigned char**,float);
  void (*unpoint_frame)(struct CamContext*);
  void (*get_last_timestamp)(struct CamContext*,double*);
  void (*get_last_framenumber)(struct CamContext*,unsigned long*);
  void (*get_num_trigger_modes)(struct CamContext*,int*);
  void (*get_trigger_mode_string)(struct CamContext*,int,char*,int);
  void (*get_trigger_mode_number)(struct CamContext*,int*);
  void (*set_trigger_mode_number)(struct CamContext*,int);
  void (*get_frame_offset)(struct CamContext*,int*,int*);
  void (*set_frame_offset)(struct CamContext*,int,int);
  void (*get_frame_size)(struct CamContext*,int*,int*);
  void (*set_frame_size)(struct CamContext*,int,int);
  void (*get_max_frame_size)(struct CamContext*,int*,int*);
  void (*get_buffer_size)(struct CamContext*,int*);
  void (*get_framerate)(struct CamContext*,float*);
  void (*set_framerate)(struct CamContext*,float);
  void (*get_num_framebuffers)(struct CamContext*,int*);
  void (*set_num_framebuffers)(struct CamContext*,int);

} CamContext_functable;

typedef struct CamContext { /* These are READ ONLY. To change, call the appropriate CamContext_set function. */
  CamContext_functable *vmt;
  void *cam;                     /* opaque pointer to backend-dependent camera (e.g. CBcam) */
  void *backend_extras;          /* opaque pointer to backend-dependent additional camera data */
  CameraPixelCoding coding; /* CAM_IFACE_MONO8 etc. */
  int depth;           /* mean bits per pixel (e.g. 8 for MONO8, 12 for YUV411, 16 for YUV422) */
  int device_number;
} CamContext;

CAM_IFACE_API void delete_CamContext(CamContext*);
CAM_IFACE_API void CamContext_CamContext(CamContext *ccntxt,
					 int device_number,int NumImageBuffers,
					 int mode_number);
CAM_IFACE_API void CamContext_close(CamContext *ccntxt);

CAM_IFACE_API void CamContext_start_camera(CamContext *ccntxt);
CAM_IFACE_API void CamContext_stop_camera(CamContext *ccntxt);

CAM_IFACE_API void CamContext_get_num_camera_properties(CamContext *ccntxt,
							int* num_properties); /* output parameter */

CAM_IFACE_API void CamContext_get_camera_property_info(CamContext *ccntxt,
						       int property_number,
						       CameraPropertyInfo *info);

CAM_IFACE_API void CamContext_get_camera_property(CamContext *ccntxt,
						  int property_number,
						  long* Value,
						  int* Auto);

CAM_IFACE_API void CamContext_set_camera_property(CamContext *ccntxt,
						  int property_number,
						  long Value,
						  int Auto);

/* copy the image data into a buffer passed in */
CAM_IFACE_API void CamContext_grab_next_frame_blocking(CamContext *ccntxt, unsigned char* out_bytes, float timeout);

/* copy the image data into a buffer passed in (with buffer stride information) */
CAM_IFACE_API void CamContext_grab_next_frame_blocking_with_stride(CamContext *ccntxt, unsigned char* out_bytes, intptr_t stride0, float timeout);

/* get a pointer to the image data */
CAM_IFACE_API void CamContext_point_next_frame_blocking(CamContext *ccntxt, unsigned char** buf_ptr, float timeout);
/* release point to the image data */
CAM_IFACE_API void CamContext_unpoint_frame(CamContext *ccntxt);

CAM_IFACE_API void CamContext_get_last_timestamp( CamContext *ccntxt,
					   double* timestamp );
CAM_IFACE_API void CamContext_get_last_framenumber( CamContext *ccntxt,
						    unsigned long* framenumber );

CAM_IFACE_API void CamContext_get_num_trigger_modes( CamContext *ccntxt,
						     int *num_exposure_modes ); /* output parameter */
CAM_IFACE_API void CamContext_get_trigger_mode_string( CamContext *ccntxt,
						       int exposure_mode_number,
						       char* exposure_mode_string, /* output parameter */
						       int exposure_mode_string_maxlen);
CAM_IFACE_API void CamContext_get_trigger_mode_number( CamContext *ccntxt,
						       int *exposure_mode_number ); /* output parameter */
CAM_IFACE_API void CamContext_set_trigger_mode_number( CamContext *ccntxt,
						       int exposure_mode_number );

CAM_IFACE_API void CamContext_get_frame_offset( CamContext *ccntxt,
					 int *left, int *top );
CAM_IFACE_API void CamContext_set_frame_offset( CamContext *ccntxt,
					 int left, int top );

CAM_IFACE_API void CamContext_get_frame_size( CamContext *ccntxt,
				       int *width, int *height );
CAM_IFACE_API void CamContext_set_frame_size( CamContext *ccntxt,
				       int width, int height );

CAM_IFACE_API void CamContext_get_max_frame_size( CamContext *ccntxt,
						  int *width,
						  int *height );

CAM_IFACE_API void CamContext_get_buffer_size( CamContext *ccntxt,
					       int *size);

CAM_IFACE_API void CamContext_get_framerate( CamContext *ccntxt,
					     float *framerate );
CAM_IFACE_API void CamContext_set_framerate( CamContext *ccntxt,
					     float framerate );

CAM_IFACE_API void CamContext_get_num_framebuffers( CamContext *ccntxt,
					     int *num_framebuffers );

CAM_IFACE_API void CamContext_set_num_framebuffers( CamContext *ccntxt,
					     int num_framebuffers );

#ifdef __cplusplus
}
#endif
#endif /* CAM_IFACE_H */
