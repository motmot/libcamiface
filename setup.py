# setup.py for pynet. Note: this file is only for the pynet
# backend. Do not bother with this file unless you are interested in
# pynet.

# See pynet_notes.txt for a description of pynet.

from distutils.core import setup, Extension

setup(name='_pynet',
      version='0.0.1',
      ext_modules=[Extension(name='_pynet',
                             sources=['src/_pynetmodule.c'],
                             )],
      )
