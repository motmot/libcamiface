/* cam_iface_bcam.cpp - implement cam_iface.h for Basler BCAM drivers */

/////////////////////////////////////////////////////////////////////////////
///
/// This file is in C++, but all functions callable using plain C

#include "cam_iface.h"

/* I don't understand why the next line fixes problems... ADS */
#define WINVER		0x500   // <--- !!!!

#include "Bcam.h"
#include "BcamUtility.h"
#include "bcamversion.h"

#include "BcamExtension.h" // SFF stuff
//#include "DataBuffer/SffChunk.h"

using namespace Bcam;

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <assert.h>

#include <iostream>
#include <deque>

//#include <guiddef.h>

// SFF stuff
#include <initguid.h>
#include "Core/SffSfExtendedDataStream.h"
#include "Core/SffSfFrameCounterStamp.h"
#include "Core/SffSfCycleTimeStamp.h"
#include "Core/SffSfDcamValuesStamp.h"
#include "Core/SffSfCrc.h"
#include "Core/SffCrc16.h"

#ifdef __cplusplus
extern "C" {
#endif

// global variables (globals)
int cam_iface_error;
#define CAM_IFACE_MAX_ERROR_LEN 255
char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN];

#define CAM_IFACE_MSG_MAX 255
char cam_iface_msg [CAM_IFACE_MSG_MAX];

#define CAM_IFACE_ERROR_FORMAT(m) \
_snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,"%s (%d): %s\n",__FILE__,__LINE__,(m));

