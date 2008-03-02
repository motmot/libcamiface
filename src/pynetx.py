import socket
import ctypes
import sys,os
from ctypes import *
import _pynet # defined in cam_iface_pynet.c
_set_vmt_ptr_and_size = _pynet._set_vmt_ptr_and_size
import numpy

if int(os.environ.get('PYNET_SHOWPATH','0')):
    print 'pynet sys.path', sys.path

PYNET_DEBUG_CALLS=int(os.environ.get('PYNET_DEBUG_CALLS','0'))

module_name = os.environ.get('PYNET_IMPLEMENTATION_MODULE',None)
if module_name is None:
    raise RuntimeError('The implementation of pynet must be specified '
                       'by the PYNET_IMPLEMENTATION_MODULE environment '
                       'variable')

def my_import(name):
    mod = __import__(name)
    components = name.split('.')
    for comp in components[1:]:
        mod = getattr(mod, comp)
    return mod
module = my_import( module_name )

global keepalive, pycmap
pycmap = {}

# Prevent reference counting from removing objects we still want by
# keeping reference:
keepalive = []

# Fixup ctypes

ctypes.pythonapi.PyCObject_AsVoidPtr.restype = ctypes.c_void_p
ctypes.pythonapi.PyCObject_AsVoidPtr.argtypes = [ ctypes.py_object ]

_pynet_fname = os.path.abspath(_pynet.__file__)
lib_pynet = ctypes.cdll.LoadLibrary(_pynet_fname)
lib_pynet.c_set_vmt.restype = None
lib_pynet.c_set_vmt.argtypes = [ ctypes.c_void_p ]

### Type definitions ######################
class Camwire_id(ctypes.Structure):
    _fields_ = [('vendor',ctypes.c_char*101),
                ('model',ctypes.c_char*101),
                ('chip',ctypes.c_char*101),
                ]
class CamContext(ctypes.Structure):
    pass # forward declaration
class CCpynet(ctypes.Structure):
    pass # forward declaration
class CamContext_functable(ctypes.Structure):
    pass # forward declaration
class CameraPropertyInfo(ctypes.Structure):
    pass

PTR_CamContext_t = ctypes.POINTER( CamContext )
PTR_CCpynet_t = ctypes.POINTER( CCpynet )
PTR_CamContext_functable_t = ctypes.POINTER( CamContext_functable )
PTR_CameraPropertyInfo_t = ctypes.POINTER( CameraPropertyInfo )


# modified from numpy:
def _getintp_ctype():
    char = numpy.dtype('p').char
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

cam_iface_constructor_func_t = CFUNCTYPE( PTR_CamContext_t, ctypes.c_int, ctypes.c_int, ctypes.c_int )

CamContext._fields_ = [
    ('vmt',PTR_CamContext_functable_t),
    ('cam',ctypes.c_void_p),
    ('backend_extras',ctypes.c_void_p),
    ('coding',ctypes.c_int),
    ('depth',ctypes.c_int),
    ('device_number',ctypes.c_int),
    ]
CCpynet._fields_ = [
    ("inherited", CamContext),
    ('self',ctypes.py_object),
    ]
CamContext_functable._fields_ = [
    # for the benefit of ctypes, we are naming these field types PTR_CCpynet_t instead of PTR_CamContext_t
    ('construct',cam_iface_constructor_func_t),
    ('destruct', CFUNCTYPE( None, PTR_CCpynet_t)),
    ('CamContext',CFUNCTYPE( None, PTR_CCpynet_t,c_int,c_int,c_int)),
    ('close',CFUNCTYPE( None, PTR_CCpynet_t)),
    ('start_camera',CFUNCTYPE( None, PTR_CCpynet_t)),
    ('stop_camera',CFUNCTYPE( None, PTR_CCpynet_t)),
    ('get_num_camera_properties',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(c_int))),
    ('get_camera_property_info',CFUNCTYPE( None, PTR_CCpynet_t, c_int, PTR_CameraPropertyInfo_t )),
    ('get_camera_property', CFUNCTYPE( None, PTR_CCpynet_t, c_int, POINTER(c_long), POINTER(c_int))),
    ('set_camera_property',CFUNCTYPE( None, PTR_CCpynet_t, c_int, c_long, c_int)),
    ('grab_next_frame_blocking',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(c_ubyte), c_float)),
    ('grab_next_frame_blocking_with_stride',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(c_ubyte), intptr_type, c_float)),
    ('point_next_frame_blocking',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(POINTER(c_ubyte)), c_float)),
    ('unpoint_frame',CFUNCTYPE( None, PTR_CCpynet_t)),
    ('get_last_timestamp',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(c_double))),
    ('get_last_framenumber',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(c_long))),
    ('get_num_trigger_modes',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(c_int))),
    ('get_trigger_mode_string',CFUNCTYPE( None, PTR_CCpynet_t, c_int, POINTER(c_char), c_int)),
    ('get_trigger_mode_number',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(c_int))),
    ('set_trigger_mode_number',CFUNCTYPE( None, PTR_CCpynet_t, c_int)),
    ('get_frame_offset',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(c_int), POINTER(c_int))),
    ('set_frame_offset',CFUNCTYPE( None, PTR_CCpynet_t, c_int, c_int)),
    ('get_frame_size',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(c_int), POINTER(c_int))),
    ('set_frame_size',CFUNCTYPE( None, PTR_CCpynet_t, c_int, c_int)),
    ('get_max_frame_size',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(c_int), POINTER(c_int))),
    ('get_buffer_size',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(c_int))),
    ('get_framerate',CFUNCTYPE( None, PTR_CCpynet_t, POINTER(c_float))),
    ('set_framerate',CFUNCTYPE( None, PTR_CCpynet_t, c_float)),
    ('get_num_framebuffers',CFUNCTYPE( None, PTR_CCpynet_t,POINTER(c_int))),
    ('set_num_framebuffers',CFUNCTYPE( None, PTR_CCpynet_t,c_int)),
    ]
