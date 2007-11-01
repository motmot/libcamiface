import unittest
import os

class TestWrapper(unittest.TestCase):

    # For now, this just loads the default backend with the ctypes
    # wrapper (usually "blank"). Other backends could be tested by
    # using the subprocess module to launch a process per backend,
    # setting the CAM_IFACE_CTYPES_BACKEND environment variable prior
    # to each call.

    def _get_cam_iface(self):
        import cam_iface_ctypes as cam_iface
        return cam_iface
        
    def _get_cam(self,device_num=0):
        cam_iface = self._get_cam_iface()
        num_cameras = cam_iface.get_num_cameras()
        assert num_cameras >= 1
        num_buffers,mode_num = 3,0
        cam = cam_iface.Camera(device_num,num_buffers,mode_num)
        return cam

    def test_import(self):
        cam_iface = self._get_cam_iface()

    def test_num_cams(self):
        cam_iface = self._get_cam_iface()
        num_cameras = cam_iface.get_num_cameras()

    def test_driver_name(self):
        cam_iface = self._get_cam_iface()
        driver_name = cam_iface.get_driver_name()

    def test_get_num_modes(self):
        cam_iface = self._get_cam_iface()
        num_cameras = cam_iface.get_num_cameras()
        for cam in range(num_cameras):
            num_modes = cam_iface.get_num_modes(cam)

    def test_get_mode_string(self):
        cam_iface = self._get_cam_iface()
        num_cameras = cam_iface.get_num_cameras()
        for cam in range(num_cameras):
            num_modes = cam_iface.get_num_modes(cam)
            for i in range(num_modes):
                mode_string = cam_iface.get_mode_string(cam,i)

    def test_camera_init(self):
        cam_iface = self._get_cam_iface()
        cam = self._get_cam()

    def test_grab_next_frame_blocking(self):
        cam_iface = self._get_cam_iface()
        cam = self._get_cam()
        cam.start_camera()
        buf = cam.grab_next_frame_blocking()
        timestamp = cam.get_last_timestamp()
        
def get_test_suite():
    ts=unittest.TestSuite((unittest.makeSuite(TestWrapper),))
    return ts

if __name__ == '__main__':
    unittest.main()