#define CHECK_CC(m)							\
  if (!(m)) {								\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");	\
    return;								\
  }

#define NOT_IMPLEMENTED					\
  cam_iface_error = -1;					\
  CAM_IFACE_ERROR_FORMAT("not yet implemented");	\
  return;

#ifdef CAM_IFACE_DEBUG
#define _dping(m)					\
  printf("%s (%d): %s\n",__FILE__,__LINE__,(m))
#else
#define _dping(m)
#endif

static std::deque<int> cam_iface_bufs_in_use;
  static char driver_name[255];

typedef struct cam_iface_BCAM_backend_extras cam_iface_BCAM_backend_extras;
struct cam_iface_BCAM_backend_extras {
  long last_framenumber;
  double last_timestamp;
  PBYTE * ppBuffers;
  int NumImageBuffers;
  int ImageBufferSize;
  int max_width;
  int max_height;
  bool IsCrcChecked;
};

    
// implementations

// stolen from Python source code:
void
time_clock(double* result)
{
	static LARGE_INTEGER ctrStart;
	static double divisor = 0.0;
	LARGE_INTEGER now;
	double diff;

	if (divisor == 0.0) {
		LARGE_INTEGER freq;
		QueryPerformanceCounter(&ctrStart);
		if (!QueryPerformanceFrequency(&freq) || freq.QuadPart == 0) {
			/* Unlikely to happen - this works on all intel
			   machines at least!  Revert to clock() */
		  cam_iface_error = -7;
		  CAM_IFACE_ERROR_FORMAT("QueryPerformanceFrequency() failed");
		}
		divisor = (double)freq.QuadPart;
	}
	QueryPerformanceCounter(&now);
	diff = (double)(now.QuadPart - ctrStart.QuadPart);
	*result = (diff / divisor);
}

const char *cam_iface_get_driver_name() {
  _snprintf(driver_name,255,"BCAM %s.%s.%s",BCAM_VERSIONSTRING_MAJOR,BCAM_VERSIONSTRING_MINOR,BCAM_VERSIONSTRING_BUILD);
  return driver_name;
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

void cam_iface_startup(void) {
}
void cam_iface_shutdown(void) {
}

int cam_iface_get_num_cameras() {
  return CBcam::DeviceNames().size();
}

void cam_iface_get_camera_info(int index, Camwire_id *out_camid) {
  if (out_camid==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("cam_iface_get_camera_info called with NULL pointer");
    return;
  }
  
  if (index >= CBcam::DeviceNames().size()) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("requested too high index");
    return;
  }

  strcpy(out_camid->vendor,"vendor unknown");
  strcpy(out_camid->model,"model unknown");
  strcpy(out_camid->chip,"chip unknown"); 

  printf("copied OK\n");
  try {
    std::list<CString> device_names = CBcam::DeviceNames();
    std::list<CString>::iterator i = device_names.begin();
    for (int j=0; j<index; j++) i++;
    CString DeviceName = *(i);
    //printf("(device name in C: '%s')\n",DeviceName);
    strncpy(out_camid->chip,DeviceName,CAMWIRE_ID_MAX_CHARS);
  } catch ( BcamException &e ) {
    cam_iface_error = -5;
    strncpy(cam_iface_error_string,e.Description(),CAM_IFACE_MAX_ERROR_LEN);
  } catch ( exception &e ) {
    cam_iface_error = -6;
    CAM_IFACE_ERROR_FORMAT("unknown C++ exception");
  }
}

CamContext * new_CamContext( int device_number, int NumImageBuffers,
			     int format, int mode, int rate_index)
{
  CamContext *out_cr = NULL;
  CBcamExtension * this_bcam = NULL;
  PBYTE* this_ppBuffers = NULL;
  cam_iface_BCAM_backend_extras* this_backend_extras;
  CSize ImageSize;
  DCSVideoFormat bcam_format;
  DCSVideoMode bcam_mode;
  DCSVideoFrameRate bcam_rate_index;
  int num_cams;
  
  try {

    switch (format) {
    case 0: bcam_format = DCS_Format0; break;
    case 1: bcam_format = DCS_Format1; break;
    case 2: bcam_format = DCS_Format2; break;
    case 6: bcam_format = DCS_Format6; break;
    case 7: bcam_format = DCS_Format7; break;
    default:
      cam_iface_error = 1;
      CAM_IFACE_ERROR_FORMAT("unknown format");
      return NULL;
    };

    switch (mode) {
    case -1: bcam_mode = DCS_IgnoreVideoMode; break;
    case 0: bcam_mode = DCS_Mode0; break;
    case 1: bcam_mode = DCS_Mode1; break;
    case 2: bcam_mode = DCS_Mode2; break;
    case 3: bcam_mode = DCS_Mode3; break;
    case 4: bcam_mode = DCS_Mode4; break;
    case 5: bcam_mode = DCS_Mode5; break;
    case 6: bcam_mode = DCS_Mode6; break;
    case 7: bcam_mode = DCS_Mode7; break;
    default:
      cam_iface_error = 1;
      CAM_IFACE_ERROR_FORMAT("unknown mode");
      return NULL;
    };

    switch (rate_index) {
    case -1: bcam_rate_index = DCS_IgnoreFrameRate; break;
    case 0: bcam_rate_index = DCS_1_875fps; break;
    case 1: bcam_rate_index = DCS_3_75fps; break;
    case 2: bcam_rate_index = DCS_7_5fps; break;
    case 3: bcam_rate_index = DCS_15fps; break;
    case 4: bcam_rate_index = DCS_30fps; break;
    case 5: bcam_rate_index = DCS_60fps; break;
    case 6: bcam_rate_index = DCS_120fps; break;
    case 7: bcam_rate_index = DCS_240fps; break;
    default:
      cam_iface_error = 1;
      CAM_IFACE_ERROR_FORMAT("unknown rate index");
      return NULL;
    };

    out_cr = new CamContext;
    
    // initialize
    out_cr->cam = (void *)NULL;
    out_cr->backend_extras = (void *)NULL;

    this_bcam = new CBcamExtension;

    out_cr->cam = (void *)this_bcam; // keep copy of pointer (in opaque structure)

    num_cams = CBcam::DeviceNames().size();
    if (num_cams < 1) {
      cam_iface_error = -3;
      CAM_IFACE_ERROR_FORMAT("no cameras available");
      return NULL;
    }

    if (device_number >= num_cams) {
      cam_iface_error = -6;
      CAM_IFACE_ERROR_FORMAT("requesting too high camera number");
      return NULL;
    }

    std::list<CString> device_names = CBcam::DeviceNames();
    
    std::list<CString>::iterator i = device_names.begin();
    for (int j=0; j<device_number; j++) i++;
    CString DeviceName = *(i);

    if (bcam_format!=DCS_Format7) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("cam_iface_bcam driver currently only supports Format 7");
      return NULL;
    }

    if (bcam_mode!=DCS_Mode0) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("cam_iface_bcam driver currently only supports Mode 0");
      return NULL;
    }
    this_bcam->Open(DeviceName);
    
    this_bcam->SetVideoMode(bcam_format, bcam_mode);

    this_bcam->TryActivateSmartFeature( Sff::GUID_SF_EXTENDED_DATA_STREAM );
    this_bcam->TryActivateSmartFeature( Sff::GUID_SF_FRAME_COUNTER_STAMP );
    this_bcam->TryActivateSmartFeature( Sff::GUID_SF_DCAMVALUES_STAMP );
    this_bcam->TryActivateSmartFeature( Sff::GUID_SF_CYCLETIME_STAMP );

    // Make sure the camera will run at full speed
    this_bcam->FormatSeven[bcam_mode].Size = this_bcam->FormatSeven[bcam_mode].MaxSize();
    this_bcam->FormatSeven[bcam_mode].BytePerPacket = this_bcam->FormatSeven[bcam_mode].BytePerPacket.Max();
    this_bcam->Shutter.Raw = this_bcam->Shutter.Raw.Min();
    
    // Turn off any test image
    this_bcam->TestImage = TestImage_Disabled; // This is not in the DCAM spec.

    this_backend_extras = new cam_iface_BCAM_backend_extras;
    out_cr->backend_extras = (void *)this_backend_extras; // keep copy
    out_cr->depth = 8; // hack until I figure how to query from Bcam
    
    // Create the image buffers
    ImageSize = this_bcam->FormatSeven[bcam_mode].MaxSize();
    this_backend_extras->ImageBufferSize = this_bcam->FormatSeven[ bcam_mode ].BytePerFrame(); // can be larger than w*h because of extra features
    this_backend_extras->max_width = ImageSize.cx;
    this_backend_extras->max_height = ImageSize.cy;
    this_backend_extras->IsCrcChecked = this_bcam->TryActivateSmartFeature( Sff::GUID_SF_CRC );
    
    this_ppBuffers = new PBYTE[NumImageBuffers];

    for ( int i = 0; i < NumImageBuffers; ++i )
      this_ppBuffers[i] =  new BYTE[this_backend_extras->ImageBufferSize];
    //printf("%s: line %d\n",__FILE__,__LINE__);
    
    this_backend_extras->last_framenumber = -1;
    this_backend_extras->last_timestamp = 0.0;
    this_backend_extras->ppBuffers = this_ppBuffers; // keep copy
    this_backend_extras->NumImageBuffers = NumImageBuffers;

    // Allocate Resources (MaxBuffers, MaxBufferSize)
    this_bcam->AllocateResources(NumImageBuffers, this_backend_extras->ImageBufferSize);
    //printf("%s: line %d\n",__FILE__,__LINE__);

    for(int i=0; i<NumImageBuffers; ++i) {
      this_bcam->GrabImageAsync(this_ppBuffers[i], this_backend_extras->ImageBufferSize, (void*)i, false);
    }
    //printf("%s: line %d\n",__FILE__,__LINE__);

    return out_cr;
    } catch ( BcamException&e ) {
      cam_iface_error = -5;
      strncpy(cam_iface_error_string,e.Description(),CAM_IFACE_MAX_ERROR_LEN);
      return NULL;
    } catch ( exception&e ) {
      cam_iface_error = -6;
      CAM_IFACE_ERROR_FORMAT("unknown C++ exception");
      return NULL;
  }
  
}

