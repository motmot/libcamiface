/* $Id: $ */
#include <windows.h>
#include <WinError.h>
#include "1394Camera.h"

extern "C" {
#include "cam_iface.h"
#include <stdlib.h>
#include <stdio.h>

//#define USEOWNTIME
  // At a certain point, I thought these time functions were causing
  // cmu1394 to die. It seems more likely that numarray does that.
#ifdef USEOWNTIME
#include <time.h>
#include <sys/timeb.h>
// This time stuff from Python/Modules/timemodule.c
#ifndef CLOCKS_PER_SEC
#ifdef CLK_TCK
#define CLOCKS_PER_SEC CLK_TCK
#else
#define CLOCKS_PER_SEC 1000000
#endif
#endif
#endif

/* globals -- allocate space */
int cam_iface_error;
#define CAM_IFACE_MAX_ERROR_LEN 255
char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN];

#define CAM_IFACE_ERROR_FORMAT(m) \
_snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,"%s (%d): %s\n",__FILE__,__LINE__,(m));

#define CAM_IFACE_MAX_CAMS 16

int num_cameras;
double last_timestamps[CAM_IFACE_MAX_CAMS];
long framenumbers[CAM_IFACE_MAX_CAMS];

#ifdef CAM_IFACE_DEBUG2
#define _dping(m)					\
  printf("%s (%d): %s\n",__FILE__,__LINE__,(m))
#else
#define _dping(m)
#endif

#define _dprint_status(m,err_str)				\
  switch ((m)) {						\
  case CAM_SUCCESS:						\
    break;							\
  default:							\
    fprintf(stderr,"%s error %l\n",err_str);			\
    fflush(stderr);						\
    exit(1);							\
    break;							\
  }

void _dprint_controlsize_info(C1394Camera* cam) {
#ifdef CAM_IFACE_DEBUG2
  printf("cam->m_controlSize.m_height = %d\n",cam->m_controlSize.m_height);
  printf("cam->m_controlSize.m_width = %d\n",cam->m_controlSize.m_width);
  printf("cam->m_controlSize.m_maxH = %d\n",cam->m_controlSize.m_maxH);
  printf("cam->m_controlSize.m_maxV = %d\n",cam->m_controlSize.m_maxV);
  printf("cam->m_controlSize.m_unitH = %d\n",cam->m_controlSize.m_unitH);
  printf("cam->m_controlSize.m_unitV = %d\n",cam->m_controlSize.m_unitV);
  printf("cam->m_controlSize.m_top = %d\n",cam->m_controlSize.m_top);
  printf("cam->m_controlSize.m_left = %d\n",cam->m_controlSize.m_left);
  printf("cam->m_controlSize.m_unitHpos = %d\n",cam->m_controlSize.m_unitHpos);
  printf("cam->m_controlSize.m_unitVpos = %d\n",cam->m_controlSize.m_unitVpos);
  printf("cam->m_controlSize.m_bytesPacketMin = %d\n",cam->m_controlSize.m_bytesPacketMin);
  printf("cam->m_controlSize.m_bytesPacketMax = %d\n",cam->m_controlSize.m_bytesPacketMax);
  printf("cam->m_controlSize.m_bytesPacket = %d\n",cam->m_controlSize.m_bytesPacket);
  printf("cam->m_controlSize.m_bytesFrameLow = %d\n",cam->m_controlSize.m_bytesFrameLow);
  printf("cam->m_controlSize.m_packetsFrame = %d\n",cam->m_controlSize.m_packetsFrame);
  printf("cam->m_controlSize.m_colorCode = %d\n",cam->m_controlSize.m_colorCode);
#endif
}

typedef struct cam_iface_CMU1394_backend_extras cam_iface_CMU1394_backend_extras;
struct cam_iface_CMU1394_backend_extras {
  int NumImageBuffers;
};

#ifdef USEOWNTIME
double time_time(){
  struct _timeb t;
  _ftime(&t);
  return (double)t.time + (double)t.millitm * (double)0.001;
}
#endif

