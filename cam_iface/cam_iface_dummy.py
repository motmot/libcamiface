import time, random
import numpy

def get_wrapper_name():
    return 'dummy'

def get_driver_name():
    return 'dummy'

num_cameras = 1

def set_num_cameras(n):
    global num_cameras
    num_cameras = n

def get_num_cameras():
    global num_cameras
    return num_cameras

def get_camera_info(index):
    return 'Dummy Vendor', 'Dummy Model', 'Dummy Chip ID'

class Camera:
    def __init__(self,*args,**kw):
        self.w = 656
        self.h = 491
        self.zim = numpy.zeros( (self.h,self.w), dtype=numpy.uint8 )
        self.last_timestamp = None
        self.last_framenumber = 0
        self.set_camera_property = self.noop
    def get_pixel_coding(self):
        return 'MONO8'
    def get_frame_size(self):
        return self.w, self.h
    def get_max_width(self):
        return self.w
    def get_max_height(self):
        return self.h
    def noop(self,*args,**kw):
        pass
    def get_num_camera_properties(self):
        return 0
    def get_camera_property(self,*args,**kw):
        raise ValueError("not a valid property")
    def get_camera_property_range(self,*args,**kw):
        raise ValueError("not a valid property")
    def get_trigger_mode_number(self):
        return 0
    def get_framerate(self,*args,**kw):
        return 100.0
    def start_camera(self):
        return
    def grab_next_frame_into_buf_blocking(self,buf):
        a = numpy.asarray(buf)
        self.last_timestamp = time.time()
        self.last_framenumber += 1
        a[:,:] = self.zim # copy data
        for i in range(3):
            y = random.randint(0,self.h-1)
            x = random.randint(0,self.w-1)
            a[y,x] = 200
            if y==0:
                a[y+1,x]=190
            else:
                a[y-1,x]=190

    def grab_next_frame_blocking(self):
        buf = numpy.empty((self.h,self.w),dtype=numpy.uint8)
        self.grab_next_frame_into_buf_blocking(buf)
        return buf

    def get_last_timestamp(self):
        return self.last_timestamp
    def get_last_framenumber(self):
        return self.last_framenumber
    def get_frame_offset(self):
        return 0,0
    def set_frame_offset(self, left, top):
        if left!=0 or top!=0:
            raise RuntimeError('not supported')
    def set_frame_size(self, width, height):
        if width!=self.w or height!=self.h:
            raise RuntimeError('not supported')
        
class CamIFaceError(Exception):
    pass
class BuffersOverflowed(Exception):
    pass
