/*

Copyright (c) 2004-2010, California Institute of Technology. All
rights reserved.

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

/* CAVEATS:
   *  I don't see any way to set the framerate or resolution in Pylon;
      so there will probably only be a very limited set of modes
      (corresponding to various pixel formats at the sensor's resolution)
   *  point_next_frame_blocking and unpoint_frame methods are not implemented.
   *  some of the Basler-Pylon camera properties
      are floating-point numbers.  As you presumably know, the
      libcamiface properties are always integers (modulo
      scale_offset/scale_gain).  There are countless ways to
      hack around this, but I am going to the easiest thing
      at just round floats to integer on "get_property" calls,
      and "set_property" will be limited to setting properties
      to integral values.  (Alternatively I could "normalize"
      floating-point values to a fixed range say 0..100, and
      then use the min/max values to interpolate) (This may be
      totally hypothetical too -- it's quite possible that all
      the camera props of interest are integer-valued anyway)
   *  Getting the "automatic" aspect of properties right is also
      potentially tricky; currently I plan to just say that no
      properies are auto-capable.  Time will tell if that is adequate.
 */

/* Backend for libbasler_pylon version XXX */
#include "cam_iface.h"
#include <pylon/PylonIncludes.h>

#if 1
#define DPRINTF(...)
#define DEBUG_ONLY(x)
#else
#define DPRINTF(...) printf(__VA_ARGS__)
#define DEBUG_ONLY(x) x
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/select.h>
#include <errno.h>

#define INVALID_FILENO 0
#define DELAY 50000

struct CCbasler_pylon; // forward declaration

// keep functable in sync across backends
typedef struct {
  cam_iface_constructor_func_t construct;
  void (*destruct)(struct CamContext*);

  void (*CCbasler_pylon)(struct CCbasler_pylon*,int,int,int,const char*);
  void (*close)(struct CCbasler_pylon*);
  void (*start_camera)(struct CCbasler_pylon*);
  void (*stop_camera)(struct CCbasler_pylon*);
  void (*get_num_camera_properties)(struct CCbasler_pylon*,int*);
  void (*get_camera_property_info)(struct CCbasler_pylon*,
                                   int,
                                   CameraPropertyInfo*);
  void (*get_camera_property)(struct CCbasler_pylon*,int,long*,int*);
  void (*set_camera_property)(struct CCbasler_pylon*,int,long,int);
  void (*grab_next_frame_blocking)(struct CCbasler_pylon*,
                                   unsigned char*,
                                   float);
  void (*grab_next_frame_blocking_with_stride)(struct CCbasler_pylon*,
                                               unsigned char*,
                                               intptr_t,
                                               float);
  void (*point_next_frame_blocking)(struct CCbasler_pylon*,unsigned char**,float);
  void (*unpoint_frame)(struct CCbasler_pylon*);
  void (*get_last_timestamp)(struct CCbasler_pylon*,double*);
  void (*get_last_framenumber)(struct CCbasler_pylon*,unsigned long*);
  void (*get_num_trigger_modes)(struct CCbasler_pylon*,int*);
  void (*get_trigger_mode_string)(struct CCbasler_pylon*,int,char*,int);
  void (*get_trigger_mode_number)(struct CCbasler_pylon*,int*);
  void (*set_trigger_mode_number)(struct CCbasler_pylon*,int);
  void (*get_frame_roi)(struct CCbasler_pylon*,int*,int*,int*,int*);
  void (*set_frame_roi)(struct CCbasler_pylon*,int,int,int,int);
  void (*get_max_frame_size)(struct CCbasler_pylon*,int*,int*);
  void (*get_buffer_size)(struct CCbasler_pylon*,int*);
  void (*get_framerate)(struct CCbasler_pylon*,float*);
  void (*set_framerate)(struct CCbasler_pylon*,float);
  void (*get_num_framebuffers)(struct CCbasler_pylon*,int*);
  void (*set_num_framebuffers)(struct CCbasler_pylon*,int);
} CCbasler_pylon_functable;

typedef struct CCbasler_pylon {
  CamContext inherited;
  Pylon::IPylonDevice *device;

  // These are true if the camera is collecting images
  Pylon::IStreamGrabber *grabber;
  Pylon::StreamBufferHandle *buffer_handles;
  void *buffers;

  unsigned sensor_width, sensor_height;
  unsigned roi_left;
  unsigned roi_top;
  unsigned roi_width;
  unsigned roi_height;
  unsigned buffer_size;     // bytes per frame

  unsigned num_image_buffers;

  unsigned trigger_mode;		// 0 for continue acquistion; nonzero for 1+trigger_source value

  double last_timestamp;
  unsigned last_frameno;
  bool grabber_open;
} CCbasler_pylon;

// forward declarations
CCbasler_pylon* CCbasler_pylon_construct( int device_number, int NumImageBuffers,
                              int mode_number, const char *interface);
void delete_CCbasler_pylon(struct CCbasler_pylon*);

void CCbasler_pylon_CCbasler_pylon(struct CCbasler_pylon*,int,int,int,const char *);
void CCbasler_pylon_close(struct CCbasler_pylon*);
void CCbasler_pylon_start_camera(struct CCbasler_pylon*);
void CCbasler_pylon_stop_camera(struct CCbasler_pylon*);
void CCbasler_pylon_get_num_camera_properties(struct CCbasler_pylon*,int*);
void CCbasler_pylon_get_camera_property_info(struct CCbasler_pylon*,
                              int,
                              CameraPropertyInfo*);
