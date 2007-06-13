// from Prosilica GigE SDK/examples/Stream/StdAfx.h
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// from Prosilica GigE SDK/examples/Stream/Stream.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#ifdef _WINDOWS
#define u_int8_t unsigned char
#endif

#if defined(_LINUX) || defined(_QNX)
#include <unistd.h>
#include <time.h>
#include <signal.h>
#endif

#include "PvApi.h"

#include <list> // from STL
#include <queue> // from STL

#include <OpenThreads/Mutex>  
#include <OpenThreads/Condition>

extern "C" {
#include "cam_iface.h"

#ifdef _WINDOWS
#define _STDCALL __stdcall
#else
#define _STDCALL
#endif

#if 1
#define DPRINTF(...)
#else
#define DPRINTF(...) printf(__VA_ARGS__)
#endif

#if defined(_LINUX) || defined(_QNX)
void Sleep(unsigned int time)
{
    struct timespec t,r;
    
    t.tv_sec    = time / 1000;
    t.tv_nsec   = (time % 1000) * 1000000;    
    
    while(nanosleep(&t,&r)==-1)
        t = r;
}
#endif

/* globals -- allocate space */
  u_int64_t prev_ts_uint64; //tmp

static int cam_iface_error;
#define CAM_IFACE_MAX_ERROR_LEN 255
static char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN];

#define PV_MAX_ENUM_LEN 32

/* global variables */
#define PV_MAX_NUM_CAMERAS 2
#define PV_MAX_NUM_BUFFERS 80
static int num_cameras = 0;
static tPvCameraInfo camera_list[PV_MAX_NUM_CAMERAS];

typedef std::list<tPvFrame*, std::allocator<tPvFrame*> > FRAMEPTRLIST;
typedef std::queue<tPvFrame*,FRAMEPTRLIST >  FRAMEPTRQUEUE;
typedef FRAMEPTRQUEUE* FRAMEPTRQUEUEPTR;
static FRAMEPTRQUEUEPTR camera_frames_queue_list[PV_MAX_NUM_CAMERAS];

OpenThreads::Mutex frames_ready_lock_cam0;
OpenThreads::Mutex wait_func_mutex_cam0;
OpenThreads::Condition frames_ready_condition_cam0;

// forward declarations
void _STDCALL OnNewFrameCallbackCam0(tPvFrame* frame);

typedef struct cam_iface_backend_extras cam_iface_backend_extras;
struct cam_iface_backend_extras {
  int num_buffers;
  int buf_size; // current buffer size (number of bytes)
  unsigned long malloced_buf_size; // maximum buffer size (number of bytes)
  int current_height;
  intptr_t current_width;
  int max_height;
  int max_width;
  tPvFrame** frames;
  int frame_number_currently_waiting_for;
  unsigned long last_framecount;
  u_int64_t last_timestamp;
  double timestamp_tick;
  int exposure_mode_number;
};

#define PV_NUM_ATTR 2
const char *pv_attr_strings[PV_NUM_ATTR] = {
  "gain",
  "shutter" // exposure
};
#define PV_ATTR_GAIN 0
#define PV_ATTR_SHUTTER 1

  // from PvApi.h
#define PV_ERROR_NUM 22
const char *pv_error_strings[PV_ERROR_NUM] = {
  "No error",
  "Unexpected camera fault",
  "Unexpected fault in PvApi or driver",
  "Camera handle is invalid",
  "Bad parameter to API call",
  "Sequence of API calls is incorrect",
  "Camera or attribute not found",
  "Camera cannot be opened in the specified mode",
  "Camera was unplugged",
  "Setup is invalid (an attribute is invalid)",
  "System/network resources or memory not available",
  "1394 bandwidth not available",
  "Too many frames on queue",
  "Frame buffer is too small",
  "Frame cancelled by user",
  "The data for the frame was lost",
  "Some data in the frame is missing",
  "Timeout during wait",
  "Attribute value is out of the expected range",
  "Attribute is not this type (wrong access function)",
  "Attribute write forbidden at this time",
  "Attribute is not available at this time"
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

#define CAM_IFACE_ERROR_FORMAT(m)					\
  cam_iface_snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,	\
		     "%s (%d): %s\n",__FILE__,__LINE__,(m));

#define CAM_IFACE_THROW_ERROR(m)			\
  {							\
    cam_iface_error = -1;				\
    CAM_IFACE_ERROR_FORMAT((m));			\
    return;						\
  }
  
#define CAM_IFACE_THROW_ERRORV(m)			\
  {							\
    cam_iface_error = -1;				\
    CAM_IFACE_ERROR_FORMAT((m));			\
    return NULL;						\
  }

#define CHECK_CC(m)							\
  if (!(m)) {								\
    CAM_IFACE_THROW_ERROR("no CamContext specified (NULL argument)");	\
  }

#define NOT_IMPLEMENTED CAM_IFACE_THROW_ERROR("not yet implemented");

#define CAM_IFACE_CHECK_DEVICE_NUMBER(m)				\
  if ( ((m)<0) | ((m)>=num_cameras) ) {					\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("invalid device_number");			\
    return;								\
  }

#define CAM_IFACE_CHECK_DEVICE_NUMBERV(m)				\
  if ( ((m)<0) | ((m)>=num_cameras) ) {					\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("invalid device_number");			\
    return NULL;							\
  }

