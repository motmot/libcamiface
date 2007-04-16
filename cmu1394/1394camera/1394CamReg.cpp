///////////////////////////////////////////////////////////////////////////////
//
// 1394CamReg.cpp
//
// source for C1394Camera::
//  - Reg(Load/Save)Settings
//
// Provides an encapsulated means of storing/retrieving camera settings 
// to and from the system registry.
//
//  Copyright 6/2003
//
//  Christopher Baker
//  Robotics Institute
//  Carnegie Mellon University
//  Pittsburgh, PA
//  tallbaker@cmu.edu
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
#include "pch.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

int C1394Camera::RegLoadSettings(const char *pname)
{
	HKEY  hKey;
	DWORD dwRet,dwFoo,dwSize,dwType = REG_DWORD;
	char buf[256];
	unsigned long mask;

#define REG_READ(name){\
		dwFoo = 0;\
		dwType = 0;\
		dwSize = sizeof(DWORD);\
		dwRet = RegQueryValueEx(hKey,name,0,&dwType,(LPBYTE)&dwFoo,&dwSize);\
		if(dwRet != ERROR_SUCCESS)\
		{\
			DllTrace(DLL_TRACE_ERROR,\
			"RegLoadSettings: error %d querying registry key for %s\n",\
			dwRet,name);}\
			return CAM_ERROR;\
		}	

	sprintf(buf,"Software\\CMU\\1394Camera\\%08x%08x\\CameraSettings\\%s",
		m_UniqueID.HighPart,m_UniqueID.LowPart,pname);

	DllTrace(DLL_TRACE_CHECK,"RegLoadSettings: Trying to open \"%s\"\n",buf);

	dwRet = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		buf,
		0,
		KEY_ALL_ACCESS,
		&hKey);

	if(dwRet == ERROR_SUCCESS)
	{
		REG_READ("Format");
		SetVideoFormat(dwFoo);
		REG_READ("Mode");
		SetVideoMode(dwFoo);
		REG_READ("Rate");
		SetVideoFrameRate(dwFoo);

		// Feature_Hi_Inq
		ReadQuadlet(0x404, &mask);

		if(mask & 0x80000000)
		{
			REG_READ("Brightness");
			m_controlBrightness.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			m_controlBrightness.m_value2 = (unsigned short)((dwFoo & 0xffff));
		}
		if(mask & 0x40000000)
		{
			REG_READ("AutoExposure");
			m_controlAutoExposure.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			m_controlAutoExposure.m_value2 = (unsigned short)((dwFoo & 0xffff));
		}
		if(mask & 0x20000000)
		{		
			REG_READ("Sharpness");
			m_controlSharpness.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			m_controlSharpness.m_value2 = (unsigned short)((dwFoo & 0xffff));
		}
		if(mask & 0x10000000)
		{
			REG_READ("WhiteBalance");
			m_controlWhiteBalance.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			m_controlWhiteBalance.m_value2 = (unsigned short)((dwFoo & 0xffff));
		}
		if(mask & 0x08000000)
		{
			REG_READ("Hue");
			m_controlHue.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			m_controlHue.m_value2 = (unsigned short)((dwFoo & 0xffff));
		}
		if(mask & 0x04000000)
		{
			REG_READ("Saturation");
			m_controlSaturation.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			m_controlSaturation.m_value2 = (unsigned short)((dwFoo & 0xffff));
		}
		if(mask & 0x02000000)
		{
			REG_READ("Gamma");
			m_controlGamma.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			m_controlGamma.m_value2 = (unsigned short)((dwFoo & 0xffff));
		}
		if(mask & 0x01000000)
		{
			REG_READ("Shutter");
			m_controlShutter.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			m_controlShutter.m_value2 = (unsigned short)((dwFoo & 0xffff));
		}
		if(mask & 0x00800000)
		{
			REG_READ("Gain");
			m_controlGain.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			m_controlGain.m_value2 = (unsigned short)((dwFoo & 0xffff));
		}
		if(mask & 0x00400000)
		{
			REG_READ("Iris");
			m_controlIris.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			m_controlIris.m_value2 = (unsigned short)((dwFoo & 0xffff));
		}
		if(mask & 0x00200000)
		{
			REG_READ("Focus");
			m_controlFocus.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			m_controlFocus.m_value2 = (unsigned short)((dwFoo & 0xffff));
		}
/*
		if(mask & 0x00080000)
		{
			REG_READ("Trigger");
			m_controlTrigger.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			//m_controlTrigger.m_value2 = (unsigned short)((dwFoo & 0xffff);
		}
*/
		// Feature_Lo_Inq
		ReadQuadlet(0x408, &mask);
		
		if(mask & 0x80000000)
		{
			REG_READ("Zoom");
			m_controlZoom.m_value1 = (unsigned short)((dwFoo >> 16) & 0xffff);
			m_controlZoom.m_value2 = (unsigned short)((dwFoo & 0xffff));
		}

		RegCloseKey(hKey);
	} else {
		DllTrace(DLL_TRACE_ERROR,"RegLoadSettings: Error %d opening key %s\n",dwRet,buf);
	}


	return CAM_SUCCESS;
}

