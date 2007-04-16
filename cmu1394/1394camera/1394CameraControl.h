//  1394CameraControl.h: interface for the C1394CameraControl class.
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

#ifndef __1394CAMERACONTROL_H__
#define __1394CAMERACONTROL_H__

// register offsets

// the root register offsets
#define INQUIRY_INDEX 0x500
#define STATUS_INDEX  0x800

// the individual control offsets within the root offsets
#define BRIGHTNESS_OFFSET 0x00
#define AUTO_EXPOSURE_OFFSET 0x04
#define SHARPNESS_OFFSET 0x08
#define WHITE_BALANCE_OFFSET 0x0C
#define HUE_OFFSET 0x10
#define SATURATION_OFFSET 0x14
#define GAMMA_OFFSET 0x18
#define SHUTTER_OFFSET 0x1C
#define GAIN_OFFSET 0x20
#define IRIS_OFFSET 0x24
#define FOCUS_OFFSET 0x28
#define TEMPERATURE_OFFSET 0x2C
#define TRIGGER_MODE_OFFSET 0x30
// 0x34 - 0x7C is reserved for other FEATURE_LO
#define ZOOM_OFFSET 0x80
#define PAN_OFFSET  0x84
#define TILT_OFFSET 0x88
#define OPTICAL_FILTER_OFFSET 0x8C
// 0x90 - 0xBC is reserved for other FEATURE_HI
#define CAPTURE_SIZE_OFFSET 0xC0
#define CAPTURE_QUALITY_OFFSET 0xC4
// 0xC8 - 0xFC is reserved for other FEATURE_HI


// prototype the Camera class so we can make our parent pointer to it.
class C1394Camera;

class CAMAPI C1394CameraControl  
{
friend C1394Camera;

public:
	int TurnOn(BOOL on);
	int SetOnePush();
	int SetAutoMode(BOOL on);
	int SetValues();
	unsigned long Status();
	unsigned long Inquire();
	int SetOffset(unsigned short offset);
	C1394CameraControl();
	~C1394CameraControl();

	C1394Camera* m_pCamera;

	// feature inquiry flags
	bool m_present;	
	bool m_onePush;
	bool m_readout;
	bool m_onoff;
	bool m_auto;
	bool m_manual;
	// feature status flags
	bool m_statusPresent;
	bool m_statusOnePush;
	bool m_statusOnOff;
	bool m_statusAutoManual;

	// min/max from inquiry register
	unsigned short m_min;
	unsigned short m_max;

	// current values from status register
	unsigned short m_value1;					// bits 20 to 31
	unsigned short m_value2;					// bits 8 to 19

private:

	// current mode bits from status register
	unsigned short  m_mode;
	// the control offset from 1394CameraControl.h
	unsigned short  m_offset;
};

#endif // __1394CAMERACONTROL_H__
