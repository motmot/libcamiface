import time

GAIN = 1
SHUTTER = 2
BRIGHTNESS = 3

class DummyCam:
    def __init__(self):
        self.max_width = 640
        self.max_height = 480
    def grab_next_frame_blocking(self, buffer):
        buffer[20:30,50:60] = 244
        time.sleep(0.01)
    def get_camera_property(self, feature):
        return 50, 0
    def get_camera_property_range(self, feature):
        return 0, 0, 100
    def set_camera_property(self, *args):
        pass
    def start_camera(self):
        pass

def CamContext(*args):
    return DummyCam()
def get_driver_abbreviation():
    return '(dummy)'
def get_num_cameras():
    return 2

    