const char *cam_iface_get_driver_name() {
  return "cmu1394";
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

extern void cam_iface_startup() {
  C1394Camera cam;
  
  _dping("cam_iface_startup");

  //  _tzset(); // what tha??

  if(cam.CheckLink() != CAM_SUCCESS) {
    num_cameras = 0;
  };
  num_cameras = cam.GetNumberCameras();
}

extern void cam_iface_shutdown() {
  _dping("cam_iface_shutdown");
}

int cam_iface_get_num_cameras() {
  _dping("cam_iface_get_num_cameras");
  return num_cameras;
}

void cam_iface_get_camera_info(int index, Camwire_id *out_camid) {
  _dping("cam_iface_get_camera_info");

  C1394Camera cam;

  if(cam.CheckLink() != CAM_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("CheckLink() failed");
    strcpy(out_camid->vendor,"vendor unknown");
    strcpy(out_camid->model,"model unknown");
    strcpy(out_camid->chip,"chip unknown");
    return;
  }
  if(cam.SelectCamera(index) != CAM_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("SelectCamera(index) failed");
    strcpy(out_camid->vendor,"vendor unknown");
    strcpy(out_camid->model,"model unknown");
    strcpy(out_camid->chip,"chip unknown");
    return;
  }

  /* XXX Workaround CMU 1394 bug hack. CMU1394 copy null-byte terminated strings! */
  memset(cam.m_nameVendor,0,256);
  memset(cam.m_nameModel,0,256);

  cam.InitCamera();

  strcpy(out_camid->vendor,cam.m_nameVendor);
  strcpy(out_camid->model,cam.m_nameModel);
  sprintf(out_camid->chip,"%x",cam.m_UniqueID);
  return;
}

CamContext * new_CamContext( int device_number, int NumImageBuffers,
			     int format, int mode, int rate_index)
{
  _dping("new_CamContext");
  CamContext *out_cr = NULL;
  cam_iface_CMU1394_backend_extras* this_backend_extras;

  out_cr = (CamContext*)malloc(sizeof(CamContext));
  if (!out_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("malloc failed");
    return NULL;
  }

  out_cr->cam = (void *)NULL;
  out_cr->device_number = device_number;
  
  if (device_number >= num_cameras) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("requested too high camera number");
    return NULL;
  }

  if (device_number >= CAM_IFACE_MAX_CAMS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("requested too high camera number");
    return NULL;
  }

  C1394Camera* cam;
  cam = new C1394Camera;
  out_cr->cam = (void*)cam;

  if(cam->CheckLink() != CAM_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("CheckLink() failed");
    return NULL;
  }
  if(cam->SelectCamera(device_number) != CAM_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("SelectCamera(device_number) failed");
    return NULL;
  }
  
  _dping("");
  //_dprint_status(cam->InitCamera(),"InitCamera()");
  cam->InitCamera();
  _dping("");
  cam->InquireControlRegisters();
  _dping("");
  cam->StatusControlRegisters();
  _dping("");

  int mode_set_ok=0;
  if (format==7) {
    _dping("");
    //_dprint_status(cam->SetVideoFormat(format),"cam->SetVideoFormat(format)");
    cam->SetVideoFormat(format);
    fprintf(stderr,""); // this is necessary for program to work?!?

    _dping("");
    if (cam->m_controlSize.ModeSupported(mode)) {
      // pick an available partial scan mode
      for (int i=0; i<8; i++)
	if (cam->m_controlSize.ModeSupported(i))
	  cam->SetVideoMode(i);
      mode_set_ok=1;
      _dping("");
    }
  } else if (cam->m_videoFlags[format][mode][rate_index]) {
    cam->SetVideoFormat(format);
    cam->SetVideoMode(mode);
    cam->SetVideoFrameRate(rate_index);
    mode_set_ok=1;
  } 
  //printf("format: %d, mode: %d\n",cam->GetVideoFormat(),cam->GetVideoMode());

  if (mode_set_ok==0) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("video mode not set (does camera support it?)");
    return NULL;    
  }

  /*
  // set initial format, mode, and rate...
  for (int format=0; format<3; format++)
    for (int mode=0; mode<8; mode++)
      for (int rate=0; rate<6; rate++)
	if (cam->m_videoFlags[format][mode][rate])
	  {
	    cam->SetVideoFormat(format);
	    cam->SetVideoMode(mode);
	    cam->SetVideoFrameRate(rate);
	    final_format = format;
	    final_mode = mode;
	    final_rate = rate;
	  }
  */
  _dping("");

  /*
  out_cr->max_width = cam->m_width;
  out_cr->max_height = cam->m_height;
  */

  /*
  if (format==7) {
    out_cr->roi_left = cam->m_controlSize.m_left/cam->m_controlSize.m_unitHpos;
    out_cr->roi_top = cam->m_controlSize.m_top/cam->m_controlSize.m_unitVpos;
  } else {
    out_cr->roi_left = 0;
    out_cr->roi_top = 0;
  }

  out_cr->roi_width = cam->m_width;
  out_cr->roi_height = cam->m_height;

  */
  _dping("");  
  //printf("format = %d\n",format);
  if (format==0) {
    if (mode==0) {
      out_cr->coding = CAM_IFACE_YUV444;
    } else if (mode==1) {
      out_cr->coding = CAM_IFACE_YUV422;
    } else if (mode==2) {
      out_cr->coding = CAM_IFACE_YUV411;
    } else if (mode==3) {
      out_cr->coding = CAM_IFACE_YUV422;
    } else if (mode==4) {
      out_cr->coding = CAM_IFACE_RGB8;
    } else if (mode==5) {
      out_cr->coding = CAM_IFACE_MONO8;
    } else {
      out_cr->coding = CAM_IFACE_UNKNOWN;
    }
  } else if (format==7) {
    switch (cam->m_controlSize.m_colorCode) {
    case 0: out_cr->coding = CAM_IFACE_MONO8; break;
    case 1: out_cr->coding = CAM_IFACE_YUV411; break;
    case 2: out_cr->coding = CAM_IFACE_YUV422; break;
    case 3: out_cr->coding = CAM_IFACE_YUV444; break;
    case 4: out_cr->coding = CAM_IFACE_RGB8; break;
    case 5: out_cr->coding = CAM_IFACE_MONO16; break;
    case 6: out_cr->coding = CAM_IFACE_RGB16; break;
    case 7: out_cr->coding = CAM_IFACE_MONO16S; break;
    case 8: out_cr->coding = CAM_IFACE_RGB16S; break;
    case 9: out_cr->coding = CAM_IFACE_RAW8; break;
    case 10: out_cr->coding = CAM_IFACE_RAW16; break;
    default: out_cr->coding = CAM_IFACE_UNKNOWN; break;
    }
  } else {
    out_cr->coding = CAM_IFACE_UNKNOWN;
  }

  _dping("");  
  switch (out_cr->coding) {
  case CAM_IFACE_YUV422: out_cr->depth=16; break;
  case CAM_IFACE_MONO8: out_cr->depth=8; break;
  case CAM_IFACE_RGB8: out_cr->depth=24; break;
  default: 
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("unknown depth for coding");
    return NULL;
  }

  if (format==7) {
    // WARNING: these 2 lines are needed for format 7 to prevent computer shutdown
    cam->m_controlSize.Inquire();
    cam->m_controlSize.Status();
  }

  this_backend_extras = new cam_iface_CMU1394_backend_extras;
  out_cr->backend_extras = (void *)this_backend_extras; // keep copy
  this_backend_extras->NumImageBuffers = NumImageBuffers;

  return out_cr;
}