#define CIPVCHK(err) {							\
  tPvErr m = err;							\
  if (m!=ePvErrSuccess) {						\
    cam_iface_error = -1;						\
    if (m<PV_ERROR_NUM) {						\
      cam_iface_snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN, \
			 "%s (%d): Prosilica GigE err %d: %s\n",__FILE__,__LINE__, \
			 m,						\
			 pv_error_strings[m]);				\
    } else {								\
      cam_iface_snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN, \
			 "%s (%d): Prosilica GigE err %d: (unknown error)\n", \
			 __FILE__,__LINE__,				\
			 m);						\
    }									\
    return;								\
  }									\
  }
  
#define CIPVCHKV(err) {							\
  tPvErr m = err;							\
  if (m!=ePvErrSuccess) {						\
    cam_iface_error = -1;						\
    if (m<PV_ERROR_NUM) {						\
      cam_iface_snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN, \
			 "%s (%d): Prosilica GigE err %d: %s\n",__FILE__,__LINE__, \
			 m,						\
			 pv_error_strings[m]);				\
    } else {								\
      cam_iface_snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN, \
			 "%s (%d): Prosilica GigE err %d: (unknown error)\n", \
			 __FILE__,__LINE__,				\
			 m);						\
    }									\
    return NULL;							\
  }									\
}
  
#define INTERNAL_CHK() {						\
    if (cam_iface_error) {						\
      return;								\
    }									\
  }

#define INTERNAL_CHKV() {						\
    if (cam_iface_error) {						\
      return void;							\
    }									\
  }

void _internal_start_streaming( CamContext * ccntxt, 
				tPvHandle* handle_ptr, 
				cam_iface_backend_extras* backend_extras ) {
  // modeled after CFinderWindow::OnStart() in 
  // in Prosilica's examples/SampleViewer/src/FinderWindow.cpp
  unsigned long lCapturing;
  tPvHandle iHandle = *handle_ptr;
  tPvUint32 iBytesPerFrame;
  
  CIPVCHK(PvCaptureQuery(iHandle,&lCapturing));
  if(lCapturing) {
    CAM_IFACE_THROW_ERROR("camera not in IDLE mode");
  }
  CIPVCHK(PvCaptureStart(iHandle));
  PvAttrUint32Get(iHandle,"TotalBytesPerFrame",&iBytesPerFrame);
  if(!iBytesPerFrame) {
    CAM_IFACE_THROW_ERROR("incorrect frame size");
  }
  backend_extras->buf_size = iBytesPerFrame;
  if ((backend_extras->malloced_buf_size) < iBytesPerFrame) {
    CAM_IFACE_THROW_ERROR("buffer is larger than allocated memory");
  }
  // I guess this doesn't "cross" a thread boundary because we're not capturing
  for (int i=0; i<backend_extras->num_buffers; i++) {
    backend_extras->frames[i]->ImageBufferSize = backend_extras->buf_size;
  }

  tPvFrameCallback frame_callback_func;  
  switch (ccntxt->device_number) {  
  case 0: frame_callback_func = OnNewFrameCallbackCam0; break;  
    //case 1: frame_callback_func = OnNewFrameCallbackCam1; break;  
  default: CAM_IFACE_THROW_ERROR("support for >1 cameras not implemented");  
  }

  for (int i=0; i<backend_extras->num_buffers; i++) {
    CIPVCHK(PvCaptureQueueFrame(*handle_ptr,backend_extras->frames[i],frame_callback_func));
  }
  
  CIPVCHK(PvCommandRun(*handle_ptr,"AcquisitionStart"));
}

void _internal_stop_streaming( CamContext * ccntxt, 
			       tPvHandle* handle_ptr, 
			       cam_iface_backend_extras* backend_extras ) {
  // modeled after CFinderWindow::OnStop() in 
  // in Prosilica's examples/SampleViewer/src/FinderWindow.cpp
  unsigned long lCapturing;
  tPvHandle iHandle = *handle_ptr;
  tPvUint32 iBytesPerFrame;

  CIPVCHK(PvCaptureQuery(iHandle,&lCapturing));
  if(lCapturing) {
    CIPVCHK(PvCommandRun(*handle_ptr,"AcquisitionStop"));

    // According to the PvAPI manual, this should follow the
    // PvCaptureQueueClear() call, but this order is what their sample
    // code does.
    CIPVCHK(PvCaptureEnd(iHandle));

    // Unintelligible comment from Prosilica code:
    // then dequeue all the frames still in the queue (we
    // will ignore any error as the capture was stopped anyway)
    CIPVCHK(PvCaptureQueueClear(iHandle));
  } else {
    // Comment from Prosilica code:
    // then dequeue all the frame still in the queue
    // in case there is any left in it and that the camera
    // was unplugged (we will ignore any error as the
    // capture was stopped anyway)
    CIPVCHK(PvCaptureQueueClear(iHandle));
  }
}

const char *cam_iface_get_driver_name() {
  return "prosilica_gige";
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
  for (int i=0;i<PV_MAX_NUM_CAMERAS;i++) {
    camera_frames_queue_list[i] = new FRAMEPTRQUEUE;
  }
  CIPVCHK(PvInitialize());

  //OpenThreads::Thread::Init();
  
  /*
    // test to see if "<<32" does what I want...
  unsigned long TimestampLo, TimestampHi;
  TimestampLo = 0xFFFFFFFF;
  TimestampHi = 0xFFFFFFFF;
  u_int64_t ts_uint64;
  ts_uint64 = (((u_int64_t)TimestampHi)<<32) + (TimestampLo);
  printf("ts_uint64 0x%llx\n",ts_uint64);
  printf("ts_uint64 %llu\n",ts_uint64);
  double tsd;
  tsd = (double)ts_uint64;
  printf("tsd %f\n",tsd);
  */

  for (int i=0;i<4;i++) {
    if (PvCameraCount()) { // wait for a camera for 4*250 msec = 1 sec
      break;
    }
    Sleep(250);
  }

  // get list of reachable cameras
  unsigned long     ul_nc, numCamerasAvail;
  ul_nc = PvCameraList(camera_list, PV_MAX_NUM_CAMERAS, &numCamerasAvail);

  if (ul_nc != numCamerasAvail) {
    CAM_IFACE_THROW_ERROR("more cameras available than PV_MAX_NUM_CAMERAS");
  }

  if (ul_nc < PV_MAX_NUM_CAMERAS) {
    DPRINTF("trying unreachable cameras...\n");
    ul_nc += PvCameraListUnreachable(&camera_list[ul_nc],
				     PV_MAX_NUM_CAMERAS-ul_nc,
				     NULL);
  }

  num_cameras = (int)ul_nc; // cast to integer
}

