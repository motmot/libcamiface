/* $Id: $ */
#include <CyXMLDocument.h>
#include <CyConfig.h>
#include <CyGrabber.h>
#include <CyCameraRegistry.h>
#include <CyCameraInterface.h>
#include <CyImageBuffer.h>
#include <CyDeviceFinder.h>

#include "cam_iface.h"

#include <stdlib.h>
#include <stdio.h>

#define BRIGHTNESS_CH1 0
#define BRIGHTNESS_CH2 1
#define GAIN 2
#define SHUTTER 3
#define NUM_PROPS 4

/* globals -- allocate space */
int num_cameras; // number of cameras
int cam_iface_error;
#define CAM_IFACE_MAX_ERROR_LEN 255
char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN];
CyXMLDocument lDocument( "IPX-2M30-G.xml" ); // XML docment containing configuration data for camera
CyAdapterID lAdapterID; // network adapter identification class
CyConfig lConfig; // camera name/address configure class
CyGrabber lGrabber; // configure/control grabber class

// contains information specific to the ImperX backend
// (buffer, timestamp, imageid)

typedef struct cam_iface_ImperX_backend_extras cam_iface_ImperX_backend_extras;
struct cam_iface_ImperX_backend_extras {
  CyImageBuffer* lBuffer;
  unsigned long last_timestamp;
  unsigned long last_imageID;
};

#ifdef CAM_IFACE_DEBUG
#define dping(m)					\
  printf("%s (%d): %s\n",__FILE__,__LINE__,(m));
#else
#define dping(m)
#endif

#define CAM_IFACE_ERROR_FORMAT(m) \
  _snprintf_s(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,CAM_IFACE_MAX_ERROR_LEN,"%s (%d): %s\n",__FILE__,__LINE__,(m));

