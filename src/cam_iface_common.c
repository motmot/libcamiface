#include "cam_iface.h"
void delete_CamContext(CamContext*this) {
  this->vmt->destruct(this);
}

void CamContext_CamContext(CamContext *this,int device_number, int NumImageBuffers,
			   int mode_number ) {
  // Must call derived class to make instance.
  this->vmt = NULL;
}

void CamContext_close(struct CamContext *this) {
  this->vmt->close(this);
}

void CamContext_start_camera(CamContext *this) {
  this->vmt->start_camera(this);
}

void CamContext_stop_camera(CamContext *this) {
  this->vmt->stop_camera(this);
}

void CamContext_get_num_camera_properties(CamContext *this,
					  int* num_properties){
  this->vmt->get_num_camera_properties(this,num_properties);
}

void CamContext_get_camera_property_info(CamContext *this,
					 int property_number,
					 CameraPropertyInfo *info){
  this->vmt->get_camera_property_info(this,property_number,info);
}

void CamContext_get_camera_property(CamContext *this,
				    int property_number,
				    long* Value,
				    int* Auto){
  this->vmt->get_camera_property(this,property_number,Value,Auto);
}

void CamContext_set_camera_property(CamContext *this,
				    int property_number,
				    long Value,
				    int Auto) {
  this->vmt->set_camera_property(this,property_number,Value,Auto);
}

void CamContext_grab_next_frame_blocking(CamContext *this, unsigned char* out_bytes, float timeout){
  this->vmt->grab_next_frame_blocking(this,out_bytes,timeout);
}
void CamContext_grab_next_frame_blocking_with_stride(CamContext *this, unsigned char* out_bytes, intptr_t stride0, float timeout){
  this->vmt->grab_next_frame_blocking_with_stride(this,out_bytes,stride0,timeout);
}
void CamContext_point_next_frame_blocking(CamContext *this, unsigned char** buf_ptr, float timeout){
  this->vmt->point_next_frame_blocking(this,buf_ptr,timeout);
}
void CamContext_unpoint_frame(CamContext *this){
  this->vmt->unpoint_frame(this);
}
void CamContext_get_last_timestamp( CamContext *this,
				    double* timestamp ){
  this->vmt->get_last_timestamp(this,timestamp);
}
void CamContext_get_last_framenumber( CamContext *this,
				      unsigned long* framenumber ){
  this->vmt->get_last_framenumber(this,framenumber);
}
void CamContext_get_num_trigger_modes( CamContext *this,
				       int *num_exposure_modes ){
  this->vmt->get_num_trigger_modes(this,num_exposure_modes);
}
void CamContext_get_trigger_mode_string( CamContext *this,
					 int exposure_mode_number,
					 char* exposure_mode_string, //output parameter
					 int exposure_mode_string_maxlen){
  this->vmt->get_trigger_mode_string(this,exposure_mode_number,exposure_mode_string,exposure_mode_string_maxlen);
}
void CamContext_get_trigger_mode_number( CamContext *this,
					 int *exposure_mode_number ){
  this->vmt->get_trigger_mode_number(this,exposure_mode_number);
}
void CamContext_set_trigger_mode_number( CamContext *this,
					 int exposure_mode_number ){
  this->vmt->set_trigger_mode_number(this,exposure_mode_number);
}
void CamContext_get_frame_offset( CamContext *this,
				  int *left, int *top ){
  this->vmt->get_frame_offset(this,left,top);
}
void CamContext_set_frame_offset( CamContext *this,
				  int left, int top ){
  this->vmt->set_frame_offset(this,left,top);
}
void CamContext_get_frame_size( CamContext *this,
				int *width, int *height ){
  this->vmt->get_frame_size(this,width,height);
}
void CamContext_set_frame_size( CamContext *this,
				int width, int height ){
  this->vmt->set_frame_size(this,width,height);
}
void CamContext_get_max_frame_size( CamContext *this,
				    int *width,
				    int *height ){
  this->vmt->get_max_frame_size(this,width,height);
}
void CamContext_get_buffer_size( CamContext *this,
				 int *size){
  this->vmt->get_buffer_size(this,size);
}
void CamContext_get_framerate( CamContext *this,
			       float *framerate ) {
  this->vmt->get_framerate(this,framerate);
}
void CamContext_set_framerate( CamContext *this,
			       float framerate ){
  this->vmt->set_framerate(this,framerate);
}
void CamContext_get_num_framebuffers( CamContext *this,
				      int *num_framebuffers ){
  this->vmt->get_num_framebuffers(this,num_framebuffers);
}
void CamContext_set_num_framebuffers( CamContext *this,
				      int num_framebuffers ){
  this->vmt->set_num_framebuffers(this,num_framebuffers);
}
