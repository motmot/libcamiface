import unittest
import os, sys
import numpy

if sys.platform.startswith('linux'):
    import cam_iface._cam_iface_shm as cishm

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

class TestShm(unittest.TestCase):
    def test_copy_shm(self):
        source_manager = cishm.ShmManager(create=True)
        shape = (5,6)
        fake_image = numpy.arange(shape[0]*shape[1],dtype=numpy.uint8)
        fake_image.shape = shape
        curmsg = source_manager.copy_into_shm(0, fake_image) # copy into shared memory
        result_new = cishm.get_data_copy(curmsg)
        assert numpy.allclose( result_new, fake_image)

    def test_copy_shm_preallocated(self):
        source_manager = cishm.ShmManager(create=True)
        shape = (5,6)
        fake_image = numpy.arange(shape[0]*shape[1],dtype=numpy.uint8)
        fake_image.shape = shape
        curmsg = source_manager.copy_into_shm(0, fake_image) # copy into shared memory
        result_preallocated = numpy.empty( shape, dtype=numpy.uint8 )
        result_preallocated2 = cishm.get_data_copy(curmsg,result_preallocated)
        assert numpy.allclose( result_preallocated, fake_image )
        assert result_preallocated2 is result_preallocated
        
def get_test_suite():
    suites = [
        unittest.makeSuite(TestWrapper),
        ]
    if sys.platform.startswith('linux'):
        suites.append( unittest.makeSuite(TestShm) )
    ts=unittest.TestSuite(suites)
    return ts

if __name__ == '__main__':
    ts = get_test_suite()
    unittest.TextTestRunner().run(ts)
