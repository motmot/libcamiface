/*

Copyright (c) 2004-2012, John Stowers. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/* Backend for libaravis-0.2 */
#include "cam_iface.h"

#define ARAVIS_INCLUDE_FAKE_CAMERA                0
#define ARAVIS_DEBUG_ENABLE                       0
#define ARAVIS_DEBUG_FRAME_ACQUSITION_BLOCKING    0
#define ARAVIS_DEBUG_FRAME_ACQUSITION_STRIDE      0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/select.h>
#include <errno.h>

#include <arv.h>
#include <glib.h>
#include <glib/gprintf.h>

struct CCaravis; // forward declaration

// keep functable in sync across backends
typedef struct {
  cam_iface_constructor_func_t construct;
  void (*destruct)(struct CamContext*);

  void (*CCaravis)(struct CCaravis*,int,int,int);
  void (*close)(struct CCaravis*);
  void (*start_camera)(struct CCaravis*);
  void (*stop_camera)(struct CCaravis*);
  void (*get_num_camera_properties)(struct CCaravis*,int*);
  void (*get_camera_property_info)(struct CCaravis*,
                                   int,
                                   CameraPropertyInfo*);
  void (*get_camera_property)(struct CCaravis*,int,long*,int*);
  void (*set_camera_property)(struct CCaravis*,int,long,int);
  void (*grab_next_frame_blocking)(struct CCaravis*,
                                   unsigned char*,
                                   float);
  void (*grab_next_frame_blocking_with_stride)(struct CCaravis*,
                                               unsigned char*,
                                               intptr_t,
                                               float);
  void (*point_next_frame_blocking)(struct CCaravis*,unsigned char**,float);
  void (*unpoint_frame)(struct CCaravis*);
  void (*get_last_timestamp)(struct CCaravis*,double*);
  void (*get_last_framenumber)(struct CCaravis*,unsigned long*);
  void (*get_num_trigger_modes)(struct CCaravis*,int*);
  void (*get_trigger_mode_string)(struct CCaravis*,int,char*,int);
  void (*get_trigger_mode_number)(struct CCaravis*,int*);
  void (*set_trigger_mode_number)(struct CCaravis*,int);
  void (*get_frame_roi)(struct CCaravis*,int*,int*,int*,int*);
  void (*set_frame_roi)(struct CCaravis*,int,int,int,int);
  void (*get_max_frame_size)(struct CCaravis*,int*,int*);
  void (*get_buffer_size)(struct CCaravis*,int*);
  void (*get_framerate)(struct CCaravis*,float*);
  void (*set_framerate)(struct CCaravis*,float);
  void (*get_num_framebuffers)(struct CCaravis*,int*);
  void (*set_num_framebuffers)(struct CCaravis*,int);
} CCaravis_functable;

typedef struct CCaravis {
  CamContext inherited;

  int roi_left;
  int roi_top;
  int roi_width;
  int roi_height;

  int started;

  int cam_iface_mode_number;

	guint32 last_frame_id;
	guint64 last_timestamp_ns;

  ArvCamera *camera;
  ArvStream *stream;
  int num_buffers;

  char **trigger_modes;
  int num_trigger_modes;

} CCaravis;

// forward declarations
CCaravis* CCaravis_construct( int device_number, int NumImageBuffers,
                              int mode_number);
void delete_CCaravis(struct CCaravis*);

void CCaravis_CCaravis(struct CCaravis*,int,int,int);
void CCaravis_close(struct CCaravis*);
void CCaravis_start_camera(struct CCaravis*);
void CCaravis_stop_camera(struct CCaravis*);
void CCaravis_get_num_camera_properties(struct CCaravis*,int*);
void CCaravis_get_camera_property_info(struct CCaravis*,
                              int,
                              CameraPropertyInfo*);
void CCaravis_get_camera_property(struct CCaravis*,int,long*,int*);
void CCaravis_set_camera_property(struct CCaravis*,int,long,int);
void CCaravis_grab_next_frame_blocking(struct CCaravis*,
                              unsigned char*,
                              float);
void CCaravis_grab_next_frame_blocking_with_stride(struct CCaravis*,
                                          unsigned char*,
                                          intptr_t,
                                          float);