void delete_CamContext(CamContext *in_cr) {
  _dping("delete_CamContext");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  C1394Camera* cam;
  cam_iface_CMU1394_backend_extras *this_backend_extras;

  cam = (C1394Camera*)in_cr->cam;
  delete cam;
  in_cr->cam = (void*)NULL;

  this_backend_extras = (cam_iface_CMU1394_backend_extras *)in_cr->backend_extras;
  delete this_backend_extras;
  in_cr->backend_extras = (void*)NULL;

  free(in_cr);
  in_cr = (CamContext*)NULL;
}

void CamContext_start_camera( CamContext *in_cr ) {
  int FrameTimeout;
  int Flags;
  cam_iface_CMU1394_backend_extras * this_backend_extras;

  _dping("CamContext_start_camera");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  C1394Camera* cam;
  cam = (C1394Camera*)in_cr->cam;

  FrameTimeout=10; // Not currently implemented in CMU1394
  Flags=0; // "Reserved, set to zero"

  this_backend_extras = (cam_iface_CMU1394_backend_extras *)in_cr->backend_extras;

  if (cam->StartImageAcquisitionEx(this_backend_extras->NumImageBuffers,FrameTimeout,Flags)) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("cam->StartImageAcquisitionEx(...) failed");
    return;
  }
}