#define CHECK_CC(m)							\
  if (!(m)) {								\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");	\
    return;								\
  }

#define CHKIMPERX(m)							\
  if ((m) != CY_RESULT_OK) {						\
    CyResult m_copy=(m);						\
    cam_iface_error = -1;						\
    _snprintf_s(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,		\
		CAM_IFACE_MAX_ERROR_LEN,				\
		"%s (%d): imperx err %d: %s\n",__FILE__,__LINE__,	\
		(m),							\
		CoyoteStatusMessage(m_copy));				\
    return;								\
  }

#define NOT_IMPLEMENTED							\
  cam_iface_error = -1;							\
  CAM_IFACE_ERROR_FORMAT("function not implemented.");			\
  return;

#define ROI_NOT_IMPLEMENTED						\
  cam_iface_error = -1;							\
  CAM_IFACE_ERROR_FORMAT("adjustable Region-Of-Interest not implemented."); \
  return;

#define SAFE_CAMERA_DEREF						\
  CyCameraInterface* lCamera = (CyCameraInterface*)(in_cr->cam);	\
  cam_iface_ImperX_backend_extras* backend_extras = (cam_iface_ImperX_backend_extras*)(in_cr->backend_extras); \
  CyImageBuffer* lBuffer = backend_extras->lBuffer;

#define CAM_IFACE_CHECK_DEVICE_NUMBER(m)				\
  if ( ((m)<0) | ((m)>=num_cameras) ) {					\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("invalid device_number");			\
    return;								\
  }

#define CAM_IFACE_CHECK_MODE_NUMBER(m)			\
  if ((m)!=0) {						\
    cam_iface_error = -1;				\
    CAM_IFACE_ERROR_FORMAT("only mode 0 exists");	\
    return;						\
  }

#define TRIGGER_OFF 0
#define TRIGGER_EXTERNAL_STANDARD 1
#define TRIGGER_EXTERNAL_FAST 2
#define TRIGGER_EXTERNAL_DOUBLE 3
#define TRIGGER_CC_STANDARD 4
#define TRIGGER_CC_FAST 5
#define TRIGGER_CC_DOUBLE 6

#define NUM_TRIGGER_MODES 7
#define CAM_IFACE_CHECK_TRIGGER_MODE_NUMBER(m)				\
  if ( ((m)<0) | ((m)>=NUM_TRIGGER_MODES) ) {				\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("invalid trigger mode");			\
    return;								\
  }

const char *cam_iface_get_driver_name() {
  return "ImperX LYNX GigE";
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
  CyAdapterID::GetIdentifier( CyConfig::MODE_DRV, 0, lAdapterID );
  dping("cam_iface_startup");								\
  if ( lDocument.LoadDocument() != CY_RESULT_OK ) {
    cam_iface_error = -1;						
    CAM_IFACE_ERROR_FORMAT("could not load file");
    return;
  }
  if ( lConfig.LoadFromXML( lDocument ) != CY_RESULT_OK ) {
    cam_iface_error = -1;						
    CAM_IFACE_ERROR_FORMAT("could not load from XML");
    return;
  }
  num_cameras = 1;
}

extern void cam_iface_shutdown() {
  dping("cam_iface_shutdown");								\
}


int cam_iface_get_num_cameras() {
  return num_cameras;
}

void cam_iface_get_camera_info(int index, Camwire_id *out_camid) {
  _snprintf_s(out_camid->vendor,
	      CAMWIRE_ID_MAX_CHARS,
	      CAMWIRE_ID_MAX_CHARS,
	      "ImperX");
  _snprintf_s(out_camid->model,
	      CAMWIRE_ID_MAX_CHARS,
	      CAMWIRE_ID_MAX_CHARS,
	      "2M30-G");
  _snprintf_s(out_camid->chip,
	      CAMWIRE_ID_MAX_CHARS,
	      CAMWIRE_ID_MAX_CHARS,
	      "KAI-2020");
}

// Get the number of video modes possible (e.g. 640x480 x MONO8 or 1600x1200xYUV422)
void cam_iface_get_num_modes(int device_number, int *num_modes) {
  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  *num_modes = 1;
}

// Describe the video mode possible (returns string "640x480xMONO8")
void cam_iface_get_mode_string(int device_number,
			       int mode_number,
			       char* mode_string, //output parameter
			       int mode_string_maxlen) {
  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  CAM_IFACE_CHECK_MODE_NUMBER(mode_number);
  _snprintf_s(mode_string,mode_string_maxlen,mode_string_maxlen,"default mode");
}
						
CamContext *new_CamContext(int device_number,int NumImageBuffers,
			   int mode_number) {
  CamContext *out_cr = NULL;

  if ( ((device_number)<0) | ((device_number)>=num_cameras) ) {		
    cam_iface_error = -1;						
    CAM_IFACE_ERROR_FORMAT("invalid device_number");			
    return NULL;								
  }
  if ((mode_number)!=0) {
    cam_iface_error = -1;				
    CAM_IFACE_ERROR_FORMAT("only mode 0 exists in this backend");	
    return NULL;						
  }

  out_cr = (CamContext*)malloc(sizeof(CamContext));
  if (!out_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("malloc failed");
    return NULL;
  }

// this section of code redundant?
//   if (device_number >= num_cameras) {
//     cam_iface_error = -1;
//     CAM_IFACE_ERROR_FORMAT("requested too high camera number");
//     return NULL;
//   }
  lConfig.SetIndex( 0 );

  if ( lGrabber.Connect( lConfig ) != CY_RESULT_OK ) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("grabber could not connect to camera");
    return NULL;
  }
  
  char lCameraType[128];
  lConfig.GetDeviceType( lCameraType, sizeof( lCameraType ) );
  
  // Find the camera in the registry
  CyCameraRegistry lReg;
  if ( lReg.FindCamera( lCameraType ) != CY_RESULT_OK ) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("could not find camera type in registry");
    return NULL;
  }

  

  /* initialize */
  out_cr->cam = (void *)NULL;
  out_cr->backend_extras = (void *)NULL;

  CyCameraInterface* lCamera = 0;

  if ( lReg.CreateCamera( &lCamera, &lGrabber ) != CY_RESULT_OK ) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("could not create camera");
    return NULL;
  }

  if ( lCamera->LoadFromXML( lDocument ) != CY_RESULT_OK ) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("could not set camera settings from XML");
    delete lCamera;
    return NULL;
  };

  
  if ( lCamera->UpdateToCamera() != CY_RESULT_OK ) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("could send the settings to iPORT and the camera");
    delete lCamera;
    return NULL;
  };

  

  // Get some information from the camera
  __int64 lSizeX, lSizeY, lDecimationX, lDecimationY, lBinningX, lBinningY;
  
  CyPixelTypeID lPixelType;
  lCamera->GetParameter( CY_CAMERA_PARAM_SIZE_X, lSizeX );
  //printf("lSizeX = %hu.\n",lSizeX);
  lCamera->GetParameter( CY_CAMERA_PARAM_SIZE_Y, lSizeY  );
  //printf("lSizeY = %hu.\n",lSizeY);  
  lCamera->GetParameter( CY_CAMERA_PARAM_DECIMATION_X, lDecimationX );
  //printf("lDecimationX = %hu.\n",lDecimationX);
  lCamera->GetParameter( CY_CAMERA_PARAM_DECIMATION_Y, lDecimationY );
  //printf("lDecimationY = %hu.\n",lDecimationY);
  lCamera->GetParameter( CY_CAMERA_PARAM_BINNING_X, lBinningX );
  //printf("lBinningX = %hu.\n",lBinningX);
  lCamera->GetParameter( CY_CAMERA_PARAM_BINNING_Y, lBinningY );
  //printf("lBinningY = %hu.\n",lBinningY);
  lCamera->GetPixelType( lPixelType );

  if (lPixelType != CyGrayscale8::ID) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("only CyGrayscale8 pixel type currently supported");
    delete lCamera;
    return NULL;
  }

  out_cr->coding = CAM_IFACE_MONO8;
  out_cr->depth = 8;
  out_cr->device_number = device_number;

  if ( ( lDecimationX != 0 ) && ( lDecimationY != 0 ) && ( lBinningX != 0 ) && ( lBinningY != 0 ) ) {
    lSizeX = (( lSizeX / lBinningX ) + (( lSizeX % lBinningX ) ? 1 : 0));
    lSizeX = (( lSizeX / lDecimationX ) + (( lSizeX % lDecimationX ) ? 1 : 0));
    lSizeY = (( lSizeY / lBinningY ) + (( lSizeY % lBinningY ) ? 1 : 0));
    lSizeY = (( lSizeY / lDecimationY ) + (( lSizeY % lDecimationY ) ? 1 : 0));
  }
  
  cam_iface_ImperX_backend_extras* this_backend_extras = new cam_iface_ImperX_backend_extras;
  out_cr->backend_extras = (void *)this_backend_extras; // keep copy

  // Create the buffer.
  this_backend_extras->lBuffer = new CyImageBuffer( lSizeX, lSizeY, lPixelType );
  this_backend_extras->last_timestamp = 0;
  this_backend_extras->last_imageID = 0;

  out_cr->cam = (void*)lCamera;
  return out_cr;
}

