import numpy as N
import ctypes
assert ctypes.__version__ >= '1.0.1'
import sys, os
import pkg_resources
THREAD_DEBUG = 0
if THREAD_DEBUG:
    import threading

__all__ = ['CamIFaceError','BuffersOverflowed',
           'FrameTimeout',
           'FrameDataMissing',
           'get_driver_name','get_num_cameras',
           'get_num_modes','get_mode_string',
           'get_camera_info','Camera',
           'get_wrapper_name','shutdown']

backend_fname = os.environ.get('CAM_IFACE_CTYPES_BACKEND',None)
if backend_fname is None:
    if sys.platform.startswith('linux'):
        #backend_fname = 'libcam_iface_camwire.so'
        backend_fname = 'libcam_iface_prosilica_gige.so'
        #backend_fname = 'libcam_iface_dc1394.so'
    elif sys.platform.startswith('win'):
        backend_fname = 'cam_iface_imperx.dll'
    elif sys.platform.startswith('darwin'):
        backend_fname = 'libcam_iface_dc1394.so'
    else:
        raise ValueError("unknown platform '%s'"%sys.platform)

backend_fullpath = pkg_resources.resource_filename(__name__,backend_fname)
if sys.platform.startswith('linux'):
    c_cam_iface = ctypes.cdll.LoadLibrary(backend_fullpath)
elif sys.platform.startswith('win'):
    c_cam_iface = ctypes.CDLL(backend_fullpath)
elif sys.platform.startswith('darwin'):
    c_cam_iface = ctypes.CDLL(backend_fullpath)
else:
    raise ValueError("unknown platform '%s'"%sys.platform)

##################################################

# modified from numpy:
def _getintp_ctype():
    char = N.dtype('p').char
    if (char == 'i'):
        val = ctypes.c_int
    elif char == 'l':
        val = ctypes.c_long
    elif char == 'q':
        val = ctypes.c_longlong
    else:
        raise ValueError, "confused about intp->ctypes."
    _getintp_ctype.cache = val
    return val
intptr_type = _getintp_ctype()

##################################################
# function prototypes ############################
#CameraPixelCoding
CAM_IFACE_UNKNOWN=0
CAM_IFACE_MONO8=1
CAM_IFACE_YUV411=2
CAM_IFACE_YUV422=3
CAM_IFACE_YUV444=4
CAM_IFACE_RGB8=5
CAM_IFACE_MONO16=6
CAM_IFACE_RGB16=7
CAM_IFACE_MONO16S=8
CAM_IFACE_RGB16S=9
CAM_IFACE_RAW8=10
CAM_IFACE_RAW16=11
CAM_IFACE_ARGB8=12

class Camwire_id(ctypes.Structure):
    _fields_ = [('vendor',ctypes.c_char*101),
                ('model',ctypes.c_char*101),
                ('chip',ctypes.c_char*101),
                ]

class CamContext(ctypes.Structure):
    _fields_ = [('cam',ctypes.c_void_p),
                ('backend_extras',ctypes.c_void_p),
                ('coding',ctypes.c_int),
                ('depth',ctypes.c_int),
                ('device_number',ctypes.c_int),
                ]

class CameraPropertyInfo(ctypes.Structure):
    _fields_ = [('name',ctypes.c_char_p),
                ('is_present',ctypes.c_int),
                ('min_value',ctypes.c_long),
                ('max_value',ctypes.c_long),
                ('has_auto_mode',ctypes.c_int),
                ('has_manual_mode',ctypes.c_int),
                ('is_scaled_quantity',ctypes.c_int),
                ('scaled_unit_name',ctypes.c_char_p),
                ('scale_offset',ctypes.c_double),
                ('scale_gain',ctypes.c_double),
                ]

c_cam_iface.cam_iface_get_driver_name.restype = ctypes.c_char_p
c_cam_iface.cam_iface_get_error_string.restype = ctypes.c_char_p
c_cam_iface.cam_iface_get_num_modes.argtypes = [ctypes.c_int,
                                                ctypes.POINTER(ctypes.c_int)]
c_cam_iface.cam_iface_get_mode_string.argtypes = [ctypes.c_int,
                                                  ctypes.c_int,
                                                  ctypes.POINTER(ctypes.c_char),
                                                  ctypes.c_int]
c_cam_iface.new_CamContext.restype = ctypes.POINTER(CamContext)
c_cam_iface.new_CamContext.argtypes = [ctypes.c_int,ctypes.c_int,
                                       ctypes.c_int]
c_cam_iface.delete_CamContext.argtypes = [ctypes.POINTER(CamContext)]
c_cam_iface.CamContext_start_camera.argtypes = [ctypes.POINTER(CamContext)]
c_cam_iface.CamContext_stop_camera.argtypes = [ctypes.POINTER(CamContext)]
c_cam_iface.CamContext_get_frame_size.argtypes = [ctypes.POINTER(CamContext),
                                                  ctypes.POINTER(ctypes.c_int),
                                                  ctypes.POINTER(ctypes.c_int)]
