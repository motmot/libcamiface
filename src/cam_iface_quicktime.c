#include "cam_iface.h"

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

#include <stdlib.h>
#include <stdio.h>

/* typedefs */

/* globals */
int cam_iface_error = 0;
#define CAM_IFACE_MAX_ERROR_LEN 255
char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN]  = {0x00}; //...

uint32_t num_cameras = 0;
Component *components_by_device_number=NULL;

#define CAM_IFACE_ERROR_FORMAT(m)					\
  snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,		\
	   "%s (%d): %s\n",__FILE__,__LINE__,(m));

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


void do_qt_error(OSErr err,char* file,int line) {
  char* str=NULL;
  Handle h;

  h = GetResource('Estr', err);

  if (h) {
    HLock(h);
    str = (char *)*h;
    memcpy(cam_iface_error_string, str+1, (unsigned char)str[0]);
    cam_iface_error_string[(unsigned char)str[0]] = '\0';
    HUnlock(h);
    ReleaseResource(h);
  }
  else {
    snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN, "Mac OS error code %d (%s,line %d)", 
	     err,file,line);
  }

  cam_iface_error = -1;
  return;
}

#define CHK_QT(m) {							\
    if ((m)!=noErr) {							\
      do_qt_error(m,__FILE__,__LINE__);					\
      return;								\
    }									\
}

