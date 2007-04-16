import cam_iface_choose
import sys

def main():
    for wrapper,backends in cam_iface_choose.wrappers_and_backends.iteritems():

        if wrapper == 'ctypes' and len(backends)>1:
            for backend in backends:
                if backend != 'blank':
                    # use something more sophisticated than blank backend
                    first_non_blank_backend = backend
                    break
            non_used_backends = [ b for b in backends if b != first_non_blank_backend]
            print 'WARNING: cam_iface_ctypes can only be imported once, only trying "%s" ctypes backend'%(
                first_non_blank_backend,)
            print '         (non-used backends: ',non_used_backends
            backends = [first_non_blank_backend]

        for backend in backends:
            print 'importing %s: %s ...'%(wrapper, backend),
            sys.stdout.flush()
            cam_iface = cam_iface_choose.import_backend( backend, wrapper )
            this_wrapper = cam_iface.get_wrapper_name()
            if wrapper != this_wrapper:
                raise ValueError('wrapper "%s" (not %s)'%(this_wrapper,wrapper))
            this_backend = cam_iface.get_driver_name()
            if backend != this_backend:
                raise ValueError('backend "%s" (not %s)'%(this_backend,backend))
            print 'OK'
if __name__=='__main__':
    main()
