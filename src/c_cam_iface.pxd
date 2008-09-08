#emacs, this is -*-Python-*- mode
cdef extern from "stdlib.h":
    ctypedef long intptr_t
    
cdef extern from "cam_iface.h":
    ctypedef struct Camwire_id:
        char* vendor
        char* model
        char* chip

    ctypedef struct CameraPropertyInfo:
        char* name
        int is_present
        long min_value
        long max_value
        int has_auto_mode
        int has_manual_mode
  
        int is_scaled_quantity

        char* scaled_unit_name
        double scale_offset
        double scale_gain

    char *cam_iface_get_driver_name()
    void cam_iface_clear_error()
    int cam_iface_have_error()
    char * cam_iface_get_error_string()
    
    void cam_iface_startup_with_version_check()
    void cam_iface_startup()
    void cam_iface_shutdown()

    int cam_iface_get_num_cameras()
    void cam_iface_get_camera_info(int index, Camwire_id *out_camid)

    ctypedef enum CameraPixelCoding:
        CAM_IFACE_UNKNOWN
        CAM_IFACE_MONO8
        CAM_IFACE_YUV411
        CAM_IFACE_YUV422
        CAM_IFACE_YUV444
        CAM_IFACE_RGB8
        CAM_IFACE_MONO16
        CAM_IFACE_RGB16
        CAM_IFACE_MONO16S
        CAM_IFACE_RGB16S
        CAM_IFACE_RAW8
        CAM_IFACE_RAW16

    ctypedef enum CamErrors:
        CAM_IFACE_BUFFER_OVERFLOW_ERROR
        CAM_IFACE_FRAME_DATA_MISSING_ERROR
        CAM_IFACE_HARDWARE_FEATURE_NOT_AVAILABLE
        
    ctypedef struct CamContext:
        void *cam
        void *ppBuffers
        int NumImageBuffers
        int ImageBufferSize
        int max_width
        int max_height
        int depth
        int coding #CameraPixelCoding coding # (why doesn't it work?)
        int buffer_size
        int roi_width
        int roi_height

        
    void cam_iface_get_num_modes(int device_number, int *num_modes)

    void cam_iface_get_mode_string(int device_number,
                                   int mode_number,
                                   char* mode_string,
                                   int mode_string_maxlen)
    
    CamContext *new_CamContext(int device_number,int NumImageBuffers,
                               int mode_number)
    void delete_CamContext(CamContext *ccntxt)

    void CamContext_start_camera(CamContext *ccntxt)
    void CamContext_stop_camera(CamContext *ccntxt)
    
    void CamContext_get_num_camera_properties(CamContext *ccntxt, 
                                              int* num_properties)
    void CamContext_get_camera_property_info(CamContext *ccntxt,
                                             int property_number,
                                             CameraPropertyInfo *info)
    void CamContext_get_camera_property(CamContext *ccntxt,
                                        int property_number,
                                        long* Value,
                                        int* Auto)
    void CamContext_set_camera_property(CamContext *ccntxt,
                                        int property_number,
                                        long Value,
                                        int Auto)
    
    void CamContext_grab_next_frame_blocking(CamContext *ccntxt, unsigned char* out_bytes, float timeout)
    void CamContext_grab_next_frame_blocking_with_stride(CamContext *ccntxt, unsigned char* out_bytes,intptr_t stride0, float timeout)
    void CamContext_point_next_frame_blocking(CamContext *ccntxt, unsigned char** buf_ptr, float timeout)
    void CamContext_unpoint_frame(CamContext *ccntxt)
    
    void CamContext_get_last_timestamp( CamContext *ccntxt, 
                                        double* timestamp )
    void CamContext_get_last_framenumber( CamContext *ccntxt,
                                          unsigned long* framenumber )

    void CamContext_get_num_trigger_modes( CamContext *ccntxt, 
                                           int *num_exposure_modes )
    void CamContext_get_trigger_mode_string( CamContext *ccntxt,
                                             int exposure_mode_number,
                                             char* exposure_mode_string,
                                             int exposure_mode_string_maxlen)
    void CamContext_get_trigger_mode_number( CamContext *ccntxt,
                                             int *exposure_mode_number )
    void CamContext_set_trigger_mode_number( CamContext *ccntxt,
                                             int exposure_mode_number )
    
    void CamContext_get_trigger_source( CamContext *ccntxt, 
                                        int *exttrig )
    void CamContext_set_trigger_source( CamContext *ccntxt, 
                                        int exttrig )
    
    void CamContext_get_frame_offset( CamContext *ccntxt, 
					 int *left, int *top )
    void CamContext_set_frame_offset( CamContext *ccntxt, 
                                      int left, int top )

    void CamContext_get_frame_size( CamContext *ccntxt, 
                                    int *width, int *height )
    void CamContext_set_frame_size( CamContext *ccntxt, 
                                    int width, int height )

    void CamContext_get_max_frame_size( CamContext *ccntxt, 
                                        int *width, int *height )

    void CamContext_get_framerate( CamContext *ccntxt, 
                                   float *framerate )
    void CamContext_set_framerate( CamContext *ccntxt, 
                                   float framerate )

    void CamContext_get_num_framebuffers( CamContext *ccntxt, 
                                          int *num_framebuffers )
    void CamContext_set_num_framebuffers( CamContext *ccntxt, 
                                          int num_framebuffers )
    
