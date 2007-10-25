import unittest
import os

class TestWrapperCtypes(unittest.TestCase):

    # For now, this just loads the default backend (usually
    # "blank"). Other backends could be tested by using the subprocess
    # module to launch a process per backend, setting the
    # CAM_IFACE_CTYPES_BACKEND environment variable prior to each
    # call.

    def test_import(self):
        import cam_iface_ctypes

    def test_num_cams(self):
        import cam_iface_ctypes as cam_iface
        num_cameras = cam_iface.get_num_cameras()

    def test_driver_name(self):
        import cam_iface_ctypes as cam_iface
        driver_name = cam_iface.get_driver_name()

    def test_get_num_modes(self):
        import cam_iface_ctypes as cam_iface
        num_cameras = cam_iface.get_num_cameras()
        for cam in range(num_cameras):
            num_modes = cam_iface.get_num_modes(cam)

    def test_get_mode_string(self):
        import cam_iface_ctypes as cam_iface
        num_cameras = cam_iface.get_num_cameras()
        for cam in range(num_cameras):
            num_modes = cam_iface.get_num_modes(cam)
            for i in range(num_modes):
                mode_string = cam_iface.get_mode_string(cam,i)

def get_test_suite():
    ts=unittest.TestSuite((unittest.makeSuite(TestWrapperCtypes),))
    return ts

if __name__ == '__main__':
    unittest.main()
