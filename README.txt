= Overview =

cam_iface ("camera interface") is a C API that provides a camera and OS
independent image acquisition framework.

There is also a Python wrapper of the cam_iface libraries.

= Backend status =

A number of backends are supported. 

{{{
#!html

<!-- This raw HTML draws a table of the supported backends. -->

<!--

---- The following is the MoinMoin wiki text that produced this HTML output: ----

 Status of backends and platforms::
 || '''Backend''' || '''linux i386''' || '''linux x86_64''' || '''win32 (XP)''' || '''Mac OS X''' ||
 || [http://damien.douxchamps.net/ieee1394/libdc1394/index.php libdc1394 v2] ||<style="background-color: green;">works ||<style="background-color: green;">works || NA ||<style="background-color: yellowgreen;">triggering options disabled ||
 || [http://kauri.auck.irl.cri.nz/~johanns/camwire/ camwire/libdc1394 v1] ||<style="background-color: green;">works ||<style="background-color: green;">works || NA || NA ||
 || [http://www.prosilica.com/ Prosilica GigE Vision] ||<style="background-color: green;">works || NA ||<style="background-color: green;">works || NA ||
 || [http://www.imperx.com/ ImperX GigE] || NA || NA ||<style="background-color: orange;">rudiments present || NA ||
 || [http://www.baslerweb.com/indizes/beitrag_index_en_21486.html Basler BCAM 1.8] || NA || NA ||<style="background-color: orange;">rudiments present, frequent BSOD || NA ||
 || [http://www.baslerweb.com/beitraege/beitrag_en_53074.html Basler Pylon] ||<style="background-color: red;"> not started || supported by driver? ||<style="background-color: red;"> not started || supported by driver? ||
 || [http://developer.apple.com/quicktime/ QuickTime SequenceGrabber] || NA || NA || supported by driver? ||<style="background-color: yellowgreen;">basic functionality works ||

-->

<table><tbody><tr>  <td><p class="line862"> <strong>Backend</strong> </td>

  <td><p class="line862"> <strong>linux i386</strong> </td>
  <td><p class="line862"> <strong>linux x86_64</strong> </td>
  <td><p class="line862"> <strong>win32 (XP)</strong> </td>
  <td><p class="line862"> <strong>Mac OS X</strong> </td>

</tr>
<tr>  <td><span class="anchor" id="line-65"></span><p class="line862"> <a class="http" href="http://damien.douxchamps.net/ieee1394/libdc1394/index.php">libdc1394 v2</a> </td>
  <td style="background-color: green;"><p class="line862">works </td>
  <td style="background-color: green;"><p class="line862">works </td>
  <td><p class="line862"> NA </td>
  <td style="background-color: yellowgreen;"><p class="line862">triggering options disabled </td>

</tr>
<tr>  <td><span class="anchor" id="line-66"></span><p class="line862"> <a class="http" href="http://kauri.auck.irl.cri.nz/~johanns/camwire/">camwire/libdc1394 v1</a> </td>
  <td style="background-color: green;"><p class="line862">works </td>
  <td style="background-color: green;"><p class="line862">works </td>
  <td><p class="line862"> NA </td>
  <td><p class="line862"> NA </td>

</tr>
<tr>  <td><span class="anchor" id="line-67"></span><p class="line862"> <a class="http" href="http://www.prosilica.com/">Prosilica GigE Vision</a> </td>
  <td style="background-color: green;"><p class="line862">works </td>
  <td><p class="line862"> NA </td>
  <td style="background-color: green;"><p class="line862">works </td>
  <td><p class="line862"> NA </td>

</tr>
<tr>  <td><span class="anchor" id="line-68"></span><p class="line862"> <a class="http" href="http://www.imperx.com/">ImperX GigE</a> </td>
  <td><p class="line862"> NA </td>
  <td><p class="line862"> NA </td>
  <td style="background-color: orange;"><p class="line862">rudiments present </td>

  <td><p class="line862"> NA </td>
</tr>
<tr>  <td><span class="anchor" id="line-69"></span><p class="line862"> <a class="http" href="http://www.baslerweb.com/indizes/beitrag_index_en_21486.html">Basler BCAM 1.8</a> </td>
  <td><p class="line862"> NA </td>
  <td><p class="line862"> NA </td>

  <td style="background-color: orange;"><p class="line862">rudiments present, frequent BSOD </td>
  <td><p class="line862"> NA </td>
</tr>
<tr>  <td><span class="anchor" id="line-70"></span><p class="line862"> <a class="http" href="http://www.baslerweb.com/beitraege/beitrag_en_53074.html">Basler Pylon</a> </td>
  <td style="background-color: red;"><p class="line862"> not started </td>

  <td><p class="line862"> supported by driver? </td>
  <td style="background-color: red;"><p class="line862"> not started </td>
  <td><p class="line862"> supported by driver? </td>
</tr>
<tr>  <td><span class="anchor" id="line-71"></span><p class="line862"> <a class="http" href="http://developer.apple.com/quicktime/">QuickTime SequenceGrabber</a> </td>

  <td><p class="line862"> NA </td>
  <td><p class="line862"> NA </td>
  <td><p class="line862"> supported by driver? </td>
  <td style="background-color: yellowgreen;"><p class="line862">basic functionality works </td>
</tr>
</tbody></table>
}}}


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