void cam_iface_shutdown() {
  PvUnInitialize();
}

int cam_iface_get_num_cameras() {
  return num_cameras;
}

void cam_iface_get_camera_info(int device_number, Camwire_id *out_camid) {
  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  if (out_camid==NULL) { CAM_IFACE_THROW_ERROR("return structure NULL"); }

  cam_iface_snprintf(out_camid->vendor, CAMWIRE_ID_MAX_CHARS, "Prosilica");
  cam_iface_snprintf(out_camid->model, CAMWIRE_ID_MAX_CHARS, "%s", camera_list[device_number].DisplayName);
  cam_iface_snprintf(out_camid->chip, CAMWIRE_ID_MAX_CHARS, "%llXh", (long long unsigned int)camera_list[device_number].UniqueId);
}


void cam_iface_get_num_modes(int device_number, int *num_modes) {
  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  *num_modes = 1; // only one mode
}

void cam_iface_get_mode_string(int device_number,
			       int mode_number,
			       char* mode_string,
			       int mode_string_maxlen) {
  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  cam_iface_snprintf(mode_string, mode_string_maxlen, "(Prosilica GigE default mode)");
}
  
CamContext * new_CamContext( int device_number, int NumImageBuffers,
			     int mode_number) {
  CamContext *ccntxt = NULL;

  CAM_IFACE_CHECK_DEVICE_NUMBERV(device_number);
  if (mode_number!=0) { CAM_IFACE_THROW_ERRORV("mode number not 0"); }
  
  ccntxt = new CamContext; // C++ equivalent to malloc
  memset(ccntxt,0,sizeof(CamContext));
  ccntxt->device_number = device_number;
  ccntxt->backend_extras = new cam_iface_backend_extras;
  memset(ccntxt->backend_extras,0,sizeof(cam_iface_backend_extras));

  tPvHandle* handle_ptr = new tPvHandle; // C++ equivalent to malloc
  CIPVCHKV(PvCameraOpen(camera_list[device_number].UniqueId, 
			ePvAccessMaster, 
			handle_ptr ));
  ccntxt->cam = (void*)handle_ptr; // save pointer

  // Check firmware version
  unsigned long FirmwareVerMajor = 0;
  unsigned long FirmwareVerMinor = 0;
  unsigned long FirmwareVerBuild = 0;

  CIPVCHKV(PvAttrUint32Get(*handle_ptr,"FirmwareVerMajor",&FirmwareVerMajor));
  CIPVCHKV(PvAttrUint32Get(*handle_ptr,"FirmwareVerMinor",&FirmwareVerMinor));
  CIPVCHKV(PvAttrUint32Get(*handle_ptr,"FirmwareVerBuild",&FirmwareVerBuild));

  // DPRINTF("firmware %d %d %d\n",FirmwareVerMajor,FirmwareVerMinor,FirmwareVerBuild);

  if ( ! ((FirmwareVerMajor >= 1) && (FirmwareVerMinor >= 24) ) ) {
    CAM_IFACE_THROW_ERRORV("firmware too old - see http://www.prosilica.com/support/gige/ge_download.html");
  }

  const char *attr_names[5] = {
    "Width",
    "ExposureValue",
    "FrameStartTriggerMode",
    "RegionX",
    "FrameRate",
  };
  tPvAttributeInfo attrInfo;
  for (int i=0;i<5;i++) {
    DPRINTF("%s\n",attr_names[i]);
    CIPVCHKV(PvAttrInfo(*handle_ptr,attr_names[i],&attrInfo));
    DPRINTF("     impact: %s\n",attrInfo.Impact);
    DPRINTF("     category: %s\n",attrInfo.Category);
    if (attrInfo.Flags & ePvFlagRead) {
      DPRINTF("       Read access is permitted\n");
    }
    if (attrInfo.Flags & ePvFlagWrite) {
      DPRINTF("       Write access is permitted\n");
    }
    if (attrInfo.Flags & ePvFlagVolatile) {
      DPRINTF("       The camera may change the value any time\n");
    }
    if (attrInfo.Flags & ePvFlagConst) {
      DPRINTF("       Value is read only and never changes\n");
    }
  }

  /*
  if (NumImageBuffers!=5) {
    DPRINTF("forcing num_buffers to 5 for performance reasons\n"); // seems to work well - ADS 20061204
    NumImageBuffers = 5;
  }
  */

  // hard-coded mono8 for now...
  ccntxt->depth = 8;
  ccntxt->coding = CAM_IFACE_MONO8;

  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);

  unsigned long FrameSize = 0;
  CIPVCHKV(PvAttrUint32Get(*handle_ptr,"TotalBytesPerFrame",&FrameSize));
  backend_extras->buf_size = FrameSize; // XXX should check for int overflow...

  tPvUint32 tsf;
  CIPVCHKV(PvAttrUint32Get(*handle_ptr,"TimeStampFrequency",&tsf));
  backend_extras->timestamp_tick = 1.0/((double)tsf);
  
  CamContext_set_trigger_mode_number( ccntxt, 0 ); // set to freerun

  /*
  char buf[PV_MAX_ENUM_LEN];
  unsigned long enum_size;
  CIPVCHKV(PvAttrEnumGet(*handle_ptr,"ExposureMode",buf,PV_MAX_ENUM_LEN,&enum_size));
  fprintf(stderr,"ExposureMode enum: %s\n",buf);

  if (strncmp(buf,"FreeRun",enum_size)==0) {
    backend_extras->exposure_mode_number=0;
  } else if (strncmp(buf,"Manual",enum_size)==0) {
    backend_extras->exposure_mode_number=1;
  } else {
    fprintf(stderr,"ExposureMode enum: %s\n",buf);
    CAM_IFACE_THROW_ERRORV("unknown ExposureMode enum");
  }
  */

  tPvUint32 MinWidth,MaxWidth,Width;
  CIPVCHKV(PvAttrRangeUint32(*handle_ptr,"Width",&MinWidth,&MaxWidth));
  CIPVCHKV(PvAttrUint32Get(*handle_ptr,"Width",&Width));
  backend_extras->current_width = Width;
  backend_extras->max_width = MaxWidth;  // XXX should check for int overflow...

  tPvUint32 MinHeight,MaxHeight,Height;
  CIPVCHKV(PvAttrRangeUint32(*handle_ptr,"Height",&MinHeight,&MaxHeight));
  CIPVCHKV(PvAttrUint32Get(*handle_ptr,"Height",&Height));
  backend_extras->current_height = Height;  // XXX should check for int overflow...
  backend_extras->max_height = MaxHeight;  // XXX should check for int overflow...

  backend_extras->malloced_buf_size = MaxWidth*MaxHeight;

  if (NumImageBuffers>PV_MAX_NUM_BUFFERS) {
    CAM_IFACE_THROW_ERRORV("requested too many buffers");
  }
  
  // allocate image buffers
  backend_extras->num_buffers = NumImageBuffers;
  backend_extras->frames = (tPvFrame**)malloc( NumImageBuffers*sizeof(tPvFrame*) );
  if (backend_extras->frames == NULL) {CAM_IFACE_THROW_ERRORV("could not alloc frames");}

  for (int i=0; i<NumImageBuffers; i++) {
    backend_extras->frames[i] = NULL;
  }
  for (int i=0; i<NumImageBuffers; i++) {
    backend_extras->frames[i] = new tPvFrame;
    if (backend_extras->frames[i] == NULL) {CAM_IFACE_THROW_ERRORV("could not alloc frames");}
    backend_extras->frames[i]->ImageBuffer = malloc(backend_extras->malloced_buf_size);
    if (backend_extras->frames[i]->ImageBuffer == NULL) {CAM_IFACE_THROW_ERRORV("could not alloc buffers");}
    backend_extras->frames[i]->ImageBufferSize = backend_extras->buf_size;
    backend_extras->frames[i]->AncillaryBuffer = NULL;
    backend_extras->frames[i]->AncillaryBufferSize = 0;
    backend_extras->frames[i]->Context[0] = (void*)i;
  }
  backend_extras->frame_number_currently_waiting_for=0; // first frame first

  return ccntxt;
}

