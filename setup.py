from setuptools import setup, Extension
from setuptools.dist import Distribution

import os, sys, glob, subprocess

from motmot_utils import get_svnversion_persistent
version_str = '0.4.dev%(svnversion)s'
version = get_svnversion_persistent('cam_iface/version.py',version_str)

import setupext, setup_autogen


kws={}
ext_modules = []

build_ctypes_based_wrapper = True

if sys.platform == 'win32':
    build_pyrex_based_wrappers = False
else:
    #build_pyrex_based_wrappers = False
    build_pyrex_based_wrappers = True

ctypes_backends = []
if build_ctypes_based_wrapper:
    fname = 'ctypes_backends.txt'
    if os.path.exists(fname):
        ctypes_backends.extend( open(fname,'rb').read().split() )
    else:
        print '*'*80
        print '*'*80
        print
        print 'WARNING: no ctypes_backends appear to exist '\
              '- have you run scons?'
        print
        print '*'*80
        print '*'*80
    if sys.platform == 'win32':
        prefix = 'cam_iface_'
        extension = '.dll'
    elif sys.platform.startswith('linux'):
        prefix = 'libcam_iface_'
        extension = '.so'
    elif sys.platform.startswith('darwin'):
        prefix = 'libcam_iface_'
        extension = '.dylib'
    else:
        raise ValueError('unknown platform')
    for backend in ctypes_backends:
        fname = prefix+backend+extension
        if not os.path.exists(os.path.join('cam_iface',fname)):
            print '***** WARNING: Could not find file %s'%fname
        kws.setdefault('package_data',{}).setdefault('cam_iface',[]).append(fname)

pyrex_backends = []
if build_pyrex_based_wrappers:
    #ext_modules.append( setupext.get_blank_extension() )
    if sys.platform == 'win32':
        #ext_modules.append( get_cmu1394_extension() )
        #ext_modules.append( get_bcam_extension() )
        pass # none compile easily out of the box
    elif sys.platform.startswith('linux'):
        #ext_modules.append( setupext.get_dc1394_extension() ); pyrex_backends.append('dc1394')
        try:
            ext_modules.append( setupext.get_camwire_extension() ); pyrex_backends.append('camwire')
        except Exception,err:
            print 'WARNING: Not building camwire pyrex backend (error "%s")'%str(err)
            
class PlatformDependentDistribution(Distribution):
    # Force platform-dependant build.
    def has_ext_modules(self):
        return True
    
setup_autogen.generate_choose_module(pyrex_backends, ctypes_backends)

setup(name='cam_iface',
      description='cross-platform cross-backend camera driver',
      long_description="""cam_iface is the core packge of several that
are involved with digital camera acquisition and analysis""",
      version=version,
      author='Andrew Straw',
      author_email='strawman@astraw.com',
      license="BSD",
      packages = ['cam_iface','cam_iface_choose'],
      ext_modules=ext_modules,
      zip_safe=True,
      distclass = PlatformDependentDistribution,
      install_requires = ['numpy>=1.0',
                          'setuptools>=0.6c3'],
      **kws)