void delete_CamContext(CamContext *in_cr) {
  CHECK_CC(in_cr);

  cam_iface_ImperX_backend_extras* this_backend_extras = (cam_iface_ImperX_backend_extras*)(in_cr->backend_extras);
  delete this_backend_extras->lBuffer;
  delete this_backend_extras;

  CyCameraInterface* lCamera = (CyCameraInterface*)(in_cr->cam);
  delete lCamera;
   
  free(in_cr);
}

void CamContext_start_camera( CamContext *in_cr ) {
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;

  // start the grabbing and displaying
  if ( !lCamera->IsStarted( CyChannel( 0 ) ) ) {
    lCamera->StartGrabbing( CyChannel( 0 ), *lBuffer);
  }
}

void CamContext_stop_camera( CamContext *in_cr ) {
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  
  // stop the grabbing and displaying
  if ( lCamera->IsStarted( CyChannel( 0 ) ) ) {
    lCamera->StopGrabbing( CyChannel( 0 ) );
  }
}

void CamContext_get_num_camera_properties(CamContext *in_cr, 
					  int* num_properties) {
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  *num_properties = NUM_PROPS;
}


void CamContext_get_camera_property_info(CamContext *in_cr,
					 int property_number,
					 CameraPropertyInfo *info) {
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  if ((property_number <0 ) | (property_number >= NUM_PROPS)) {
    cam_iface_error = -1;						
    CAM_IFACE_ERROR_FORMAT("invalid property number");
    return;
  }

  switch (property_number) {
  case BRIGHTNESS_CH1:
    __int64 offset_ch1_min, offset_ch1_max;
    CHKIMPERX(lCamera->GetParameterRange("Channel 1 Offset",offset_ch1_min,offset_ch1_max));

    info->name="brightness_ch1";
    info->is_present = 1;
    info->min_value = offset_ch1_min; // type will be cast from __int64 to long
    info->max_value = offset_ch1_max; // type will be case from __int64 to long

    info->has_auto_mode = 0;
    info->has_manual_mode = 1;
    info->is_scaled_quantity = 0;
    
    info->scaled_unit_name = NULL;
    break;
  case BRIGHTNESS_CH2:
    __int64 offset_ch2_min, offset_ch2_max;
    CHKIMPERX(lCamera->GetParameterRange("Channel 2 Offset",offset_ch2_min,offset_ch2_max));

    info->name="brightness_ch2";
    info->is_present = 1;
    info->min_value = offset_ch2_min; // type will be cast from __int64 to long
    info->max_value = offset_ch2_max; // type will be case from __int64 to long

    info->has_auto_mode = 0;
    info->has_manual_mode = 1;
    info->is_scaled_quantity = 0;
    
    info->scaled_unit_name = NULL;
    break;
  case SHUTTER:
    __int64 shutter_min, shutter_max;
    CHKIMPERX(lCamera->GetParameterRange("Shutter Time",shutter_min,shutter_max));
    info->name="shutter";
    info->is_present = 1;
    info->min_value = shutter_min;
    info->max_value = shutter_max;

    info->has_auto_mode = 0;
    info->has_manual_mode = 1;
    info->is_scaled_quantity = 0;
    
    info->scaled_unit_name = NULL;
    break;
  case GAIN:
    double gain_ch1_min, gain_ch1_max, gain_ch2_min, gain_ch2_max;
    CHKIMPERX(lCamera->GetParameterRange("Channel 1 Gain",gain_ch1_min,gain_ch1_max));
    CHKIMPERX(lCamera->GetParameterRange("Channel 2 Gain",gain_ch2_min,gain_ch2_max));
    if (gain_ch1_min!=gain_ch2_min){
      printf("Warning: The minimum gains on each channel do not match. Returning Channel 1 value.\n");
    }
    if (gain_ch1_max!=gain_ch2_max){
      printf("Warning: The maximum gains on each channel do not match. Returning Channel 1 value.\n");
    }   

    info->name="gain";
    info->is_present = 1;
    info->min_value = gain_ch1_min; // type will be cast from __int64 to long
    info->max_value = gain_ch1_max; // type will be case from __int64 to long

    info->has_auto_mode = 0;
    info->has_manual_mode = 1;
    info->is_scaled_quantity = 0;
    
    info->scaled_unit_name = NULL;
    break;
  default:
    cam_iface_error = -1;						
    CAM_IFACE_ERROR_FORMAT("Invalid property number");
    return;
  }
}
  