void delete_CamContext(CamContext *ccntxt) {
  if (!ccntxt) {CAM_IFACE_THROW_ERROR("no CamContext specified (NULL argument)");}
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;

  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);

  _internal_stop_streaming(ccntxt,handle_ptr,backend_extras);INTERNAL_CHK();

  if (backend_extras!=NULL) {
    if (backend_extras->frames!=NULL) {

      //CIPVCHK(PvCaptureQueueClear(*handle_ptr));
      CIPVCHK(PvCaptureEnd(*handle_ptr));

      for (int i=0; i<(backend_extras->num_buffers); i++) {
	if (backend_extras->frames[i] != NULL) {
	  if (backend_extras->frames[i]->ImageBuffer != NULL) {
	    free(backend_extras->frames[i]->ImageBuffer);
	    backend_extras->frames[i]->ImageBuffer = (void*)NULL;
	  }
	  delete backend_extras->frames[i];
	}
      }
      free(backend_extras->frames);
    }
    delete backend_extras;
    ccntxt->backend_extras = (void*)NULL;
  }

  delete handle_ptr;
  ccntxt->cam = (void*)NULL;

  delete ccntxt;
  ccntxt = (CamContext*)NULL;
}

void CamContext_start_camera( CamContext *ccntxt ) {
  CHECK_CC(ccntxt);
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  _internal_start_streaming(ccntxt,handle_ptr,backend_extras);INTERNAL_CHK();
}

void CamContext_stop_camera( CamContext *ccntxt ) {
  CHECK_CC(ccntxt);
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  _internal_stop_streaming(ccntxt,handle_ptr,backend_extras);INTERNAL_CHK();
}

void CamContext_get_num_camera_properties(CamContext *ccntxt, 
					  int* num_properties) {
  CHECK_CC(ccntxt);
  /*
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;
  tPvAttrListPtr attr_list;
  unsigned long num_props_pv=0;
  CIPVCHK(PvAttrList(*handle_ptr,&attr_list,&num_props_pv));
  for (int i=0;i<num_props_pv;i++) {
    DPRINTF("attr: %d %s\n",i,attr_list[i]);
  }
  */
  *num_properties = PV_NUM_ATTR;
}