c_cam_iface.CamContext_grab_next_frame_blocking.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.c_void_p]
c_cam_iface.CamContext_grab_next_frame_blocking_with_stride.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.c_void_p,
    intptr_type]
c_cam_iface.CamContext_get_last_framenumber.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.POINTER(ctypes.c_long)]
c_cam_iface.CamContext_get_num_camera_properties.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.POINTER(ctypes.c_int)]
c_cam_iface.CamContext_get_camera_property_info.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.c_int,
    ctypes.POINTER(CameraPropertyInfo)]
c_cam_iface.CamContext_get_camera_property.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_long),
    ctypes.POINTER(ctypes.c_int),
    ]
c_cam_iface.CamContext_set_camera_property.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.c_int,
    ctypes.c_long,
    ctypes.c_int,
    ]
c_cam_iface.CamContext_set_frame_offset.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.c_int,
    ctypes.c_int]
c_cam_iface.CamContext_set_frame_size.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.c_int,
    ctypes.c_int]
c_cam_iface.CamContext_get_num_trigger_modes.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.POINTER(ctypes.c_int)]
c_cam_iface.CamContext_get_trigger_mode_string.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_char),
    ctypes.c_int]
c_cam_iface.CamContext_get_trigger_mode_number.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.POINTER(ctypes.c_int)]
c_cam_iface.CamContext_set_trigger_mode_number.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.c_int]
c_cam_iface.CamContext_get_framerate.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.POINTER(ctypes.c_float)]
c_cam_iface.CamContext_set_framerate.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.c_float]
c_cam_iface.CamContext_get_num_framebuffers.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.POINTER(ctypes.c_int)]
c_cam_iface.CamContext_set_num_framebuffers.argtypes = [
    ctypes.POINTER(CamContext),
    ctypes.c_int]

##################################################
# define Python exception
class CamIFaceError(Exception):
    pass

class BuffersOverflowed(CamIFaceError):
    pass

class FrameTimeout(CamIFaceError):
    pass

class FrameDataMissing(CamIFaceError):
    pass

class HardwareFeatureNotAvailable(CamIFaceError):
    pass

def _check_error():
    CAM_IFACE_BUFFER_OVERFLOW_ERROR=-392081
    CAM_IFACE_FRAME_DATA_MISSING_ERROR=-392073
    CAM_IFACE_FRAME_TIMEOUT=-392074
    CAM_IFACE_FRAME_DATA_LOST_ERROR=-392075
    CAM_IFACE_HARDWARE_FEATURE_NOT_AVAILABLE=-392076
    
    errnum = c_cam_iface.cam_iface_have_error()
    if errnum != 0:
        err_str=c_cam_iface.cam_iface_get_error_string()
        if errnum == CAM_IFACE_BUFFER_OVERFLOW_ERROR:
            exc_type = BuffersOverflowed
        elif errnum == CAM_IFACE_FRAME_DATA_MISSING_ERROR:
            exc_type = FrameDataMissing
        elif errnum == CAM_IFACE_FRAME_TIMEOUT:
            exc_type = FrameTimeout
        elif errnum == CAM_IFACE_HARDWARE_FEATURE_NOT_AVAILABLE:
            exc_type = HardwareFeatureNotAvailable
        else:
            exc_type = CamIFaceError
        c_cam_iface.cam_iface_clear_error()
        raise exc_type(err_str)
    
def get_driver_name():
    return c_cam_iface.cam_iface_get_driver_name()

def get_num_cameras():
    return c_cam_iface.cam_iface_get_num_cameras()

def get_num_modes(device_number):
    num_modes = ctypes.c_int()
    c_cam_iface.cam_iface_get_num_modes(device_number,
                                        ctypes.byref(num_modes))
    return num_modes.value

def get_mode_string(device_number, mode_number):
    strlen = 255
    mode_string = ctypes.create_string_buffer(strlen)
    c_cam_iface.cam_iface_get_mode_string(device_number, mode_number,
                                          mode_string,strlen)
    mode_string = mode_string.value
    mode_string = mode_string.split('\0')[0]
    return mode_string

def get_camera_info(index):
    #cdef c_cam_iface.Camwire_id camid
    camid = Camwire_id()
    c_cam_iface.cam_iface_get_camera_info(index, ctypes.byref(camid))
    _check_error()
    return camid.vendor, camid.model, camid.chip