void CamContext_get_camera_property(CamContext *in_cr,
				    int property_number,
				    long* Value,
				    int* Auto) {
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;

  *Auto = 0;

  switch (property_number) {
  case BRIGHTNESS_CH1:
    unsigned long offset_ch1; // offset values between 0 and 255 
    CHKIMPERX(lCamera->GetParameter("Channel 1 Offset", offset_ch1));
    *Value = (long) offset_ch1;
    break;
  case BRIGHTNESS_CH2:
    unsigned long offset_ch2; // offset values between 0 and 255 
    CHKIMPERX(lCamera->GetParameter("Channel 2 Offset", offset_ch2));
    *Value = (long) offset_ch2;
    break;
  case SHUTTER:
    unsigned long shutter; // exposure time between 50 and 500000 (microseconds)
    CHKIMPERX(lCamera->GetParameter("Shutter Time",shutter));
    *Value = (long) shutter;
    //printf("Exposure time = %lu microseconds.\n", shutter);
    break;
  case GAIN:
    float gain_ch1, gain_ch2; // gain values between 6.0 and 40.0 dB

    CHKIMPERX(lCamera->GetParameter("Channel 1 Gain", gain_ch1));
    //printf("Channel 1 gain = %5.2f dB.\n",gain_ch1);
    CHKIMPERX(lCamera->GetParameter("Channel 2 Gain", gain_ch2));
    //printf("Channel 2 gain = %5.2f dB.\n", gain_ch2);
    if (gain_ch1 != gain_ch2) {
      printf("Warning: Channel 1 Gain and Channel 2 Gain not equal. Returning Channel 1");
    }
    *Value = (long) gain_ch1;
    break;
  default:
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("unknown camera property");
    return;
    break;
  }
}

