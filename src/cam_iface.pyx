#emacs, this is -*-Python-*- mode
# $Id: cam_iface.pyx 1273 2006-08-03 10:19:46Z astraw $

cimport c_cam_iface
cimport c_python
cimport c_lib

# This code uses __array_struct__ interface.

# for PyArrayInterface:
CONTIGUOUS=0x01
FORTRAN=0x02
ALIGNED=0x100
NOTSWAPPED=0x200
WRITEABLE=0x400

ctypedef struct PyArrayInterface:
    int version                   # contains the integer 2 as a sanity check
    int nd                        # number of dimensions
    char typekind                 # kind in array --- character code of typestr
    int itemsize                  # size of each element
    int flags                     # flags indicating how the data should be interpreted
    c_python.Py_intptr_t *shape   # A length-nd array of shape information
    c_python.Py_intptr_t *strides # A length-nd array of stride information
    void *data                    # A pointer to the first element of the array

cdef void free_array_interface( void* ptr, void *arr ):
    arrpy = <object>arr
    c_python.Py_DECREF( arrpy )
    c_lib.free(ptr)

ctypedef unsigned char fi
ctypedef fi* fiptr
cdef class CamIfaceBuf:
    """iff depth is 8 bits, self.shape[1]==image width"""
    cdef fiptr data
    cdef c_python.Py_intptr_t shape[2]
    cdef c_python.Py_intptr_t strides[2]
    cdef int view
    
    def __new__(self,*args,**kw):
        self.view = 1
        self.strides[1]=1 # 1 byte per element (contiguous)
        
    def __init__(self, int width_bytes, int height):
        cdef int bufsize
        bufsize = width_bytes*height
        self.data=<fiptr>c_lib.malloc( bufsize )
        if self.data==NULL: raise MemoryError("Error allocating memory")
        self.strides[0]=width_bytes
        
        self.shape[0]=height
        self.shape[1]=width_bytes
    
        self.view = 0
        
    def __dealloc__(self):
        if not self.view:
            c_lib.free(self.data)
            
    def dump_to_file(self,fobj):
        cdef int nbytes
        cdef c_lib.FILE* fd

        if not c_python.PyFile_Check(fobj):
            raise ValueError('need file object')
        fd = c_python.PyFile_AsFile(fobj)

        c_python.Py_BEGIN_ALLOW_THREADS
        nbytes = c_lib.fwrite( self.data,
                               1,
                               self.shape[0]*self.shape[1],
                               fd )
        c_python.Py_END_ALLOW_THREADS
        return nbytes

    property __array_struct__:
        def __get__(self):
            cdef PyArrayInterface* inter

            inter = <PyArrayInterface*>c_lib.malloc( sizeof( PyArrayInterface ) )
            inter.version = 2
            inter.nd = 2
            inter.typekind = 'u'[0] # unsigned int
            inter.itemsize = 1
            inter.flags = NOTSWAPPED | ALIGNED | WRITEABLE
            inter.strides = self.strides
            inter.shape = self.shape
            inter.data = self.data
            c_python.Py_INCREF(self)
            obj = c_python.PyCObject_FromVoidPtrAndDesc( <void*>inter, <void*>self, free_array_interface)
            return obj
            
def get_driver_name():
    return c_cam_iface.cam_iface_get_driver_name()

def get_num_cameras():
    return c_cam_iface.cam_iface_get_num_cameras()

def get_camera_info(index):
    cdef c_cam_iface.Camwire_id camid
    c_cam_iface.cam_iface_get_camera_info(index, &camid)
    _check_error()
    return camid.vendor, camid.model, camid.chip

# define Python exception
class CamIFaceError(Exception):
    pass

class BuffersOverflowed(CamIFaceError):
    pass

class FrameDataMissing(CamIFaceError):
    pass

class HardwareFeatureNotAvailable(CamIFaceError):
    pass