int C1394Camera::RegSaveSettings(const char *pname)
{
	HKEY  hKey;
	DWORD dwRet,dwDisposition,dwFoo,dwSize = sizeof(DWORD),dwType = REG_DWORD;
	LRESULT lRetval = 0;
	char buf[256];
	unsigned long mask;

#define REG_WRITE(name,val){\
		dwFoo = val;\
		dwRet = RegSetValueEx(hKey,name,0,REG_DWORD,(LPBYTE)&dwFoo,dwSize);\
		if(dwRet != ERROR_SUCCESS)\
			DllTrace(DLL_TRACE_ERROR,\
			"RegSaveSettings: error %d setting registry key for %s\n",\
			dwRet,name);}\

	sprintf(buf,"Software\\CMU\\1394Camera\\%08x%08x\\CameraSettings\\%s",
		m_UniqueID.HighPart,m_UniqueID.LowPart,pname);

	DllTrace(DLL_TRACE_CHECK,"RegSaveSettings: Trying to open \"%s\"\n",buf);

	dwRet = RegCreateKeyEx(
		HKEY_LOCAL_MACHINE,
		buf,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,
		&hKey,
		&dwDisposition);
	
	if(dwRet == ERROR_SUCCESS)
	{
		REG_WRITE("Format",m_videoFormat);
		REG_WRITE("Mode",m_videoMode);
		REG_WRITE("Rate",m_videoFrameRate);

		// Feature_Hi_Inq
		ReadQuadlet(0x404, &mask);

		if(mask & 0x80000000)
			REG_WRITE("Brightness",(m_controlBrightness.Status()));
		if(mask & 0x40000000)
			REG_WRITE("AutoExposure",(m_controlAutoExposure.Status()));
		if(mask & 0x20000000)
			REG_WRITE("Sharpness",(m_controlSharpness.Status()));
		if(mask & 0x10000000)
			REG_WRITE("WhiteBalance",(m_controlWhiteBalance.Status()));
		if(mask & 0x08000000)
			REG_WRITE("Hue",(m_controlHue.Status()));
		if(mask & 0x04000000)
			REG_WRITE("Saturation",(m_controlSaturation.Status()));
		if(mask & 0x02000000)
			REG_WRITE("Gamma",(m_controlGamma.Status()));
		if(mask & 0x01000000)
			REG_WRITE("Shutter",(m_controlShutter.Status()));
		if(mask & 0x00800000)
			REG_WRITE("Gain",(m_controlGain.Status()));
		if(mask & 0x00400000)
			REG_WRITE("Iris",(m_controlIris.Status()));
		if(mask & 0x00200000)
			REG_WRITE("Focus",(m_controlFocus.Status()));
		if(mask & 0x00080000)
			REG_WRITE("Trigger",(m_controlTrigger.Status()));

		// Feature_Lo_Inq
		ReadQuadlet(0x408, &mask);
		
		if(mask & 0x80000000)
			REG_WRITE("Zoom",(m_controlZoom.Status()));

		RegCloseKey(hKey);
	} else {
		DllTrace(DLL_TRACE_ERROR,"RegSaveSettings: Error %d opening key %s\n",dwRet,buf);
	}

	return CAM_SUCCESS;
}

int RegGetNumSettings()
{
	//RegQueryInfoKey is your friend here
	return 0;
}

int RegEnumSettings(int index, char *buf, int buflen)
{
	// check index
	// friends are RegEnumKeyEx, RegQueryInfoKey
	return 0;
}