void CamContext_stop_camera( CamContext *in_cr ) {
  _dping("CamContext_stop_camera");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  C1394Camera* cam;
  cam = (C1394Camera*)in_cr->cam;
  if (cam->StopImageAcquisition()) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("cam->StopImageAcquisition() failed");
    return;
  }
}

// helper function to return a pointer to the corresponding C1394CameraControl
C1394CameraControl* CamContext_getCameraControl(C1394Camera* cam, 
		CameraProperty camProperty)
{
  _dping("CamContext_getCameraControl");
	switch(camProperty) {
		case BRIGHTNESS: 
			return &(cam->m_controlBrightness);
		case SHUTTER: 
			return &(cam->m_controlShutter);
		case GAIN: 
			return &(cam->m_controlGain);
		default:
			cam_iface_error = -1;
			CAM_IFACE_ERROR_FORMAT("invalid property type");
			return &(cam->m_controlBrightness);
	}
}

void CamContext_get_camera_property(CamContext *in_cr,
				    enum CameraProperty cameraProperty,
				    long* ValueA,
				    long* ValueB,
				    int* Auto ) {
  _dping("CamContext_get_camera_property");
  unsigned long status;
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  C1394Camera* cam = (C1394Camera*)in_cr->cam;
  C1394CameraControl* control = CamContext_getCameraControl(cam, 
							    cameraProperty);

  status = control->Status();

  *ValueA = control->m_value1;
  *ValueB = control->m_value2;
  *Auto = control->m_statusAutoManual;
}

void CamContext_get_camera_property_range(CamContext *in_cr, 
					  enum CameraProperty cameraProperty,
					  int* Present,
					  long* Min,
					  long* Max,
					  long* Default,
					  int* Auto,
					  int* Manual) {
  _dping("CamContext_get_camera_property_range");
  unsigned long status;
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  C1394Camera* cam = (C1394Camera*)in_cr->cam;
  C1394CameraControl* control = CamContext_getCameraControl(cam, 
							    cameraProperty);

  status = control->Inquire();
  
  *Present = control->m_present;
  *Min = control->m_min;
  *Max = control->m_max;
  *Default = 0; // cmu1394 doesn't seem to support default values 
  *Auto = control->m_auto;
  *Manual = control->m_manual;
}

void CamContext_set_camera_property(CamContext *in_cr, 
				    enum CameraProperty cameraProperty,
				    long ValueA,
				    long ValueB,
				    int Auto ) {
  _dping("CamContext_set_camera_property");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  C1394Camera* cam = (C1394Camera*)in_cr->cam;
  C1394CameraControl* control = CamContext_getCameraControl(cam, 
							    cameraProperty);
  control->m_value1 = (unsigned short)ValueA;
  control->m_value2 = (unsigned short)ValueB;
  //control->m_statusAutoManual = Auto;
  if(control->SetAutoMode(Auto) != CAM_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("SetAutoMode() failed");
    return;
  }
  if(control->SetValues() != CAM_SUCCESS) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("SetValues() failed");
    return;
  }
}

