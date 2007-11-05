import _cam_iface_shm

import socket, struct, sys
import numpy

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

class _GlobalCameraState:
    def _send_cmd(self,cmd_str):
        self.tcp_control_socket.send(cmd_str)
        buf = self.tcp_control_socket.recv(4096)
        buf = buf.strip()
        assert buf=='OK'
    def set_prop(self,device_number,property_number,new_value,auto):
        cmd_str = 'set_prop(%d,%d,%d,%d)\r\n'%(device_number,property_number,new_value,auto)
        self._send_cmd(cmd_str)
    def set_trig(self,device_number,trigger_mode_number):
        cmd_str = 'set_trig(%d,%d)\r\n'%(device_number,trigger_mode_number)
        self._send_cmd(cmd_str)
    def set_tcp_control_socket(self,tcp_control_socket):
        self.tcp_control_socket = tcp_control_socket
        
_camera_state = _GlobalCameraState()

##class ClassDict:
##    def __init__(self,dict):
##        self.__dict__ = dict

class _CameraProperties:
    def __init__(self,name,prop_str):
        print name,'prop_str',repr(prop_str)
        #vals = prop_str.split('.')
        self.name = name
        value,auto,prop_info_dict = eval(prop_str)

        self.value = value
        self.auto = auto

        self.prop_info = prop_info_dict

    def set(self,new_value,auto):
        raise NotImplementedError("")
        
##        self.has_auto_mode = pi['has_auto_mode']
##        self.max_value = pi['max_value']
##        self.min_value = pi['min_value']
##        self.is_present = pi['is_present']
##        self.has_manual_mode = pi['has_manual_mode']
##        self.is_scaled_quantity = pi['is_scaled_quantity']

class _Camera:
    def __init__(self,name):
        self.name = name
        self.w = None
        self.h = None
        self.properties = []
    def set_resolution(self,w=None,h=None):
        if w is not None:
            self.w = w
        if h is not None:
            self.h = h
    def set_trigger_modes(self,val):
        self.trigger_modes = val
##    def set_properties(self,properties):
##        self.properties = properties
##        print 'self.properties', properties

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
    
    _camera_state.set_tcp_control_socket(tcp_control_socket)

    print 'received','='*80
    print repr(buf)
    print '='*80
    print
    
    # micro parser
    section = None
    cams = {}
    for line in buf.split('\n'):
        line = line.strip()
        print 'line',repr(line)
        if not len(line):
            continue
        if line.startswith('#'):
            continue
        elif line.startswith('['):
            section = line
            continue
        else:
            split_idx = line.index(':')
            key = line[:split_idx].strip()
            value = line[(split_idx+1):].strip()
            print 'value',repr(value)
            if value == ['']:
                print 'compare OK'
                value = []
            
        if section == '[general]' and key == 'udp_packet_size':
            _camera_state.udp_packet_size = int(value)
        if section == '[general]' and key == 'cameras':
            camera_names = value.split()
            print 'camera_names',camera_names
            for cn in camera_names:
                cams[ '['+cn+']' ] = _Camera(cn)
        if section in cams.keys():
            cam = cams[section]
            name = key
            valstr = value

            # Naughty: width and height exceptions to our property parsing code
            if name == 'width':
                cam.set_resolution(w=int(valstr))
            elif name == 'height':
                cam.set_resolution(h=int(valstr))
            elif name == 'trigger_modes':
                vals = valstr.strip()
                trigger_modes = eval(vals)
                print 'trigger_modes',trigger_modes
                cam.set_trigger_modes(trigger_modes)
            else:
                cam.properties.append( _CameraProperties(name,valstr) )

##    cam = _PerCameraState()
##    cam.w, cam.h = resolution
    _camera_state.cams = [cams['['+cn+']'] for cn in camera_names]

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
        self.device_number = device_number
        self.w = _camera_state.cams[self.device_number].w
        self.h = _camera_state.cams[self.device_number].h
        self.last_timestamp = None
        self.next_framenumber = None
        self.last_framenumber = None
        self.savebuf = None
        self.close = self.noop
    def get_pixel_coding(self):
        return 'MONO8'
    
    def get_frame_size(self):
        return self.w, self.h
    def get_max_width(self):
        return self.w
    def get_max_height(self):
        return self.h
    def get_frame_offset(self):
        return 0,0
    def set_frame_offset(self, left, top):
        if left!=0 or top!=0:
            raise RuntimeError('not supported')
    def set_frame_size(self, width, height):
        if width!=self.w or height!=self.h:
            raise RuntimeError('not supported')
    
    def noop(self,*args,**kw):
        pass
    def get_num_camera_properties(self):
        return len(_camera_state.cams[self.device_number].properties)
    def get_camera_property(self,i):
        p = _camera_state.cams[self.device_number].properties[i]
        return p.value, p.auto
    def set_camera_property(self, i, new_value, auto):
        _camera_state.set_prop(self.device_number,i,new_value,auto)
    def set_trigger_mode_number(self, value):
        _camera_state.set_trig(self.device_number,value)
    def get_camera_property_range(self,*args,**kw):
        raise ValueError("not a valid property")
    def get_camera_property_info(self,i):
        return _camera_state.cams[self.device_number].properties[i].prop_info
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
        # XXX this method isn't done yet...

        # XXX change to read data out of shared memory as quickly as
        # possible (no self.savemsg)
        
        if self.savebuf is not None:
            
            #print 'ADS 1'
            if self.save_framenumber == self.next_framenumber:
                #print 'ADS 1.1'
                self.last_timestamp = self.save_timestamp
                self.last_framenumber = self.save_framenumber
                self.next_framenumber = self.save_framenumber+1
                outbuf = self.savebuf
                self.savebuf = None
            else:
                #print 'ADS 1.2'
                assert self.save_framenumber > self.next_framenumber
                self.next_framenumber += 1
                raise FrameDataMissing("missing data")
        else:
            #print 'ADS 2'
            curmsg = self._get_network_buf()
            cur_timestamp = curmsg.timestamp
            cur_framenumber = curmsg.framenumber
            #print 'cur_framenumber',cur_framenumber
            curbuf = _cam_iface_shm.get_data_copy(curmsg,optional_preallocated_buf=outbuf)
            
            if self.last_framenumber is None:
                #print 'ADS 2.1'
                # first frame
                self.last_timestamp = cur_timestamp
                self.last_framenumber = cur_framenumber
                self.next_framenumber = cur_framenumber+1
                outbuf = curbuf
            else:
                #print 'ADS 2.2'
                if self.next_framenumber == cur_framenumber:
                    #print 'ADS 2.3'
                    # no frames skipped
                    self.last_timestamp = cur_timestamp
                    self.last_framenumber = cur_framenumber
                    self.next_framenumber = cur_framenumber+1
                    outbuf = curbuf
                else:
                    #print 'ADS 2.4'
                    assert cur_framenumber > self.next_framenumber
                    self.savebuf = curbuf
                    self.save_framenumber = cur_framenumber
                    self.save_timestamp = cur_timestamp
                    self.next_framenumber += 1
                    raise FrameDataMissing("missing data")
        return outbuf
                
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
    def get_num_trigger_modes(self):
        return len(_camera_state.cams[self.device_number].trigger_modes)
    def get_trigger_mode_string(self,i):
        return _camera_state.cams[self.device_number].trigger_modes[i]
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