void CamContext_set_camera_property(CamContext *in_cr,
				    int property_number,
				    long Value,
				    int Auto) {

  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  
  switch (property_number){
  case BRIGHTNESS_CH1:
    __int64 offset_ch1;
    offset_ch1 = (long) Value;
    if (offset_ch1 < 0 || offset_ch1 > 255){
      //printf("Offset value specified is out of range. Resetting to zero.\n");
      offset_ch1 = 0;
    }
    //printf("Offset value set to: %lu.\n",offset_ch1);
    CHKIMPERX(lCamera->SetParameter("Channel 1 Offset", offset_ch1));
    CHKIMPERX(lCamera->UpdateToCamera());
    break;
  case BRIGHTNESS_CH2:
    __int64 offset_ch2;
    offset_ch2 = (long) Value;
    if (offset_ch2 < 0 || offset_ch2 > 255){
      //printf("Offset value specified is out of range. Resetting to zero.\n");
      offset_ch2 = 0;
    }
    //printf("Offset value set to: %lu.\n",offset_ch2);
    CHKIMPERX(lCamera->SetParameter("Channel 2 Offset", offset_ch2));
    CHKIMPERX(lCamera->UpdateToCamera());
    break;

  case SHUTTER:
    __int64 shutter;
    shutter = (long) Value;
    if (shutter < 50 || shutter > 500000){
      //printf("Shutter time specified is out of range. Resetting to 50 microseconds.\n");
      shutter = 50;
    }
    //printf("Shutter value set to: %lu microseconds.\n",shutter);
    CHKIMPERX(lCamera->SetParameter("Shutter Time", shutter)); 
    CHKIMPERX(lCamera->UpdateToCamera());
   break;

  case GAIN:
    double gain_ch1, gain_ch2;

    gain_ch1 = (double) Value/100.0; // Workaround since Cam_Context_set_camera_property takes long for ValueA but
    gain_ch2 = (double) Value/100.0; // gains need to be set via double 
    printf("Current implementation specifies identical gain values on both channels.\n");
    if (gain_ch1 < 6.00 || gain_ch2 > 40.00 ){
      //printf("Gain value specified is out of range. Resetting to 9.96 dB.\n");
      gain_ch1 = (double) 9.96;
      gain_ch2 = (double) 9.96;
    }
    //printf("Gain value set to: %5.2f dB.\n",gain_ch1);
    CHKIMPERX(lCamera->SetParameter("Channel 1 Gain",gain_ch1));
    CHKIMPERX(lCamera->SetParameter("Channel 2 Gain",gain_ch2));
    CHKIMPERX(lCamera->UpdateToCamera());
   break;

  }
}