void CamContext_grab_next_frame_blocking( CamContext *in_cr, unsigned char *out_bytes ) {
  _dping("CamContext_grab_next_frame_blocking");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  C1394Camera* cam = (C1394Camera*)in_cr->cam;
  int n_dropped_frames;
  int result;

  result = cam->AcquireImageEx(TRUE,&n_dropped_frames);  // don't drop stale frames
  //result = cam->AcquireImage();
  if (result) {
    cam_iface_error = -1;
    switch (result) {
    case CAM_ERROR_NOT_INITIALIZED: 
      CAM_IFACE_ERROR_FORMAT("cam->AcquireImageEx() CAM_ERROR_NOT_INITIALIZED"); 
      break;
    case CAM_ERROR_FRAME_TIMEOUT:
      CAM_IFACE_ERROR_FORMAT("cam->AcquireImageEx() CAM_ERROR_FRAME_TIMEOUT"); 
      break;
    case ERROR_INVALID_PARAMETER:
      CAM_IFACE_ERROR_FORMAT("cam->AcquireImageEx() ERROR_INVALID_PARAMETER"); 
      break;
    case ERROR_NO_SYSTEM_RESOURCES:
      CAM_IFACE_ERROR_FORMAT("cam->AcquireImageEx() ERROR_NO_SYSTEM_RESOURCES"); 
      break;
    case -1: 
      CAM_IFACE_ERROR_FORMAT("cam->AcquireImageEx() Error on IOTCTL_ISOCH_LISTEN"); 
      break;
    default:
      fprintf(stderr,"error return %d\n",result);
      CAM_IFACE_ERROR_FORMAT("cam->AcquireImageEx(TRUE,&n_dropped_frames) undefined failure");
    }
    return;
  }
  
#ifdef USEOWNTIME
  last_timestamps[in_cr->device_number] = time_time();
#else
  last_timestamps[in_cr->device_number] = 0.0;
#endif
  //memcpy(out_bytes,cam->m_pData,in_cr->buffer_size);
  //  int buffer_size = cam->m_width*cam->m_height; // XXX will fail on other than MONO8
  int buffer_size;
  CamContext_get_buffer_size(in_cr,&buffer_size);
  memcpy(out_bytes,cam->m_pData,buffer_size);
  
  ++framenumbers[in_cr->device_number]; // increment frame count
  if (n_dropped_frames != 0) {
    framenumbers[in_cr->device_number] += n_dropped_frames;
    cam_iface_error = CAM_IFACE_BUFFER_OVERFLOW_ERROR;
    CAM_IFACE_ERROR_FORMAT("Buffers overflowed.");
    return;
  }

}

void CamContext_point_next_frame_blocking( CamContext *in_cr, unsigned char **buf_ptr){
  _dping("CamContext_point_next_frame_blocking");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("not implemented");
  return;

}

void CamContext_unpoint_frame( CamContext *in_cr){
  _dping("CamContext_unpoint_frame");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("not implemented");
  return;

}

void CamContext_get_last_timestamp( CamContext *in_cr, double* timestamp ) {
  _dping("CamContext_get_last_timestamp");

  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  *timestamp = last_timestamps[in_cr->device_number];

}

void CamContext_get_last_framenumber( CamContext *in_cr, long* framenumber ){
  _dping("CamContext_get_last_framenumber");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  *framenumber = framenumbers[in_cr->device_number];

}

void CamContext_get_trigger_source( CamContext *in_cr, 
				    int *exttrig ) {
  
  _dping("CamContext_get_trigger_source");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  C1394Camera* cam;
  cam = (C1394Camera*)in_cr->cam;

  *exttrig = cam->m_controlTrigger.m_statusOnOff;

}

void CamContext_set_trigger_source( CamContext *in_cr, 
				    int exttrig )  {  
  _dping("CamContext_set_trigger_source");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  C1394Camera* cam;
  cam = (C1394Camera*)in_cr->cam;

  cam->m_controlTrigger.TurnOn(exttrig);
}
  
void CamContext_get_frame_offset( CamContext *in_cr, 
				  int *left, int *top ) {
  _dping("CamContext_get_frame_offset");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  C1394Camera* cam;
  cam = (C1394Camera*)in_cr->cam;

  /*
  *left = cam->m_controlSize.m_left;
  *top = cam->m_controlSize.m_top;
  */
  
  if (cam->GetVideoFormat() == 7) {
    *left = cam->m_controlSize.m_left/cam->m_controlSize.m_unitHpos;
    *top = cam->m_controlSize.m_top/cam->m_controlSize.m_unitVpos;
  } else {
    *left = 0;
    *top = 0;
  }
}

