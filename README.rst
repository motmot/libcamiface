.. _libcamiface:

************************************
libcamiface - camera interface C API
************************************

Overview
========

libcamiface ("camera interface") is a C API that provides a camera and OS
independent image acquisition framework.

There is also a Python wrapper (:mod:`cam_iface`) of the libcamiface
libraries.

Backend status
==============

A number of backends are supported.

.. list-table::
  :header-rows: 1

  * - Backend
    - GNU/Linux i386
    - GNU/Linux x86_64 (amd64)
    - win32 (XP)
    - Mac OS X
  * - libdc1394_
    - |works|
    - |works|
    - |orange| newest library version supports Windows, but untested with libcamiface
    - |mostly works| triggering options disabled
  * - `Prosilica GigE Vision`_
    - |works|
    - |works|
    - |works|
    - |NA|
  * - `ImperX`_
    - |NA|
    - |NA|
    - |orange| rudiments present
    - |NA|
  * - `Basler BCAM 1.8`_
    - |NA|
    - |NA|
    - |orange| rudiments present, frequent BSOD
    - |NA|
  * - `QuickTime SequenceGrabber`_
    - |NA|
    - |NA|
    - |NA| (supported by driver?)
    - |mostly works| basic functionality works

Key to the above symbols:

* |works| Works well, no known bugs or missing features
* |mostly works| Mostly works, some missing features
* |orange| Not working, although with some effort, could presumably be
   made to work
* |NA| The upstream driver does not support this configuration

.. _libdc1394: http://damien.douxchamps.net/ieee1394/libdc1394/
.. _Prosilica GigE Vision: http://www.prosilica.com
.. _ImperX: http://www.imperx.com/
.. _Basler BCAM 1.8: http://www.baslerweb.com/indizes/beitrag_index_en_21486.html
.. _QuickTime SequenceGrabber: http://developer.apple.com/quicktime/

.. |works| image:: _static/greenlight.png
  :alt: works
  :width: 22
  :height: 22
.. |mostly works| image:: _static/yellowgreenlight.png
  :alt: works
  :width: 22
  :height: 22
.. |orange| image:: _static/redlight.png
  :alt: works
  :width: 22
  :height: 22
.. |NA| replace:: NA


Download
========

Release tarballs/zip files
--------------------------

.. Also keep motmot/doc/source/overview.rst in sync with download page.

Download official releases from `the download page`__.

__ http://github.com/motmot/libcamiface/downloads

git source code repository
--------------------------

The `development version of libcamiface`__ may be downloaded via git::

  git clone git://github.com/motmot/libcamiface.git

__ http://github.com/motmot/libcamiface

Build and install
=================

Prerequisites
-------------

On all platforms, you need to install cmake. cmake is available from
http://www.cmake.org/

You will also need the libraries for any camera software. Cmake should
automatically find these if they are installed in the default
locations.

linux
-----

::

  mkdir build
  cd build
  cmake ..
  make
  make install

Mac OS X
--------

Download and install Apple's XCode. This requires signing up (free) as
an Apple ADC member.

::

  mkdir build
  cd build
  cmake ..
  make
  make install

Backend notes
=============

prosilica_gige
--------------

Here is an example of setting attributes on the camera using
Prosilica's command line tools::

  export CAM_IP=192.168.1.63
  CamAttr -i $CAM_IP -s StreamBytesPerSecond 123963084
  CamAttr -i $CAM_IP -s PacketSize 1500

Environment variables:

  * *PROSILICA_BACKEND_DEBUG* print various debuggin information.

libdc1394
---------

Environment variables:

 * *DC1394_BACKEND_DEBUG* print libdc1394 error messages. (You may
   also be interested in libdc1394's own *DC1394_DEBUG* environment
   variable, which prints debug messages.)

 * *DC1394_BACKEND_1394B* attempt to force use of firewire
    800. (Otherwise defaults to 400.)

unity
-----

Environment variables:

 * *UNITY_BACKEND_DEBUG* print debugging messages.

 * *UNITY_BACKEND_DIR* set to directory in which to look for
    libcamiface shared libraries.


License
=======

libcamiface is licensed under the BSD license. See the LICENSE.txt file
for the full description.
