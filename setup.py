from setuptools import setup, Extension

setup(name='_pynet',
      version='0.0.1',
      ext_modules=[Extension(name='_pynet',
                             sources=['src/_pynetmodule.c'],
                             )],
      )