void CCaravis_point_next_frame_blocking(struct CCaravis*,unsigned char**,float);
void CCaravis_unpoint_frame(struct CCaravis*);
void CCaravis_get_last_timestamp(struct CCaravis*,double*);
void CCaravis_get_last_framenumber(struct CCaravis*,unsigned long*);
void CCaravis_get_num_trigger_modes(struct CCaravis*,int*);
void CCaravis_get_trigger_mode_string(struct CCaravis*,int,char*,int);
void CCaravis_get_trigger_mode_number(struct CCaravis*,int*);
void CCaravis_set_trigger_mode_number(struct CCaravis*,int);
void CCaravis_get_frame_roi(struct CCaravis*,int*,int*,int*,int*);
void CCaravis_set_frame_roi(struct CCaravis*,int,int,int,int);
void CCaravis_get_max_frame_size(struct CCaravis*,int*,int*);
void CCaravis_get_buffer_size(struct CCaravis*,int*);
void CCaravis_get_framerate(struct CCaravis*,float*);
void CCaravis_set_framerate(struct CCaravis*,float);
void CCaravis_get_num_framebuffers(struct CCaravis*,int*);
void CCaravis_set_num_framebuffers(struct CCaravis*,int);

CCaravis_functable CCaravis_vmt = {
  (cam_iface_constructor_func_t)CCaravis_construct,
  (void (*)(CamContext*))delete_CCaravis,
  CCaravis_CCaravis,
  CCaravis_close,
  CCaravis_start_camera,
  CCaravis_stop_camera,
  CCaravis_get_num_camera_properties,
  CCaravis_get_camera_property_info,
  CCaravis_get_camera_property,
  CCaravis_set_camera_property,
  CCaravis_grab_next_frame_blocking,
  CCaravis_grab_next_frame_blocking_with_stride,
  CCaravis_point_next_frame_blocking,
  CCaravis_unpoint_frame,
  CCaravis_get_last_timestamp,
  CCaravis_get_last_framenumber,
  CCaravis_get_num_trigger_modes,
  CCaravis_get_trigger_mode_string,
  CCaravis_get_trigger_mode_number,
  CCaravis_set_trigger_mode_number,
  CCaravis_get_frame_roi,
  CCaravis_set_frame_roi,
  CCaravis_get_max_frame_size,
  CCaravis_get_buffer_size,
  CCaravis_get_framerate,
  CCaravis_set_framerate,
  CCaravis_get_num_framebuffers,
  CCaravis_set_num_framebuffers
};

// See the following for a hint on how to make thread thread-local without __thread.
// http://lists.apple.com/archives/Xcode-users/2006/Jun/msg00551.html
#ifdef __APPLE__
#define myTLS
#else
#define myTLS __thread
#endif

#ifdef MEGA_BACKEND
  #define BACKEND_GLOBAL(m) aravis_##m
#else
  #define BACKEND_GLOBAL(m) m
#endif

/* globals -- allocate space */
typedef struct {
  char    *device_name;
  gint64  *aravis_formats;
  int     num_modes;
  gint    maxw,maxh;
} ArvGlobalCamera;

myTLS int BACKEND_GLOBAL(cam_iface_error) = 0;
#define CAM_IFACE_MAX_ERROR_LEN 255
myTLS char BACKEND_GLOBAL(cam_iface_error_string)[CAM_IFACE_MAX_ERROR_LEN]  = {0x00}; //...

uint32_t aravis_num_cameras = 0;
ArvGlobalCamera *aravis_cameras = NULL;

/* one aravis thread and one mainloop per process, not per camera. I don't know
how much of libcamiface supports threading anyway, so im not sure of the gain in
making each camera threaded, or indeed if aravis already does this... */
GThread *aravis_thread = NULL;
GMainContext *aravis_context = NULL;
GMainLoop *aravis_mainloop = NULL;

#if ARAVIS_INCLUDE_FAKE_CAMERA
# define GET_ARAVIS_DEVICE_INDEX(i) (i)
#else
# define GET_ARAVIS_DEVICE_INDEX(i) (i+1)
#endif

#if !ARAVIS_DEBUG_ENABLE
# define DPRINTF(...)
#else
# define DPRINTF(...) printf("DEBUG:    " __VA_ARGS__); fflush(stdout);
#endif

#define DWARNF(...) fprintf(stderr, "WARN :    " __VA_ARGS__); fflush(stderr);

#ifdef MEGA_BACKEND
#define CAM_IFACE_ERROR_FORMAT(m)                                       \
  snprintf(aravis_cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,              \
           "%s (%d): %s\n",__FILE__,__LINE__,(m));
#else
#define CAM_IFACE_ERROR_FORMAT(m)                                       \
  snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,              \
           "%s (%d): %s\n",__FILE__,__LINE__,(m));
#endif

#ifdef MEGA_BACKEND
# define NOT_IMPLEMENTED                                  \
    aravis_cam_iface_error = -1;                          \
    fprintf(stderr,"WARN :    %s (%d): not yet implemented\n",__FILE__,__LINE__); fflush(stderr); \
    CAM_IFACE_ERROR_FORMAT("not yet implemented");        \
    return;
