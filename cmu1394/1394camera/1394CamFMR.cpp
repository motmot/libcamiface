///////////////////////////////////////////////////////////////////////////////
//
// 1394CamFMR.cpp
//
// source for C1394Camera::
//  - (Get/Set/Inquire)Video(Format/Mode/Rate)
//
//  Video Settings management code that was moved from 1394Camera.cpp
//  to make that file more manageable
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


/*
 * SetVideoFormat
 *
 * Public
 *
 * Sets current video format unless the camera is actively grabbing frames
 *
 * Arguments
 *  - format: the format in [0,7] that you wish to set
 *
 * Returns:
 *  - CAM_SUCCESS
 *  - CAM_ERROR_NOT_INITIALIZED: No camera celected and/or camera not initialized
 *  - CAM_ERROR_BUSY: The camera is actively acquiring images
 *  - CAM_ERROR: WriteRegister has failed, use GetLastError() to find out why.
 *
 * Comments
 *   Something doesn't seem quite right about writing to the camera's register right
 *   away.  I will have to think on a better alternative.
 */

int C1394Camera::SetVideoFormat(unsigned long format)
{
	DWORD dwRet;

	DllTrace(DLL_TRACE_ENTER,"ENTER SetVideoFormat (%d)\n",format);

	if (!m_pName || !m_cameraInitialized)
	{
		DllTrace(DLL_TRACE_ERROR,"SetVideoFormat: Camera is not initialized\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoFormat (%d)\n",CAM_ERROR_NOT_INITIALIZED);
		return CAM_ERROR_NOT_INITIALIZED;
	}

	if(m_hDeviceAcquisition || m_hDeviceCapture)
	{
		DllTrace(DLL_TRACE_ERROR,"SetVideoFormat: Camera is busy\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoFormat (%d)\n",CAM_ERROR_BUSY);
		return CAM_ERROR_BUSY;
	}
	
	if (format <= 7)
	{
		if(!m_bxAvailableFormats[format])
		{
			// the desired format is not supported
			DllTrace(DLL_TRACE_ERROR,"SetVideoFormat: Format %d not supported\n",format);
			DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoFormat (%d)\n",CAM_ERROR_INVALID_VIDEO_SETTINGS);
			return CAM_ERROR_INVALID_VIDEO_SETTINGS;
		}

		// shift it over into the most significant bits
		if(dwRet = WriteQuadlet(0x608, format << 29))
		{
			DllTrace(DLL_TRACE_ERROR,"SetVideoFormat: error %08x on WriteRegister\n",dwRet);
			DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoFormat (%d)\n",CAM_ERROR);
			return CAM_ERROR;
		}

		m_videoFormat = format;
		// update parameters is a little funky, but leave it anyway
		UpdateParameters();
	} else {
		DllTrace(DLL_TRACE_ERROR,"SetVideoFormat: format %d out of range\n",format);
		DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoFormat (%d)\n",CAM_ERROR_PARAM_OUT_OF_RANGE);
		return CAM_ERROR_PARAM_OUT_OF_RANGE;
	}

	if (format == 7)
		m_controlSize.Status();

	DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoFormat (%d)\n",CAM_SUCCESS);
	return CAM_SUCCESS;
}


/*
 * SetVideoMode
 *
 * Public
 *
 * Sets current video mode unless the camera is actively grabbing frames
 *
 * Arguments
 *  - mode: the desired mode in [0,7] that you wish to set
 *
 * Returns:
 *  - CAM_SUCCESS
 *  - CAM_ERROR_NOT_INITIALIZED: No camera celected and/or camera not initialized
 *  - CAM_ERROR_BUSY: The camera is actively acquiring images
 *  - CAM_ERROR: WriteRegister has failed, use GetLastError() to find out why.
 */

