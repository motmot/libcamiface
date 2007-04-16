import cam_iface_choose

#cam_iface = cam_iface_choose.import_backend( 'prosilica_gige', 'ctypes' )
cam_iface = cam_iface_choose.import_backend( 'dc1394', 'ctypes' )

import numpy as nx
import time, sys, os
from optparse import OptionParser

# for save mode:
import FlyMovieFormat
import Queue
import threading

def main():
    usage = '%prog [options]'
    
    parser = OptionParser(usage)
    
    parser.add_option("--mode-num", type="int",
                      help="mode number")
    
    parser.add_option("--save", action='store_true',
                      help="save frames to .fmf")
    
    (options, args) = parser.parse_args()
    
    print 'options.mode_num',options.mode_num

    if options.mode_num is not None:
        mode_num = options.mode_num
    else:
        mode_num = 0
    doit(mode_num=mode_num,save=options.save)

def save_func( fly_movie, save_queue ):
    while 1:
        fnt = save_queue.get()
        frame,timestamp = fnt
        fly_movie.add_frame(frame,timestamp)
        if 1:
            import numpy
            f16 = numpy.fromstring(frame.tostring(),dtype=numpy.uint16)
            f16.shape = frame.shape[0], frame.shape[1]/2
            print
            print 'f16[:5,:5]'
            print f16[:5,:5]
            print
    
def doit(device_num=0,mode_num=0,num_buffers=30,save=False):
    num_modes = cam_iface.get_num_modes(device_num)
    for this_mode_num in range(num_modes):
        mode_str = cam_iface.get_mode_string(device_num,this_mode_num)
        print 'mode %d: %s'%(this_mode_num,mode_str)

    print 'choosing mode %d'%(mode_num,)
    
    cam = cam_iface.Camera(device_num,num_buffers,mode_num)

    if save:
        format = cam.get_pixel_coding()
        depth = cam.get_pixel_depth()
        filename = 'simple.fmf'
        fly_movie = FlyMovieFormat.FlyMovieSaver(filename,
                                                 version=3,
                                                 format=format,
                                                 bits_per_pixel=depth,
                                                 )
        save_queue = Queue.Queue()
        save_thread = threading.Thread( target=save_func, args=(fly_movie,save_queue))
        save_thread.setDaemon(True)
        save_thread.start()

    num_props = cam.get_num_camera_properties()
    for i in range(num_props):
        print "property %d: %s"%(i,str(cam.get_camera_property_info(i)))
    
    cam.start_camera()
    n_frames = 0
    last_fps_print = time.time()
    last_fno = None
    while 1:
        try:
            buf = nx.asarray(cam.grab_next_frame_blocking())
        except cam_iface.FrameDataMissing:
            sys.stdout.write('M')
            sys.stdout.flush()            
            continue

        timestamp = cam.get_last_timestamp()
        
        fno = cam.get_last_framenumber()
        if last_fno is not None:
            skip = (fno-last_fno)-1
            if skip != 0:
                print 'WARNING: skipped %d frames'%skip
    ##    if n_frames==50:
    ##        print 'sleeping'
    ##        time.sleep(10.0)
    ##        print 'wake'
        last_fno=fno
        now = time.time()
        sys.stdout.write('.')
        sys.stdout.flush()
        n_frames += 1

        t_diff = now-last_fps_print
        if t_diff > 5.0:
            fps = n_frames/t_diff
            print "%.1f fps"%fps
            last_fps_print = now
            n_frames = 0

        if save:
            save_queue.put( (buf,timestamp) )

if __name__=='__main__':
    main()