void CCbasler_pylon_get_camera_property(struct CCbasler_pylon*,int,long*,int*);
void CCbasler_pylon_set_camera_property(struct CCbasler_pylon*,int,long,int);
void CCbasler_pylon_grab_next_frame_blocking(struct CCbasler_pylon*,
                              unsigned char*,
                              float);
void CCbasler_pylon_grab_next_frame_blocking_with_stride(struct CCbasler_pylon*,
                                          unsigned char*,
                                          intptr_t,
                                          float);
void CCbasler_pylon_point_next_frame_blocking(struct CCbasler_pylon*,unsigned char**,float);
void CCbasler_pylon_unpoint_frame(struct CCbasler_pylon*);
void CCbasler_pylon_get_last_timestamp(struct CCbasler_pylon*,double*);
void CCbasler_pylon_get_last_framenumber(struct CCbasler_pylon*,unsigned long*);
void CCbasler_pylon_get_num_trigger_modes(struct CCbasler_pylon*,int*);
void CCbasler_pylon_get_trigger_mode_string(struct CCbasler_pylon*,int,char*,int);
void CCbasler_pylon_get_trigger_mode_number(struct CCbasler_pylon*,int*);
void CCbasler_pylon_set_trigger_mode_number(struct CCbasler_pylon*,int);
void CCbasler_pylon_get_frame_roi(struct CCbasler_pylon*,int*,int*,int*,int*);
void CCbasler_pylon_set_frame_roi(struct CCbasler_pylon*,int,int,int,int);
void CCbasler_pylon_get_max_frame_size(struct CCbasler_pylon*,int*,int*);
void CCbasler_pylon_get_buffer_size(struct CCbasler_pylon*,int*);
void CCbasler_pylon_get_framerate(struct CCbasler_pylon*,float*);
void CCbasler_pylon_set_framerate(struct CCbasler_pylon*,float);
void CCbasler_pylon_get_num_framebuffers(struct CCbasler_pylon*,int*);
void CCbasler_pylon_set_num_framebuffers(struct CCbasler_pylon*,int);

CCbasler_pylon_functable CCbasler_pylon_vmt = {
  (cam_iface_constructor_func_t)CCbasler_pylon_construct,
  (void (*)(CamContext*))delete_CCbasler_pylon,
  CCbasler_pylon_CCbasler_pylon,
  CCbasler_pylon_close,
  CCbasler_pylon_start_camera,
  CCbasler_pylon_stop_camera,
  CCbasler_pylon_get_num_camera_properties,
  CCbasler_pylon_get_camera_property_info,
  CCbasler_pylon_get_camera_property,
  CCbasler_pylon_set_camera_property,
  CCbasler_pylon_grab_next_frame_blocking,
  CCbasler_pylon_grab_next_frame_blocking_with_stride,
  CCbasler_pylon_point_next_frame_blocking,
  CCbasler_pylon_unpoint_frame,
  CCbasler_pylon_get_last_timestamp,
  CCbasler_pylon_get_last_framenumber,
  CCbasler_pylon_get_num_trigger_modes,
  CCbasler_pylon_get_trigger_mode_string,
  CCbasler_pylon_get_trigger_mode_number,
  CCbasler_pylon_set_trigger_mode_number,
  CCbasler_pylon_get_frame_roi,
  CCbasler_pylon_set_frame_roi,
  CCbasler_pylon_get_max_frame_size,
  CCbasler_pylon_get_buffer_size,
  CCbasler_pylon_get_framerate,
  CCbasler_pylon_set_framerate,
  CCbasler_pylon_get_num_framebuffers,
  CCbasler_pylon_set_num_framebuffers
};

// See the following for a hint on how to make thread thread-local without __thread.
// http://lists.apple.com/archives/Xcode-users/2006/Jun/msg00551.html
#ifdef __APPLE__
#define myTLS
#else
#define myTLS __thread
#endif

#ifdef MEGA_BACKEND
  #define BACKEND_GLOBAL(m) basler_pylon_##m
#else
  #define BACKEND_GLOBAL(m) m
#endif

/* globals -- allocate space */
myTLS int BACKEND_GLOBAL(cam_iface_error) = 0;
#define CAM_IFACE_MAX_ERROR_LEN 255
myTLS char BACKEND_GLOBAL(cam_iface_error_string)[CAM_IFACE_MAX_ERROR_LEN]  = {0x00}; //...

static int basler_pylon_n_cameras = -1;
static Pylon::IPylonDevice **basler_pylon_cameras = 0;

#define CI_BASLER_NUM_ATTR 2
const char *BACKEND_GLOBAL(pv_attr_strings)[CI_BASLER_NUM_ATTR] = {
  "gain",
  "shutter" // exposure
};
#define CI_BASLER_ATTR_GAIN 0
#define CI_BASLER_ATTR_SHUTTER 1


#define CAM_IFACE_ERROR(message)                            \
  do {                                                      \
    BACKEND_GLOBAL(cam_iface_error) = -1;                   \
    snprintf(BACKEND_GLOBAL(cam_iface_error_string),        \
             CAM_IFACE_MAX_ERROR_LEN,                       \
             "%s (%d): %s\n",__FILE__,__LINE__,(message));  \
  }while(0)

/* Used in catch block for when an exception occurs */
#define CAM_IFACE_ERROR_EXCEPTION(message, exception)       \
  do {                                                      \
    BACKEND_GLOBAL(cam_iface_error) = -1;                   \
    snprintf(BACKEND_GLOBAL(cam_iface_error_string),        \
             CAM_IFACE_MAX_ERROR_LEN,                       \
             "%s (%d): %s: %s\n",                           \
             __FILE__,__LINE__,(message),(exception).what());\
    std::cerr<<BACKEND_GLOBAL(cam_iface_error_string)<<std::endl;\
  }while(0)
