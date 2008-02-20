import numpy

def get_camera_info(device_number):
    return {'vendor':'(python: no vendor)',
            'model':'(python: no model)',
            'chip':'(python: no chip)',
            }

def get_num_cameras():
    return 1

def get_num_modes(device_number):
    return 1

def get_mode_string(device_number, mode_number):
    return "(default Python mode string)"

class CamContextPy(object):
    def __init__(self, coding=None, depth=None, device_number=None):
        self.coding = 1 # CAM_IFACE_MONO8
        self.depth = 8
        self.device_number = device_number

    def get_frame_size(self):
        width,height=640,480
        return width,height

    def get_max_frame_size(self):
        width,height=640,480
        return width,height

    def set_num_framebuffers(self,n):
        self._num_framebuffers = n

    def get_num_framebuffers(self):
        return self._num_framebuffers

    def get_num_camera_properties(self):
        return 0

    def get_buffer_size(self):
        w,h = self.get_frame_size()
        return w*h*self.depth//8

    def start_camera(self):
        return

    # XXX TODO: make INPUTOUTPUT type and framebuffer should be it
    def grab_next_frame_blocking(self,timeout):
        w,h = self.get_frame_size()
        assert self.depth==8
        return numpy.zeros( (h,w), dtype=numpy.uint8 )