void CamContext_grab_next_frame_blocking_with_stride( CamContext *in_cr, unsigned char *out_bytes, intptr_t stride0) {
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;

  int w,h;

  w=lBuffer->GetSizeX();
  // printf("GetSizeX() in GRAB_NEXT_FRAME_BLOCKING_WITH_STRIDE() returns: %d\n",w); 
  h=lBuffer->GetSizeY();
  // printf("GetSizeY() in grab_next_frame_blocking_with_stride() returns: %d\n",h); 

  // Copy image
  
  const unsigned char* lPtr;
  unsigned long lSize;
  if ( lBuffer->LockForRead( (void**) &lPtr, &lSize, CyBuffer::FLAG_NO_WAIT ) == CY_RESULT_OK ) {
    // Now, lPtr points to the data and lSize contains the number of bytes available.
    for (int row=0; row<h; row++) {
      memcpy((void*)(out_bytes+row*stride0), (const void*)(lPtr+row*w), w);
    }
    backend_extras->last_timestamp = lBuffer->GetReadTimestamp();
    backend_extras->last_imageID = lBuffer->GetReadImageID();
    
    // Also, the GetRead...() methods are available to inquire information
    // about the buffer.
    
    // Now release the buffer
    lBuffer->SignalReadEnd();
  } else {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error reading image buffer");
    return;
  }
}

void CamContext_grab_next_frame_blocking( CamContext *in_cr, unsigned char *out_bytes ) {
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  int w;
  w=lBuffer->GetSizeX();
  CamContext_grab_next_frame_blocking_with_stride(in_cr,out_bytes,w);
}

void CamContext_point_next_frame_blocking( CamContext *in_cr, unsigned char **buf_ptr){
  CHECK_CC(in_cr);
  NOT_IMPLEMENTED;
}
void CamContext_unpoint_frame( CamContext *in_cr){
  CHECK_CC(in_cr);
  NOT_IMPLEMENTED;
}

void CamContext_get_last_timestamp( CamContext *in_cr, double* timestamp ) {
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  
  // According to LYNX GigE C++ SDK Rev. R01 a tick is 480 nanoseconds
  // (pg. 241, sec 4.7.10, "GetReadTimestamp")
  *timestamp = (double)(backend_extras->last_timestamp) * 480e-9;
}

void CamContext_get_last_framenumber( CamContext *in_cr, long* framenumber ){
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  
  *framenumber = (long)(backend_extras->last_imageID);
}

void CamContext_get_num_trigger_modes( CamContext *in_cr, 
				       int *num_exposure_modes ) {
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  
  *num_exposure_modes = NUM_TRIGGER_MODES;
}

void CamContext_get_trigger_mode_string( CamContext *in_cr,
					 int exposure_mode_number,
					 char* exposure_mode_string, //output parameter
					 int exposure_mode_string_maxlen) {
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  
  CAM_IFACE_CHECK_TRIGGER_MODE_NUMBER(exposure_mode_number);
  switch(exposure_mode_number) {
  case TRIGGER_OFF:
    _snprintf_s(exposure_mode_string,
		exposure_mode_string_maxlen,
		exposure_mode_string_maxlen,
		"internal, freerunning");
    break;
  case TRIGGER_EXTERNAL_STANDARD:
    _snprintf_s(exposure_mode_string,
		exposure_mode_string_maxlen,
		exposure_mode_string_maxlen,
		"external, standard");
    break;
  case TRIGGER_EXTERNAL_FAST:
    _snprintf_s(exposure_mode_string,
		exposure_mode_string_maxlen,
		exposure_mode_string_maxlen,
		"external, fast");
    break;
  case TRIGGER_EXTERNAL_DOUBLE:
    _snprintf_s(exposure_mode_string,
		exposure_mode_string_maxlen,
		exposure_mode_string_maxlen,
		"external, double");
    break;
  case TRIGGER_CC_STANDARD:
    _snprintf_s(exposure_mode_string,
		exposure_mode_string_maxlen,
		exposure_mode_string_maxlen,
		"cc, standard");
    break;
  case TRIGGER_CC_FAST:
    _snprintf_s(exposure_mode_string,
		exposure_mode_string_maxlen,
		exposure_mode_string_maxlen,
		"cc, fast");
    break;
  case TRIGGER_CC_DOUBLE:
    _snprintf_s(exposure_mode_string,
		exposure_mode_string_maxlen,
		exposure_mode_string_maxlen,
		"cc, double");
    break;
  }
}