# error checking routine
cdef void _check_error() except *:
    cdef char* err_str

    errnum = c_cam_iface.cam_iface_have_error()
    if errnum != 0:
        err_str=c_cam_iface.cam_iface_get_error_string()
        
        if errnum == c_cam_iface.CAM_IFACE_BUFFER_OVERFLOW_ERROR:
            exc_type = BuffersOverflowed
        elif errnum == c_cam_iface.CAM_IFACE_FRAME_DATA_MISSING_ERROR:
            exc_type = FrameDataMissing
        elif errnum == c_cam_iface.CAM_IFACE_HARDWARE_FEATURE_NOT_AVAILABLE:
            exc_type = HardwareFeatureNotAvailable
        else:
            exc_type = CamIFaceError
        c_cam_iface.cam_iface_clear_error()
        raise exc_type(err_str)
    
def get_num_modes(int device_number):
    cdef int num_modes
    c_cam_iface.cam_iface_get_num_modes(device_number, &num_modes)
    _check_error()
    return num_modes

def get_mode_string(int device_number,
                    int mode_number):
    cdef char mode_string[255]
    c_cam_iface.cam_iface_get_mode_string(device_number,
                                          mode_number,
                                          &(mode_string[0]),
                                          255)
    _check_error()
    py_mode_string = c_python.PyString_FromString(mode_string)
    return py_mode_string
    