int C1394Camera::SetVideoMode(unsigned long mode)
{
	DWORD dwRet;

	DllTrace(DLL_TRACE_ENTER,"ENTER SetVideoMode (%d)\n",mode);

	if (!m_pName || !m_cameraInitialized)
	{
		DllTrace(DLL_TRACE_ERROR,"SetVideoMode: Camera is not initialized\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoMode (%d)\n",CAM_ERROR_NOT_INITIALIZED);
		return CAM_ERROR_NOT_INITIALIZED;
	}

	if(m_hDeviceAcquisition || m_hDeviceCapture)
	{
		DllTrace(DLL_TRACE_ERROR,"SetVideoMode: Camera is busy\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoMode (%d)\n",CAM_ERROR_BUSY);
		return CAM_ERROR_BUSY;
	}
		
	if (mode<=7)
	{
		if(!m_bxAvailableModes[m_videoFormat][mode])
		{
			DllTrace(DLL_TRACE_ERROR,"SetVideoMode: mode %d is not supported under format %d\n",mode,m_videoFormat);
			DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoMode (%d)\n",CAM_ERROR_INVALID_VIDEO_SETTINGS);
			return CAM_ERROR_INVALID_VIDEO_SETTINGS;
		}

		if(dwRet = WriteQuadlet(0x604, mode << 29))
		{
			DllTrace(DLL_TRACE_ERROR,"SetVideoMode: error %08x on WriteRegister\n",dwRet);
			DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoMode (%d)\n",CAM_ERROR);
			return CAM_ERROR;
		}

		m_videoMode = mode;
		UpdateParameters();
	} else {
		DllTrace(DLL_TRACE_ERROR,"SetVideoMode: mode %d out of range\n",mode);
		DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoMode (%d)\n",CAM_ERROR_PARAM_OUT_OF_RANGE);
		return CAM_ERROR_PARAM_OUT_OF_RANGE;
	}

	DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoMode (%d)\n",CAM_SUCCESS);
	return CAM_SUCCESS;
}


/*
 * SetVideoFrameRate
 *
 * Public
 *
 * Sets current video FrameRate unless the camera is actively grabbing frames
 *
 * Arguments
 *  - rate: the desired frame rate in [0,5] that you wish to set
 *
 * Returns:
 *  - CAM_SUCCESS
 *  - CAM_ERROR_NOT_INITIALIZED: No camera celected and/or camera not initialized
 *  - CAM_ERROR_BUSY: The camera is actively acquiring images
 *  - CAM_ERROR: WriteRegister has failed, use GetLastError() to find out why.
 */

int C1394Camera::SetVideoFrameRate(unsigned long rate)
{
	DWORD dwRet;

	DllTrace(DLL_TRACE_ENTER,"ENTER SetVideoFramteRate (%d)\n",rate);

	if (!m_pName || !m_cameraInitialized)
	{
		DllTrace(DLL_TRACE_ERROR,"SetVideoFrameRate: Camera is not initialized\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoFrameRate (%d)\n",CAM_ERROR_NOT_INITIALIZED);
		return CAM_ERROR_NOT_INITIALIZED;
	}

	if(m_hDeviceAcquisition || m_hDeviceCapture)
	{
		DllTrace(DLL_TRACE_ERROR,"SetVideoFrameRate: Camera is busy\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoFrameRate (%d)\n",CAM_ERROR_BUSY);
		return CAM_ERROR_BUSY;
	}

	if(m_videoMode != 7) 
	{
		if (rate <= 5)
		{
			if(!m_videoFlags[m_videoFormat][m_videoMode][rate])
			{
				DllTrace(DLL_TRACE_ERROR,"SetVideoFrameRate: rate %d is not supported under format %d, mode %d\n",rate,m_videoFormat,m_videoMode);
				DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoFrameRate (%d)\n",CAM_ERROR_INVALID_VIDEO_SETTINGS);
				return CAM_ERROR_INVALID_VIDEO_SETTINGS;
			}

			if(dwRet = WriteQuadlet(0x600, rate << 29))
			{
				DllTrace(DLL_TRACE_ERROR,"SetVideoFrameRate: error %08x on WriteRegister\n",dwRet);
				DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoFrameRate (%d)\n",CAM_ERROR);
				return CAM_ERROR;
			}

			m_videoFrameRate = rate;

			UpdateParameters();
		} else {
			DllTrace(DLL_TRACE_ERROR,"SetVideoFrameRate: rate %d out of range\n",rate);
			DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoFrameRate (%d)\n",CAM_ERROR_PARAM_OUT_OF_RANGE);
			return CAM_ERROR_PARAM_OUT_OF_RANGE;
		}
	} else {
		DllTrace(DLL_TRACE_WARNING,"SetVideoFramerate: it is not meaningful to set the framerate for format 7\n");
	}
	
	DllTrace(DLL_TRACE_EXIT,"EXIT SetVideoFrameRate (%d)\n",CAM_SUCCESS);
	return CAM_SUCCESS;
}