void CamContext_get_trigger_mode_number( CamContext *in_cr,
					 int *exposure_mode_number ) {
  unsigned long mode, type;

  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF; 

  CHKIMPERX(lCamera->GetParameter("Trigger Mode",mode));
  CHKIMPERX(lCamera->GetParameter("Trigger Type",type));
  switch (mode) {
  case 0:
    *exposure_mode_number = TRIGGER_OFF;
    break;
  case 1:
    switch (type) {
    case 0:
      *exposure_mode_number = TRIGGER_EXTERNAL_STANDARD;
      break;
    case 1:
      *exposure_mode_number = TRIGGER_EXTERNAL_FAST;
      break;
    case 2:
      *exposure_mode_number = TRIGGER_EXTERNAL_DOUBLE;
      break;
    default:
      NOT_IMPLEMENTED;
      break;
    }
    break;
  case 2:
    switch (type) {
    case 0:
      *exposure_mode_number = TRIGGER_CC_STANDARD;
      break;
    case 1:
      *exposure_mode_number = TRIGGER_CC_FAST;
      break;
    case 2:
      *exposure_mode_number = TRIGGER_CC_DOUBLE;
      break;
    default:
      NOT_IMPLEMENTED;
      break;
    }
    break;
  default:
    NOT_IMPLEMENTED;
    break;
  }
}

void CamContext_set_trigger_mode_number( CamContext *in_cr,
					 int exposure_mode_number ) {
  unsigned long mode;
  unsigned long type;

  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;

  switch (exposure_mode_number) {
  case TRIGGER_OFF:
    mode = 0;
    break;
  case TRIGGER_EXTERNAL_STANDARD: case TRIGGER_EXTERNAL_FAST: case TRIGGER_EXTERNAL_DOUBLE:
    mode = 1;
    break;
  case TRIGGER_CC_STANDARD: case TRIGGER_CC_FAST: case TRIGGER_CC_DOUBLE:
    mode = 2;
    break;
  default:
    NOT_IMPLEMENTED;
    break;
  }

  switch (exposure_mode_number) {
  case TRIGGER_OFF: case TRIGGER_EXTERNAL_STANDARD: case TRIGGER_CC_STANDARD:
    type = 0;
    break;
  case TRIGGER_EXTERNAL_FAST: case TRIGGER_CC_FAST:
    type = 1;
    break;
  case TRIGGER_EXTERNAL_DOUBLE: case TRIGGER_CC_DOUBLE:
    type = 2;
    break;
  }
  
  CHKIMPERX(lCamera->SetParameter("Trigger Mode", mode));
  CHKIMPERX(lCamera->SetParameter("Trigger Type", type));
  CHKIMPERX(lCamera->UpdateToCamera());
}

// From LYNX GigE C++ SDK Reference Guide.pdf:
// After the SetParameter call, application must call UpdateToCamera
// method to apply the changes.
//
// The integer exttrig must be an integer in {0, 1, ..., 6}.  The values have
// the corresponding meanings:
//
// Trigger Mode       Trigger Type      exttrig
//     off                N/A              0
//   external           standard           1
//   external             fast             2
//   external            double            3
//     CC               standard           4
//     CC                 fast             5
//     CC                double            6

void CamContext_get_frame_offset( CamContext *in_cr, 
       				  int *left, int *top ) {
  
  __int64 left_line, top_line;

  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  CHKIMPERX(lCamera->GetParameter("Horizontal Window Position Left",left_line));
  CHKIMPERX(lCamera->GetParameter("Vertical Window Top Lines",top_line));
  *left = (int) left_line;
  *top = (int) top_line;
}

