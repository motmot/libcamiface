//  1394CameraControl.cpp: implementation of the C1394CameraControl class.
//
//	Version 5.0 : 11/27/2001
//
//  Copyright 11/2001
//
//  Christopher Baker
//  Robotics Institute
//  Carnegie Mellon University
//  Pittsburgh, PA
//
//	Copyright 5/2000
// 
//	Iwan Ulrich
//	Robotics Institute
//	Carnegie Mellon University
//	Pittsburgh, PA
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
#include "1394Camera.h"
#include "1394CameraControl.h"

extern "C" {
#include "pch.h"
}

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

C1394CameraControl::C1394CameraControl()
{
	m_value1 = 0;
	m_value2 = 0;
	m_mode = 0;
	m_offset = 0;
}


C1394CameraControl::~C1394CameraControl()
{
}


unsigned long C1394CameraControl::Inquire()
{
	unsigned long ulBits;
	int ret;

	if((ret = m_pCamera->ReadQuadlet(m_offset + INQUIRY_INDEX, &ulBits) != CAM_SUCCESS))
	{
		DllTrace(DLL_TRACE_ERROR,"C1394CameraControl::Inquire: error %d on ReadQuadlet\n", ret);
		return CAM_ERROR;
	}

	m_present = (ulBits & 0x80000000) != 0;
	m_onePush = (ulBits & 0x10000000) != 0;
	m_readout = (ulBits & 0x08000000) != 0;
	m_onoff   = (ulBits & 0x04000000) != 0;
	m_auto    = (ulBits & 0x02000000) != 0;
	m_manual  = (ulBits & 0x01000000) != 0;
	m_min     = (unsigned short) ((ulBits & 0x00fff000) >> 12);
	m_max     = (unsigned short)  (ulBits & 0x00000fff);

	DllTrace(DLL_TRACE_EXIT,"EXIT Control::Inquire@0x%02x: %08x\n", m_offset,ulBits);

	return ulBits;
}


unsigned long C1394CameraControl::Status()
{
	unsigned long ulBits;
	int ret;

	if((ret = m_pCamera->ReadQuadlet(m_offset + STATUS_INDEX, &ulBits) != CAM_SUCCESS))
	{
		DllTrace(DLL_TRACE_ERROR,"C1394CameraControl::Status: error %d on ReadQuadlet\n", ret);
		return CAM_ERROR;
	}

	m_statusPresent    = (ulBits & 0x80000000) != 0;
	m_statusOnePush    = (ulBits & 0x04000000) != 0;
	m_statusOnOff      = (ulBits & 0x02000000) != 0;
	m_statusAutoManual = (ulBits & 0x01000000) != 0;
	m_value2           = (unsigned short) ((ulBits & 0x00fff000) >> 12);
	m_value1           = (unsigned short)  (ulBits & 0x00000fff);
	m_mode             = (unsigned short) ((ulBits & 0xff000000) >> 24);

	DllTrace(DLL_TRACE_EXIT,"EXIT Control::Status@0x%02x: %08x\n", m_offset,ulBits);

	return ulBits;
}


// writes values 1 and 2 to camera
int C1394CameraControl::SetValues()
{
	unsigned long ulBits = 0;
	int ret;

	ulBits += m_mode & 0x00ff;
	ulBits <<= 12;
	ulBits += m_value2 & 0x0fff;
	ulBits <<= 12;
	ulBits += m_value1 & 0x0fff;

	DllTrace(DLL_TRACE_CHECK,"Control::SetValues: writing %08x to offset %04x\n",
		ulBits,m_offset + STATUS_INDEX);
	
	if(ret = m_pCamera->WriteQuadlet(m_offset + STATUS_INDEX, ulBits))
		DllTrace(DLL_TRACE_ERROR,"Control::SetValues: error %d on WriteQuadlet\n",ret);

	return ret;
}


int C1394CameraControl::SetAutoMode(BOOL on)
{
	int ret;

	if (on)	
		m_mode |= 0x0001;
	else 
		m_mode &= 0x00fe;

	if(ret = SetValues())
		DllTrace(DLL_TRACE_ERROR,"Control::SetAutoMode: error %d on SetValues()\n",ret);

	return ret;

}


int C1394CameraControl::SetOnePush()
{
	int ret;

	m_mode |= 0x0004;

	if(ret = SetValues())
		DllTrace(DLL_TRACE_ERROR,"Control::SetOnePush: error %d on SetValues()\n",ret);

	return ret;
}


int C1394CameraControl::TurnOn(BOOL on)
{
	int ret;

	if (on)	
		m_mode |= 0x0002;
	else 
		m_mode &= 0x00fd;

	if(ret = SetValues())
		DllTrace(DLL_TRACE_ERROR,"Control::TurnOn: error %d on SetValues()\n",ret);

	return ret;
}

int C1394CameraControl::SetOffset(unsigned short offset)
{
	if(offset > 0x00fc)
		return CAM_ERROR_PARAM_OUT_OF_RANGE;

	m_offset = offset;

	return CAM_SUCCESS;
}
