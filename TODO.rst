To-do items for libcamiface
===========================

General
-------

 * Change API such that error values are not thread-local, but are
   return values from each function call.
 * Add color to YUV422 decoder in demo/liveview-glut.
 * Add micro-manager backend

Windows
-------

 * Implement backends: DirectShow, Pt. Grey FlyCap, AVT, Basler

Linux
-----

 * Implement V4L2 backend to handle uvc cameras.

Mac OS X
--------

 * Port Prosilica GigE backend