void CamContext_set_frame_offset( CamContext *in_cr, 
				  int left, int top ) {
  
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;

  NOT_IMPLEMENTED;

  /*
  
  // This throws an error, so it's disabled for now. ADS 20070321

  // Sets windowing mode (0 is normal, 1 is window, 2 is binning)
  CHKIMPERX(lCamera->SetParameter("Horizontal Window Mode", 1));
  CHKIMPERX(lCamera->SetParameter("Verical Window Mode",1));
  CHKIMPERX(lCamera->SetParameter("Horizontal Window Position Left", (__int64) left));
  CHKIMPERX(lCamera->SetParameter("Vertical Window Top Lines", (__int64) top));
  CHKIMPERX(lCamera->UpdateToCamera());
  */
}

void CamContext_get_frame_size( CamContext *in_cr, 
				int *width, int *height ) {
  
  __int64 left_line, right_line, top_line, bottom_line;
  
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  CHKIMPERX(lCamera->GetParameter("Horizontal Window Position Left",left_line));  
  CHKIMPERX(lCamera->GetParameter("Vertical Window Top Lines",top_line));
  CHKIMPERX(lCamera->GetParameter("Horizontal Window Position Right",right_line));
  CHKIMPERX(lCamera->GetParameter("Vertical Window Bottom Lines",bottom_line));

  *width = (int) right_line - left_line + 1;
  *height = (int) bottom_line - top_line + 1;

}

void CamContext_set_frame_size( CamContext *in_cr, 
				int width, int height ) {
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  int right, bottom, left, top;
  
  CHKIMPERX(lCamera->GetParameter("Horizontal Window Position Left",left));
  CHKIMPERX(lCamera->GetParameter("Vertical Window Top Lines",top));
    
  right = left + width - 1;
  bottom = top + height - 1;  
    
  CHKIMPERX(lCamera->SetParameter("Horizontal Window Position Right", (__int64) right));
  CHKIMPERX(lCamera->SetParameter("Vertical Window Bottom Lines", (__int64) bottom));
  CHKIMPERX(lCamera->UpdateToCamera());

}

void CamContext_get_max_frame_size( CamContext *in_cr, 
				    int *width, int *height ){
  CHECK_CC(in_cr);

  cam_iface_ImperX_backend_extras* backend_extras = NULL;
  backend_extras = (cam_iface_ImperX_backend_extras*)(in_cr->backend_extras);
  CyImageBuffer* lBuffer = backend_extras->lBuffer;

  // we know format == CyGrayscale8::ID
  unsigned int w,h;
  w = lBuffer->GetSizeX();
  h = lBuffer->GetSizeY();
  *width = w;
  *height = h;
}

void CamContext_get_buffer_size( CamContext *in_cr,
					int *size){
  CHECK_CC(in_cr);

  cam_iface_ImperX_backend_extras* backend_extras = NULL;
  backend_extras = (cam_iface_ImperX_backend_extras*)(in_cr->backend_extras);
  CyImageBuffer* lBuffer = backend_extras->lBuffer;

  // we know format == CyGrayscale8::ID
  unsigned int w,h;
  w = lBuffer->GetSizeX();
  h = lBuffer->GetSizeY();
  *size = (w*h);
}

void CamContext_get_framerate( CamContext *in_cr, 
			       float *framerate ) {
  __int64 rate;  

  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;
  CHKIMPERX(lCamera->GetParameter("Lynx Frame Rate", rate));
  // rate should be >= 2.
  //printf("frame rate: %d\n", rate);
  *framerate = (float) rate;
}

void CamContext_set_framerate( CamContext *in_cr, 
			       float framerate ) {
  CHECK_CC(in_cr);
  SAFE_CAMERA_DEREF;

  CHKIMPERX(lCamera->SetParameter("Lynx Frame Rate", (__int64) framerate));
  CHKIMPERX(lCamera->UpdateToCamera());
}

void CamContext_get_num_framebuffers( CamContext *in_cr, 
				      int *num_framebuffers ) {
  CHECK_CC(in_cr);
  *num_framebuffers = 3;
}

void CamContext_set_num_framebuffers( CamContext *in_cr, 
				      int num_framebuffers ) {
  CHECK_CC(in_cr);
  // no point in doint this with this backend
}