# define ARAVIS_ERROR(_code, _msg)                        \
    aravis_cam_iface_error = _code;                       \
    CAM_IFACE_ERROR_FORMAT(_msg);
#else
# define NOT_IMPLEMENTED                                  \
    cam_iface_error = -1;                                 \
    fprintf(stderr,"WARN :    %s (%d): not yet implemented\n",__FILE__,__LINE__); fflush(stderr); \
    CAM_IFACE_ERROR_FORMAT("not yet implemented");        \
    return;
# define ARAVIS_ERROR(_code, _msg)                        \
    cam_iface_error = _code;                              \
    CAM_IFACE_ERROR_FORMAT(_msg);
#endif

#define NOT_IMPLEMENTED_WARN                            \
  fprintf(stderr,"WARN :    %s (%d): not yet implemented\n",__FILE__,__LINE__); fflush(stderr);

#include "cam_iface_aravis.h"

const char *BACKEND_METHOD(cam_iface_get_driver_name)() {
  return "aravis";
}

void BACKEND_METHOD(cam_iface_clear_error)() {
  BACKEND_GLOBAL(cam_iface_error) = 0;
}

int BACKEND_METHOD(cam_iface_have_error)() {
  return BACKEND_GLOBAL(cam_iface_error);
}

const char * BACKEND_METHOD(cam_iface_get_error_string)() {
  return BACKEND_GLOBAL(cam_iface_error_string);
}

const char* BACKEND_METHOD(cam_iface_get_api_version)() {
  return CAM_IFACE_API_VERSION;
}

static gpointer aravis_thread_func(gpointer data) {
  GMainContext *context = data;

  DPRINTF("startup thread\n");

  g_main_context_push_thread_default (context);
  aravis_mainloop = g_main_loop_new (context, FALSE);
  g_main_loop_run (aravis_mainloop);

  DPRINTF("stop thread\n");

  return 0;
}


void BACKEND_METHOD(cam_iface_startup)() {
  unsigned int i;
  const char *delay_env;
  float delay_sec;

  DPRINTF("startup\n");

#if !GLIB_CHECK_VERSION (2, 31, 0)
    g_thread_init (NULL);
#endif

  g_type_init ();

  /* this creates an association between list index and device IDs. This association
  will not change until the next call to this function, so I consider the list
  index to be canonical */
  arv_update_device_list ();

  /* default to a 1second delay after startup - this can be overwritten by
  changing the value of LIBCAMIFACE_ARAVIS_STARTUP_DELAY */
  delay_env = g_getenv("LIBCAMIFACE_ARAVIS_STARTUP_DELAY");
  if (delay_env)
    delay_sec = g_ascii_strtod (delay_env, NULL);
  else
    delay_sec = 1.0;

  delay_sec = CLAMP(delay_sec,0.0,5.0);

  DPRINTF("startup delay %.1fs\n", delay_sec);
  g_usleep (delay_sec * G_USEC_PER_SEC);
  
  aravis_num_cameras = arv_get_n_devices ();
  aravis_cameras = calloc(aravis_num_cameras, sizeof(ArvGlobalCamera));

  if (aravis_cameras == NULL) {
    BACKEND_GLOBAL(cam_iface_error) = -1;
    CAM_IFACE_ERROR_FORMAT("error allocating memory");
    return;
  }

  for (i = 0; i < aravis_num_cameras; i++) {
    aravis_cameras[i].device_name = g_strdup( arv_get_device_id (i) );
    aravis_cameras[i].aravis_formats = NULL;
    aravis_cameras[i].num_modes = -1;
  }

  /* start the threading and mainloop */
  aravis_context = g_main_context_new ();
  aravis_thread = g_thread_new("aravis", aravis_thread_func, aravis_context);

}

void BACKEND_METHOD(cam_iface_shutdown)() {
  if (aravis_mainloop)
    g_main_loop_quit (aravis_mainloop);
  arv_shutdown ();
}

int BACKEND_METHOD(cam_iface_get_num_cameras)() {
#if ARAVIS_INCLUDE_FAKE_CAMERA
  return aravis_num_cameras;
#else
  return aravis_num_cameras - 1;
#endif
}