#define CHK_QT_VOID(m) {						\
    if ((m)!=noErr) {							\
      do_qt_error(m,__FILE__,__LINE__);					\
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


struct _cam_iface_backend_extras {
  GWorldPtr gworld; // offscreen gworld
  SGChannel sg_video_channel;
  Rect rect;
  TimeValue last_timestamp;
  long last_framenumber;
  int buffer_size;
  ImageSequence decompression_sequence;

  /* These are private parameters to be used in DataProc and SGIdle caller only! */
  int image_was_copied;
  intptr_t stride0;
  unsigned char * image_buf;
};

typedef struct _cam_iface_backend_extras cam_iface_backend_extras;

const char *cam_iface_get_driver_name() {
  return "QuickTime SequenceGrabber";
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

OSErr handle_frame_func(SGChannel chan, Rect *source_rect, GWorldPtr gworldptr, ImageSequence *im_sequence) {
  OSErr err = noErr;
 
  ImageDescriptionHandle im_desc = (ImageDescriptionHandle)NewHandle(sizeof(ImageDescription));
  if (im_desc) {
    err = SGGetChannelSampleDescription(chan,(Handle)im_desc);
    if (err == noErr) {
      MatrixRecord matrixrecord;
      Rect rect;
      
      rect.left = 0;
      rect.right = (*im_desc)->width;
      rect.top = 0;
      rect.bottom = (*im_desc)->height;

      RectMatrix(&matrixrecord, &rect, source_rect);
      err = DecompressSequenceBegin(im_sequence, im_desc, gworldptr, 0, nil, &matrixrecord,srcCopy,nil,0, codecNormalQuality, bestSpeedCodec);
    }
    DisposeHandle((Handle)im_desc);
  }
  else {
    err = MemError();
  }
  
  return err;
}

pascal OSErr process_data_callback(SGChannel c, Ptr p, long len, long *offset, 
			  long chRefCon, TimeValue time, short writeType, 
			  long refCon) {
  /* This is called within SGIdle */
  ComponentResult err;
  CodecFlags ignore;
  cam_iface_backend_extras* be=(cam_iface_backend_extras*)refCon;
  unsigned char* pucs;
  Rect bounds;

  /*
  if (be->buffer_size != len ) {
    fprintf(stderr,"buffer_size did not equal image length!\n");
    exit(-1);
  }
  */

  DecompressSequenceFrameS(be->decompression_sequence,p,len,0,&ignore,NULL);

  GetPortBounds(be->gworld, &bounds);
  OffsetRect(&bounds, -bounds.left, -bounds.top); // XXX deprecated call

  PixMapHandle	pix = GetGWorldPixMap(be->gworld); // XXX deprecated call
  int h = bounds.bottom;
  int wb = GetPixRowBytes(pix); // XXX deprecated call
  int row;
  unsigned char *baseAddr;

  baseAddr = GetPixBaseAddr(pix); // XXX deprecated call

  for (row=0; row<h; row++) {
    //fprintf(stdout, "row = %d, baseAddr = %d, wb = %d, stride0 = %d\n",row,baseAddr,wb,be->stride0);
    memcpy( (void*)(be->image_buf + row*be->stride0), // dest
	    baseAddr, // src
	    be->stride0 );
    baseAddr += wb;
  }

  be->image_was_copied = 1;
  be->last_timestamp = time;
  (be->last_framenumber)++;

  /*
  if (be->is_buffer_waiting_to_be_filled) {
    // this should happen majority of time -- listener is waiting for frame...
    if (len > be->max_buffer_size) {
      // throw error
      fprintf(stderr,"ERROR: data too big, should quit now!\n");
    }
    memcpy(be->buffer_to_fill,pucs,len);
    signal_frame_ready();
  } else {
    // in this case, we'll have to copy the data and then return it when the listener asks for next frame
    //copy_image_data_into_circular_quete(be);
  }
  */

  /*
  fprintf(stdout,"pucs %d\n",(long)pucs);
  fprintf(stdout,"received %d values, first few: %d %d %d %d %d %d %d %d\n",len,pucs[0],pucs[1],pucs[2],pucs[3],pucs[4],pucs[5],pucs[6],pucs[7]);
  */

  /*
  fprintf(stdout,".");
  fflush(stdout);
  */

  return noErr;
}

void cam_iface_startup() {
  OSErr err;

  err=EnterMovies();
  CHK_QT(err); 

  ComponentDescription	theDesc;
  Component		sgCompID;
  //  ComponentDescription  foundDesc = {0, 0, 0, 0, 0};
  int i;

  theDesc.componentType = SeqGrabComponentType;
  theDesc.componentSubType = 0L;
  theDesc.componentManufacturer = 0L;
  theDesc.componentFlags = 0L;
  theDesc.componentFlagsMask = 0L;

  num_cameras = CountComponents( &theDesc );
  components_by_device_number = (Component*)malloc( num_cameras*sizeof(Component));
  if (components_by_device_number==NULL) {
    cam_iface_error = -1;			
    CAM_IFACE_ERROR_FORMAT("malloc failed");
    return;
  }

  for (i=0; i<num_cameras; i++) {
    sgCompID 	= FindNextComponent(sgCompID, &theDesc);
    components_by_device_number[i] = sgCompID;

    //GetComponentInfo(sgCompID,&foundDesc,NULL,NULL,NULL);

    /*
    printf("found a device to capture with \n");
    printf("Type: %s\n", &foundDesc.componentType);
    printf("Manufacturer: %s\n", &foundDesc.componentManufacturer);
    */
  }

}

void cam_iface_shutdown() {
}

int cam_iface_get_num_cameras() {
  return num_cameras;
}

void cam_iface_get_camera_info(int device_number, Camwire_id *out_camid) {
  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  snprintf(out_camid->vendor,CAMWIRE_ID_MAX_CHARS,"unknown vendor");
  snprintf(out_camid->model,CAMWIRE_ID_MAX_CHARS,"unknown model");
  snprintf(out_camid->chip,CAMWIRE_ID_MAX_CHARS,"unknown chip");
}

void cam_iface_get_num_modes(int device_number, int *num_modes) {
  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  *num_modes = 1;
}

void cam_iface_get_mode_string(int device_number,
			       int mode_number,
			       char* mode_string,
			       int mode_string_maxlen) {
  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  CAM_IFACE_CHECK_MODE_NUMBER(mode_number);
  snprintf(mode_string,mode_string_maxlen,"default mode");
}

CamContext * new_CamContext( int device_number, int NumImageBuffers,
			     int mode_number) {
  CamContext *in_cr = NULL;
  SeqGrabComponent cam;
  Component sgCompID;
  OSErr err;

  CAM_IFACE_CHECK_DEVICE_NUMBER(device_number);
  CAM_IFACE_CHECK_MODE_NUMBER(mode_number);

  sgCompID = components_by_device_number[device_number];

  cam = OpenComponent(sgCompID);
  if (cam==NULL) {
    cam_iface_error=-1; CAM_IFACE_ERROR_FORMAT("Sequence Grabber could not be opened"); return;
  }

  in_cr = (CamContext*)malloc(sizeof(CamContext));
  if (!in_cr) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("malloc failed");
    return NULL;
  }

  in_cr->cam = (void *)cam;
  in_cr->backend_extras = malloc(sizeof(cam_iface_backend_extras));
  cam_iface_backend_extras* be=in_cr->backend_extras;
  if (be==NULL) {
    cam_iface_error=-1; CAM_IFACE_ERROR_FORMAT("malloc error"); return;
  }

  be->gworld=NULL;
  err = SGInitialize(cam);
  CHK_QT_VOID(err); 

  err = SGSetDataRef(cam, 0, 0, seqGrabDontMakeMovie);
  CHK_QT_VOID(err); 

  err = SGNewChannel(cam, VideoMediaType, &(be->sg_video_channel));
  CHK_QT_VOID(err); 

  be->rect.left=0;
  be->rect.top=0;
  be->rect.right=640;
  be->rect.bottom=480;

  be->last_framenumber = -1;

  in_cr->device_number=device_number;

  err = SGSetChannelBounds(be->sg_video_channel, &(be->rect));
  CHK_QT_VOID(err); 

#if 0
  in_cr->coding = CAM_IFACE_ARGB8;
  in_cr->depth=32;
  err = QTNewGWorld( &(be->gworld), 
		     k32ARGBPixelFormat,// let Mac do conversion to RGB
		     &(be->rect),
		     0, NULL, 0);
  CHK_QT_VOID(err); 
#else
  in_cr->coding = CAM_IFACE_YUV422,
  in_cr->depth=16;
  err = QTNewGWorld( &(be->gworld), 
		     k422YpCbCr8PixelFormat,
		     &(be->rect),
		     0, NULL, 0);
  CHK_QT_VOID(err); 
#endif
  
  PixMapHandle pix = GetGWorldPixMap(be->gworld); // XXX deprecated call
  int width_bytes = GetPixRowBytes(pix); // XXX deprecated call
  int height = be->rect.bottom;
  be->buffer_size = width_bytes*height;

  err = SGSetGWorld(cam, be->gworld, NULL);
  CHK_QT_VOID(err); 
  
  err = SGSetChannelUsage(be->sg_video_channel, seqGrabRecord);
  CHK_QT_VOID(err);

  err = SGSetDataProc(cam, NewSGDataUPP(process_data_callback), (long)be);
  CHK_QT_VOID(err); 

  err = SGPrepare(cam,false,true);
  CHK_QT_VOID(err); 

  err = handle_frame_func(be->sg_video_channel, &(be->rect), be->gworld, &(be->decompression_sequence));
  CHK_QT_VOID(err); 

  return in_cr;
}

void delete_CamContext(CamContext *in_cr) {
  CHECK_CC(in_cr);
  if ((in_cr->backend_extras)!=NULL) {
    free( in_cr->backend_extras );
    in_cr->backend_extras=NULL;
  }
  if (in_cr!=NULL) {
    free( in_cr );
    in_cr=NULL;
  }
}

void CamContext_get_frame_size( CamContext *in_cr, 
				int *width, int *height ) {
  CHECK_CC(in_cr);
  *width=((cam_iface_backend_extras*)(in_cr->backend_extras))->rect.right;
  *height=((cam_iface_backend_extras*)(in_cr->backend_extras))->rect.bottom;
}

void CamContext_set_frame_size( CamContext *in_cr, 
				int width, int height ) {
  CHECK_CC(in_cr);
  if (width!=((cam_iface_backend_extras*)(in_cr->backend_extras))->rect.right) {
    cam_iface_error=-1; CAM_IFACE_ERROR_FORMAT("frame width cannot be changed"); return;
  }
  if (height!=((cam_iface_backend_extras*)(in_cr->backend_extras))->rect.bottom) {
    cam_iface_error=-1; CAM_IFACE_ERROR_FORMAT("frame height cannot be changed"); return;
  }
}

CAM_IFACE_API void CamContext_get_max_frame_size( CamContext *in_cr, 
						  int *width, 
						  int *height ) {
  CHECK_CC(in_cr);
  *width=((cam_iface_backend_extras*)(in_cr->backend_extras))->rect.right;
  *height=((cam_iface_backend_extras*)(in_cr->backend_extras))->rect.bottom;
}

void CamContext_get_num_camera_properties(CamContext *in_cr, 
					  int* num_properties) {
  CHECK_CC(in_cr);
  num_properties = 0;
}

void CamContext_get_camera_property_info(CamContext *in_cr,
					 int property_number,
					 CameraPropertyInfo *info) {
  NOT_IMPLEMENTED;
}

void CamContext_get_camera_property(CamContext *in_cr,
				    int property_number,
				    long* Value,
				    int* Auto ) {
  NOT_IMPLEMENTED;
}

void CamContext_set_camera_property(CamContext *in_cr, 
				    int property_number,
				    long Value,
				    int Auto ) {
  NOT_IMPLEMENTED;
}

CAM_IFACE_API void CamContext_get_buffer_size( CamContext *in_cr,
					       int *size) {
  CHECK_CC(in_cr);
  *size=((cam_iface_backend_extras*)(in_cr->backend_extras))->buffer_size;
}

void CamContext_start_camera( CamContext *in_cr ) {
  OSErr err;
  CHECK_CC(in_cr);
  err = SGStartRecord(in_cr->cam);
  CHK_QT(err); 
}

void CamContext_stop_camera( CamContext *in_cr ) {
  OSErr err;
  CHECK_CC(in_cr);
  err = SGStop(in_cr->cam);
  CHK_QT(err); 
}

void CamContext_grab_next_frame_blocking_with_stride( CamContext *ccntxt, 
						      unsigned char *out_bytes, 
						      intptr_t stride0) {
  SeqGrabComponent cam;
  OSErr err=noErr;
  cam_iface_backend_extras* be;

  CHECK_CC(ccntxt);
  cam = ccntxt->cam;
  be = ccntxt->backend_extras;

  be->image_was_copied = 0;
  be->stride0 = stride0;
  be->image_buf = out_bytes;
  
  while(1) {
    err = SGIdle(cam); // this will call our callback
    CHK_QT(err); 

    // check for image
    if (be->image_was_copied) {
      // we've got one
      break;
    } else {
      // sleep and repeat
      usleep( 1000 ); // 1 millisecond
    }
  }
}

void CamContext_grab_next_frame_blocking( CamContext *ccntxt, unsigned char *out_bytes ) {
  cam_iface_backend_extras* be;
  intptr_t stride0;

  CHECK_CC(ccntxt);
  be = ccntxt->backend_extras;
  stride0 = be->rect.right*(ccntxt->depth/8);
  CamContext_grab_next_frame_blocking_with_stride( ccntxt, out_bytes, stride0 ) ;
}

void CamContext_get_last_timestamp( CamContext *in_cr, double* timestamp ) {
  CHECK_CC(in_cr);
  // convert from microseconds to seconds
  //*timestamp = (double)(((cam_iface_backend_extras*)(in_cr->backend_extras))->last_timestamp) * 1e-6;
  *timestamp = 0.0;
}

void CamContext_get_last_framenumber( CamContext *in_cr, long* framenumber ){
  CHECK_CC(in_cr);
  *framenumber=((cam_iface_backend_extras*)(in_cr->backend_extras))->last_framenumber;
}

void CamContext_get_num_trigger_modes( CamContext *in_cr, 
				       int *num_exposure_modes ) {
  *num_exposure_modes = 1;
}

void CamContext_get_trigger_mode_string( CamContext *ccntxt,
					 int exposure_mode_number,
					 char* exposure_mode_string, //output parameter
					 int exposure_mode_string_maxlen) {
  CHECK_CC(ccntxt);
  if (exposure_mode_number!=0) {
    cam_iface_error=-1; CAM_IFACE_ERROR_FORMAT("only trigger mode 0 supported"); return;
  }
  snprintf(exposure_mode_string,exposure_mode_string_maxlen,"internal freerunning");
}

void CamContext_get_trigger_mode_number( CamContext *ccntxt,
					 int *exposure_mode_number ) {
  CHECK_CC(ccntxt);
  *exposure_mode_number=0;
}

void CamContext_set_trigger_mode_number( CamContext *ccntxt,
					 int exposure_mode_number ) {
  CHECK_CC(ccntxt);
  if (exposure_mode_number!=0) {
    cam_iface_error=-1; CAM_IFACE_ERROR_FORMAT("only trigger mode 0 supported"); return;
  }
}

void CamContext_get_frame_offset( CamContext *in_cr, 
				  int *left, int *top ) {
  CHECK_CC(in_cr);
  *left = 0;
  *top = 0;
}

void CamContext_set_frame_offset( CamContext *in_cr, 
				  int left, int top ) {
  CHECK_CC(in_cr);
  if ((left != 0) | (top != 0) ) {
     cam_iface_error=-1; CAM_IFACE_ERROR_FORMAT("frame offset can only be 0,0"); return;
  }
}
void CamContext_get_framerate( CamContext *in_cr, 
			       float *framerate ) {
  CHECK_CC(in_cr);
  *framerate=30.0;
}

void CamContext_set_framerate( CamContext *in_cr, 
			       float framerate ) {
  NOT_IMPLEMENTED;
}

void CamContext_get_num_framebuffers( CamContext *in_cr, 
				      int *num_framebuffers ) {

  CHECK_CC(in_cr);
  *num_framebuffers=4; // unknown, really
}

void CamContext_set_num_framebuffers( CamContext *in_cr, 
				      int num_framebuffers ) {
  CHECK_CC(in_cr);
  NOT_IMPLEMENTED;
}