CamContext_functable_signatures = {}
for name,sig in CamContext_functable._fields_:
    CamContext_functable_signatures[name]=sig

# Python/C type-map stuff ######################################

def get_self(this):
    """INPUT: convert PTR_CCpynet_t into CamContextPy instance"""
    return this.contents.self
def get_value(cval):
    """INPUT: return cval.value"""
    return cval.value
def set_ptr_contents(cval,pyval):
    """OUTPUT: convert set the memory pointed to by cval to the value in pyval"""
    cval[0] = pyval
def convert_framebuffer(cval,pyval):
    """OUTPUT: copy numpy array into cval contents"""
    assert len(pyval.shape)==2
    h,w = pyval.shape
    stride0 = pyval.strides[0]
    bufsize = stride0*h
    # XXX TODO not finished
def copy_string_to_C(cval0, cval1, pyval):
    #cval0 is a char*
    #cval1 is a length
    #pyval is a string
    cval0[0] = '\0' # XXX TODO not finished - null string

INPUT = 'input'
OUTPUT = 'output'
OUTPUT2 = 'output2' # consumes 2 C arguments

class generate_func(object):
    def __init__(self,name,ios):
        self.name = name
        self.ios = ios
        self.method = getattr(CamContextPy,self.name)
    def __call__(self,*args):
        if PYNET_DEBUG_CALLS:
            print 'calling 1',self.name
            print ' with args',args
        newargs = []
        next_cidx = 0
        for (DIRECTION, transform_func) in self.ios:
            cidx = next_cidx
            if DIRECTION==OUTPUT2:
                next_cidx = cidx+2
            else:
                next_cidx = cidx+1

            if DIRECTION==INPUT:
                #print 'cidx',cidx
                if transform_func is None:
                    pyval = args[cidx]
                else:
                    pyval = transform_func( args[cidx] )
                newargs.append( pyval )

        if PYNET_DEBUG_CALLS:
            print 'calling 2',self.name
        results = self.method(*newargs)
        if not isinstance(results,tuple):
            # contain result within a tuple for indexing as sequence
            results = (results,)
        pyidx = 0
        if PYNET_DEBUG_CALLS:
            print 'self.name, results, ios:',self.name, results, self.ios
        next_cidx = 0
        for (DIRECTION, transform_func) in self.ios:
            cidx = next_cidx
            if DIRECTION==OUTPUT2:
                next_cidx = cidx+2
            else:
                next_cidx = cidx+1
            if PYNET_DEBUG_CALLS:
                print 'cidx,pyidx,args,transform_func:',cidx,pyidx,args,transform_func
            if DIRECTION==OUTPUT:
                if transform_func is None:
                    args[cidx] = results[pyidx]
                else:
                    transform_func( args[cidx], results[pyidx] )
                pyidx += 1
            elif DIRECTION==OUTPUT2:
                assert transform_func is not None
                transform_func( args[cidx], args[cidx+1], results[pyidx] )
                pyidx += 1
        if PYNET_DEBUG_CALLS:
            print 'done'

#########################################################

class cinfo:
    def __init__(self, carglist ):
        self.carglist = [(INPUT,get_self)]+list(carglist) # prepend self conversion
    def __call__(self,f):
        pycmap[f.__name__] = self.carglist
        return f

# implementation #########################