void BACKEND_METHOD(cam_iface_get_camera_info)(int device_number, Camwire_id *out_camid) {
  gchar **tokens;
  const gchar *id;
  int device_index = GET_ARAVIS_DEVICE_INDEX(device_number);

  DPRINTF("get_info %d\n",device_number);

  if (out_camid==NULL) {
    BACKEND_GLOBAL(cam_iface_error) = -1;
    CAM_IFACE_ERROR_FORMAT("return structure NULL");
    return;
  }

  id = arv_get_device_id(device_index);

  /* In theory we could create a camera here, query the full information, and then unref it.
  However, in practice this is really slow - as shown getting the camera mode strings, etc. So
  just get what we can quickly - the model (GUID) is the most important field anyway */
  tokens = g_strsplit (id, "-", 2);

  snprintf(out_camid->vendor, CAMWIRE_ID_MAX_CHARS, "%s", tokens[0]);
  snprintf(out_camid->chip, CAMWIRE_ID_MAX_CHARS, "%s", id);
  snprintf(out_camid->model, CAMWIRE_ID_MAX_CHARS, "%s", "");

  g_strfreev(tokens);

}

void BACKEND_METHOD(cam_iface_get_num_modes)(int device_number, int *num_modes) {
  ArvCamera *camera;
  int *cached_modes;
  int device_index = GET_ARAVIS_DEVICE_INDEX(device_number);

  cached_modes = &(aravis_cameras[device_index].num_modes);
  if (*cached_modes == -1) {

    DPRINTF("get_modes %d\n",device_number);

    camera = arv_camera_new ( aravis_cameras[device_index].device_name );

    if (ARV_IS_CAMERA(camera)) {
      guint n_pixel_formats;  
      gint64 *aravis_formats = arv_camera_get_available_pixel_formats (camera, &n_pixel_formats);
      *cached_modes = n_pixel_formats;
      g_object_unref (camera);
    } else {
      ARAVIS_ERROR(CAM_IFACE_GENERIC_ERROR, "error listing modes");
      *cached_modes = 0;
    }
  }

  *num_modes = *cached_modes;
}

#define FORMAT_TO_FORMAT7(_c,_m,_s,_q,_d) case _c:\
  *ret = "DC1394_VIDEO_MODE_FORMAT7_" _m " " _s;  \
  *coding = _q;                                   \
  *depth = _d;                                    \
  break;
#define FORMAT_IGNORE(_c) case _c:                \
  *ret = #_c;                                     \
  *coding = CAM_IFACE_UNKNOWN;                    \
  *depth = -1;                                    \
  break;

static void aravis_format_to_camiface(ArvPixelFormat format,
                                       const char **ret,
                                       CameraPixelCoding *coding,
                                       int *depth) {

  /* AIUI we can always set binning, ROI, etc. This makes us FORMAT7_0 */

  switch (format) {
    FORMAT_TO_FORMAT7(ARV_PIXEL_FORMAT_MONO_8, "0", "MONO8", CAM_IFACE_MONO8, 8);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_MONO_8_SIGNED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_MONO_10);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_MONO_10_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_MONO_12);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_MONO_12_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_MONO_14);
    FORMAT_TO_FORMAT7(ARV_PIXEL_FORMAT_MONO_16, "0", "MONO16", CAM_IFACE_MONO16, 16);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BAYER_GR_8);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BAYER_RG_8);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BAYER_GB_8);
    FORMAT_TO_FORMAT7(ARV_PIXEL_FORMAT_BAYER_BG_8, "2", "MONO8", CAM_IFACE_MONO8_BAYER_BGGR, 8);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BAYER_GR_10);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BAYER_RG_10);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BAYER_GB_10);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BAYER_BG_10);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BAYER_GR_12);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BAYER_RG_12);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BAYER_GB_12);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BAYER_BG_12);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BAYER_BG_12_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_RGB_8_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BGR_8_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_RGBA_8_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BGRA_8_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_RGB_10_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BGR_10_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_RGB_12_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_BGR_12_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_YUV_411_PACKED);
    FORMAT_TO_FORMAT7(ARV_PIXEL_FORMAT_YUV_422_PACKED, "2", "MONO8", CAM_IFACE_YUV422, 8);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_YUV_444_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_RGB_8_PLANAR);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_RGB_10_PLANAR);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_RGB_12_PLANAR);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_RGB_16_PLANAR);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_12_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_12_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_12_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_12_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_CUSTOM_YUV_422_YUYV_PACKED);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_16);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_16);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_16);
    FORMAT_IGNORE(ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_16);
    default:
      *ret = "unknown color coding";
      break;
  }
}