/*
 * GetVideoFormat
 *
 * Public
 *
 * one-line acessor for m_videoFrameRate
 */

int C1394Camera::GetVideoFormat()
{
	return m_videoFormat;
}


/*
 * GetVideoMode
 *
 * Public
 *
 * one-line acessor for m_videoFrameRate
 */

int C1394Camera::GetVideoMode()
{
	return m_videoMode;
}


/*
 * GetVideoFrameRate
 *
 * Public
 *
 * one-line acessor for m_videoFrameRate
 */

int C1394Camera::GetVideoFrameRate()
{
	return m_videoFrameRate;
}


/*
 * InquireVideoFormats
 *
 * Private
 *
 * Reads the format register and fills in our stuff
 * CAM_ERROR indicates a failed ReadRegister
 */

BOOL C1394Camera::InquireVideoFormats()
{
	ULONG temp,value,format;
	DWORD dwRet;

	// inquire video formats
	if(dwRet = ReadQuadlet(0x100,&value))
	{
		DllTrace(DLL_TRACE_ERROR,"InquireVideoFormats, Error %08x on ReadRegister(0x100)\n",dwRet);
		return FALSE;
	}

	if(!(value&0xff000000))
		DllTrace(DLL_TRACE_WARNING,"InquireVideoFormats: warning: no video formats available\n");

	// temp will be our shifty bitmask
	temp = 0x80000000;
	for (format=0; format<8; format++)
	{
		m_bxAvailableFormats[format] = ((value & temp) != 0);
		temp >>= 1;
	}

	// to prevent stupidity, force format 6 to false until we (ever) implement it
	m_bxAvailableFormats[6] = false;
	return TRUE;
}


/*
 * InquireVideoModes
 *
 * Private
 *
 * Reads the mode registers and fills in our stuff
 * FALSE indicates a failed ReadRegister
 */

BOOL C1394Camera::InquireVideoModes()
{
	ULONG value,temp,format,mode;
	DWORD dwRet;

	for (format=0; format<8; format++)
	{
		if (m_bxAvailableFormats[format])
		{
			// inquire video mode for current format
			if(dwRet = ReadQuadlet(0x180+format*4,&value))
			{
				DllTrace(DLL_TRACE_ERROR,"InquireVideoModes, Error %08x on ReadRegister(%03x)\n",dwRet,0x180+format*4);
				return FALSE;
			}

			if(!(value & 0xff000000))
				DllTrace(DLL_TRACE_WARNING,"InquireVideoModes: warning: no available modes for format %d\n",format);

			temp = 0x80000000;
			for (mode=0; mode<8; mode++)
			{
				m_bxAvailableModes[format][mode] = ((value & temp) != 0);
				temp >>= 1;
			}
		}
	}
	return TRUE;
}


/*
 * InquireVideoRates
 *
 * Private
 *
 * Reads the rate registers and fills in our stuff
 * CAM_ERROR indicates a failed ReadRegister
 */

BOOL C1394Camera::InquireVideoRates()
{
	ULONG format, mode, rate, value, temp;
	DWORD dwRet;

	// only modes 0-5 have a concept of framerate
	// 6 has a concept of revisions, which fits within the videoflags structure
	// *** the driver set currently has no support for the stored image format (6)
	// 7 apparently has no concept of framerates, and has quadlet offsets where
	// the framerates usually go... who knows.

	for (format=0; format<7; format++)
	{
		if(m_bxAvailableFormats[format])
		{
			for (mode=0; mode<8; mode++)
			{
				if (m_bxAvailableModes[format][mode])
				{
					// inquire video mode for current format
					if(dwRet = ReadQuadlet(0x200+format*32+mode*4,&value))
					{
						DllTrace(DLL_TRACE_ERROR,"InquireVideoRates, Error %08x on ReadRegister(%03x)\n",dwRet,0x200+format*32+mode*4);
						return FALSE;
					}
					if(!(value & 0xff000000))
						DllTrace(DLL_TRACE_WARNING,"InquireVideoRates: warning: no available rates for format %d mode %d\n",format,mode);
					temp = 0x80000000;
					for (rate=0; rate<8; rate++)
					{
						m_videoFlags[format][mode][rate] = ((value & temp) != 0);
						temp >>= 1;
					}
				}
			}
		}
	}
	return TRUE;
}