void CamContext_set_frame_offset( CamContext *in_cr, 
				  int left, int top ) {
  _dping("CamContext_set_frame_offset");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  
  C1394Camera* cam;
  cam = (C1394Camera*)in_cr->cam;

  if (cam->GetVideoFormat() == 7) {
    if (cam->m_controlSize.SetPosition( left*cam->m_controlSize.m_unitHpos, 
					top*cam->m_controlSize.m_unitVpos) ) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("Error calling SetPosition()");
      return;
    }
    /*
    in_cr->roi_left = left;
    in_cr->roi_top = top;
    */
  } else {
    if (!(left==0 && top == 0)) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("Cannot set position unless in format 7");
      return;
    }
  }

}


void CamContext_get_frame_size( CamContext *in_cr, 
				int *width, int *height ) {
  _dping("CamContext_get_frame_size");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }

  C1394Camera* cam;
  cam = (C1394Camera*)in_cr->cam;

  if (cam->GetVideoFormat() == 7) {
    *width = cam->m_controlSize.m_width/cam->m_controlSize.m_unitH;
    *height = cam->m_controlSize.m_height/cam->m_controlSize.m_unitV;
  } else {
    *width = cam->m_width;
    *height = cam->m_width;
  }
    /*
     *width = cam->m_controlSize.m_width;
     *height = cam->m_controlSize.m_height;
     */
}

void CamContext_set_frame_size( CamContext *in_cr, 
				int width, int height ) {
  _dping("CamContext_set_frame_size");
  
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  C1394Camera* cam;
  cam = (C1394Camera*)in_cr->cam;
  
  if (cam->GetVideoFormat() == 7) {
    if (cam->m_controlSize.SetSize( width*cam->m_controlSize.m_unitH, 
				    height*cam->m_controlSize.m_unitV) ) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("Error calling SetSize()");
      return;
    }
    /*
    in_cr->roi_width = width;
    in_cr->roi_height = height;
    */
  } else {
    if (!(width==cam->m_width && height == cam->m_height)) {
      cam_iface_error = -1;
      CAM_IFACE_ERROR_FORMAT("Cannot change width, height unless in format 7");
      return;
    }
  }
}

void CamContext_get_max_frame_size( CamContext *in_cr, 
				    int *width, int *height ) {
  _dping("CamContext_get_max_frame_size");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  C1394Camera* cam;
  cam = (C1394Camera*)in_cr->cam;
  if (cam->GetVideoFormat() == 7) {
    *width = cam->m_controlSize.m_maxH/cam->m_controlSize.m_unitH;
    *height = cam->m_controlSize.m_maxV/cam->m_controlSize.m_unitV;
  } else {
    *width = cam->m_width;
    *height = cam->m_height;
  }
}

void CamContext_get_buffer_size( CamContext *in_cr, 
				 int *size ) {
  _dping("CamContext_get_buffer_size");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  C1394Camera* cam = (C1394Camera*)in_cr->cam;
  *size = cam->m_width*cam->m_height; // XXX fixme: will fail on other than MONO8
}

void CamContext_get_framerate( CamContext *in_cr, 
			       float *framerate ) {
  _dping("CamContext_get_framerate");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("not implemented");
  return;
}

void CamContext_set_framerate( CamContext *in_cr, 
			       float framerate ) {
  _dping("CamContext_set_framerate");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("not implemented");
  return;
}

void CamContext_get_num_framebuffers( CamContext *in_cr, 
				      int *num_framebuffers ) {
  _dping("CamContext_get_num_framebuffers");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("not implemented");
  return;

}

void CamContext_set_num_framebuffers( CamContext *in_cr, 
				      int num_framebuffers ) {
  _dping("CamContext_set_num_framebuffers");
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");
    return;
  }
  cam_iface_error = -1;
  CAM_IFACE_ERROR_FORMAT("not implemented");
  return;
}

} // closes: extern "C"