void BACKEND_METHOD(cam_iface_get_mode_string)(int device_number,
                               int mode_number,
                               char* mode_string,
                               int mode_string_maxlen) {

  const char *format7_mode_string;
  CameraPixelCoding coding;
  int depth;
  gint64 *aravis_formats;
  ArvGlobalCamera *cache;
  const char *framerate_string = "(user selectable framerate)";
  int device_index = GET_ARAVIS_DEVICE_INDEX(device_number);

  DPRINTF("get mode string %d\n", device_number);

  cache = &(aravis_cameras[device_index]);
  if (!cache->aravis_formats) {
    ArvCamera *camera;
    gint minw,minh;
    guint n_pixel_formats;

    DPRINTF("create mode strings\n");

    camera = arv_camera_new ( aravis_cameras[device_index].device_name );

    if (!ARV_IS_CAMERA(camera)) {
      ARAVIS_ERROR(CAM_IFACE_GENERIC_ERROR, "error getting mode string");
      *mode_string = '\0';
      return;
    }

    arv_camera_get_width_bounds (camera, &minw, &(cache->maxw));
    arv_camera_get_height_bounds (camera, &minh, &(cache->maxh));
    cache->aravis_formats = arv_camera_get_available_pixel_formats (camera, &n_pixel_formats);

    g_object_unref(camera);
  }

  aravis_format_to_camiface (
           cache->aravis_formats[mode_number],
           &format7_mode_string, &coding, &depth);
  snprintf(mode_string,mode_string_maxlen,
           "%d x %d %s %s",
           cache->maxw, cache->maxh, format7_mode_string, framerate_string);

}

cam_iface_constructor_func_t BACKEND_METHOD(cam_iface_get_constructor_func)(int device_number) {
  return (CamContext* (*)(int, int, int))CCaravis_construct;
}

CCaravis* CCaravis_construct( int device_number, int NumImageBuffers,
                              int mode_number) {
  CCaravis* this=NULL;

  this = malloc(sizeof(CCaravis));
  if (this==NULL) {
    BACKEND_GLOBAL(cam_iface_error) = -1;
    CAM_IFACE_ERROR_FORMAT("error allocating memory");
  } else {
    CCaravis_CCaravis( this,
                       device_number, NumImageBuffers,
                       mode_number);
    if (BACKEND_GLOBAL(cam_iface_error)) {
      free(this);
      return NULL;
    }
  }
  return this;
}

void delete_CCaravis( CCaravis *this ) {
  CCaravis_close(this);
  this->inherited.vmt = NULL;

  if (this->trigger_modes) {
    int i;
    for (i=0; i<this->num_trigger_modes; i++)
      g_free(this->trigger_modes[i]);
    free(this->trigger_modes);
  }
  free(this);

  this = NULL;
}

void CCaravis_CCaravis( CCaravis *this,
                        int device_number, int NumImageBuffers,
                        int mode_number) {
  ArvDevice *device;
  ArvGcNode *node;
  CameraPixelCoding coding;
  gint minw,minh;
  int depth;
  const char *format7_mode_string;
  gint64 *aravis_formats;
  guint n_pixel_formats;
  const char *id;
  int device_index = GET_ARAVIS_DEVICE_INDEX(device_number);

  /* call parent */
  CamContext_CamContext((CamContext*)this,device_number,NumImageBuffers,mode_number);
  this->inherited.vmt = (CamContext_functable*)&CCaravis_vmt;

  /* initialize */
  this->inherited.cam = (void *)NULL;
  this->inherited.backend_extras = (void *)NULL;
  if (!this) {
    BACKEND_GLOBAL(cam_iface_error) = -1;
    CAM_IFACE_ERROR_FORMAT("malloc failed");
    return;
  }

  this->inherited.device_number = device_number;

  this->cam_iface_mode_number = mode_number;
	this->last_frame_id = 0;
	this->last_timestamp_ns = 0;
  this->num_buffers = NumImageBuffers;
  this->started = 0;

  id = aravis_cameras[device_index].device_name;
  this->camera = arv_camera_new (id);
  aravis_formats = arv_camera_get_available_pixel_formats (this->camera, &n_pixel_formats);

  DPRINTF("construct number: %d index: %d id: %s uid: %s mode: %d (gst mode: %s) nbuffers: %d\n",
          device_number, device_index, id,
          arv_camera_get_device_id(this->camera),
          mode_number,
          arv_pixel_format_to_gst_caps_string(
            aravis_formats[mode_number]),
          NumImageBuffers);

  arv_camera_set_binning (this->camera, -1, -1);
  arv_camera_set_pixel_format (this->camera, aravis_formats[mode_number]);

  /* puts the camera into continuous acquision mode, in case the last user selected a weird
  trigger mode */
  arv_camera_set_frame_rate (this->camera, 10);


  /* Fill out camera specific data. If this was non-const then I would cache
  it globally, but it isn't, so I store it here */
  device = arv_camera_get_device (this->camera);
  node = arv_device_get_feature (device, "TriggerSource"); 

  if (node && ARV_IS_GC_ENUMERATION (node)) {
    const GSList *childs;
    const GSList *iter;
    int i;

    childs = arv_gc_enumeration_get_entries (ARV_GC_ENUMERATION (node));

    this->num_trigger_modes =  g_slist_length ((GSList *)childs);
    this->trigger_modes = calloc(this->num_trigger_modes, sizeof(const char *));

    for (iter = childs, i = 0; iter != NULL; iter = iter->next, i++) {
      this->trigger_modes[i] = g_strdup( arv_gc_feature_node_get_name ARV_GC_FEATURE_NODE ((iter->data)) );
    }
  } else {
    ARAVIS_ERROR(CAM_IFACE_GENERIC_ERROR, "error getting trigger modes");
    this->num_trigger_modes = 0;
    this->trigger_modes = NULL;
  }

  aravis_format_to_camiface (
    aravis_formats[mode_number],
    &format7_mode_string,
    &coding,
    &depth);

  this->inherited.depth = depth;
  this->inherited.coding= coding;

  /* set roi to max width etc */
  this->roi_left = 0;
  this->roi_top = 0;
  arv_camera_get_width_bounds (this->camera, &minw, &(this->roi_width));
  arv_camera_get_height_bounds (this->camera, &minh, &(this->roi_height));
  DPRINTF("construct set roi to 0,0 x %d,%d\n", this->roi_width, this->roi_height);
  arv_camera_set_region (this->camera, this->roi_left, this->roi_top, this->roi_width, this->roi_height);

}