class Camera:
    def __init__(self,device_number,num_buffers,mode_number):
        if THREAD_DEBUG:
            self.mythread=threading.currentThread()
        self.cval = c_cam_iface.new_CamContext(device_number,num_buffers,
                                               mode_number)
        _check_error()

    def __del__(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        c_cam_iface.delete_CamContext(self.cval)
        _check_error()

    def close(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        c_cam_iface.delete_CamContext(self.cval)
        _check_error()

    def start_camera(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        c_cam_iface.CamContext_start_camera(self.cval)
        _check_error()

    def stop_camera(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        c_cam_iface.CamContext_stop_camera(self.cval)
        _check_error()

    def grab_next_frame_blocking(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        w = ctypes.c_int(0)
        h = ctypes.c_int(0)
        c_cam_iface.CamContext_get_frame_size(self.cval,
                                              ctypes.byref(w),
                                              ctypes.byref(h))
        _check_error()
        buf = N.empty((h.value,(w.value*self.cval.contents.depth)/8),dtype=N.uint8)
        if isinstance(buf.ctypes.data,ctypes.c_void_p):
            data_ptr = buf.ctypes.data
        else:
            data_ptr = ctypes.c_void_p(buf.ctypes.data)
        c_cam_iface.CamContext_grab_next_frame_blocking(self.cval,
                                                        data_ptr)
        _check_error()
        return buf
    
    def grab_next_frame_into_buf_blocking(self,buf,bypass_buffer_checks=False):
        """grab frame into user-supplied buffer"""
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        buf = N.asarray(buf) # convert to numpy for ctypes attribute
        if isinstance(buf.ctypes.data,ctypes.c_void_p):
            data_ptr = buf.ctypes.data
        else:
            data_ptr = ctypes.c_void_p(buf.ctypes.data)
        c_cam_iface.CamContext_grab_next_frame_blocking_with_stride(self.cval,
                                                                    data_ptr,
                                                                    buf.ctypes.strides[0])
        _check_error()

    def grab_next_frame_into_alloced_buf_blocking(self,buf_alloc,bypass_buffer_checks=False):
        """grab frame into a newly allocated buffer using user allocation function"""
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        h = ctypes.c_int(0)
        w = ctypes.c_int(0)
        c_cam_iface.CamContext_get_frame_size(self.cval,ctypes.byref(w),ctypes.byref(h))
        _check_error()

        buf = buf_alloc((w.value*self.cval.contents.depth)/8,
                        h.value)
        self.grab_next_frame_into_buf_blocking(buf,bypass_buffer_checks=bypass_buffer_checks)
        return buf
        
    def get_last_timestamp(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        timestamp = ctypes.c_double(0.0)
        c_cam_iface.CamContext_get_last_timestamp(self.cval,ctypes.byref(timestamp))
        _check_error()
        return timestamp.value
    
    def get_last_framenumber(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        framenumber = ctypes.c_long(0)
        c_cam_iface.CamContext_get_last_framenumber(self.cval,ctypes.byref(framenumber))
        _check_error()
        return framenumber.value

    def get_num_camera_properties(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        val = ctypes.c_int()
        c_cam_iface.CamContext_get_num_camera_properties(self.cval,ctypes.byref(val))
        _check_error()
        return val.value
    
    def get_camera_property_info(self,num):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        info = CameraPropertyInfo()
        c_cam_iface.CamContext_get_camera_property_info(self.cval,num,ctypes.byref(info))
        _check_error()
        result = {}
        result = {'name':info.name,
                  'is_present':info.is_present,
                  'min_value':info.min_value,
                  'max_value':info.max_value,
                  'has_auto_mode':info.has_auto_mode,
                  'has_manual_mode':info.has_manual_mode,
                  'is_scaled_quantity':info.is_scaled_quantity}
        if result['is_scaled_quantity']:
            result.update({'scaled_unit_name':info.scaled_unit_name,
                           'scale_offset':info.scale_offset,
                           'scale_gain':info.scale_gain,
                           })
        return result
    
    def set_camera_property(self,
                            property_number,
                            Value,
                            Auto):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        c_cam_iface.CamContext_set_camera_property(self.cval,
                                                   property_number,
                                                   Value,
                                                   Auto )
        _check_error()
    
    def get_camera_property(self,property_number):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        Value = ctypes.c_long(0)
        Auto = ctypes.c_int(0)
        c_cam_iface.CamContext_get_camera_property(self.cval,
                                                   property_number,
                                                   ctypes.byref(Value),
                                                   ctypes.byref(Auto) )
        _check_error()
        return (Value.value, Auto.value)
    
    def get_pixel_depth(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
	return self.cval.contents.depth

    def get_pixel_coding(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        if self.cval.contents.coding == CAM_IFACE_UNKNOWN:
            return 'UNKNOWN'
        elif self.cval.contents.coding == CAM_IFACE_MONO8:
            return 'MONO8'
        elif self.cval.contents.coding == CAM_IFACE_MONO16:
            return 'MONO16'
        elif self.cval.contents.coding == CAM_IFACE_YUV411:
            return 'YUV411'
        elif self.cval.contents.coding == CAM_IFACE_YUV422:
            return 'YUV422'
        elif self.cval.contents.coding == CAM_IFACE_YUV444:
            return 'YUV444'
        elif self.cval.contents.coding == CAM_IFACE_RGB8:
            return 'RGB8'
        elif self.cval.contents.coding == CAM_IFACE_ARGB8:
            return 'ARGB8'
        else:
            raise NotImplementedError("Can't convert pixel coding representation to string")
        
    def get_max_width(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        max_width = ctypes.c_int(0)
        max_height = ctypes.c_int(0)
        c_cam_iface.CamContext_get_max_frame_size(self.cval,
                                                  ctypes.byref(max_width),
                                                  ctypes.byref(max_height))
        _check_error()
        return max_width.value
    
    def get_max_height(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        max_width = ctypes.c_int(0)
        max_height = ctypes.c_int(0)
        c_cam_iface.CamContext_get_max_frame_size(self.cval,
                                                  ctypes.byref(max_width),
                                                  ctypes.byref(max_height))
        _check_error()
        return max_height.value
    
    def get_frame_offset(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        left = ctypes.c_int(0)
        top = ctypes.c_int(0)
        c_cam_iface.CamContext_get_frame_offset(self.cval,
                                                ctypes.byref(left),
                                                ctypes.byref(top))
        _check_error()
        return left.value, top.value
    
    def get_frame_size(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        width = ctypes.c_int(0)
        height = ctypes.c_int(0)
        c_cam_iface.CamContext_get_frame_size(self.cval,
                                              ctypes.byref(width),
                                              ctypes.byref(height))
        _check_error()
        return width.value, height.value
    
    def set_frame_offset(self, left, top):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        c_cam_iface.CamContext_set_frame_offset(self.cval, left, top)
        _check_error()
        
    def set_frame_size(self, width, height):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        c_cam_iface.CamContext_set_frame_size(self.cval, width, height)
        _check_error()

    def get_num_trigger_modes(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        num_modes = ctypes.c_int(0)
        c_cam_iface.CamContext_get_num_trigger_modes(self.cval,
                                                     ctypes.byref(num_modes))
        _check_error()
        return num_modes.value
    
    def get_trigger_mode_string(self,mode_number):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        strlen = 255
        mode_string = ctypes.create_string_buffer(strlen)
        c_cam_iface.CamContext_get_trigger_mode_string(self.cval,
                                                       mode_number,
                                                       mode_string,
                                                       strlen)
        _check_error()
        mode_string = mode_string.value
        mode_string = mode_string.split('\0')[0]
        return mode_string
    
    def get_trigger_mode_number(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        val = ctypes.c_int(0)
        c_cam_iface.CamContext_get_trigger_mode_number(self.cval,
                                                       ctypes.byref(val))
        _check_error()
        return val.value
    
    def set_trigger_mode_number(self,source):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        source = ctypes.c_int(source)
        c_cam_iface.CamContext_set_trigger_mode_number(self.cval, source)
        _check_error()

    def get_framerate(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        framerate = ctypes.c_float(0)
        c_cam_iface.CamContext_get_framerate(self.cval, ctypes.byref(framerate))
        _check_error()        
        return framerate.value
    
    def set_framerate(self, framerate):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        c_cam_iface.CamContext_set_framerate(self.cval,
                                             framerate)
        _check_error()
        
    def get_num_framebuffers(self):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        val = ctypes.c_int(0)
        c_cam_iface.CamContext_get_num_framebuffers(self.cval,
                                                    ctypes.byref(val))
        _check_error()
        return val.value
    
    def set_num_framebuffers(self,val):
        if THREAD_DEBUG:
            if self.mythread!=threading.currentThread():
                raise RuntimeError("Camera class is not thread safe!")
        c_cam_iface.CamContext_set_num_framebuffers(self.cval,
                                                    val)
        _check_error()

    def set_thread_owner(self,newowner=None):
        if THREAD_DEBUG:
            if newowner is None:
                self.mythread=threading.currentThread()
            else:
                self.mythread=newowner

def test():
    print 'get_driver_name()',get_driver_name()
    n_cams = c_cam_iface.cam_iface_get_num_cameras()
    print 'n_cams',n_cams
    for i in range(n_cams):
        print 'camera',i
        print 'camera info:',get_camera_info(i)

        cam = Camera(i,200,7,0,0)
        
        print

####################################
# Initialization code

c_cam_iface.cam_iface_startup()
_check_error()

def get_wrapper_name():
    return 'ctypes'

def shutdown():
    c_cam_iface.cam_iface_shutdown()
    _check_error()

####################################
        
if __name__=='__main__':
    test()