/* Used in catch block for when an exception occurs */
#define CAM_IFACE_ERROR_GENICAM_EXCEPTION(message, exception) \
  do {                                                      \
    BACKEND_GLOBAL(cam_iface_error) = -1;                   \
    snprintf(BACKEND_GLOBAL(cam_iface_error_string),        \
             CAM_IFACE_MAX_ERROR_LEN,                       \
             "%s (%d): %s: %s\n",                           \
             __FILE__,__LINE__,(message),(exception).GetDescription());\
    std::cerr<<BACKEND_GLOBAL(cam_iface_error_string)<<std::endl;\
  }while(0)


/* Assert that we have the correct type of camera on-hand. */

#ifdef MEGA_BACKEND
#define CHECK_CC(camera)						\
  if (!(camera)) {							\
    basler_pylon_cam_iface_error = -1;					\
    CAM_IFACE_ERROR("no CamContext specified (NULL argument)");		\
    return;                                                             \
  } else {								\
    if (!((camera)->inherited.vmt ==					\
	  (CamContext_functable *) (&CCbasler_pylon_vmt))) {		\
      basler_pylon_cam_iface_error = -1;				\
      CAM_IFACE_ERROR("no CamContext specified (NULL argument)");	\
      return;								\
    }									\
    if ((camera)->grabber == NULL) {					\
      basler_pylon_cam_iface_error = -1;				\
      CAM_IFACE_ERROR("no CamContext grabber (camera not started)");	\
      return;								\
    }									\
  }
#else
#define CHECK_CC(camera)                                                     \
  if (!(camera)) {							\
    cam_iface_error = -1;                                               \
    CAM_IFACE_ERROR("no CamContext specified (NULL argument)");		\
    return;                                                             \
  } else {								\
    if (!((camera)->inherited.vmt ==					\
	  (CamContext_functable *) (&CCbasler_pylon_vmt))) {		\
      cam_iface_error = -1;						\
      CAM_IFACE_ERROR("no CamContext specified (NULL argument)");	\
      return;								\
    }									\
    if ((camera)->grabber == NULL) {					\
      cam_iface_error = -1;						\
      CAM_IFACE_ERROR("no CamContext grabber (camera not started)");	\
      return;								\
    }									\
  }
#endif


#include "cam_iface_basler_pylon.h"