void CCaravis_close(CCaravis *this) {
  arv_camera_stop_acquisition (this->camera);
}



void CCaravis_start_camera( CCaravis *this ) {
  int i;
  unsigned int payload;

  this->stream = arv_camera_create_stream (this->camera, NULL, NULL);
  if (!ARV_IS_STREAM(this->stream)) {
    ARAVIS_ERROR(CAM_IFACE_CAMERA_NOT_AVAILABLE_ERROR, "error connecting to camera");
  }

  payload = arv_camera_get_payload(this->camera);
  for (i = 0; i < this->num_buffers; i++)
    arv_stream_push_buffer (this->stream, arv_buffer_new (payload, NULL));

  arv_camera_get_region (this->camera,
                         &(this->roi_left), &(this->roi_top),
                         &(this->roi_width), &(this->roi_height));

  arv_camera_set_acquisition_mode (this->camera, ARV_ACQUISITION_MODE_CONTINUOUS);
  arv_camera_start_acquisition (this->camera);
  this->started = 1;

}

void CCaravis_stop_camera( CCaravis *this ) {
  arv_camera_stop_acquisition (this->camera);
  this->started = 0;
}

typedef enum {
  ARAVIS_PROPERTY_SHUTTER = 0,
  ARAVIS_PROPERTY_GAIN,
  NUM_ARAVIS_PROPERTIES
} AravisProperties_t;

void CCaravis_get_num_camera_properties(CCaravis *this,
                                        int* num_properties) {
  *num_properties = NUM_ARAVIS_PROPERTIES;
}

void CCaravis_get_camera_property_info(CCaravis *this,
                                       int property_number,
                                       CameraPropertyInfo *info) {

  gint imin, imax;
  double dmin, dmax;

  /* nice cameras do no bother with dirty scaled values */  
  info->is_scaled_quantity = 0;

  /* nice cameras have auto and manual mode */
  info->has_auto_mode = 1;
  info->has_manual_mode = 1;

  /* nice cameras can read their properties */
  info->readout_capable = 1;

  /* backen implementations of absolute functions is not consistent. Flydra camnodes do
  not use this API AFAICT */
  info->absolute_capable = 0;
  info->absolute_control_mode = 0;
  info->absolute_min_value = 0.0;
  info->absolute_max_value = 0.0;

  info->available = 1;
  info->is_present = 1;

  switch (property_number) {
    case ARAVIS_PROPERTY_SHUTTER:
      info->name = "shutter";
      arv_camera_get_exposure_time_bounds (this->camera, &dmin, &dmax);
      info->min_value = dmin;
      info->max_value = dmax;
      info->is_scaled_quantity = 1;
      info->scaled_unit_name = "msec";
      info->scale_offset = 0;
      info->scale_gain = 1e-3;
      break;
    case ARAVIS_PROPERTY_GAIN:
      info->name = "gain";
      arv_camera_get_gain_bounds (this->camera, &imin, &imax);
      info->min_value = imin;
      info->max_value = imax;
      break;
    default:
      info->available = 0;
      info->is_present = 0;
      info->name = "";
      info->min_value = info->max_value = 0;
      ARAVIS_ERROR(CAM_IFACE_HARDWARE_FEATURE_NOT_AVAILABLE, "unknown property");
      break;
  }
}

