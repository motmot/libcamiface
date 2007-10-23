import _cam_iface_shm

import socket, time, struct
import numpy

shmwrap_msg_ready_t_fmt = 'BBLHHHHdLH'
shmwrap_msg_ready_t_fmt_size = struct.calcsize(shmwrap_msg_ready_t_fmt)

class _GlobalCameraState:
    pass
_camera_state = _GlobalCameraState()

class _PerCameraState:
    pass

def printf(msg):
    print msg

def get_wrapper_name():
    return 'sharedmem'

def get_driver_name():
    return 'sharedmem'

def get_num_modes(cam_no):
    return 1

def get_mode_string(cam_no,i):
    return 'default'

def _startup():
    global _camera_state

    tcp_control_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #tcp_control_socket.setsockopt(socket.SOL_SOCKET, socket.TCP_NODELAY, 1)
    tcp_control_socket.connect(('127.0.0.1',_cam_iface_shm.shmwrap_control_port_))
    tcp_control_socket.send('info\r\n')
    buf = tcp_control_socket.recv(4096)

    assert buf.startswith('[general]\r\ncameras: cam1\r\n[cam1]\r\nresolution: ')
    res_string = buf.replace('[general]\r\ncameras: cam1\r\n[cam1]\r\nresolution: ','').strip()
    
    resolution = map(int,res_string.split('x'))
    cam = _PerCameraState()
    cam.w, cam.h = resolution
    _camera_state.cams = [cam]

    udp_listen_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_listen_sock.bind(('127.0.0.1', _cam_iface_shm.shmwrap_frame_ready_port_ ))
    
    _camera_state.udp_listen_sock = udp_listen_sock
    _camera_state.tcp_control_socket = tcp_control_socket

def get_num_cameras():
    global _camera_state
    return len(_camera_state.cams)

def get_camera_info(index):
    return 'Sharedmem Vendor', 'Sharedmem Model', 'Sharedmem Chip ID'

class Camera:
    def __init__(self,device_number,num_buffers,mode_number):
        assert (0<=device_number) and (device_number<get_num_cameras())
        self.w = _camera_state.cams[device_number].w
        self.h = _camera_state.cams[device_number].h
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

    def _get_network_buf(self):
        buf, (remote_ip,remote_port) = _camera_state.udp_listen_sock.recvfrom(
            shmwrap_msg_ready_t_fmt_size)
        if len(buf) != shmwrap_msg_ready_t_fmt_size:
            raise ValueError("unexpected buffer length")
        curmsg = buf2class(buf)
        return curmsg

    def grab_next_frame_into_buf_blocking(self,outbuf,bypass_buffer_checks=False):
        """grab frame into user-supplied buffer"""
        curmsg = self._get_network_buf()
        self.last_timestamp = curmsg.timestamp
        self.last_framenumber = curmsg.framenumber
        
        outbuf = _cam_iface_shm.get_data_copy(curmsg,optional_preallocated_buf=outbuf)

    def grab_next_frame_into_alloced_buf_blocking(self,buf_alloc,bypass_buffer_checks=False):
        """grab frame into a newly allocated buffer using user allocation function"""
        curmsg = self._get_network_buf()
        self.last_timestamp = curmsg.timestamp
        self.last_framenumber = curmsg.framenumber

        buf = buf_alloc( self.w, self.h )
        self.grab_next_frame_into_buf_blocking(buf,bypass_buffer_checks=bypass_buffer_checks)
        return buf

    def grab_next_frame_blocking(self):
        curmsg = self._get_network_buf()
        self.last_timestamp = curmsg.timestamp
        self.last_framenumber = curmsg.framenumber
        
        outbuf = _cam_iface_shm.get_data_copy(curmsg)
        return outbuf

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
        
    def get_num_trigger_modes(self):
        return 0

    def get_num_framebuffers(self):
        return 0
        
class CamIFaceError(Exception):
    pass
class BuffersOverflowed(CamIFaceError):
    pass
class FrameDataMissing(CamIFaceError):
    pass

def buf2class(buf):
    class ShmwrapMsgReady:
        pass

    vals = struct.unpack(shmwrap_msg_ready_t_fmt,buf)
    result = ShmwrapMsgReady()
    result.cam_id       = vals[0]
    result.sendnumber   = vals[1]
    result.framenumber  = vals[2]
    result.width        = vals[3]
    result.height       = vals[4]
    result.roi_x        = vals[5]
    result.roi_y        = vals[6]
    result.timestamp    = vals[7]
    result.start_offset = vals[8]
    result.stride       = vals[9]
    return result

def test_simple():
    tcp_control_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp_control_socket.connect(('127.0.0.1',_cam_iface_shm.shmwrap_control_port_))
    tcp_control_socket.send('hello\r\n')
    print 'connected'
    
    udp_listen_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_listen_sock.bind(('127.0.0.1', _cam_iface_shm.shmwrap_frame_ready_port_ ))
    last_framenumber = 0
    while 1:
        buf, (remote_ip,remote_port) = udp_listen_sock.recvfrom(shmwrap_msg_ready_t_fmt_size)
        if len(buf) != shmwrap_msg_ready_t_fmt_size:
            raise ValueError("unexpected buffer length")
        curmsg = buf2class(buf)

        pixels = _cam_iface_shm.get_data_copy(curmsg)
        
        now=time.time()
        tdiff = now-curmsg.timestamp
        printf("%d: latency %f msec"%(curmsg.framenumber,tdiff*1e3))

        if ((curmsg.framenumber-last_framenumber)!=1):
            printf("                                MISSING!")
        last_framenumber=curmsg.framenumber;
        
        printf(' '+str(map(hex,pixels[0,:5])))

_startup()

if __name__=='__main__':
    test_simple()