const char *BACKEND_METHOD(cam_iface_get_driver_name)() {
  return "basler_pylon";
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

void BACKEND_METHOD(cam_iface_startup)() {
  Pylon::PylonInitialize();
}

void BACKEND_METHOD(cam_iface_shutdown)() {
  DEBUG_ONLY(std::cerr << "PylonTerminate" << std::endl);
  Pylon::PylonTerminate();
}


int BACKEND_METHOD(cam_iface_get_num_cameras)() {
  if (basler_pylon_n_cameras != -1)
    return basler_pylon_n_cameras;
  try {
    Pylon::DeviceInfoList_t devices;
    Pylon::CTlFactory::GetInstance().EnumerateDevices(devices);
    basler_pylon_n_cameras = devices.size();
    basler_pylon_cameras = new Pylon::IPylonDevice* [basler_pylon_n_cameras];
    for (int i = 0; i < basler_pylon_n_cameras; i++)
      basler_pylon_cameras[i] = 0;
    return basler_pylon_n_cameras;
  } catch (GenICam::GenericException e) {
    CAM_IFACE_ERROR_EXCEPTION("GenICam::GenericException in get_num_cameras",e);
    return -1;
  } catch (std::exception e) {
    CAM_IFACE_ERROR_EXCEPTION("std::exception in get_num_cameras",e);
    return -1;
  } catch (...) {
    CAM_IFACE_ERROR("in get_num_cameras");
    return -1;
  }
}

void BACKEND_METHOD(cam_iface_get_camera_info)(int device_number, Camwire_id *out_camid) {
  try {
    Pylon::DeviceInfoList_t devices;
    Pylon::CTlFactory::GetInstance().EnumerateDevices(devices);
    Pylon::CDeviceInfo info = devices[device_number];
    snprintf(out_camid->vendor, CAMWIRE_ID_MAX_CHARS, "%s", info.GetVendorName().c_str());
    snprintf(out_camid->model, CAMWIRE_ID_MAX_CHARS, "%s", info.GetModelName().c_str());
    snprintf(out_camid->chip, CAMWIRE_ID_MAX_CHARS, "%s", info.GetDeviceClass().c_str());
  } catch (GenICam::GenericException e) {
    std::cerr<<"GenericException: " << e.GetDescription() << std::endl;
    CAM_IFACE_ERROR_EXCEPTION("GenICam::GenericException in get_camera_info",e);
    return;
  } catch (std::exception e) {
    CAM_IFACE_ERROR_EXCEPTION("std::exception in get_camera_info",e);
    return;
  } catch (...) {
    CAM_IFACE_ERROR("in get_camera_info");
    return;
  }
}

static const struct
{
  CameraPixelCoding coding;
  const char *pylon_name;
  unsigned depth;
} basler_pylon_pixel_coding_mapping[] =
{
#define ENTRY(camiface_enum, bp_shortname, depth) \
  { CAM_IFACE_ ## camiface_enum, #bp_shortname, depth }
  ENTRY (MONO8,            Mono8,        8),
  ENTRY (YUV411,           YUV411Packed, 6),
  ENTRY (YUV422,           YUV422Packed, 8),
  ENTRY (YUV444,           YUV444Packed, 12),
  ENTRY (RGB8,             RGB8Packed,   24),
  ENTRY (MONO16,           Mono16,       16),
  ENTRY (MONO16S,          Mono16Signed, 16),
  ENTRY (MONO8_BAYER_BGGR, BayerBG8,     8),
  ENTRY (MONO8_BAYER_RGGB, BayerRG8,     8),
  ENTRY (MONO8_BAYER_GRBG, BayerGR8,     8),
  ENTRY (MONO8_BAYER_GBRG, BayerGB8,     8),
#undef ENTRY
};
#define BASLER_PYLON_N_PIXEL_CODINGS                      \
((int)(   sizeof(basler_pylon_pixel_coding_mapping)       \
        / sizeof(basler_pylon_pixel_coding_mapping[0])    )  )
void BACKEND_METHOD(cam_iface_get_num_modes)(int device_number, int *num_modes) {
  *num_modes = BASLER_PYLON_N_PIXEL_CODINGS;
}

static Pylon::IPylonDevice *force_device (int device_number)
{
  if (device_number >= basler_pylon_n_cameras
   || basler_pylon_n_cameras == -1)
    return NULL;
  if (basler_pylon_cameras[device_number] == 0) {
    try {
      Pylon::DeviceInfoList_t devices;
      Pylon::CTlFactory::GetInstance().EnumerateDevices(devices);
      Pylon::CDeviceInfo info = devices[device_number];
      assert(device_number<devices.size());
      assert(device_number>=0);
      
      // the following call can emit SIGABRT and kill the program, despite our exception catching
      basler_pylon_cameras[device_number] = Pylon::CTlFactory::GetInstance().CreateDevice(devices[device_number]);

      assert(basler_pylon_cameras[device_number]);
      basler_pylon_cameras[device_number]->Open();
    } catch (GenICam::GenericException e) {
      CAM_IFACE_ERROR_GENICAM_EXCEPTION ("getting the camera", e);
      return NULL;
    } catch (std::exception e) {
      CAM_IFACE_ERROR_EXCEPTION ("std::exception getting the camera", e);
      return NULL;
    } catch (...) { 
      CAM_IFACE_ERROR("unknown exception getting the camera");
      return NULL;
    }
  }
  return basler_pylon_cameras[device_number];
}


void PrintNames(const GenApi::CNodePtr& node, unsigned indent)
{
  GenApi::CCategoryPtr ptrCategory ( node );
  // If node is a category, visit all children,
  // else print the name of the node
  if ( ptrCategory ) {
    GenApi::FeatureList_t features;
    ptrCategory->GetFeatures( features );
    GenApi::FeatureList_t::const_iterator it;
    for ( it = features.begin(); it != features.end(); ++it )
      PrintNames( *it, 1 + indent);
  } else {
    // If the feature is implemented, print name of node
    if ( GenApi::IsImplemented( node ) ) {
      for (unsigned i = 0; i < indent; i++) std::cout<<"  ";
      std::cout << node->GetName() << std::endl;
    }
  }
}


void BACKEND_METHOD(cam_iface_get_mode_string)(int device_number,
                               int mode_number,
                               char* mode_string,
                               int mode_string_maxlen) {
  Pylon::IPylonDevice *device = force_device (device_number);
  if (device == 0) {
    mode_string[0] = 0;
    // error is already set
    return;
  }
    
  if (mode_number >= BASLER_PYLON_N_PIXEL_CODINGS) {
    CAM_IFACE_ERROR("bad mode");
    mode_string[0] = 0;
    return;
  }
  //PrintNames(device->GetNodeMap()->GetNode("Root"), 0);
#if 0
  GenApi::NodeList_t list;
  for (GenApi::NodeList_t::iterator it = list.begin();
       it != list.end();
       ++it)
  {
    PrintNames(*it);
  }
#endif
  try
  {
    GenApi::CIntegerPtr width = device->GetNodeMap()->GetNode("Width");
    GenApi::CIntegerPtr height = device->GetNodeMap()->GetNode("Height");
    DEBUG_ONLY(std::cerr << "mode_string_maxlen=" << mode_string_maxlen << "; w/h=" << width->GetValue() << "/" << height->GetValue() << std::endl);
    snprintf (mode_string, mode_string_maxlen,
              "%u x %u: %s",
              (unsigned) width->GetValue(), (unsigned) height->GetValue(),
              basler_pylon_pixel_coding_mapping[mode_number].pylon_name);
  } catch (GenICam::GenericException e) {
    CAM_IFACE_ERROR_GENICAM_EXCEPTION ("getting the camera", e);
    mode_string[0] = 0;
    return;
  } catch (std::exception e) {
    CAM_IFACE_ERROR_EXCEPTION ("std::exception getting the camera", e);
    mode_string[0] = 0;
    return;
  } catch (...) { 
    CAM_IFACE_ERROR("unknown exception getting the camera");
    mode_string[0] = 0;
    return;
  }
}

cam_iface_constructor_func_t BACKEND_METHOD(cam_iface_get_constructor_func)(int device_number) {
  return (CamContext* (*)(int, int, int, const char*))CCbasler_pylon_construct;
}

CCbasler_pylon *
CCbasler_pylon_construct(int device_number,
                         int num_image_buffers,
                         int mode_number,
                         const char *interface) {
  CCbasler_pylon *rv;

  rv = (CCbasler_pylon *) malloc(sizeof(CCbasler_pylon));
  if (rv == 0) {
    BACKEND_GLOBAL(cam_iface_error) = -1;
    CAM_IFACE_ERROR("error allocating memory");
    return NULL;
  } else {
    CCbasler_pylon_CCbasler_pylon(rv,
                                  device_number,
                                  num_image_buffers,
                                  mode_number,
                                  interface);
    if (BACKEND_GLOBAL(cam_iface_error)) {
      free(rv);
      return NULL;
    }
  }
  return rv;
}

void delete_CCbasler_pylon(CCbasler_pylon *cam) {
  CCbasler_pylon_close(cam);
  cam->inherited.vmt = NULL;
  free(cam);
}
static void
camera_set_enum (CCbasler_pylon *cam,
                 const char     *name,
                 const char     *value)
{
  GenApi::CEnumerationPtr ptr = cam->device->GetNodeMap()->GetNode(name);
  ptr->FromString(value);
}
static void
camera_set_enum_value (CCbasler_pylon *cam,
                       const char     *name,
                       unsigned value)
{
  GenApi::CEnumerationPtr ptr = cam->device->GetNodeMap()->GetNode(name);
  ptr->SetIntValue(value);
}
static void
camera_set_int  (CCbasler_pylon *cam,
                 const char     *name,
                 int64_t         value)
{
  GenApi::CIntegerPtr ptr = cam->device->GetNodeMap()->GetNode(name);
  ptr->SetValue(value);
}

static int64_t
camera_get_int  (CCbasler_pylon *cam,
                 const char     *name)
{
  GenApi::CIntegerPtr ptr = cam->device->GetNodeMap()->GetNode(name);
  return ptr->GetValue();
}

static void
camera_set_float  (CCbasler_pylon *cam,
                   const char     *name,
                   double          value)
{
  GenApi::CFloatPtr ptr = cam->device->GetNodeMap()->GetNode(name);
  ptr->SetValue(value);
}

static void
camera_execute  (CCbasler_pylon *cam,
                 const char *name)
{
  GenApi::CCommandPtr ptr = cam->device->GetNodeMap()->GetNode(name);
  ptr->Execute();
}

static void
grabber_set_int  (Pylon::IStreamGrabber *grabber,
                  const char     *name,
                  int64_t         value)
{
  GenApi::CIntegerPtr ptr = grabber->GetNodeMap()->GetNode(name);
  return ptr->SetValue(value);
}

static void
camera_get_int_range  (CCbasler_pylon  *cam,
                       const char     *name,
                       int64_t        *min_out,
                       int64_t        *max_out)
{
  GenApi::CIntegerPtr ptr = cam->device->GetNodeMap()->GetNode(name);
  *min_out = ptr->GetMin();
  *max_out = ptr->GetMax();
}


void
CCbasler_pylon_CCbasler_pylon(CCbasler_pylon *cam,
                              int device_number,
                              int num_image_buffers,
                              int mode_number,
                              const char *interface) {
  int i;

  // call parent
  CamContext_CamContext((CamContext*)cam,device_number,num_image_buffers,mode_number,interface);
  cam->inherited.vmt = (CamContext_functable*)&CCbasler_pylon_vmt;

  if ((device_number < 0)|(device_number >= basler_pylon_n_cameras)) {
    BACKEND_GLOBAL(cam_iface_error) = -1;
    CAM_IFACE_ERROR("requested invalid camera number");
    return;
  }
  if (mode_number < 0
    || mode_number >= BASLER_PYLON_N_PIXEL_CODINGS) {
    BACKEND_GLOBAL(cam_iface_error) = -1;
    CAM_IFACE_ERROR("invalid mode_number");
    return;
  }

  /* initialize */
  cam->inherited.cam = (void *)NULL;
  cam->inherited.backend_extras = (void *)NULL;
  if (!cam) {
    BACKEND_GLOBAL(cam_iface_error) = -1;
    CAM_IFACE_ERROR("malloc failed");
    return;
  }

  // camera isn't running
  cam->grabber = 0;
  cam->buffers = 0;
  cam->buffer_handles = 0;

  cam->last_timestamp = 0;
  cam->last_frameno = 0;
  cam->grabber_open = false;
  cam->trigger_mode = 0;

  cam->inherited.device_number = device_number;
  cam->inherited.coding = basler_pylon_pixel_coding_mapping[mode_number].coding;
  cam->inherited.depth = basler_pylon_pixel_coding_mapping[mode_number].depth;

  Pylon::IPylonDevice *device = force_device (device_number);
  if (device == 0) {
    // error is already set
    return;
  }

  cam->device = device;
  try{
    GenApi::CIntegerPtr width = device->GetNodeMap()->GetNode("WidthMax");
    GenApi::CIntegerPtr height = device->GetNodeMap()->GetNode("HeightMax");
    cam->sensor_width = width->GetValue();
    cam->sensor_height = height->GetValue();

    // setup pixel format
    GenApi::CEnumerationPtr fmt = device->GetNodeMap()->GetNode("PixelFormat");
    fmt->FromString(basler_pylon_pixel_coding_mapping[mode_number].pylon_name);

    // Insist on getting timestamp with each frame.
    GenApi::CBooleanPtr chunk_mode_active = device->GetNodeMap()->GetNode("ChunkModeActive");
    chunk_mode_active->SetValue(true);
    GenApi::CEnumerationPtr cs = device->GetNodeMap()->GetNode("ChunkSelector");
    GenApi::CBooleanPtr ce = device->GetNodeMap()->GetNode("ChunkEnable");

    // awesome unthreadsafe design!
    cs->FromString("Timestamp");
    ce->SetValue(true );

  } catch (GenICam::GenericException e) {
    CAM_IFACE_ERROR_GENICAM_EXCEPTION("creating camera", e);
    return;
  } catch (std::exception e) {
    CAM_IFACE_ERROR_EXCEPTION("creating camera", e);
    return;
  }

  // reset ROI
  cam->roi_left = 0;
  cam->roi_top = 0;
  cam->roi_width = cam->sensor_width;
  cam->roi_height = cam->sensor_height;


  cam->num_image_buffers = num_image_buffers;
  cam->buffer_size = cam->roi_width
                   * cam->roi_height
                   * cam->inherited.depth
                   / 8;

#ifdef CAM_IFACE_DEBUG
  fprintf(stdout,"new cam context 1\n");
  fflush(stdout);
  usleep(5000000);
#endif

  return;
}

static bool
basler_pylon_stop_camera (CCbasler_pylon *cam)
{
  /* stop camera (if needed) */
  Pylon::IPylonDevice *device = cam->device;
  try {
    if (cam->grabber) {
      Pylon::IStreamGrabber *grabber = cam->grabber;
      cam->grabber = 0;

      for (unsigned i = 0; i < cam->num_image_buffers; i++) {
	if (grabber->GetWaitObject().Wait(1000)) {
	  Pylon::GrabResult result;
	  DEBUG_ONLY(std::cerr << "RetrieveResult..." << std::endl);
	  grabber->RetrieveResult (result);
	}
      }

      DEBUG_ONLY(std::cerr << "AcquisitionStop " << std::endl);
      camera_execute (cam, "AcquisitionStop");

      // deregister buffers
      for (unsigned i = 0; i < cam->num_image_buffers; i++) {
        DEBUG_ONLY(std::cerr << "DeregisterBuffer " << i << std::endl);
        grabber->DeregisterBuffer(cam->buffer_handles[i]);
      }
      DEBUG_ONLY(std::cerr << "FinishGrab" << std::endl);
      grabber->FinishGrab();
      grabber->Close();
      cam->grabber_open = false;
      delete [] cam->buffer_handles;
      free (cam->buffers);
      cam->buffers = 0;
      cam->buffer_handles = 0;
    }
  } catch (GenICam::GenericException e) {
      CAM_IFACE_ERROR_GENICAM_EXCEPTION ("closing device failed", e);
      return NULL;
  } catch (std::exception e) {
    CAM_IFACE_ERROR_EXCEPTION("closing device failed", e);
    return false;
  } catch (...) { 
    CAM_IFACE_ERROR("unknown exception closing device");
    return false;
  }
  return true;
}

void CCbasler_pylon_close(CCbasler_pylon *cam) {
  CHECK_CC(cam);

  basler_pylon_stop_camera (cam);

  Pylon::IPylonDevice *device = cam->device;

  DEBUG_ONLY(std::cerr << "IsOpen" << std::endl);
  if (device->IsOpen()) {
    DEBUG_ONLY(std::cerr << "Close" << std::endl);
    device->Close();
  }

  /* destroy device */
  DEBUG_ONLY(std::cerr << "deleting device" << std::endl);
  ////delete cam->device;
  cam->device = 0;
}

void CCbasler_pylon_start_camera(CCbasler_pylon *cam) {
  // note: we always use the first grabber.
  unsigned stream_grabber_index = 0;
  if (cam->grabber)
    return;                    // already started
  Pylon::IStreamGrabber *grabber;
  size_t size;
  try {
    grabber = cam->device->GetStreamGrabber(stream_grabber_index);
    if (!(cam->grabber_open)) {
      grabber->Open();
      cam->grabber_open = true;
    }

    // Set the camera to continuous frame mode
    if (cam->trigger_mode == 0) {
      camera_set_enum(cam, "TriggerSelector", "AcquisitionStart");
      camera_set_enum(cam, "TriggerMode", "Off");
      camera_set_enum(cam, "AcquisitionMode", "Continuous");
    } else {
      camera_set_enum(cam, "TriggerSelector", "AcquisitionStart");
      camera_set_enum(cam, "TriggerMode", "On");
      camera_set_enum_value(cam, "TriggerSource", cam->trigger_mode - 1);
      camera_set_enum(cam, "AcquisitionMode", "Continuous");
    }

    // Select the input line
    camera_set_enum(cam, "LineSelector", "Line1");
    // Set the parameter value to 100 microseconds
    camera_set_float(cam, "LineDebouncerTimeAbs", 100);

    camera_set_enum(cam, "ExposureMode", "Timed");

    // Get the image buffer size
    size = camera_get_int (cam, "PayloadSize");

    // We won't use image buffers greater than ImageSize
    grabber_set_int(grabber, "MaxBufferSize", size);

    // We won't queue more than c_nBuffers image buffers at a time
    grabber_set_int(grabber, "MaxNumBuffer", cam->num_image_buffers);

    grabber->PrepareGrab();

    cam->buffers = malloc (size * cam->num_image_buffers);
    if (cam->buffers == 0) {
      CAM_IFACE_ERROR("out of memory allocating image buffers");
      return;
    }
    cam->buffer_handles = new Pylon::StreamBufferHandle[cam->num_image_buffers];
    char *buf_at = (char *) cam->buffers;
    for (unsigned i = 0; i < cam->num_image_buffers; i++) {
      DEBUG_ONLY(std::cerr << "registering buffer " << std::endl);
      Pylon::StreamBufferHandle h;
	h = grabber->RegisterBuffer(buf_at, size);
      cam->buffer_handles[i] = h;
      buf_at += size;
      grabber->QueueBuffer(h);
    }
    camera_execute (cam, "AcquisitionStart");
    cam->grabber = grabber;
  } catch (GenICam::GenericException e) {
    CAM_IFACE_ERROR_GENICAM_EXCEPTION("GenICam exception in start_camera", e);
    return;
  } catch (std::exception e) {
    CAM_IFACE_ERROR_EXCEPTION("std::exception in start_camera", e);
    return;
  } catch (...) {
    CAM_IFACE_ERROR("unknown exception in start_camera");
    return;
  }
}

void CCbasler_pylon_stop_camera(CCbasler_pylon *cam) {
  basler_pylon_stop_camera (cam);
}

void CCbasler_pylon_get_num_camera_properties(CCbasler_pylon *cam,
                                        int* num_properties) {
  *num_properties = CI_BASLER_NUM_ATTR;
}

void CCbasler_pylon_get_camera_property_info(CCbasler_pylon *cam,
                                       int property_number,
                                       CameraPropertyInfo *info) {
  if (info==NULL) {
    CAM_IFACE_ERROR("no info argument specified (NULL argument)");
  }
  info->is_present = 1;

  info->min_value = 0;
  //info->max_value = min(MAX_LONG,MAX_UINT32);
  info->max_value = 0x7FFFFFFF;

  info->has_auto_mode = 1;
  info->has_manual_mode = 1;

  info->is_scaled_quantity = 0;

  info->original_value = 0;

  info->available = 1;
  info->readout_capable = 1;
  info->on_off_capable = 0;

  info->absolute_capable = 0;
  info->absolute_control_mode = 0;
  info->absolute_min_value = 0.0;
  info->absolute_max_value = 0.0;

  int64_t mymin,mymax;

  switch (property_number) {
    case CI_BASLER_ATTR_GAIN:
      info->name = "gain";
      info->has_auto_mode = 0;
      camera_get_int_range (cam, "GainRaw", &mymin, &mymax);
      info->min_value = mymin;
      info->max_value = mymax;
      break;
    case CI_BASLER_ATTR_SHUTTER:
      info->name = "shutter";
      info->is_scaled_quantity = 1;
      info->scaled_unit_name = "msec";
      info->scale_offset = 0;
      info->scale_gain = 1e-3; // convert from microsecond to millisecond
      camera_get_int_range (cam, "ExposureTimeRaw", &mymin, &mymax);
      info->min_value = mymin;
      info->max_value = mymax;
      break;
    default:
      CAM_IFACE_ERROR("invalid property number");
      break;
  }
  return;
}

void CCbasler_pylon_get_camera_property(CCbasler_pylon *cam,
                                  int property_number,
                                  long* Value,
                                  int* Auto ) {
  switch (property_number) {
    case CI_BASLER_ATTR_GAIN:
      *Value = camera_get_int (cam, "GainRaw");
      break;
    case CI_BASLER_ATTR_SHUTTER:
      *Value = camera_get_int(cam, "ExposureTimeRaw");
      break;
    default: 
      CAM_IFACE_ERROR("invalid property number");
      return;
  }
  *Auto = false;
}

void CCbasler_pylon_set_camera_property(CCbasler_pylon *cam,
                                        int property_number,
                                        long Value,
                                        int Auto )
{
  if (Auto) {
    CAM_IFACE_ERROR("automatic camera properties not supported");
    return;
  }
  switch (property_number) {
    case CI_BASLER_ATTR_GAIN:
      camera_set_int (cam, "GainRaw", Value);
      break;
    case CI_BASLER_ATTR_SHUTTER:
      camera_set_int(cam, "ExposureTimeRaw", Value);
      break;
    default: 
      CAM_IFACE_ERROR("invalid property number");
      return;
  }
}

void CCbasler_pylon_grab_next_frame_blocking_with_stride(CCbasler_pylon *cam,
                                                         unsigned char *out_bytes,
                                                         intptr_t stride0,
                                                         float timeout)
{
  Pylon::GrabResult result;
  if (cam->grabber == 0) {
      CCbasler_pylon_start_camera (cam);
      if (cam->grabber == 0)
          return;
  }
  try {
    Pylon::WaitObject &obj = cam->grabber->GetWaitObject();
    if (!obj.Wait(timeout * 1000.0)) {
      CAM_IFACE_ERROR("timed-out waiting for frame-grabber");
      return;
    }
    cam->grabber->RetrieveResult (result);
  } catch (std::exception e) {
    CAM_IFACE_ERROR_EXCEPTION("grabbing next frame", e);
    return;
  }
  unsigned stride = (cam->roi_width * cam->inherited.depth + 7) / 8;
  if (stride0 == stride) {
    // same stride
    memcpy((void*)out_bytes, /*dest*/
	   result.Buffer(),
	   cam->buffer_size);
  } else {
    // different strides
    for (int row=0; row<cam->roi_height; row++) {
      memcpy((void*)(out_bytes + row * stride0), /*dest*/
	     (const void*)((char*)result.Buffer() + row * stride),/*src*/
	     stride);/*size*/
    }
  }
  cam->last_timestamp = 0.001 * result.GetTimeStamp() / 125000.0; // XXX scale from 1394 cycles?
  cam->last_frameno = result.FrameNr();

  cam->grabber->QueueBuffer(result.Handle(), NULL);
}

void CCbasler_pylon_grab_next_frame_blocking(CCbasler_pylon *cam,
                                             unsigned char *out_bytes,
                                             float timeout)
{
  CCbasler_pylon_grab_next_frame_blocking_with_stride (cam, out_bytes,
                                             cam->roi_width * cam->inherited.depth / 8,
                                             timeout);
}

void CCbasler_pylon_point_next_frame_blocking(CCbasler_pylon *cam,
                                              unsigned char **buf_ptr,
                                              float timeout)
{
  CAM_IFACE_ERROR("point_next_frame_blocking: unimplemented");
  return;
}

void CCbasler_pylon_unpoint_frame( CCbasler_pylon *cam)
{
  CAM_IFACE_ERROR("unpoint_frame: unimplemented");
  return;
}

void CCbasler_pylon_get_last_timestamp(CCbasler_pylon *cam,
                                       double* timestamp)
{
  *timestamp = cam->last_timestamp;
}

void CCbasler_pylon_get_last_framenumber(CCbasler_pylon *cam,
                                         unsigned long* framenumber )
{
  *framenumber = cam->last_frameno;
}

void CCbasler_pylon_get_num_trigger_modes( CCbasler_pylon *cam,
                                     int *num_trigger_modes )
{
  Pylon::IPylonDevice *device = cam->device;
  GenApi::CEnumerationPtr cs = device->GetNodeMap()->GetNode("TriggerSource");
  GenApi::NodeList_t entries;
  cs->GetEntries(entries);
  *num_trigger_modes = entries.size();
}

void CCbasler_pylon_get_trigger_mode_string(CCbasler_pylon *cam,
                                            int trigger_mode_number,
                                            char* trigger_mode_string,
                                            int trigger_mode_string_maxlen)
{
  Pylon::IPylonDevice *device = cam->device;
  GenApi::CEnumerationPtr cs = device->GetNodeMap()->GetNode("TriggerSource");
  GenApi::NodeList_t entries;
  cs->GetEntries(entries);
  if (trigger_mode_number == 0) {
    strncpy (trigger_mode_string, "CONTINUOUS", trigger_mode_string_maxlen);
  } else {
    strncpy (trigger_mode_string,
             entries[trigger_mode_number]->GetName().c_str(),
             trigger_mode_string_maxlen);
  }
}

void CCbasler_pylon_get_trigger_mode_number(CCbasler_pylon *cam,
                                            int *trigger_mode_number)
{
  *trigger_mode_number = cam->trigger_mode;
}

void CCbasler_pylon_set_trigger_mode_number(CCbasler_pylon *cam,
                                            int trigger_mode_number)
{
  bool is_grabbing = cam->grabber != 0;
  if (is_grabbing)
    basler_pylon_stop_camera (cam);
  if (trigger_mode_number == 0)
    cam->trigger_mode = 0;
  else {
#if 0
    GenApi::NodeList_t nodes;
    GenApi::CEnumerationPtr cs = device->GetNodeMap()->GetNode("TriggerSource");
    cs->GetEntries(nodes);
    ...
#else
    cam->trigger_mode = trigger_mode_number + 1;		// HACK: what if enums are out-of-order????
#endif
  }
  if (is_grabbing)
    CCbasler_pylon_start_camera (cam);
}

void CCbasler_pylon_get_frame_roi(CCbasler_pylon *cam,
                                  int *left,
                                  int *top,
                                  int* width,
                                  int* height)
{
  *left = cam->roi_left;
  *top = cam->roi_top;
  *width = cam->roi_width;
  *height = cam->roi_height;
}

void CCbasler_pylon_set_frame_roi(CCbasler_pylon *cam,
                                  int left,
                                  int top,
                                  int width,
                                  int height)
{

  bool is_grabbing = cam->grabber != 0;
  if (is_grabbing) {
    if (!basler_pylon_stop_camera (cam))
      return;
  }

  cam->roi_left = left;
  cam->roi_top = top;
  cam->roi_width = width;
  cam->roi_height = height;
  cam->buffer_size = (cam->roi_width * cam->roi_height * cam->inherited.depth + 7) / 8;

  if (is_grabbing)
    CCbasler_pylon_start_camera (cam);
}

void CCbasler_pylon_get_framerate( CCbasler_pylon *cam,
                             float *framerate )
{
  *framerate = 30;              // XXX
}

void CCbasler_pylon_set_framerate( CCbasler_pylon *cam,
                             float framerate )
{
  CAM_IFACE_ERROR("set_framerate not implemented");
  return;
}

void CCbasler_pylon_get_max_frame_size( CCbasler_pylon *cam,
                                  int *width, int *height )
{
  *width = cam->sensor_width;
  *height = cam->sensor_height;
}

void CCbasler_pylon_get_buffer_size(CCbasler_pylon *cam,
                                    int *size)
{
  *size=cam->buffer_size;
}

void CCbasler_pylon_get_num_framebuffers(CCbasler_pylon *cam,
                                         int *num_framebuffers)
{
  *num_framebuffers=cam->num_image_buffers;
}

void CCbasler_pylon_set_num_framebuffers(CCbasler_pylon *cam,
                                         int num_framebuffers)
{
  CHECK_CC(cam);
  if (cam->num_image_buffers == (unsigned) num_framebuffers)
    return;
  bool is_grabbing = cam->grabber != 0;
  if (is_grabbing) {
    if (!basler_pylon_stop_camera (cam))
      return;
  }
  cam->num_image_buffers = num_framebuffers;
  if (is_grabbing) {
    CCbasler_pylon_start_camera (cam);
  }
}