void CCaravis_get_camera_property(CCaravis *this,
                                  int property_number,
                                  long* Value,
                                  int* Auto ) {

  switch (property_number) {
    case ARAVIS_PROPERTY_SHUTTER:
      *Value = arv_camera_get_exposure_time (this->camera);
      *Auto = arv_camera_get_exposure_time_auto (this->camera) == ARV_AUTO_CONTINUOUS;
      break;
    case ARAVIS_PROPERTY_GAIN:
      *Value = arv_camera_get_gain (this->camera);
      *Auto = arv_camera_get_gain_auto (this->camera) == ARV_AUTO_CONTINUOUS;
      break;
    default:
      *Value = 0;
      *Auto = 0;
      ARAVIS_ERROR(CAM_IFACE_HARDWARE_FEATURE_NOT_AVAILABLE, "unknown property");
      break;
  }
}

void CCaravis_set_camera_property(CCaravis *this,
                                  int property_number,
                                  long Value,
                                  int Auto ) {
  ArvAuto aravis_auto;

  DPRINTF("set property %d to %ld (auto: %d)\n",property_number, Value, Auto);

  aravis_auto = (Auto ? ARV_AUTO_CONTINUOUS : ARV_AUTO_OFF);
  switch (property_number) {
    case ARAVIS_PROPERTY_SHUTTER:
        arv_camera_set_exposure_time_auto (this->camera, aravis_auto);
        arv_camera_set_exposure_time (this->camera, (double)Value);
      break;
    case ARAVIS_PROPERTY_GAIN:
        arv_camera_set_gain_auto (this->camera, aravis_auto);
        arv_camera_set_gain (this->camera, (int)Value);
      break;
    default:
      ARAVIS_ERROR(CAM_IFACE_HARDWARE_FEATURE_NOT_AVAILABLE, "unknown property");
      break;
  }

}

void CCaravis_grab_next_frame_blocking_with_stride( CCaravis *this,
                                                    unsigned char *out_bytes,
                                                    intptr_t stride0, float timeout) {
  ArvBuffer *buffer;
  int ok = 0;
  unsigned int stride = (this->roi_width * this->inherited.depth + 7) / 8;

  while (!ok) {
#if ARAVIS_DEBUG_FRAME_ACQUSITION_BLOCKING
    gint ib, ob;
    arv_stream_get_n_buffers (this->stream, &ib, &ob);
    printf("[G:%d/%d]\n",ib,ob); fflush(stdout);
#endif

    if (timeout <= 0) {
      buffer = arv_stream_pop_buffer(this->stream);
      if (!buffer) {
        timeout = 0.5;
        DPRINTF("failed blocking acquire, timeout next time");
      }
    } else {
      buffer = arv_stream_timed_pop_buffer(this->stream, timeout * G_USEC_PER_SEC);
    }
      

    if (buffer) {
      if (buffer->status == ARV_BUFFER_STATUS_SUCCESS) {
        int wb = buffer->width * this->inherited.depth / 8;

        if (wb>stride0) {
          *out_bytes = '\0';
          ARAVIS_ERROR(CAM_IFACE_GENERIC_ERROR, "the buffer provided is not large enough");
          return;
        }

#if ARAVIS_DEBUG_FRAME_ACQUSITION_STRIDE
        printf ("[S:%d/%d(w%d)]\n", stride, (int)stride0, this->roi_width); fflush(stdout);
#endif

        if (stride0 == stride) {
          /* same stride */
          memcpy((void*)out_bytes /*dest*/, buffer->data, buffer->size);
        } else {
          int row;

          /* different strides */
          for (row=0; row < buffer->height; row++) {
            memcpy((void*)(out_bytes + row * stride0), /*dest*/
              (const void*)((char*)buffer->data + row * stride),/*src*/
              stride);/*size*/
          }
        }

        this->last_frame_id = buffer->frame_id;
        this->last_timestamp_ns = buffer->timestamp_ns;

        ok = 1;
      }
      arv_stream_push_buffer (this->stream, buffer);
    }
  }

  if (!ok) {
    *out_bytes = '\0';
    ARAVIS_ERROR(CAM_IFACE_FRAME_DATA_MISSING_ERROR, "no frame ready");
  }
}