void delete_CamContext( CamContext *in_cr ) {
  CBcamExtension * this_bcam = NULL;
  PBYTE* this_ppBuffers;
  cam_iface_BCAM_backend_extras* this_backend_extras;
  //printf("close_camera: line %d\n",__LINE__);
  _dping("");

  if (in_cr==NULL) {
    cam_iface_error = -3;
    CAM_IFACE_ERROR_FORMAT("camera resources null when passed to delete_CamContext.");
    return;
  }
    
  _dping("");
  this_bcam = (CBcamExtension *)in_cr->cam;
  this_backend_extras = (cam_iface_BCAM_backend_extras*)in_cr->backend_extras;
  this_ppBuffers = this_backend_extras->ppBuffers;

  _dping("");
  this_bcam->ContinuousShot=false;
  _dping("");
  this_bcam->FreeResources();
  _dping("");
  delete this_bcam;
  _dping("");
  for ( int i = 0; i < this_backend_extras->NumImageBuffers; ++i ) {
    _dping("");

    delete[] this_ppBuffers[i];
  }
  _dping("");
  delete[] this_ppBuffers;
  _dping("");
  delete this_backend_extras;
  _dping("");
  delete in_cr;
}

void CamContext_start_camera( CamContext *in_cr ) {
  if (in_cr==NULL) {
    cam_iface_error = -3;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }
  CBcamExtension * this_bcam = (CBcamExtension *)in_cr->cam;
  this_bcam->ContinuousShot = true;
}

