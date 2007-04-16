//////////////////////////////////////////////////////////////////////
//  ControlWrappers.cpp: source for wrappers for the C1394CameraControl
//  members of the  C1394Camera class
//
//  Source for C1394Camera::
//   - InquireControlRegisters()
//   - StatusControlRegisters()
//   - SetBrightness()
//   - Set...
//
//	Version 5.5 : 7/28/2002
//
//	Copyright 5/2000 - 10/2001
// 
//	Iwan Ulrich
//	Robotics Institute
//	Carnegie Mellon University
//	Pittsburgh, PA
//
//  Copyright 3/2002 - 7/2002
//
//  Christopher Baker
//  Robotics Institute
//  Carnegie Mellon University
//  Pittsburgh, PA
//
//  This file is part of the CMU 1394 Digital Camera Driver
//
//  The CMU 1394 Digital Camera Driver is free software; you can redistribute 
//  it and/or modify it under the terms of the GNU Lesser General Public License 
//  as published by the Free Software Foundation; either version 2.1 of the License,
//  or (at your option) any later version.
//
//  The CMU 1394 Digital Camera Driver is distributed in the hope that it will 
//  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with the CMU 1394 Digital Camera Driver; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <string.h>
#include <mmsystem.h>
#include <stdio.h>
#include "1394Camera.h"

/*
 * InquireControlRegisters
 *
 * No Args, No Return Value
 *
 * Probes the Feature_Hi_Inq and Feature_Lo_Inq Registers
 * to find out what features are available, then probes the
 * Inquiry registers of each control that is available
 */
void C1394Camera::InquireControlRegisters()
{
	unsigned long foo;

	// Feature_Hi_Inq
	ReadQuadlet(0x404, &foo);

	if(foo & 0x80000000)
		m_controlBrightness.Inquire();
	if(foo & 0x40000000)
		m_controlAutoExposure.Inquire();
	if(foo & 0x20000000)
		m_controlSharpness.Inquire();
	if(foo & 0x10000000)
		m_controlWhiteBalance.Inquire();
	if(foo & 0x08000000)
		m_controlHue.Inquire();
	if(foo & 0x04000000)
		m_controlSaturation.Inquire();
	if(foo & 0x02000000)
		m_controlGamma.Inquire();
	if(foo & 0x01000000)
		m_controlShutter.Inquire();
	if(foo & 0x00800000)
		m_controlGain.Inquire();
	if(foo & 0x00400000)
		m_controlIris.Inquire();
	if(foo & 0x00200000)
		m_controlFocus.Inquire();
	if(foo & 0x00080000)
		m_controlTrigger.Inquire();

	// Feature_Lo_Inq
	ReadQuadlet(0x408, &foo);
	
	if(foo & 0x80000000)
		m_controlZoom.Inquire();
}


/*
 * StatusControlRegisters
 *
 * No Args, No Return Value
 *
 * Probes the Feature_Hi_Inq and Feature_Lo_Inq Registers
 * to find out what features are available, then probes the
 * Status registers of each control that is available
 */
void C1394Camera::StatusControlRegisters()
{
	unsigned long foo;

	// Feature_Hi_Inq
	ReadQuadlet(0x404, &foo);

	if(foo & 0x80000000)
		m_controlBrightness.Status();
	if(foo & 0x40000000)
		m_controlAutoExposure.Status();
	if(foo & 0x20000000)
		m_controlSharpness.Status();
	if(foo & 0x10000000)
		m_controlWhiteBalance.Status();
	if(foo & 0x08000000)
		m_controlHue.Status();
	if(foo & 0x04000000)
		m_controlSaturation.Status();
	if(foo & 0x02000000)
		m_controlGamma.Status();
	if(foo & 0x01000000)
		m_controlShutter.Status();
	if(foo & 0x00800000)
		m_controlGain.Status();
	if(foo & 0x00400000)
		m_controlIris.Status();
	if(foo & 0x00200000)
		m_controlFocus.Status();
	if(foo & 0x00080000)
		m_controlTrigger.Status();

	// Feature_Lo_Inq
	ReadQuadlet(0x408, &foo);
	
	if(foo & 0x80000000)
		m_controlZoom.Status();
}


/*
 * The Set<BLAH> functions
 *
 * Wraps the SetValues() function of m_control<BLAH>
 */

void C1394Camera::SetBrightness(int value)
{
	if (m_controlBrightness.m_value1 == value)
		return;
	m_controlBrightness.m_value1 = value;
	m_controlBrightness.SetValues();
}


void C1394Camera::SetAutoExposure(int value)
{
	if (m_controlAutoExposure.m_value1 == value)
		return;
	m_controlAutoExposure.m_value1 = value;
	m_controlAutoExposure.SetValues();
}


void C1394Camera::SetGain(int value)
{
	if (m_controlGain.m_value1 == value)
		return;
	m_controlGain.m_value1 = value;
	m_controlGain.SetValues();
}


void C1394Camera::SetGamma(int value)
{
	if (m_controlGamma.m_value1 == value)
		return;
	m_controlGamma.m_value1 = value;
	m_controlGamma.SetValues();
}


void C1394Camera::SetHue(int value)
{
	if (m_controlHue.m_value1 == value)
		return;
	m_controlHue.m_value1 = value;
	m_controlHue.SetValues();
}


void C1394Camera::SetSaturation(int value)
{
	if (m_controlSaturation.m_value1 == value)
		return;
	m_controlSaturation.m_value1 = value;
	m_controlSaturation.SetValues();
}


void C1394Camera::SetSharpness(int value)
{
	if (m_controlSharpness.m_value1 == value)
		return;
	m_controlSharpness.m_value1 = value;
	m_controlSharpness.SetValues();
}


void C1394Camera::SetShutter(int value)
{
	if (m_controlShutter.m_value1 == value)
		return;
	m_controlShutter.m_value1 = value;
	m_controlShutter.SetValues();
}


void C1394Camera::SetZoom(int value)
{
	if (m_controlZoom.m_value1 == value)
		return;
	m_controlZoom.m_value1 = value;
	m_controlZoom.SetValues();
}


void C1394Camera::SetFocus(int value)
{
	if (m_controlFocus.m_value1 == value)
		return;
	m_controlFocus.m_value1 = value;
	m_controlFocus.SetValues();
}


void C1394Camera::SetIris(int value)
{
	if (m_controlIris.m_value1 == value)
		return;
	m_controlIris.m_value1 = value;
	m_controlIris.SetValues();
}


void C1394Camera::SetWhiteBalance(int u, int v)
{
	if ((m_controlWhiteBalance.m_value2 == u) && (m_controlWhiteBalance.m_value1 == v))
		return;
	m_controlWhiteBalance.m_value1 = v;
	m_controlWhiteBalance.m_value2 = u;
	m_controlWhiteBalance.SetValues();
}