void CamContext_get_camera_property_info(CamContext *ccntxt,
					 int property_number,
					 CameraPropertyInfo *info) {
  CHECK_CC(ccntxt);
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;

  if (info==NULL) {
    CAM_IFACE_THROW_ERROR("no info argument specified (NULL argument)");
  }

  info->is_present = 1;
  
  info->min_value = 0;
  //info->max_value = min(MAX_LONG,MAX_UINT32);
  info->max_value = 0x7FFFFFFF;
  
  info->has_auto_mode = 1;
  info->has_manual_mode = 1;
  
  info->is_scaled_quantity = 0;
  
  tPvUint32 mymin,mymax;
  
  switch (property_number) {
  case PV_ATTR_GAIN:
    info->name = "gain";
    info->has_auto_mode = 0;
    CIPVCHK(PvAttrRangeUint32(*handle_ptr,"GainValue",&mymin,&mymax));
    info->min_value = mymin;
    info->max_value = mymax;
    break;
  case PV_ATTR_SHUTTER:
    info->name = "shutter";
    CIPVCHK(PvAttrRangeUint32(*handle_ptr,"ExposureValue",&mymin,&mymax));
    info->min_value = mymin;
    /// XXX HACK!!!
    //info->max_value = mymax;
    info->max_value = 50000;
    DPRINTF("WARNING: artificially setting max_value of shutter to 50000 in %s, %d\n",__FILE__,__LINE__);
    info->is_scaled_quantity = 1;
    info->scaled_unit_name = "msec";
    info->scale_offset = 0;
    info->scale_gain = 1e-3; // convert from microsecond to millisecond
    break;
  default:
    CAM_IFACE_THROW_ERROR("invalid property number");
    break;
  }

  return;
}

void CamContext_get_camera_property(CamContext *ccntxt,
				    int property_number,
				    long* Value,
				    int* Auto ) {
  CHECK_CC(ccntxt);
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;

  tPvUint32 value;
  char buf[PV_MAX_ENUM_LEN];
  unsigned long enum_size;
  
  switch (property_number) {
  case PV_ATTR_GAIN:
    CIPVCHK(PvAttrEnumGet(*handle_ptr,"GainMode",buf,PV_MAX_ENUM_LEN,&enum_size));
    CIPVCHK(PvAttrUint32Get(*handle_ptr,"GainValue",&value));
    *Value = value;
    if (strncmp(buf,"Manual",enum_size)==0) {
      *Auto = 0;
    } else if (strncmp(buf,"Auto",enum_size)==0) {
      *Auto = 1;
    } else {
      CAM_IFACE_THROW_ERROR("unknown enum");
    }
    break;
  case PV_ATTR_SHUTTER:
    CIPVCHK(PvAttrEnumGet(*handle_ptr,"ExposureMode",buf,PV_MAX_ENUM_LEN,&enum_size));
    CIPVCHK(PvAttrUint32Get(*handle_ptr,"ExposureValue",&value));
    *Value = value;
    if (strncmp(buf,"Manual",enum_size)==0) {
      *Auto = 0;
    } else if (strncmp(buf,"Auto",enum_size)==0) {
      *Auto = 1;
    } else {
      CAM_IFACE_THROW_ERROR("unknown enum");
    }
    break;
  default:
    CAM_IFACE_THROW_ERROR("invalid property number");
    break;
  }
  return;
}

void CamContext_set_camera_property(CamContext *ccntxt, 
				    int property_number,
				    long Value,
				    int Auto ) {
  CHECK_CC(ccntxt);
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;
  const char* mode_str=NULL;
  tPvUint32 value = Value;

  switch (property_number) {
  case PV_ATTR_GAIN:
    if (Auto!=0) {
      CAM_IFACE_THROW_ERROR("auto gain not available");
    }
    CIPVCHK(PvAttrUint32Set(*handle_ptr,"GainValue",value));
    break;
  case PV_ATTR_SHUTTER:
    if (Auto==0) {
      mode_str = "Manual";
    } else {
      mode_str = "Auto";
    }
    CIPVCHK(PvAttrEnumSet(*handle_ptr,"ExposureMode",mode_str));
    CIPVCHK(PvAttrUint32Set(*handle_ptr,"ExposureValue",value));
    break;
  default:
    CAM_IFACE_THROW_ERROR("invalid property number");
    break;
  }
  return;
}

void _STDCALL OnNewFrameCallbackCam0(tPvFrame* frame){  
  int status;
  FRAMEPTRQUEUE* q = camera_frames_queue_list[0];
  tPvErr err;

  /*
  if(frame->Status == ePvErrSuccess  || 
     frame->Status == ePvErrDataLost ||
     frame->Status == ePvErrDataMissing) {
    requeue = 1;
  } else {
    requeue = 0;
  }
  */

  status = frames_ready_lock_cam0.lock();
  q->push( frame );
  status = frames_ready_lock_cam0.unlock();  
  wait_func_mutex_cam0.lock();
  status = frames_ready_condition_cam0.broadcast();
  wait_func_mutex_cam0.unlock();

  /*
  if (requeue) {
    // Not calling CIPVCHK because we're in a different thread. Make it threadsafe and do it. XXX
    if (err!=ePvErrSuccess) {
      fprintf(stderr,"error re-queing frame in %s: %d\n",__FILE__,__LINE__);
      exit(1);
    }
  }
  */
}  