// tables of format size data, used by UpdateParameters

int tableWidth[3][8] = {         // [format][mode]
	{ 160,  320,  640,  640,  640,  640,  640,    0},
	{ 800,  800,  800, 1024, 1024, 1024,  800, 1024},
	{1280, 1280, 1280, 1600, 1600, 1600, 1280, 1600}};

int tableHeight[3][8] = {        // [format][mode]
	{ 120,  240,  480,  480,  480,  480,  480,    0},
	{ 600,  600,  600,  768,  768,  768,  600,  768},
	{ 960,  960,  960, 1200, 1200, 1200, 1024, 1200}};

// tableMaxBufferSize is, for the most part, extraneous, but it's okay to have, for now

int tableMaxBufferSize[3][8] = { // [format][mode]
	{  57600,  153600,  460800,  614400,  921600,  307200,  614400,       0},
	{ 960000, 1440000,  480000, 1572864, 2359296,  786432,  960000, 1572864},
	{2457600, 3686400, 1228800, 3840000, 5760000, 1920000, 2457600, 3840000}};

int tableQuadletsPerPacket[3][8][6] = 
// [format][mode][rate]
{	
	{	
		// format 0
		{   0,   0,  15,  30,  60,   0},					
		{   0,  20,  40,  80, 160,   0},	
		{   0,  60, 120, 240, 480,   0},	
		{   0,  80, 160, 320, 640,   0},	
		{   0, 120, 240, 480, 960,   0},	
		{   0,  40,  80, 160, 320, 640},
		{   0,  80, 160, 320, 640,   0},
		{   0,   0,   0,   0,   0,   0}

	},{
		// format 1
		{   0, 125, 250, 500,1000,   0},
		{   0,   0, 735, 750,   0,   0},	
		{   0,   0, 125, 250, 500,1000},	
		{  96, 192, 384, 768,   0,   0},	
		{ 144, 288, 576,   0,   0,   0},	
		{  48,  96, 192, 384, 768,   0},
		{   0, 125, 250, 500,1000,   0},
		{  96, 192, 384, 768,   0,   0}
	
	},{	
		// format 2
		{ 160, 320, 640,   0,   0,   0},
		{ 240, 480, 960,   0,   0,   0},	
		{  80, 160, 320, 640,   0,   0},	
		{ 250, 500,1000,   0,   0,   0},	
		{ 375, 750,   0,   0,   0,   0},	
		{ 125, 250, 500,1000,   0,   0},
		{ 160, 320, 640,   0,   0,   0},
		{ 250, 500,1000,   0,   0,   0}
	}
};

/*
 * UpdateParameters
 *
 * Private
 *
 * Helper function that tweaks m_width, m_height, m_maxBytes, m_maxBufferSize
 * whenever the video settings change.
 *
 * Doing this right away seems just as shady as writing the registers right away
 * but we will leave it for now
 *
 * Called by SetVideo*
 */

void C1394Camera::UpdateParameters()
{
	if (m_videoFormat != 7)
	{
		m_maxBytes = 4 * (tableQuadletsPerPacket[m_videoFormat][m_videoMode][m_videoFrameRate]);
		m_width = tableWidth[m_videoFormat][m_videoMode];
		m_height = tableHeight[m_videoFormat][m_videoMode];
		m_maxBufferSize = tableMaxBufferSize[m_videoFormat][m_videoMode];
	} else {
		m_maxBytes = m_controlSize.m_bytesPacket;
		m_width = m_controlSize.m_width;
		m_height = m_controlSize.m_height;
		m_maxBufferSize = m_controlSize.m_bytesPacket * m_controlSize.m_packetsFrame;
	}
}