# wrap C calls in OO interface
ctypedef class Camera:
    cdef c_cam_iface.CamContext* cval
    
    def __init__(self,int device_number, int num_buffers, int mode_number):
        self.cval = c_cam_iface.new_CamContext(device_number,num_buffers,
                                               mode_number)
        _check_error()
        
    def __del__(self):
        c_cam_iface.delete_CamContext(self.cval)
        _check_error()

    def close(self):
        c_cam_iface.delete_CamContext(self.cval)
        _check_error()

    def __getattr__(self,name):
        cdef int max_width, max_height
        if name == 'max_width':
            c_cam_iface.CamContext_get_max_frame_size(self.cval,&max_width,&max_height)
            _check_error()
            return max_width
        elif name == 'max_height':
            c_cam_iface.CamContext_get_max_frame_size(self.cval,&max_width,&max_height)
            _check_error()
            return max_height
        else:
            raise AttributeError('class %s has no (implemented) attribute "%s"'%('Camera',name))

    def get_pixel_coding(self):
        if self.cval.coding == c_cam_iface.CAM_IFACE_UNKNOWN:
            return 'UNKNOWN'
        elif self.cval.coding == c_cam_iface.CAM_IFACE_MONO8:
            return 'MONO8'
        elif self.cval.coding == c_cam_iface.CAM_IFACE_MONO16:
            return 'MONO16'
        elif self.cval.coding == c_cam_iface.CAM_IFACE_YUV411:
            return 'YUV411'
        elif self.cval.coding == c_cam_iface.CAM_IFACE_YUV422:
            return 'YUV422'
        elif self.cval.coding == c_cam_iface.CAM_IFACE_YUV444:
            return 'YUV444'
        elif self.cval.coding == c_cam_iface.CAM_IFACE_RGB8:
            return 'RGB8'
        else:
            raise NotImplementedError("Can't convert pixel coding representation to string")
        
    def get_pixel_depth(self):
        return self.cval.depth

    def get_num_trigger_modes(self):
        cdef int num_modes
        c_cam_iface.CamContext_get_num_trigger_modes(self.cval, &num_modes)
        _check_error()
        return num_modes

    def get_trigger_mode_string(self,
                                int mode_number):
        cdef char mode_string[255]
        c_cam_iface.CamContext_get_trigger_mode_string(self.cval,
                                                       mode_number,
                                                       &(mode_string[0]),
                                                       255)
        _check_error()
        py_mode_string = c_python.PyString_FromString(mode_string)
        return py_mode_string

    def get_trigger_mode_number(self):
        cdef int external
        c_cam_iface.CamContext_get_trigger_mode_number(self.cval, &external)
        _check_error()
        return external
        
    def set_trigger_mode_number(self, external):
        c_cam_iface.CamContext_set_trigger_mode_number(self.cval, external)
        _check_error()
        
    def get_frame_offset(self):
        cdef int left, top
        c_cam_iface.CamContext_get_frame_offset(self.cval, &left, &top)
        _check_error()
        return left, top
        
    def set_frame_offset(self, left, top):
        c_cam_iface.CamContext_set_frame_offset(self.cval, left, top)
        _check_error()

    def get_frame_size(self):
        cdef int width, height
        c_cam_iface.CamContext_get_frame_size(self.cval, &width, &height)
        _check_error()
        return width, height
        
    def set_frame_size(self, width, height):
        c_cam_iface.CamContext_set_frame_size(self.cval, width, height)
        _check_error()
        
    def get_framerate(self):
        cdef float framerate
        c_cam_iface.CamContext_get_framerate(self.cval, &framerate)
        _check_error()
        return framerate
        
    def set_framerate(self, framerate):
        c_cam_iface.CamContext_set_framerate(self.cval, framerate)
        _check_error()
        
    def get_num_framebuffers(self):
        cdef int num_framebuffers
        c_cam_iface.CamContext_get_num_framebuffers(self.cval, &num_framebuffers)
        _check_error()
        return num_framebuffers
        
    def set_num_framebuffers(self, num_framebuffers):
        c_cam_iface.CamContext_set_num_framebuffers(self.cval, num_framebuffers)
        _check_error()
        
    def get_num_camera_properties(self):
        cdef int num_properties
        c_cam_iface.CamContext_get_num_camera_properties(self.cval,
                                                         &num_properties)
        _check_error()
        return num_properties
    
    def get_camera_property_info(self, property_number):
        cdef c_cam_iface.CameraPropertyInfo info
        c_cam_iface.CamContext_get_camera_property_info(self.cval,
                                                        property_number,
                                                        &info)
        _check_error()
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
                            long Value,
                            int Auto):
        c_cam_iface.CamContext_set_camera_property(self.cval,
                                                   property_number,
                                                   Value,
                                                   Auto )
        _check_error()
            
    def get_camera_property(self,property_number):
        cdef long Value
        cdef int Auto
        c_cam_iface.CamContext_get_camera_property(self.cval,
                                                   property_number,
                                                   &Value,
                                                   &Auto )
        _check_error()
        return (Value, Auto)

    def grab_next_frame_into_alloced_buf_blocking(self,object buf_alloc,int bypass_buffer_checks=0):
        """grab frame into a newly allocated buffer using user allocation function"""
        cdef int h, w
        c_cam_iface.CamContext_get_frame_size(self.cval,&w,&h)
        _check_error()
        buf = buf_alloc((w*self.cval.depth)/8,h)
        self.grab_next_frame_into_buf_blocking(buf,bypass_buffer_checks=bypass_buffer_checks)
        return buf
        
    def grab_next_frame_into_buf_blocking(self,object buf,int bypass_buffer_checks=0):
        """grab frame into user-supplied buffer"""
        cdef int h, w
        cdef PyArrayInterface* inter
        
        c_cam_iface.CamContext_get_frame_size(self.cval,&w,&h)
        _check_error()

        attr = buf.__array_struct__
        if not c_python.PyCObject_Check(attr):
            raise ValueError("invalid __array_struct__")
        inter = <PyArrayInterface*>c_python.PyCObject_AsVoidPtr(attr)
        if not bypass_buffer_checks:
            if inter.version != 2:
                raise ValueError("invalid __array_struct__")
            # TODO: don't really know what these flags all mean, figure out if
            # this is OK:
            if (inter.flags & (ALIGNED | WRITEABLE)) != (ALIGNED | WRITEABLE):
                raise ValueError("cannot handle misaligned or not writeable arrays.")
            if inter.nd != 2:
                raise ValueError("only 2D arrays may used as a framebuffer")
            if not (inter.typekind == "u"[0] and inter.itemsize==1):
                raise ValueError("only arrays of unsigned integers of 1 byte may used as a framebuffer")
            if inter.strides[1] != 1:
                raise ValueError("only arrays of strides[1]==1 may used as a framebuffer")
            if not (inter.shape[0]==h):
                raise ValueError("framebuffer array height (%d) does not match camera frame height (%d)"%(
                    inter.shape[0],h))
            if not (inter.shape[1]==(w*self.cval.depth)/8):
                raise ValueError("framebuffer array height (%d) does not match camera frame height (%d)"%(
                    inter.shape[0],h))
        c_python.Py_BEGIN_ALLOW_THREADS
        c_cam_iface.CamContext_grab_next_frame_blocking_with_stride(self.cval,<unsigned char*>inter.data,
                                                                    inter.strides[0])
        c_python.Py_END_ALLOW_THREADS
        _check_error()
    
    def grab_next_frame_blocking(self):
        """grab frame into a newly allocated buffer"""
        cdef int h, w
        cdef PyArrayInterface* inter
        cdef CamIfaceBuf buf
        
        c_cam_iface.CamContext_get_frame_size(self.cval,&w,&h)
        _check_error()
        buf=CamIfaceBuf((w*self.cval.depth)/8,h)
        c_python.Py_BEGIN_ALLOW_THREADS
        c_cam_iface.CamContext_grab_next_frame_blocking(self.cval,buf.data)
        c_python.Py_END_ALLOW_THREADS
        _check_error()
        return buf
    
    def point_next_frame_blocking(self):
        cdef CamIfaceBuf framebuffer
        cdef int h, w
        c_cam_iface.CamContext_get_frame_size(self.cval,&w,&h)
        _check_error()
        framebuffer = CamIfaceBuf.__new__(CamIfaceBuf) # allocate C struct, but don't malloc memory buffer
        c_python.Py_BEGIN_ALLOW_THREADS        
        c_cam_iface.CamContext_point_next_frame_blocking(self.cval,
                                                         &framebuffer.data)
        c_python.Py_END_ALLOW_THREADS
        _check_error()
        framebuffer.strides[0]=(w*self.cval.depth)/8
        framebuffer.shape[0]=h
        framebuffer.shape[1]=(w*self.cval.depth)/8
        return framebuffer
        
    def unpoint_frame(self):
        c_cam_iface.CamContext_unpoint_frame(self.cval)
        _check_error()

    def start_camera(self):
        c_cam_iface.CamContext_start_camera(self.cval)
        _check_error()

    def stop_camera(self):
        c_cam_iface.CamContext_stop_camera(self.cval)
        _check_error()

    def get_max_height(self):
        cdef int max_width, max_height
        c_cam_iface.CamContext_get_max_frame_size(self.cval,&max_width,&max_height)
        _check_error()
        return max_height

    def get_max_width(self):
        cdef int max_width, max_height
        c_cam_iface.CamContext_get_max_frame_size(self.cval,&max_width,&max_height)
        _check_error()        
        return max_width    

    def get_last_timestamp(self):
        cdef double timestamp
        c_cam_iface.CamContext_get_last_timestamp(self.cval,&timestamp)
        _check_error()
        return timestamp

    def get_last_framenumber(self):
        cdef long framenumber
        c_cam_iface.CamContext_get_last_framenumber(self.cval,&framenumber)
        _check_error()
        return framenumber

class _CamIfaceKeeper:
    def __init__(self):
        c_cam_iface.cam_iface_startup_with_version_check()
        _check_error()
    def __del__(self):
        c_cam_iface.cam_iface_shutdown()
        _check_error()
_cam_iface_keeper = _CamIfaceKeeper()

class FakeCamContext(object):
    def __init__(self, *args,**kw):
        import time
        self.real = RealCamContext(*args,**kw)
    def start_camera(self):
        print 'start camera',time.time()
        print
        result = self.real.start_camera()
        #time.sleep(0.5)
        return result
    def __getattr__(self, name):
        print 'getattr:', name, time.time()
        result = getattr(self.real, name)
        print type(result)
        print
        return result

def get_wrapper_name():
    return 'pyrex'