void CamContext_grab_next_frame_blocking( CamContext *ccntxt, unsigned char *out_bytes ) {
  CHECK_CC(ccntxt);
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  CamContext_grab_next_frame_blocking_with_stride(ccntxt,out_bytes,
						  backend_extras->current_width);
}

void CamContext_grab_next_frame_blocking_with_stride( CamContext *ccntxt, 
						      unsigned char *out_bytes, 
						      intptr_t stride0 ) {
  CHECK_CC(ccntxt);
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;
  tPvFrame* frame;
  tPvErr err;
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);

  int status;
  int frame_waiting;
  int qsize;

  tPvFrameCallback frame_callback_func;
  OpenThreads::Condition *frames_ready_condition;
  OpenThreads::Mutex *wait_func_mutex;  
  OpenThreads::Mutex *frames_ready_lock;  
  OpenThreads::Mutex *requeue_mutex;  
  FRAMEPTRQUEUE* q;
  int requeue_int;  
  
  switch (ccntxt->device_number) {  
  case 0:  
    frame_callback_func = OnNewFrameCallbackCam0;  
    frames_ready_condition = &frames_ready_condition_cam0;  
    wait_func_mutex = &wait_func_mutex_cam0;  
    frames_ready_lock = &frames_ready_lock_cam0; 
    q = camera_frames_queue_list[0];
    break;  
  default:  
    CAM_IFACE_THROW_ERROR("support for >1 cameras not implemented");  
  }
  
  status = wait_func_mutex->lock();
  frames_ready_lock->lock();
  qsize = q->size();
  if (qsize) {
    frame = q->front();
    q->pop();
  }
  frames_ready_lock->unlock();
  if (qsize==0) {
    // no frame yet, wait for one
    status = frames_ready_condition->wait(wait_func_mutex);
    //status = frames_ready_condition->wait(wait_func_mutex,5000); // timeout after 5000 msec (XXX should change this??)
    wait_func_mutex->unlock();
    frames_ready_lock->lock();
    qsize = q->size();
    if (qsize) {
	frame = q->front();
	q->pop();
    }
    frames_ready_lock->unlock();
  } else {
    // already have frame
    wait_func_mutex->unlock();
  }
  
  if (qsize==0) {
    cam_iface_error = CAM_IFACE_OTHER_ERROR;
    CAM_IFACE_ERROR_FORMAT("internal error: missing frame");
    return;
  }
  
  size_t wb = frame->Width;
  int height = frame->Height;

  for (int row=0;row<height;row++) {
    memcpy((void*)(out_bytes+row*stride0), //dest
	   (const void*)( ((intptr_t)(frame->ImageBuffer)) + row*wb),//src
	   wb);//size
  }
  backend_extras->last_framecount = frame->FrameCount;
  u_int64_t ts_uint64;
  ts_uint64 = (((u_int64_t)(frame->TimestampHi))<<32) + (frame->TimestampLo);
  int64_t dif64; //tmp
  dif64=ts_uint64-prev_ts_uint64;
  prev_ts_uint64 = ts_uint64;

  //  DPRINTF("got it                         (ts %llu)    (diff %lld)!\n",ts_uint64,dif64);
  backend_extras->last_timestamp = ts_uint64;

  tPvErr oldstatus = frame->Status;

  if(frame->Status == ePvErrSuccess  || 
     frame->Status == ePvErrDataLost ||
     frame->Status == ePvErrDataMissing) {
    CIPVCHK(PvCaptureQueueFrame(*handle_ptr,frame,frame_callback_func));
  }
  // XXX don't use frame below here!!

  if(oldstatus == ePvErrDataMissing) {
    cam_iface_error = CAM_IFACE_FRAME_DATA_MISSING_ERROR;
    CAM_IFACE_ERROR_FORMAT("frame data missing");
    return;
  }

  if(oldstatus == ePvErrDataLost) {
    cam_iface_error = CAM_IFACE_FRAME_DATA_LOST_ERROR;
    CAM_IFACE_ERROR_FORMAT("frame data lost");
    return;
  }
}

void CamContext_point_next_frame_blocking( CamContext *ccntxt, unsigned char **buf_ptr){
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}
void CamContext_unpoint_frame( CamContext *ccntxt){
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}

void CamContext_get_last_timestamp( CamContext *ccntxt, double* timestamp ) {
  CHECK_CC(ccntxt);
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  *timestamp = (double)(backend_extras->last_timestamp) * backend_extras->timestamp_tick;
}

void CamContext_get_last_framenumber( CamContext *ccntxt, long* framenumber ){
  CHECK_CC(ccntxt);
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  *framenumber = backend_extras->last_framecount; // XXX should check casting
}

void CamContext_get_num_trigger_modes( CamContext *ccntxt, 
				       int *num_exposure_modes ) {
  CHECK_CC(ccntxt);
  *num_exposure_modes = 5;
}