def get_camera_info(*args,**kw):
    return module.get_camera_info(*args,**kw)

def get_num_cameras(*args,**kw):
    return module.get_num_cameras(*args,**kw)

def get_num_modes(*args,**kw):
    return module.get_num_modes(*args,**kw)

def get_mode_string(*args,**kw):
    return module.get_mode_string(*args,**kw)

class CamContextPy(object):
    def __init__(self, *args, **kw):
        self.me = module.CamContextPy(*args,**kw)

    @cinfo( [(OUTPUT,set_ptr_contents), (OUTPUT, set_ptr_contents)] )
    def get_frame_size(self, *args, **kw):
        return self.me.get_frame_size(*args,**kw)

    @cinfo( [(OUTPUT,set_ptr_contents), (OUTPUT, set_ptr_contents)] )
    def get_max_frame_size(self, *args, **kw):
        return self.me.get_max_frame_size(*args,**kw)

    @cinfo( [(INPUT,None)])
    def set_num_framebuffers(self, *args, **kw):
        return self.me.set_num_framebuffers(*args,**kw)

    @cinfo( [(OUTPUT,set_ptr_contents)])
    def get_num_framebuffers(self, *args, **kw):
        return self.me.get_num_framebuffers(*args,**kw)

    @cinfo( [(OUTPUT,set_ptr_contents)])
    def get_num_camera_properties(self, *args, **kw):
        return self.me.get_num_camera_properties(*args,**kw)

    @cinfo( [(OUTPUT,set_ptr_contents)])
    def get_buffer_size(self, *args, **kw):
        return self.me.get_buffer_size(*args,**kw)

    @cinfo([(OUTPUT,set_ptr_contents)])
    def get_num_trigger_modes(self, *args, **kw):
        return self.me.get_num_trigger_modes(*args, **kw)

    @cinfo([(INPUT,None),(OUTPUT2,copy_string_to_C)])
    def get_trigger_mode_string(self, *args, **kw):
        return self.me.get_trigger_mode_string(*args, **kw)

    @cinfo([])
    def start_camera(self, *args, **kw):
        return self.me.start_camera(*args,**kw)

    # XXX TODO: make INPUTOUTPUT type and framebuffer should be it
    @cinfo([(OUTPUT,convert_framebuffer),(INPUT,None)])
    def grab_next_frame_blocking(self, *args, **kw):
        return self.me.grab_next_frame_blocking(*args,**kw)

def new_CamContextPy(CCpynet_inst,device_number,NumImageBuffers,mode_number ):
    # this is called with the CamContextpynet malloced, but it's job is to fill it
    this = cast( ctypes.pythonapi.PyCObject_AsVoidPtr( CCpynet_inst ), PTR_CCpynet_t )
    global keepalive

    self = CamContextPy(device_number = device_number)
    self.set_num_framebuffers( NumImageBuffers )

    this.contents.inherited.coding = self.me.coding
    this.contents.inherited.depth = self.me.depth
    this.contents.inherited.device_number = self.me.device_number
    this.contents.self = self
    keepalive.append( (this, self) )

def close_CamContextPy(CCpynet_inst):
    # remove everything from the Python side associated with this instance
    this = cast( ctypes.pythonapi.PyCObject_AsVoidPtr( CCpynet_inst ), PTR_CCpynet_t )
    instance = this.contents.self
    global keepalive

    idx = None
    for i,(test_this, test_instance) in enumerate(keepalive):
        if instance==test_instance:
            idx=i
            break
    if idx is not None:
        del keepalive[idx]
    else:
        raise ValueError("could not find object associated with %s"%CCpynet_inst)

class not_implemented_func:
    def __init__(self,name):
        self.name = name
    def __call__(self,*args,**kw):
        print 'not implemented function "%s" called - an exception is coming.'%self.name
        raise NotImplementedError('XXX TODO')

def create_vmt( ):
    vmt = CamContext_functable()
    global pycmap
    for name in CamContext_functable_signatures.keys():
        if name in ['construct','destruct']:
            continue # special cases handled by _pynet
        try:
            ios = pycmap[name]
        except KeyError,err:
            # function not specified
            my_func = not_implemented_func(name)
        else:
            my_func = generate_func(name,ios)
        signature = CamContext_functable_signatures[name]
        cfunc = signature( my_func )
        ## if name=='get_frame_size':
        ##     print 'python: %s 0x%x %s'%(name,addressof(cfunc),repr(cfunc))
        setattr(vmt, name, cfunc )
    return vmt

#########################################################

def initialize():
    vmt = create_vmt()
    lib_pynet.c_set_vmt( ctypes.byref(vmt) )
