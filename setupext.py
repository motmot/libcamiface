# $Id: %
from setuptools import setup, Extension
from distutils.dir_util import mkpath

import os, sys, glob

class CleanUpFile:
    """CleanUpFile deletes the specified filename when self is destroyed."""
    def __init__(self, name):
        self.name = name
        self.os_remove = os.remove
    def __del__(self):
        self.os_remove(self.name)

def temp_copy(_from, _to):
    """temp_copy copies a named file into a named temporary file.
    The temporary will be deleted when the setupext module is destructed.
    """
    # Copy the file data from _from to _to
    s = open(_from).read()
    open(_to,"w+").write(s)
    # Suppress object rebuild by preserving time stamps.
    stats = os.stat(_from)
    os.utime(_to, (stats.st_atime, stats.st_mtime))
    # Make an object to eliminate the temporary file at exit time.
    globals()["_cleanup_"+_to] = CleanUpFile(_to)

def add_system_raw1394(ext):
    """Add build requirements for system's libraw1394"""
    if not os.path.exists('/usr/include/libraw1394/raw1394.h'):
        raise RuntimeError('libraw1394 not in system headers - refusing to build')
    ext.libraries.append('raw1394')
    
def add_system_libdc1394(ext):
    """Add build requirements for system's libdc1394"""
    if not os.path.exists('/usr/include/libdc1394/dc1394_control.h'):
        raise RuntimeError('libdc1394 not in system headers - refusing to build')
    ext.libraries.append('dc1394_control')
    
def add_cmu1394(ext):
    """Add build requirements for CMU 1394 library"""
    ext.include_dirs.append(r'cmu1394\include')
    ext.library_dirs.extend(
        [r'C:\Program Files\Microsoft Platform SDK for Windows XP SP2\Lib',
         r'cmu1394\lib',])
        
    ext.libraries.append('1394camera')
    ext.define_macros.extend( [('_AFXDLL',None)] )
    
def add_bcam(ext):
    """Add build requirements for Basler BCAM library"""
    # atlstr.h seems to be in ATL (or ATLMFC), which only seems to be
    # distributed with Microsoft Visual Studio and specifically not
    # the VC++ toolkit (free) compiler or the platform SDK
    
    ext.include_dirs.extend([
        r'C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\atlmfc\include',
        'bcam/inc',
        'bcam/SFF/inc'])
    ext.sources.extend(
        ['bcam/src/Bcam.cpp',
         'bcam/src/BcamAdapter.cpp',
         'bcam/src/BcamException.cpp',
         'bcam/src/BcamPropertyBag.cpp',
         'bcam/src/md5.c'])
    ext.libraries.extend(['Version','SetupAPI','Atl'])
    ext.define_macros.extend([('_AFXDLL',None)])

def get_prosilica_gige_extension(debug=None):
    orig_fname = 'src/cam_iface.pyx'
    new_fname='src/_cam_iface_prosilica_gige.pyx'
    temp_copy(orig_fname,new_fname)
    opj = os.path.join
    prosilica_gige_include_dirs=[opj('Prosilica GigE SDK','inc-pc')]
    ext = Extension(name='cam_iface._cam_iface_prosilica_gige',
                    sources=[new_fname,'src/cam_iface_prosilica_gige.cpp'],
                    include_dirs=['inc']+prosilica_gige_include_dirs,
                    )
    ext.libraries.append('PvAPI')
    ext.libraries.append('OpenThreads')
    if sys.platform.startswith('linux'):
        ext.define_macros.extend([('_LINUX',None),])
        if os.uname()[4] in ['i686']:
            ext.define_macros.extend([('_x86',None),])
        ext.libraries.append('rt')
        ext.libraries.append('pthread')
        
        ext.library_dirs.append(opj('Prosilica GigE SDK','lib-pc','x86','4.0'))

        
    return ext
    
def get_camwire_extension(debug=None):
    orig_fname = 'src/cam_iface.pyx'
    new_fname='src/_cam_iface_camwire.pyx'
    temp_copy(orig_fname,new_fname)

    camwire_sources = ['camwire/src/camwire_1394.c',
                       'camwire/src/camwirebus_1394.c',
                       #'camwire/src/camwireconfig_1394.c',
                       #'camwire/src/camwiresettings_1394.c',
                       ]
    camwire_include_dirs = ['camwire/src'] # subdir camwire included
    
    ext = Extension(name='cam_iface._cam_iface_camwire',
                    sources=([new_fname,'src/cam_iface_camwire.c']
                             +camwire_sources),
                    include_dirs=['inc']+camwire_include_dirs,
                    )
    if debug is not None:
        ext.define_macros.extend([('CAM_IFACE_DEBUG',1),
                                  ('CAMWIRE_DEBUG',1),
                                  ])
    add_system_raw1394(ext)
    add_system_libdc1394(ext)
    return ext

def get_cmu1394_extension(debug=None):
    orig_fname = 'src/cam_iface.pyx'
    new_fname='src/_cam_iface_cmu1394.pyx'
    temp_copy(orig_fname,new_fname)
    ext = Extension(name='cam_iface._cam_iface_cmu1394',
                    sources=[new_fname,'src/cam_iface_cmu1394.cpp'],
                    include_dirs=['inc'],
                    )
    if debug is not None:
        ext.define_macros.extend([('CAM_IFACE_DEBUG',1),
                                  ])
    add_cmu1394(ext)
    return ext

def get_bcam_extension(debug=None):
    orig_fname = 'src/cam_iface.pyx'
    new_fname='src/_cam_iface_bcam.pyx'
    temp_copy(orig_fname,new_fname)
    ext = Extension(name='cam_iface._cam_iface_bcam',
                    sources=[new_fname,
                             'src/cam_iface_bcam.cpp'],
                    include_dirs=['inc'],
                    )
    if debug is not None:
        ext.define_macros.extend([('CAM_IFACE_DEBUG',1),
                                  ])
    add_bcam(ext)
    return ext

def get_blank_extension(debug=None):
    orig_fname = 'src/cam_iface.pyx'
    new_fname='src/_cam_iface_blank.pyx'
    temp_copy(orig_fname,new_fname)
    ext = Extension(name='cam_iface._cam_iface_blank',
                    sources=[new_fname,
                             'src/cam_iface_blank.c'],
                    include_dirs=['inc'],
                    )
    if debug is not None:
        ext.define_macros.extend([('CAM_IFACE_DEBUG',1),
                                  ])
    return ext

def get_shm_extension(debug=None):
    ext = Extension(name='cam_iface._cam_iface_shm',
                    sources=['src/_cam_iface_shm.pyx'],
                    include_dirs=['inc','shmwrap',],
                    )
    if debug is not None:
        ext.define_macros.extend([('CAM_IFACE_DEBUG',1),
                                  ])
    return ext
