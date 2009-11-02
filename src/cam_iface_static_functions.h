/* internal structures for mega backend implementation */

const char *BACKEND_METHOD(cam_iface_get_driver_name)();

void BACKEND_METHOD(cam_iface_clear_error)();
int BACKEND_METHOD(cam_iface_have_error)();
const char * BACKEND_METHOD(cam_iface_get_error_string)();
void BACKEND_METHOD(cam_iface_startup)();
void BACKEND_METHOD(cam_iface_shutdown)();
int BACKEND_METHOD(cam_iface_get_num_cameras)();
void BACKEND_METHOD(cam_iface_get_camera_info)(int device_number, Camwire_id *out_camid);
void BACKEND_METHOD(cam_iface_get_num_modes)(int device_number, int *num_modes);
void BACKEND_METHOD(cam_iface_get_mode_string)(int device_number,
					       int mode_number,
					       char* mode_string,
					       int mode_string_maxlen);

cam_iface_constructor_func_t BACKEND_METHOD(cam_iface_get_constructor_func)(int device_number);
