//  1394CameraControlTrigger.h: interface for the C1394CameraControlTrigger class.
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

#ifndef __1394CAMERACONTROLTRIGGER_H__
#define __1394CAMERACONTROLTRIGGER_H__

#include "1394CameraControl.h"

class C1394Camera;

class CAMAPI C1394CameraControlTrigger
{

public:
	int SetMode(int mode, int parameter);
	int SetPolarity(BOOL polarity);
	int TurnOn(BOOL on);
	unsigned long Status();
	unsigned long Inquire();
	C1394CameraControlTrigger();
	~C1394CameraControlTrigger();

	C1394Camera* m_pCamera;
	// feature inquiry values
	bool m_present;	
	bool m_readout;
	bool m_onoff;
	bool m_polarity;
	bool m_triggerMode[4];
	bool m_statusPolarity;
	bool m_statusOnOff;
	int m_statusMode;
private:	
	int SetValues();
	unsigned long m_mode;
	unsigned short m_value1;					// bits 20 to 31
};

#endif // __1394CAMERACONTROLTRIGGER_H__