void CamContext_get_trigger_mode_string( CamContext *ccntxt,
					 int exposure_mode_number,
					 char* exposure_mode_string, //output parameter
					 int exposure_mode_string_maxlen) {
  CHECK_CC(ccntxt);
  switch (exposure_mode_number) {
  case 0:
    cam_iface_snprintf(exposure_mode_string,exposure_mode_string_maxlen,"FreeRun");
    break;
  case 1:
    cam_iface_snprintf(exposure_mode_string,exposure_mode_string_maxlen,"SyncIn1");
    break;
  case 2:
    cam_iface_snprintf(exposure_mode_string,exposure_mode_string_maxlen,"SyncIn2");
    break;
  case 3:
    cam_iface_snprintf(exposure_mode_string,exposure_mode_string_maxlen,"SyncIn3");
    break;
  case 4:
    cam_iface_snprintf(exposure_mode_string,exposure_mode_string_maxlen,"SyncIn4");
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
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  *exposure_mode_number = (backend_extras->exposure_mode_number);
}

void CamContext_set_trigger_mode_number( CamContext *ccntxt,
					 int exposure_mode_number ) {
  CHECK_CC(ccntxt);
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;
  switch (exposure_mode_number) {
  case 0: 
    CIPVCHK(PvAttrEnumSet(*handle_ptr,"FrameStartTriggerMode","Freerun"));
    break;
  case 1: 
    CIPVCHK(PvAttrEnumSet(*handle_ptr,"FrameStartTriggerMode","SyncIn1"));
    break;
  case 2: 
    CIPVCHK(PvAttrEnumSet(*handle_ptr,"FrameStartTriggerMode","SyncIn2"));
    break;
  case 3: 
    CIPVCHK(PvAttrEnumSet(*handle_ptr,"FrameStartTriggerMode","SyncIn3"));
    break;
  case 4: 
    CIPVCHK(PvAttrEnumSet(*handle_ptr,"FrameStartTriggerMode","SyncIn4"));
    break;
  default: 
    CAM_IFACE_THROW_ERROR("exposure_mode_number invalid");
    break;
  }
  backend_extras->exposure_mode_number = exposure_mode_number;
}

void CamContext_get_frame_offset( CamContext *ccntxt, 
				  int *left, int *top ) {
  CHECK_CC(ccntxt);
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  *left = 0;
  *top = 0;
}

void CamContext_set_frame_offset( CamContext *ccntxt, 
				  int left, int top ) {
  CHECK_CC(ccntxt);
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);

  tPvUint32 l,t;

  l=left;// XXX should check for int overflow...
  t=top;
  _internal_stop_streaming( ccntxt, handle_ptr, backend_extras );INTERNAL_CHK();
  CIPVCHK(PvAttrUint32Set(*handle_ptr,"RegionX",l));
  CIPVCHK(PvAttrUint32Set(*handle_ptr,"RegionY",t));
  _internal_start_streaming( ccntxt, handle_ptr, backend_extras );INTERNAL_CHK();
}

void CamContext_get_frame_size( CamContext *ccntxt, 
				int *width, int *height ) {
  CHECK_CC(ccntxt);
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  *width = backend_extras->current_width;
  *height = backend_extras->current_height;
}

void CamContext_set_frame_size( CamContext *ccntxt, 
				int width, int height ) {
  CHECK_CC(ccntxt);
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  tPvUint32 w,h;
  w=width;// XXX should check for int overflow...
  h=height;
  
  _internal_stop_streaming( ccntxt, handle_ptr, backend_extras );INTERNAL_CHK();

  CIPVCHK(PvAttrUint32Set(*handle_ptr,"Width",w));
  backend_extras->current_width = width;
  CIPVCHK(PvAttrUint32Set(*handle_ptr,"Height",h));
  backend_extras->current_height = height;

  unsigned long FrameSize = 0;
  CIPVCHK(PvAttrUint32Get(*handle_ptr,"TotalBytesPerFrame",&FrameSize));
  backend_extras->buf_size = FrameSize; // XXX should check for int overflow...
  
  // XXX I can't see how this doesn't cross a thread boundary -- don't we have to de-queue the frame first?
  for (int i=0; i<backend_extras->num_buffers; i++) {
    backend_extras->frames[i]->ImageBufferSize = backend_extras->buf_size;
  }
  _internal_start_streaming( ccntxt, handle_ptr, backend_extras );INTERNAL_CHK();
}

void CamContext_get_buffer_size( CamContext *ccntxt,
				 int *size) {
  CHECK_CC(ccntxt);
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  *size = backend_extras->buf_size;
}

void CamContext_get_framerate( CamContext *ccntxt, 
			       float *framerate ) {
  CHECK_CC(ccntxt);
  tPvHandle* handle_ptr = (tPvHandle*)ccntxt->cam;
  CIPVCHK(PvAttrFloat32Get(*handle_ptr,"FrameRate",framerate));
}

void CamContext_set_framerate( CamContext *ccntxt, 
			       float framerate ) {
  CHECK_CC(ccntxt);
  CAM_IFACE_THROW_ERROR("frame rate is not settable");
}

void CamContext_get_max_frame_size( CamContext *ccntxt, 
				    int *width, int *height ){
  CHECK_CC(ccntxt);
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  *width = backend_extras->max_width;
  *height = backend_extras->max_height;
}

void CamContext_get_num_framebuffers( CamContext *ccntxt, 
				      int *num_framebuffers ) {
  CHECK_CC(ccntxt);
  cam_iface_backend_extras* backend_extras = (cam_iface_backend_extras*)(ccntxt->backend_extras);
  *num_framebuffers = backend_extras->num_buffers;
}

void CamContext_set_num_framebuffers( CamContext *ccntxt, 
				      int num_framebuffers ) {
  CHECK_CC(ccntxt);
  NOT_IMPLEMENTED;
}








#include <stdio.h>
#ifdef _WIN32
#include <Windows.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#include <stdlib.h>
#include <time.h>



double floattime() {
#ifdef _WIN32
#if _MSC_VER == 1310
  struct _timeb t;
  _ftime(&t);
  return (double)t.time + (double)t.millitm * (double)0.001;
#else
  struct _timeb t;
  if (_ftime_s(&t)==0) {
    return (double)t.time + (double)t.millitm * (double)0.001;
  }
  else {
    return 0.0;
  }
#endif
#else
  struct timeval t;
  if (gettimeofday(&t, (struct timezone *)NULL) == 0)
    return (double)t.tv_sec + t.tv_usec*0.000001;
  else
    return 0.0;
#endif
}

