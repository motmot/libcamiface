import unittest

driver = 'camwire'
REPS = 10
debug = True

if driver.lower() == 'bcam':
    import cam_iface_bcam
    cam_iface = cam_iface_bcam
elif driver.lower() == 'camwire':
    import cam_iface_camwire
    cam_iface = cam_iface_camwire
elif driver.lower() == 'dc1394':
    import cam_iface_dc1394
    cam_iface = cam_iface_dc1394

class CamIfaceTestCase(unittest.TestCase):
    def test_get_num_cameras(self):
        num_cams = cam_iface.get_num_cameras()
        if debug:
            print 'cam_iface.get_num_cameras():',num_cams
    def test_get_camera_info(self):
        for i in range(cam_iface.get_num_cameras()):
            vmc = cam_iface.get_camera_info(i)
            if debug:
                print 'cam_iface.get_camera_info(%d):'%i, vmc
    def test_cam_create(self):
        if debug:
            print 'creating and deleting %d CamContext instances'%REPS
        for i in range(REPS):
            print i
            device_number = 0
            num_buffers = 1
            cam = cam_iface.CamContext(device_number,num_buffers)
            del cam

def suite():
    cam_iface_suite = unittest.TestSuite()
    cam_iface_suite.addTest( CamIfaceTestCase("test_get_num_cameras") )
    cam_iface_suite.addTest( CamIfaceTestCase("test_get_camera_info") )
    cam_iface_suite.addTest( CamIfaceTestCase("test_cam_create") )
    
    return cam_iface_suite

runner = unittest.TextTestRunner()
runner.run(suite())