void CamContext_stop_camera( CamContext *in_cr ) {
  if (in_cr==NULL) {
    cam_iface_error = -3;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }
  CBcamExtension * this_bcam = (CBcamExtension *)in_cr->cam;
  this_bcam->ContinuousShot = false;
}

void CamContext_get_camera_property(CamContext *in_cr, 
				    enum CameraProperty cameraProperty,
				    long* ValueA,
				    long* ValueB,
				    int* Auto ) {
  if (in_cr==NULL) {
    cam_iface_error = -3;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }

  CBcamExtension * this_bcam = (CBcamExtension *)in_cr->cam;
  CBcam::CScalarProperty *prop = NULL;

  switch (cameraProperty) {
  case BRIGHTNESS: prop = &(this_bcam->Brightness); break;
  case GAIN: prop = &(this_bcam->Gain); break;
  case SHUTTER: prop = &(this_bcam->Shutter); break;
  }
  //  long res = prop->Raw();
  //  printf("res: %d\n",res);
  //  (*ValueA) = res;
  *ValueA = prop->Raw();
  *ValueB = 0;
  *Auto = 0;
}

void CamContext_get_camera_property_range(CamContext *in_cr, 
					  enum CameraProperty cameraProperty,
					  int* Present,
					  long* Min,
					  long* Max,
					  long* Default,
					  int* Auto,
					  int* Manual) {
  if (in_cr==NULL) {
    cam_iface_error = -3;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }

  CBcamExtension * this_bcam = (CBcamExtension *)in_cr->cam;
  CBcam::CScalarProperty *prop = NULL;

  switch (cameraProperty) {
  case BRIGHTNESS: prop = &(this_bcam->Brightness); break;
  case GAIN: prop = &(this_bcam->Gain); break;
  case SHUTTER: prop = &(this_bcam->Shutter); break;
  }

  *Present = prop->IsSupported();
  *Min = prop->Raw.Min();
  *Max = prop->Raw.Max();
  *Default = 0;
  *Auto = 0;
  *Manual = 0;
}