void _check_error() {
  int err;

  err = cam_iface_have_error();
  if (err != 0) {

    fprintf(stderr,cam_iface_get_error_string());
    fprintf(stderr,"\n");
    exit(1);
  }
}




int XXXhack() {
  CamContext *cc;
  unsigned char *pixels;

  int num_buffers;

  double last_fps_print, now, t_diff;
  double fps;
  int n_frames;
  int buffer_size;
  int num_modes, num_props;
  const char** mode_strings;
  char mode_string[255];
  int i,mode_number;
  CameraPropertyInfo cam_props;
  long prop_value;
  int prop_auto;

  /*
  cam_iface_startup();
  _check_error();
  */
  
  if (cam_iface_get_num_cameras()<1) {
    _check_error();

    DPRINTF("no cameras found, will now exit\n");

    cam_iface_shutdown();
    _check_error();
    
    exit(0);
  }
  _check_error();

  cam_iface_get_num_modes(0, &num_modes);
  _check_error();

  DPRINTF("%d mode(s) available\n",num_modes);
  
  for (i=0; i<num_modes; i++) {
    cam_iface_get_mode_string(0,i,mode_string,255);
    DPRINTF("%d: %s\n",i,mode_string);
  }

  mode_number = 0;
  DPRINTF("\nChoosing mode %d\n",mode_number);

  num_buffers = 80;

  cc = new_CamContext(0,num_buffers,mode_number);
  _check_error();

  DPRINTF("allocated %d buffers\n",num_buffers);

  CamContext_get_num_camera_properties(cc,&num_props);
  _check_error();

  for (i=0; i<num_props; i++) {
    CamContext_get_camera_property_info(cc,i,&cam_props);
    if (cam_props.is_present) {
      CamContext_get_camera_property(cc,i,&prop_value,&prop_auto);
      DPRINTF("  %s: %d\n",cam_props.name,prop_value);
    } else {
      DPRINTF("  %s: not present\n");
    }
  }

  CamContext_get_buffer_size(cc,&buffer_size);
  _check_error();

  pixels = (unsigned char *)malloc( buffer_size );
  if (pixels==NULL) {
    fprintf(stderr,"couldn't allocate memory in %s, line %d\n",__FILE__,__LINE__);
    exit(1);
  }

  

  CamContext_start_camera(cc);
  _check_error();

  last_fps_print = floattime();
  n_frames = 0;
  for (;;) {
#define USE_COPY 1
#ifdef USE_COPY
    CamContext_grab_next_frame_blocking(cc,pixels);
    now = floattime();
    n_frames += 1;
    _check_error();
    fprintf(stdout,".");
    fflush(stdout);
#else
    CamContext_point_next_frame_blocking(cc,&pixels);
    now = floattime();
    n_frames += 1;
    _check_error();
    fprintf(stdout,".");
    fflush(stdout);
    CamContext_unpoint_frame(cc);
    _check_error();
#endif

    t_diff = now-last_fps_print;
    if (t_diff > 5.0) {
      fps = n_frames/t_diff;
      fprintf(stdout,"%.1f fps\n",fps);
      last_fps_print = now;
      n_frames = 0;
    }
  }
  delete_CamContext(cc);

  _check_error();
  cam_iface_shutdown();
}




CamContext * XXXhack2() {
  CamContext *cc;
  unsigned char *pixels;

  int num_buffers;

  double last_fps_print, now, t_diff;
  double fps;
  int n_frames;
  int buffer_size;
  int num_modes, num_props;
  const char** mode_strings;
  char mode_string[255];
  int i,mode_number;
  CameraPropertyInfo cam_props;
  long prop_value;
  int prop_auto;

  cam_iface_startup();
  _check_error();
  
  if (cam_iface_get_num_cameras()<1) {
    _check_error();

    DPRINTF("no cameras found, will now exit\n");

    cam_iface_shutdown();
    _check_error();
    
    exit(0);
  }
  _check_error();

  cam_iface_get_num_modes(0, &num_modes);
  _check_error();

  DPRINTF("%d mode(s) available\n",num_modes);
  
  for (i=0; i<num_modes; i++) {
    cam_iface_get_mode_string(0,i,mode_string,255);
    DPRINTF("%d: %s\n",i,mode_string);
  }

  mode_number = 0;
  DPRINTF("\nChoosing mode %d\n",mode_number);

  num_buffers = 80;

  cc = new_CamContext(0,num_buffers,mode_number);
  _check_error();

  DPRINTF("allocated %d buffers\n",num_buffers);

  /*
  CamContext_get_num_camera_properties(cc,&num_props);
  _check_error();

  for (i=0; i<num_props; i++) {
    CamContext_get_camera_property_info(cc,i,&cam_props);
    if (cam_props.is_present) {
      CamContext_get_camera_property(cc,i,&prop_value,&prop_auto);
      DPRINTF("  %s: %d\n",cam_props.name,prop_value);
    } else {
      DPRINTF("  %s: not present\n");
    }
  }
  */

  /*
  CamContext_get_buffer_size(cc,&buffer_size);
  _check_error();

  pixels = (unsigned char *)malloc( buffer_size );
  if (pixels==NULL) {
    fprintf(stderr,"couldn't allocate memory in %s, line %d\n",__FILE__,__LINE__);
    exit(1);
  }
  */
  
  CamContext_start_camera(cc);
  _check_error();
  
  return cc;
}




} // closes: extern "C"
