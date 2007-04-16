//  1394CameraControlSize.h: interface for the C1394CameraControlSize class.
//
//	Version 5.2
//
//	Copyright 5/2000
// 
//	Iwan Ulrich
//	Robotics Institute
//	Carnegie Mellon University
//	Pittsburgh, PA
//
//  Copyright 3/2002
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

#ifndef __1394CAMERACONTROLSIZE_H__
#define __1394CAMERACONTROLSIZE_H__

#include "1394CameraControl.h"

class C1394Camera;

class CAMAPI C1394CameraControlSize  
{
friend C1394Camera;

public:
	bool Supported();
	bool ModeSupported(int mode);
	int SetColorCode(int code);
	int SetSize(unsigned long width, unsigned long height);
	int SetPosition(unsigned long left, unsigned long top);
	int SetBytesPerPacket(int bytes);
	int Status();
	int Inquire();
	C1394CameraControlSize();
	virtual ~C1394CameraControlSize();

	unsigned long m_maxH;
	unsigned long m_maxV;
	unsigned long m_unitH;
	unsigned long m_unitV;
	unsigned long m_unitHpos;
	unsigned long m_unitVpos;
	unsigned long m_top;
	unsigned long m_left;
	unsigned long m_width;
	unsigned long m_height;
	unsigned long m_colorCode;
	unsigned long m_pixelsFrame;
	unsigned long m_bytesFrameHigh;
	unsigned long m_bytesFrameLow;
	unsigned long m_bytesPacketMin;
	unsigned long m_bytesPacketMax;
	unsigned long m_bytesPacket;
	unsigned long m_packetsFrame;
	bool m_colorCodes[7];
	bool m_bError1,m_bError2;

	C1394Camera* m_pCamera;

private:
	int Update();
	unsigned long m_offset;
	bool m_bValueSetting;
};

#endif // __1394CAMERACONTROLSIZE_H__