void CamContext_set_camera_property(CamContext *in_cr, 
				    enum CameraProperty cameraProperty,
				    long ValueA,
				    long ValueB,
				    int Auto ) {
  if (in_cr==NULL) {
    cam_iface_error = -3;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }

  CBcamExtension * this_bcam = (CBcamExtension *)in_cr->cam;
  CBcam::CScalarProperty *prop = NULL;

  switch (cameraProperty) {
  case BRIGHTNESS: prop = &(this_bcam->Brightness); break;
  case GAIN: prop = &(this_bcam->Gain); break;
  case SHUTTER: prop = &(this_bcam->Shutter); break;
  }
  prop->Raw = ValueA;
}

void CamContext_grab_next_frame_blocking( CamContext *in_cr, unsigned char *out_bytes ) {
  intptr_t stride0;
  if (in_cr==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }

  CBcamExtension * this_bcam = (CBcamExtension *)in_cr->cam;
  CSize ImageSize = this_bcam->FormatSeven[DCS_Mode0].Size();
  stride0=ImageSize.cx;
  CamContext_grab_next_frame_blocking_with_stride(in_cr,out_bytes,stride0);
}

void CamContext_grab_next_frame_blocking_with_stride( CamContext *in_cr, unsigned char *out_bytes, intptr_t stride0 ) {
  int timeout = 3000;

  if (in_cr==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }

  if (out_bytes==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("out_bytes not allocated");
    return;
  }
  
  CBcamExtension * this_bcam = (CBcamExtension *)in_cr->cam;
  cam_iface_BCAM_backend_extras* this_backend_extras = (cam_iface_BCAM_backend_extras*)in_cr->backend_extras;
  PBYTE *this_ppBuffers = this_backend_extras->ppBuffers;
  
  try {
    // Wait for something to arrive at the completion port
    FunctionCode_t FunctionCode;
    unsigned long ErrorCode;
    void *pContext;
    
    this_bcam->WaitForCompletion(&FunctionCode, &ErrorCode, &pContext, timeout);
    
    if ( ErrorCode != 0 ) {
      cam_iface_error = -1;
      sprintf(cam_iface_msg,"WaitForCompletion returned error: %d",ErrorCode);
      strncpy(cam_iface_error_string,cam_iface_msg,CAM_IFACE_MAX_ERROR_LEN);
      return;
    }
    
    if ( FunctionCode == AsyncGrabImage ) {
      this_backend_extras->last_framenumber += 1;
      time_clock(&(this_backend_extras->last_timestamp));

      // Index of the current buffer
      int i = (int)pContext;
      // process the data
      //memcpy(out_bytes,this_ppBuffers[i],this_backend_extras->ImageBufferSize);

      int depth=8; //XXX
      CSize ImageSize = this_bcam->FormatSeven[DCS_Mode0].Size();
      int width = ImageSize.cx;
      int height = ImageSize.cy;

      int wb = width*depth/8;
      for (int row=0; row<height; row++) {
	memcpy((void*)(out_bytes+row*stride0),
	       (const void*)(this_ppBuffers[i]+row*wb),
	       wb);
      }
      
      // re-queue the buffer
      this_bcam->GrabImageAsync(this_ppBuffers[i], this_backend_extras->ImageBufferSize, (void*)i, false);
    }
  } catch ( BcamException&e ) {
    cam_iface_error = -1;
    strncpy(cam_iface_error_string,e.Description(),CAM_IFACE_MAX_ERROR_LEN);
  }
}

extern void CamContext_point_next_frame_blocking(CamContext *in_cr, unsigned char** buf_ptr) {
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("function not yet implemented");
  return;
}

void CamContext_unpoint_frame(CamContext *in_cr) {
  if (in_cr==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }

  CBcamExtension * this_bcam = (CBcamExtension *)in_cr->cam;
  cam_iface_BCAM_backend_extras* this_backend_extras = (cam_iface_BCAM_backend_extras*)in_cr->backend_extras;
  PBYTE *this_ppBuffers = this_backend_extras->ppBuffers;
  
  int i = cam_iface_bufs_in_use.front();
  cam_iface_bufs_in_use.pop_front();

  try {
    // re-queue the buffer
    this_bcam->GrabImageAsync(this_ppBuffers[i], this_backend_extras->ImageBufferSize, (void*)i, false);
  } catch ( exception&e ) {
    cam_iface_error = -6;
    CAM_IFACE_ERROR_FORMAT("unknown C++ exception");
  }
}


