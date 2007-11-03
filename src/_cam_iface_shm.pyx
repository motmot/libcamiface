#emacs, this is -*-Python-*- mode
# shared memory backend for cam_iface

cimport c_lib
cimport c_python
cimport c_lib_shm

import sys
import numpy

# numpy's __array_struct__ interface:
ctypedef struct PyArrayInterface:
    int two                       # contains the integer 2 as a sanity check
    int nd                        # number of dimensions
    char typekind                 # kind in array --- character code of typestr
    int itemsize                  # size of each element
    int flags                     # flags indicating how the data should be interpreted
    c_python.Py_intptr_t *shape   # A length-nd array of shape information
    c_python.Py_intptr_t *strides # A length-nd array of stride information
    void *data                    # A pointer to the first element of the array

cdef extern from "shmwrap.h":
    # defined in shmwrap.h
    char *shmwrap_ftok_path
    char shmwrap_shm_name
    c_lib.size_t shm_size
    int shmwrap_frame_ready_port
    int shmwrap_control_port
    ctypedef struct shmwrap_msg_ready_t:
        int cam_id
        int sendnumber
        int framenumber
        int width
        int height
        int roi_x
        int roi_y
        double timestamp
        int start_offset
        int stride
        
def perror(msg):
    print msg

cdef char* _get_shm_pointer():
    # Get key
    cdef c_lib_shm.key_t key
    cdef int shmid
    cdef char* data
    
    key = c_lib_shm.ftok(shmwrap_ftok_path, shmwrap_shm_name)
    
    if key==-1:
        perror("ftok")
        sys.exit(1)

    shmid = c_lib_shm.shmget(key, shm_size, 0644);
    if shmid==-1:
        perror("shmget")
        sys.exit(1)
        
    data = <char*>c_lib_shm.shmat(shmid, <void*>0, c_lib_shm.SHM_RDONLY);
    if data == <char*>(-1):
        perror("shmat")
        sys.exit(1)

    return data
        
cdef class ShmManager:
    cdef char *data
    def __init__(self):
        self.data = _get_shm_pointer()
    
cdef shmwrap_msg_ready_t _tmp_curmsg 
shmwrap_msg_ready_t_size = sizeof(_tmp_curmsg)

shmwrap_frame_ready_port_ = shmwrap_frame_ready_port
shmwrap_control_port_ = shmwrap_control_port

cdef ShmManager _shm_manager
_shm_manager = ShmManager()

class Stuff:
    pass

def buf2class(buf):
    cdef char* cbuf
    cdef shmwrap_msg_ready_t *pack
    
    cbuf = buf # convert to C
    pack = <shmwrap_msg_ready_t *>&(cbuf[0])
    result = Stuff()
    result.cam_id = pack.cam_id
    result.sendnumber = pack.sendnumber
    result.framenumber = pack.framenumber
    result.width = pack.width
    result.height = pack.height
    result.roi_x = pack.roi_x
    result.roi_y = pack.roi_y
    result.timestamp = pack.timestamp
    result.start_offset = pack.start_offset
    result.stride = pack.stride
    return result
    
def get_data_copy(curmsg, optional_preallocated_buf=None):
    cdef char* source_ptr
    cdef int size
    cdef unsigned long start_offset
    cdef int row
    cdef int width, height
    cdef int source_stride,dest_stride,rowoffset
    cdef void* dest_ptr
    #cdef c_python.Py_ssize_t buflen
    cdef int buflen
    cdef PyArrayInterface* inter
    
    if optional_preallocated_buf is not None:
        optional_preallocated_buf = numpy.asarray(optional_preallocated_buf)
        assert len(optional_preallocated_buf.shape)==2
        assert optional_preallocated_buf.shape[0] == curmsg.height
        assert optional_preallocated_buf.shape[1] >= curmsg.width
        assert optional_preallocated_buf.dtype == numpy.uint8
        dest_stride = optional_preallocated_buf.strides[0]

    width = curmsg.width
    height = curmsg.height
    source_stride = curmsg.stride
    start_offset = curmsg.start_offset
    
    source_ptr = _shm_manager.data + start_offset
    size = height*source_stride
    
    if optional_preallocated_buf is not None:
        result = optional_preallocated_buf
        # keep reference to prevent dealloc:
        hold_onto_until_done_with_array = result.__array_struct__ 
        inter = <PyArrayInterface*>c_python.PyCObject_AsVoidPtr( hold_onto_until_done_with_array )
        assert inter.two == 2
        
        #dest_buf = result.data
        #c_python.PyObject_AsWriteBuffer( dest_buf, &dest_ptr, &buflen)
        for row from 0<=row<height:
##            c_lib.memcpy( dest_ptr + row*dest_stride,   # dest
##                          source_ptr+row*source_stride, # src
##                          width )        # size
            c_lib.memcpy( inter.data + row*dest_stride,   # dest
                          source_ptr+row*source_stride, # src
                          width )        # size
    else:
        buf = c_python.PyString_FromStringAndSize( source_ptr, size )
        result1 = numpy.fromstring(buf,dtype=numpy.uint8)
        result1.shape = curmsg.height, curmsg.stride
        result = result1[:,:curmsg.width]
    return result


cdef PyArrayInterface* inter

    
