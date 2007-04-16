//  1394CameraControlSize.cpp: implementation of the C1394CameraControlSize class.
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
#include "1394CameraControlSize.h"

extern "C" {
#include "pch.h"
}

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW`
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

C1394CameraControlSize::C1394CameraControlSize()
{
	m_maxH = 0;
	m_maxV = 0;
	m_unitH = 0;
	m_unitV = 0;
	m_top = 0;
	m_left = 0;
	m_width = 0;
	m_height = 0;
	m_colorCode = 0;
	for (int i=0; i<7; i++)
		m_colorCodes[i] = false;

	m_pixelsFrame = 0;
	m_bytesFrameHigh = 0;
	m_bytesFrameLow = 0;
	m_bytesPacketMin = 0;
	m_bytesPacketMax = 0;
	m_bytesPacket = 0;
}


C1394CameraControlSize::~C1394CameraControlSize()
{
}

// read the static (*_INQ) registers

int C1394CameraControlSize::Inquire()
{
	unsigned long status;
	unsigned long temp;
	int ret,i;

	// size control present?
	if (!Supported())
		return CAM_ERROR;

	// mode supported?
	if (!ModeSupported(m_pCamera->m_videoMode))
		return CAM_ERROR_INVALID_VIDEO_SETTINGS;

	if(ret = m_pCamera->ReadQuadlet(0x2e0 + 4*m_pCamera->m_videoMode, &status))
	{
		DllTrace(DLL_TRACE_ERROR,"ControlSize::Inquire: error on ReadQuadlet\n");
		return ret;
	}

	m_offset = 4*status + 0xf0000000;

	DllTrace(DLL_TRACE_CHECK,"ControlSize::Inquire: Using register space at 0xffff%08x\n",m_offset);

	// maximum dimensions at 0x00
	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x00, &status)) 
		return ret;

	m_maxV = status & 0xffff;
	m_maxH = status >> 16;

	DllTrace(DLL_TRACE_CHECK,"ControlSize::Inquire: maxV = %d, maxH = %d\n",m_maxV, m_maxH);

	// size unit at 0x04
	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x04, &status))
		return ret;
	m_unitV = status & 0xffff;
	m_unitH = status >> 16;

	// position unit was added in 1.30
	// try to read 0x4c, if zero or fail, use size unit
	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x4c, &status)) 
	{
		DllTrace(DLL_TRACE_WARNING,"ControlSize::Inquire: error reading unitpos register\n");
		status = 0;
	}

	if(status == 0)
	{
		m_unitHpos = m_unitH;
		m_unitVpos = m_unitV;
	} else {
		m_unitHpos = (status >> 16) & 0xffff;
		m_unitVpos = (status) & 0xffff;
	}

	DllTrace(DLL_TRACE_CHECK,"ControlSize::Inquire: unitVpos = %d, unitHpos = %d\n",m_unitVpos, m_unitHpos);

	// color codes at 0x14
	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x14, &status))
		return ret;

	temp = 1<<31;

	for (i=0; i<8; i++)
	{
		m_colorCodes[i] = (status & temp) != 0;
		DllTrace(DLL_TRACE_CHECK,"ControlSize::Inquire: Color Code %d is %s\n",i,m_colorCodes[i] ? "supported" : "unsupported");
		temp >>= 1;
	}

	// value setting register at 0x7C as of 1.30
	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x7C, &status))
		status = 0;

	m_bValueSetting = (status & 0x80000000) != 0;

	return CAM_SUCCESS;
}


int C1394CameraControlSize::Status()
{
	unsigned long status;
	unsigned long temp = 0x80000000;
	int ret;

	// size control present?
	if (!Supported())
		return CAM_ERROR;

	// mode supported?
	if (!ModeSupported(m_pCamera->m_videoMode))
		return CAM_ERROR_INVALID_VIDEO_SETTINGS;

	if(m_offset == 0)
		return CAM_ERROR_NOT_INITIALIZED;

	// position at 0x08
	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x08, &status)) 
		return ret;
	m_top = status & 0xffff;
	m_left = status >> 16;

	// size at 0x0c
	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x0c, &status))
		return ret;
	m_height = status & 0xffff;
	m_width = status >> 16;

	// current color code at 0x10
	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x10, &status)) 
		return ret;
	m_colorCode = (status>>24) & 0x00ff;

	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x34,&m_pixelsFrame))
		return ret;

	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x38,&m_bytesFrameHigh))
		return ret;

	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x3c, &m_bytesFrameLow))
		return ret;

	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x40, &status)) 
		return ret;
	m_bytesPacketMax = status & 0xffff;
	m_bytesPacketMin = status >> 16;

	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x44, &status)) 
		return ret;
	m_bytesPacket = status >> 16;

	if(m_bytesPacket < m_bytesPacketMin)
		m_bytesPacket = m_bytesPacketMin;

	// packets per frame as of 1.30
	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x48, &status)) 
		status = 0;

	if(status != 0)
		m_packetsFrame = status;
	else
		m_packetsFrame = m_bytesFrameLow / m_bytesPacket;
		
	// value setting register at 0x7C as of 1.30
	if(ret = m_pCamera->ReadQuadlet(m_offset + 0x7C, &status))
		status = 0;

	m_bError1 = (status & 0x00800000) != 0;
	m_bError2 = (status & 0x00400000) != 0;

	return CAM_SUCCESS;
}


int C1394CameraControlSize::SetPosition(unsigned long left, unsigned long top)
{
	int ret;
	unsigned long quadlet;

	if(m_offset == 0)
		return CAM_ERROR_NOT_INITIALIZED;

	// round to the nearest m_unitXpos

	left += m_unitHpos >> 1;
	left -= left % m_unitHpos;

	top += m_unitVpos >> 1;
	top -= top % m_unitVpos;

	// insert a bit of idiotproofing here
	if((left + m_unitH) >= m_maxH || (top + m_unitV) >= m_maxV)
	{
		DllTrace(DLL_TRACE_ERROR,
			"ControlSize::SetPosition: invalid input parameters: (%d, %d)\n",
			left, top);
		return CAM_ERROR_PARAM_OUT_OF_RANGE;
	}
	
	if((left + m_width) > m_maxH || (top + m_height) > m_maxV) 
	{
		DllTrace(DLL_TRACE_ERROR,
			"ControlSize::SetPosition: new position (%d,%d) forces the window (%d x %d) out of bounds (%d,%d)\n",
			left,top,m_width,m_height,m_maxH,m_maxV);

		// bail here, size takes precedence over position
		return CAM_ERROR_PARAM_OUT_OF_RANGE;
	}

	// set members and write quadlet if things have changed
	if(left != m_left || top != m_top)
	{

		m_left = left;
		m_top = top;
		
		quadlet = left;
		quadlet <<= 16;
		quadlet += top;
		if(ret = m_pCamera->WriteQuadlet(m_offset + 0x08, quadlet))
			return ret;
	} else {
		DllTrace(DLL_TRACE_CHECK,
			"ControlSize::SetPosition: top and left are unchanged, skipped WriteQuadlet\n");
	}

	return 0;
}


int C1394CameraControlSize::SetSize(unsigned long width, unsigned long height)
{
	int ret;
	unsigned long quadlet;
	unsigned long newleft = m_left, newtop = m_top;

	
	// round to the nearest m_unitX

	width += m_unitH >> 1;
	width -= width % m_unitH;

	height += m_unitVpos >> 1;
	height -= height % m_unitV;

	// insert a bit of idiotproofing here
	if(width == 0 || width > m_maxH || height == 0 || height > m_maxV)
	{
		DllTrace(DLL_TRACE_ERROR,
			"ControlSize::SetSize: invalid input parameters: (%d, %d)\n",
			width, height);
		return CAM_ERROR_PARAM_OUT_OF_RANGE;
	}

	if((m_left + width) > m_maxH)
	{
		newleft = m_maxH - width;
		DllTrace(DLL_TRACE_WARNING,
			"ControlSize::SetSize: new width(%d) forces the window at %d out of bounds (%d), moving to %d\n",
			width,m_left,m_maxH,newleft);
	}

	if((m_top + height) > m_maxV)
	{
		newtop = m_maxV - height;
		DllTrace(DLL_TRACE_WARNING,
			"ControlSize::SetSize: new height(%d) forces the window at %d out of bounds (%d), moving to %d\n",
			width,m_left,m_maxH,newtop);		
	}

	if(m_left != newleft || m_top != newtop)
	{
		if(ret = SetPosition(newleft,newtop))
			return ret;
	}

	// set members and write quadlet if things have changed

	if(width != m_width || height != m_height)
	{
		m_width = width;
		m_height = height;
		quadlet = width;
		quadlet <<= 16;
		quadlet += height;
		if(ret = m_pCamera->WriteQuadlet(m_offset + 0x0c, quadlet))
			return ret;
		return Update();
	}

	DllTrace(DLL_TRACE_CHECK,
		"ControlSize::SetSize: height and width are unchanged, skipped WriteQuadlet\n");

	return 0;
}


int C1394CameraControlSize::SetColorCode(int code)
{
	int ret;
	unsigned long quadlet;

	if(code < 0 || code >= 8 || !m_colorCodes[code])
	{
		DllTrace(DLL_TRACE_ERROR,"ControlSize::SetColorCode: invalid color code %d\n",code);
		return CAM_ERROR_INVALID_VIDEO_SETTINGS;
	}

	m_colorCode = code;
	quadlet = code;
	quadlet <<= 24;

	if(ret = m_pCamera->WriteQuadlet(m_offset + 0x10, quadlet))
		return ret;
	return Update();
}

	
int C1394CameraControlSize::SetBytesPerPacket(int bytes)
{
	// need more idiotproofing here
	int ret;
	unsigned long quadlet;
	m_bytesPacket = bytes;
	quadlet = bytes;
	quadlet <<= 16;
	if(ret = m_pCamera->WriteQuadlet(m_offset + 0x44, quadlet))
		return ret;

	return CAM_SUCCESS;
}


bool C1394CameraControlSize::ModeSupported(int mode)
{
	unsigned long status;
	unsigned int temp = 0x80000000;
	int ret;
	if(ret = m_pCamera->ReadQuadlet(0x19c,&status))
		return false;
	temp >>= mode;
	return (status & temp) != 0;
}


bool C1394CameraControlSize::Supported()
{
	unsigned long status;
	int ret;
	
	if(ret = m_pCamera->ReadQuadlet(0x100,&status))
		return false;

	return (status & 0x01000000) != 0;
}


int C1394CameraControlSize::Update()
{
	int ret;
	unsigned long status;

	//kludge... must fix later
	if(m_bValueSetting)
	{
		if(ret = m_pCamera->WriteQuadlet(m_offset + 0x7C,0x40000000))
			return ret;

		while(ret = m_pCamera->ReadQuadlet(m_offset + 0x7C,&status))
			if(!(status & 0x40000000))
				break;
	}

	if(ret = Status())
		return ret;

	m_pCamera->m_width = m_width;
	m_pCamera->m_height = m_height;

	return CAM_SUCCESS;
}
