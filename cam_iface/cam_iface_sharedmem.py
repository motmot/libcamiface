import _cam_iface_shm

import socket, struct, sys
import numpy

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
    tcp_control_socket.connect(('127.0.0.1',_cam_iface_shm.shmwrap_control_port_))
    tcp_control_socket.send('info\r\n')
    buf = tcp_control_socket.recv(4096)

    # micro parser
    section = None
    for line in buf.split('\n'):
        line = line.strip()
        if not len(line):
            continue
        if line.startswith('#'):
            continue
        elif line.startswith('['):
            section = line
            continue
        else:
            parts = line.split(':')
            #print section,parts
            key = parts[0].strip()
            value = parts[1].strip()
            
        if section == '[general]' and key == 'udp_packet_size':
            _camera_state.udp_packet_size = int(value)
        if section == '[general]' and key == 'cameras':
            camera_names = value.split()
            assert len(camera_names)==1
            assert camera_names[0]=='cam1'
        if section == '[cam1]' and key == 'resolution':
            resolution = map(int,value.split('x'))

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
        self.last_timestamp = None
        self.last_framenumber = 0
        self.set_camera_property = self.noop
        self.close = self.noop
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
        # These are udp packets, so no need worry about data getting
        # split across multiple packets.
        buf, (remote_ip,remote_port) = _camera_state.udp_listen_sock.recvfrom(4096)
  
        if len(buf) != _camera_state.udp_packet_size:
            raise ValueError("unexpected buffer length")
        curmsg = _cam_iface_shm.buf2class(buf)
        return curmsg

    def grab_next_frame_into_buf_blocking(self,outbuf,bypass_buffer_checks=False):
        """grab frame into user-supplied buffer"""
        curmsg = self._get_network_buf()
        self.last_timestamp = curmsg.timestamp
        self.last_framenumber = curmsg.framenumber
        
        outbuf = _cam_iface_shm.get_data_copy(curmsg,optional_preallocated_buf=outbuf)

    def grab_next_frame_into_alloced_buf_blocking(self,buf_alloc,bypass_buffer_checks=False):
        """grab frame into a newly allocated buffer using user allocation function"""
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
    def get_pixel_depth(self):
        return 8
class CamIFaceError(Exception):
    pass
class BuffersOverflowed(CamIFaceError):
    pass
class FrameDataMissing(CamIFaceError):
    pass

_startup()
