= Overview =

cam_iface ("camera interface") is a C API that provides a camera and OS
independent image acquisition framework.

There is also a Python wrapper of the cam_iface libraries.

= Backend status =

A number of backends are supported. For the latest status, see http://code.astraw.com/motmot/trunk/cam_iface/backend_scorecard.png

[[Image(source:/trunk/cam_iface/backend_scorecard.png)]]

= Installing =

== A.0 Building cam_iface C library on Windows ==

Open a "Visual Studio Command Prompt" and type:

{{{
cd C:\Blah\blah\cam_iface
scons
}}}

== A.1 Building cam_iface C library on linux ==

{{{
cd /home/somebody/blah/blah/cam_iface
scons
}}}

== B. Build and install Python wrapper ==

{{{
python setup.py install
}}}

= Backend notes =

There are currently 2 ways that Python can access the backends:
through ctypes and through Pyrex-based wrappers. ctypes is the future
and the Pyrex wrappers are legacy. Please use ctypes for future
development.

== camwire ==

The camwire backend is deprecated and will be replaced by the dc1394
(v2) backend ASAP.

= License =

cam_iface is licensed under the BSD license. See the LICENSE.txt file
for the full description. Note that cam_iface is also distributed with
other open source code packages, which have their own licenses, also
listed in LICENSE.txt.

= Related Software =

* What is GenICam(TM), and how does camiface compare?

Another project with similar goals is GenICam(TM)
http://www.genicam.com/ . Primary differences between camiface and
GenICam(TM) include the following: 1) camiface has been developed by a
single individual to support a limited number of camera features from
a limited number of cameras and is necessarily narrower in scope than
an API meant to encompass every available feature on every available
camera. 2) camiface operates using existing drivers rather than
creating a new implementation of the driver layer.

One implementation of GenICam(TM) appears to be Basler's
Pylon. http://www.baslerweb.com/beitraege/beitrag_en_53074.html

* What is GigEVision(TM)?

See http://www.machinevisiononline.org/public/articles/index.cfm?cat=167
for a description of GigEVision(TM).