extern void CamContext_get_last_timestamp( CamContext *in_cr, 
					   double* timestamp ) {
  if (in_cr==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }
  cam_iface_BCAM_backend_extras* this_backend_extras = (cam_iface_BCAM_backend_extras*)in_cr->backend_extras;
  *timestamp = this_backend_extras->last_timestamp;
}

extern void CamContext_get_last_framenumber( CamContext *in_cr, 
					     long* framenumber ) {
  if (in_cr==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }
  cam_iface_BCAM_backend_extras* this_backend_extras = (cam_iface_BCAM_backend_extras*)in_cr->backend_extras;
  *framenumber = this_backend_extras->last_framenumber;
}

 
void CamContext_get_trigger_source( CamContext *in_cr, 
				    int *exttrig ) {
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("function not yet implemented");
  return;
}
void CamContext_set_trigger_source( CamContext *in_cr, 
				    int exttrig ) {
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("function not yet implemented");
  return;
}
  
void CamContext_get_frame_offset( CamContext *in_cr, 
					 int *left, int *top ){
  CSize Position;
  if (in_cr==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }
  CBcamExtension * this_bcam = (CBcamExtension *)in_cr->cam;
  if (this_bcam==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }
  Position = this_bcam->FormatSeven[DCS_Mode0].Position();
  *left = Position.cx;
  *top = Position.cy;
}
void CamContext_set_frame_offset( CamContext *in_cr, 
					 int left, int top ){
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("function not yet implemented");
  return;
}

void CamContext_get_frame_size( CamContext *in_cr, 
				       int *width, int *height ){
  CSize ImageSize;
  if (in_cr==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }
  CBcamExtension * this_bcam = (CBcamExtension *)in_cr->cam;
  if (this_bcam==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }
  ImageSize = this_bcam->FormatSeven[DCS_Mode0].Size();
  *width = ImageSize.cx;
  *height = ImageSize.cy;
}
void CamContext_set_frame_size( CamContext *in_cr, 
				int width, int height ){
  CHECK_CC(in_cr);
  CBcamExtension *this_bcam = (CBcamExtension *)in_cr->cam;

  CSize Size;
  Size.cx = width;
  Size.cy = height;

  this_bcam->FormatSeven[DCS_Mode0].Size.SetAsync(Size);
}

void CamContext_get_max_frame_size( CamContext *in_cr, 
					   int *width, int *height ){
  cam_iface_BCAM_backend_extras* this_backend_extras;

  if (in_cr==NULL) {
    cam_iface_error = -3;
    CAM_IFACE_ERROR_FORMAT("camera resources null");
    return;
  }
  this_backend_extras = (cam_iface_BCAM_backend_extras*)in_cr->backend_extras;
  *width = this_backend_extras->max_width;
  *height = this_backend_extras->max_height;
}

void CamContext_get_buffer_size( CamContext *in_cr,
					int *size){
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("function not yet implemented");
  return;
}

void CamContext_get_framerate( CamContext *in_cr, 
				      float *framerate ){
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("function not yet implemented");
  return;
}
void CamContext_set_framerate( CamContext *in_cr, 
				      float framerate ){
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("function not yet implemented");
  return;
}

void CamContext_get_num_framebuffers( CamContext *in_cr,
					     int *num_framebuffers ){
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("function not yet implemented");
  return;
}

void CamContext_set_num_framebuffers( CamContext *in_cr, 
					     int num_framebuffers ){
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("function not yet implemented");
  return;
}


#ifdef __cplusplus
}
#endif