void CCaravis_grab_next_frame_blocking( CCaravis *this, unsigned char *out_bytes, float timeout) {
  CCaravis_grab_next_frame_blocking_with_stride (
    this,
    out_bytes,
    this->roi_width * this->inherited.depth / 8,
    timeout);
}

void CCaravis_point_next_frame_blocking( CCaravis *this, unsigned char **buf_ptr, float timeout) {
  NOT_IMPLEMENTED;
}

void CCaravis_unpoint_frame( CCaravis *this){
  NOT_IMPLEMENTED;
}

void CCaravis_get_last_timestamp( CCaravis *this, double* timestamp ) {
  /* from nanoseconds to seconds */
  *timestamp = (double)(this->last_timestamp_ns) * 1e-9;
}

void CCaravis_get_last_framenumber( CCaravis *this, unsigned long* framenumber ){
  *framenumber = this->last_frame_id;
}

void CCaravis_get_num_trigger_modes( CCaravis *this,
                                     int *num_trigger_modes ) {
  *num_trigger_modes = this->num_trigger_modes;
}

void CCaravis_get_trigger_mode_string( CCaravis *this,
                                       int trigger_mode_number,
                                       char* trigger_mode_string, //output parameter
                                       int trigger_mode_string_maxlen) {
  snprintf(trigger_mode_string,trigger_mode_string_maxlen,"%s",this->trigger_modes[trigger_mode_number]);
}

void CCaravis_get_trigger_mode_number( CCaravis *this,
                                       int *trigger_mode_number ) {
  int i;
  const char *trigger_source;

  trigger_source = arv_camera_get_trigger_source (this->camera);
  if (!trigger_source) {
    ARAVIS_ERROR(CAM_IFACE_HARDWARE_FEATURE_NOT_AVAILABLE, "could not read trigger source");
    return;
  }

  for (i=0; i<this->num_trigger_modes; i++) {
    if (strcmp(trigger_source, this->trigger_modes[i]) == 0) {
      *trigger_mode_number = i;
      return;
    }
  }

  ARAVIS_ERROR(CAM_IFACE_HARDWARE_FEATURE_NOT_AVAILABLE, "unknown trigger source");
}

void CCaravis_set_trigger_mode_number( CCaravis *this,
                                       int trigger_mode_number ) {
  char *trigger_mode_name;
  if (trigger_mode_number >= this->num_trigger_modes) {
    ARAVIS_ERROR(CAM_IFACE_HARDWARE_FEATURE_NOT_AVAILABLE, "unknown trigger mode"); 
    return;
  }

  trigger_mode_name = this->trigger_modes[trigger_mode_number];

  DPRINTF("set trigger mode: %d (%s)\n", trigger_mode_number, trigger_mode_name);

  arv_camera_set_trigger (this->camera, trigger_mode_name);
}

void CCaravis_get_frame_roi( CCaravis *this,
                             int *left, int *top, int* width, int* height ) {
  *left = this->roi_left;
  *top = this->roi_top;
  *width = this->roi_width;
  *height = this->roi_height;
}

void CCaravis_set_frame_roi( CCaravis *this,
                             int left, int top, int width, int height ) {

  DPRINTF("set roi: %d,%d %dx%d\n", left, top, width, height);

  if (this->started) {
    DWARNF("Do I need to restart the camera when changing ROI\n");
  }

  this->roi_left = left;
  this->roi_top = top;
  this->roi_width = width;
  this->roi_height = height;
  arv_camera_set_region (this->camera, left, top, width, height);
}

void CCaravis_get_framerate( CCaravis *this,
                             float *framerate ) {
  *framerate = arv_camera_get_frame_rate (this->camera);
}

void CCaravis_set_framerate( CCaravis *this,
                             float framerate ) {
  DPRINTF("set framerate: %f\n", framerate);
  /* the aravis viewer widge adjusts the framerate at runtime without restarting the camera */
  arv_camera_set_frame_rate (this->camera, framerate);
}

void CCaravis_get_max_frame_size( CCaravis *this,
                                  int *width, int *height ){
  gint minw,minh;
  arv_camera_get_width_bounds (this->camera, &minw, width);
  arv_camera_get_height_bounds (this->camera, &minh, height);
}

void CCaravis_get_buffer_size( CCaravis *this,
                               int *size) {
  *size = arv_camera_get_payload(this->camera);
}

void CCaravis_get_num_framebuffers( CCaravis *this,
                                    int *num_framebuffers ) {
  *num_framebuffers = this->num_buffers;
}

void CCaravis_set_num_framebuffers( CCaravis *this,
                                    int num_framebuffers ) {
  NOT_IMPLEMENTED;
}
