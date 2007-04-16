//  1394CameraControlTrigger.cpp: implementation of the C1394CameraControlTrigger class.
//
//	Version 4.0
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
#include "1394CameraControlTrigger.h"

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

C1394CameraControlTrigger::C1394CameraControlTrigger()
{
	m_polarity = false;
	m_statusPolarity = false;
	for (int i=0; i<4; i++)
		m_triggerMode[i] = false;
	m_statusMode = 0;;
}


C1394CameraControlTrigger::~C1394CameraControlTrigger()
{
}


unsigned long C1394CameraControlTrigger::Inquire()
{
	unsigned long ulBits;
	int ret;
	if(ret = m_pCamera->ReadQuadlet(INQUIRY_INDEX + TRIGGER_MODE_OFFSET,&ulBits))
	{
		DllTrace(DLL_TRACE_ERROR,"ControlTrigger::Inquire: error %d on ReadQuadlet\n");
		return CAM_ERROR;
	}

	if (ulBits & 0x80000000) m_present = true; 
	else m_present = false;
	if (ulBits & 0x08000000) m_readout = true; 
	else m_readout = false;
	if (ulBits & 0x04000000) m_onoff = true; 
	else m_onoff = false;
	if (ulBits & 0x02000000) m_polarity = true; 
	else m_polarity = false;
	if (ulBits & 0x00008000) m_triggerMode[0] = true; 
	else m_triggerMode[0] = false;
	if (ulBits & 0x00004000) m_triggerMode[1] = true; 
	else m_triggerMode[1] = false;
	if (ulBits & 0x00002000) m_triggerMode[2] = true; 
	else m_triggerMode[2] = false;
	if (ulBits & 0x00001000) m_triggerMode[3] = true; 
	else m_triggerMode[3] = false;

	return ulBits;
}


unsigned long C1394CameraControlTrigger::Status()
{
	unsigned long ulBits;
	int ret;

	if(ret = m_pCamera->ReadQuadlet(STATUS_INDEX + TRIGGER_MODE_OFFSET,&ulBits))
	{
		DllTrace(DLL_TRACE_ERROR,"ControlTrigger::Status: error %d on ReadQuadlet\n");
		return CAM_ERROR;
	}

	if (ulBits & 0x02000000) m_statusOnOff = true; 
	else m_statusOnOff = false;
	if (ulBits & 0x01000000) m_statusPolarity = true; 
	else m_statusPolarity = false;
	m_statusMode = ulBits & 0x000f0000;
	//m_value2 = (unsigned short) ((ulBits & 0x00fff000) >> 12);
	m_value1 = (unsigned short) (ulBits & 0x00000fff);
	m_mode = ulBits & 0xff000000;

	return ulBits;
}


int C1394CameraControlTrigger::SetPolarity(BOOL polarity)
{
	int ret;

	if (polarity)
		m_mode |= 0x01000000;
	else 
		m_mode &= 0xfe000000;
	
	if(ret = SetValues())
		DllTrace(DLL_TRACE_ERROR,"ControlTrigger::SetPolarity: error %d on SetValues()\n",ret);

	return ret;
}


int C1394CameraControlTrigger::SetMode(int mode, int parameter)
{
	int ret;

	m_statusMode = mode;
	m_value1 = parameter;

	if(ret = SetValues())
		DllTrace(DLL_TRACE_ERROR,"ControlTrigger::SetMode: error %d on SetValues()\n",ret);

	return ret;
}


int C1394CameraControlTrigger::SetValues()
{
	unsigned long ulBits;
	int ret;

	ulBits = m_statusMode;
	ulBits <<= 16;
	ulBits += m_value1;
	ulBits = ulBits | m_mode;
	if(ret = m_pCamera->WriteQuadlet(STATUS_INDEX + TRIGGER_MODE_OFFSET, ulBits))
		DllTrace(DLL_TRACE_ERROR,"ControlTrigger::SetValues: error %d on WriteQuadlet\n",ret);

	return ret;
}


int C1394CameraControlTrigger::TurnOn(BOOL on)
{
	int ret;

	m_statusOnOff = (on == TRUE);

	if (on)	
		m_mode |= 0x02000000;
	else 
		m_mode &= 0xfd000000;

	if(ret = SetValues())
		DllTrace(DLL_TRACE_ERROR,"ControlTrigger::TurnOn: error %d on SetValues()\n",ret);

	return ret;
}
