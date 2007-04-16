//  1394Camera.cpp: implementation of the core members of the C1394Camera class.
//
//	Version 5.2 : 03/14/2001
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
/****************************************************/
/*                                                  */
/*            PUBLIC MEMBER FUNCTIONS               */
/*                                                  */
/****************************************************/


/*
 * Constructor
 *
 * Arguments
 *   hWnd: Defaulted to NULL: deprecated, will eventualy remove
 * 
 * Responsibilities:
 *   - init internal vars
 *   - setup member control classes
 */

C1394Camera::C1394Camera()
{
	int format,mode,rate;

	DllTrace(DLL_TRACE_ENTER,"ENTER C1394Camera Constructor\n");

	// this is the best place to trace the structure sizes
	DllTrace(DLL_TRACE_VERBOSE,"sizeof(C1394Camera) = %d\n",sizeof(C1394Camera));
	DllTrace(DLL_TRACE_VERBOSE,"sizeof(C1394CameraControl) = %d\n",sizeof(C1394CameraControl));
	DllTrace(DLL_TRACE_VERBOSE,"sizeof(C1394CameraControlSize) = %d\n",sizeof(C1394CameraControlSize));
	DllTrace(DLL_TRACE_VERBOSE,"sizeof(C1394CameraControlTrigger) = %d\n",sizeof(C1394CameraControlTrigger));
	
	m_pData = NULL;
	m_pName = NULL;
	m_linkChecked = false;
	m_cameraInitialized = false;
	m_node = 0;
	m_hDeviceAcquisition = NULL;
	m_hDeviceCapture = NULL;

	m_videoFormat = 0;
	m_videoMode = 0;
	m_videoFrameRate = 0;
	ZeroMemory(&m_spec,sizeof(CAMERA_SPECIFICATION));
	ZeroMemory(&m_DeviceData,sizeof(DEVICE_DATA));

	m_controlBrightness.m_pCamera = this;
	m_controlAutoExposure.m_pCamera = this;
	m_controlSharpness.m_pCamera = this;
	m_controlWhiteBalance.m_pCamera = this;
	m_controlHue.m_pCamera = this;
	m_controlSaturation.m_pCamera = this;
	m_controlGamma.m_pCamera = this;
	m_controlShutter.m_pCamera = this;
	m_controlGain.m_pCamera = this;
	m_controlZoom.m_pCamera = this;
	m_controlFocus.m_pCamera = this;
	m_controlIris.m_pCamera = this;
	m_controlTrigger.m_pCamera = this;
	m_controlSize.m_pCamera = this;

	m_controlBrightness.SetOffset(BRIGHTNESS_OFFSET);
	m_controlAutoExposure.SetOffset(AUTO_EXPOSURE_OFFSET);
	m_controlSharpness.SetOffset(SHARPNESS_OFFSET);
	m_controlWhiteBalance.SetOffset(WHITE_BALANCE_OFFSET);
	m_controlHue.SetOffset(HUE_OFFSET);
	m_controlSaturation.SetOffset(SATURATION_OFFSET);
	m_controlGamma.SetOffset(GAMMA_OFFSET);
	m_controlShutter.SetOffset(SHUTTER_OFFSET);
	m_controlGain.SetOffset(GAIN_OFFSET);
	m_controlZoom.SetOffset(ZOOM_OFFSET);
	m_controlFocus.SetOffset(FOCUS_OFFSET);
	m_controlIris.SetOffset(IRIS_OFFSET);

	DllTrace(DLL_TRACE_EXIT,"EXIT C1394Camera Constructor\n");

	// initialize video settings matrices to false
	for (format=0; format<8; format++)
	{
		m_bxAvailableFormats[format] = false;
		for (mode=0; mode<8; mode++)
		{
			m_bxAvailableModes[format][mode] = false;
			for (rate=0; rate<8; rate++)
				m_videoFlags[format][mode][rate] = false;
		}
	}
}


/*
 * Destructor
 *
 * Responsibilities
 *
 * If we are grabbing images, stop it.  This is the last line
 * of defense against sloppy programming.  I'm reasonably sure 
 * the destructor gets called even if the camera is a global variable.
 */

C1394Camera::~C1394Camera()
{
	DllTrace(DLL_TRACE_ENTER,"ENTER C1394Camera Destructor\n");
	if(m_hDeviceAcquisition != NULL)
		StopImageAcquisition();

	if(m_hDeviceCapture != NULL)
		StopImageCapture();
	DllTrace(DLL_TRACE_EXIT,"EXIT C1394Camera Destructor\n");
}


/*
 * CheckLink
 *
 * Public
 *
 * Arguments
 *  - <none>
 *
 * Returns
 *  - CAM_SUCCESS: I found at least one camera and have selected camera 0
 *  - CAM_ERROR: I found no cameras on the 1394 bus(es)
 *
 * Calls:
 *   GetNumberCameras
 *   SelectCamera
 *
 * Mutates:
 *   - m_linkchecked (public) indicates that this function has been called
 *       maybe remove it... once again just a crutch for sloppy programmers
 *
 * Comments:
 *   This function is very poorly named.  It is maintained for backwards compatibility.
 *   It may eventualy be replaced by something like RefreshCameraList().
 */

int C1394Camera::CheckLink()
{
	DllTrace(DLL_TRACE_ENTER,"ENTER CheckLink\n");
	m_linkChecked = false;
	if(GetNumberCameras() <= 0)
	{
		DllTrace(DLL_TRACE_ERROR,"CheckLink: no cameras found\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT CheckLink (-1)\n");
		return CAM_ERROR;
	}

	// select the first camera out of convenience
	SelectCamera(0);

	m_linkChecked = true;
	DllTrace(DLL_TRACE_EXIT,"EXIT CheckLink (0)\n");
	return 0;
}


/*
 * SelectCamera
 *
 * Public
 *
 * Indexes into the current device list to point the class at a particular camera
 *
 * Arguments
 *   - node: the (zero-indexed) index of the camera to select
 *
 * Returns:
 *   - CAM_SUCCESS on success
 *   - CAM_ERROR_PARAM_OUT_OF_RANGE: you gave a bad camera number
 *   - CAM_ERROR: general I/O error (probably GetCameraSpecification)
 *
 * Comments:
 *   This is the only class function that will generate dialog boxes and will do so
 *   if and only if it believes that the device you selected isn't *really* a camera
 *   that complies with the 1394 Digital Camera Specification
 */

int C1394Camera::SelectCamera(int node)
{
	int oldNode;
	char *oldName;
	int format, mode, rate;
	ULONG ulRet;

	DllTrace(DLL_TRACE_ENTER,"ENTER SelectCamera (%d)\n",node);

	if(node < 0 || node >= (int) m_DeviceData.numDevices)
	{
		DllTrace(DLL_TRACE_ERROR,"SelectCamera: Camera %d out of range\n",node);
		DllTrace(DLL_TRACE_EXIT,"EXIT SelectCamera (%d)\n",CAM_ERROR_PARAM_OUT_OF_RANGE);
		return CAM_ERROR_PARAM_OUT_OF_RANGE;
	}
	oldNode = m_node;
	m_node = node;
	oldName = m_pName;
	m_pName = m_DeviceData.deviceList[m_node].DeviceName;

	// check the software version
	ZeroMemory(&m_spec,sizeof(CAMERA_SPECIFICATION));
	if((ulRet = GetCameraSpecification(m_pName,&m_spec)))
	{
		DllTrace(DLL_TRACE_ERROR,"SelectCamera: Error %08x getting Camera Specification\n",ulRet);
		m_pName = oldName;
		m_node = oldNode;
		DllTrace(DLL_TRACE_EXIT,"EXIT SelectCamera (%d)\n",CAM_ERROR);
		return CAM_ERROR;
	}

	if(m_spec.ulSpecification != 0x00A02D)
	{
		char buf[128];
		DllTrace(DLL_TRACE_WARNING, "SelectCamera: Warning: Camera specification (%06x) does not match 0x00A02D\n", m_spec.ulSpecification);
		sprintf(buf,"Camera Specification 0x%06X does not match IIDC 1394 Specification (0x00A02D)",m_spec.ulSpecification);
		MessageBox(NULL,buf,"1394 Camera Error",MB_OK | MB_ICONERROR);
	}

	if (m_spec.ulVersion  < 0x000100 || m_spec.ulVersion > 0x000102)
	{
		char buf[128];
		DllTrace(DLL_TRACE_WARNING,"SelectCamera: Warning: Camera software version (%06x) is not supported\n",m_spec.ulVersion);
		sprintf(buf,"Camera Firmware Version 0x%06X is not a valid version for the IIDC 1394 Digital Camera Specification",m_spec.ulSpecification);
		MessageBox(NULL,buf,"1394 Camera Error",MB_OK | MB_ICONERROR);
	}

	// whenever we switch cameras, reset our internal stuff

	m_cameraInitialized = false;

	// reset the video settings matrices to false
	for (format=0; format<8; format++)
	{
		m_bxAvailableFormats[format] = false;
		for (mode=0; mode<8; mode++)
		{
			m_bxAvailableModes[format][mode] = false;
			for (rate=0; rate<8; rate++)
				m_videoFlags[format][mode][rate] = false;
		}
	}

	DllTrace(DLL_TRACE_CHECK,"SelectCamera: Selected \"%s\"\n",m_pName);
	DllTrace(DLL_TRACE_EXIT,"EXIT SelectCamera (%d)\n",CAM_SUCCESS);
	return CAM_SUCCESS;
}


/*
 * InitCamera
 *
 * Public
 *
 * Performs General initialization of the C1394Camera class for
 * the currently selected camera
 *
 * Arguments:
 *  -<none>
 *
 * Returns:
 *  CAM_SUCCESS
 *  CAM_ERROR_NOT_INITIALIZED: No camera selected (this is poorly named and may be changed)
 *  CAM_ERROR_BUSY: Usualy indicates broken invariants, but in and of itself prevents an
 *    init in the middle of image acquisition
 *  CAM_ERROR: Some register IO failed, GetLastError will tell why
 *
 */

int C1394Camera::InitCamera()
{
	GET_MAX_SPEED_BETWEEN_DEVICES maxSpeed;
	DWORD dwRet;
	ULONG maxSpeedNotLocal, maxSpeedLocal;

	DllTrace(DLL_TRACE_ENTER,"ENTER InitCamera\n");
	
	if(m_cameraInitialized)
	{
		// this isn't really an error case as much as just a dumb mistake
		DllTrace(DLL_TRACE_WARNING,"InitCamera: Warning: Duplicate Call to InitCamera\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT InitCamera (%d)\n",CAM_SUCCESS);
		return CAM_SUCCESS;
	}

	if(!m_pName)
	{
		DllTrace(DLL_TRACE_ERROR,"InitCamera: Error: no camera selected\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT InitCamera (%d)\n",CAM_ERROR_NOT_INITIALIZED);
		return CAM_ERROR_NOT_INITIALIZED;
	}

	if(m_hDeviceAcquisition || m_hDeviceCapture)
	{
		DllTrace(DLL_TRACE_ERROR,"InitCamera: Error: Camera is busy, stop image acquisition first\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT InitCamera (%d)\n",CAM_ERROR_BUSY);
		return CAM_ERROR_BUSY;
	}

	// reset state
	// this frees up any isoch resources that may be left behind
	// from a previous program that didn't clean up after itself
	// properly (i.e. crashed)

	// this should *probably* have a return code, but if things
	// go wrong with resetting the state, other problems will rear up

	ResetCameraState(m_pName);

	// determine max speed
	// this is used for allocating bandwidth and stuff
	maxSpeed.fulFlags = 0;
	maxSpeed.ulNumberOfDestinations = 0;
	if (dwRet = GetMaxSpeedBetweenDevices(NULL, m_pName, &maxSpeed))
	{
		DllTrace(DLL_TRACE_ERROR,"InitCamera: Error %08x on GetMaxSpeedBetweenDevices (NotLocal)\n",dwRet);
		DllTrace(DLL_TRACE_EXIT,"EXIT InitCamera (%d)\n",CAM_ERROR);
		return CAM_ERROR;
	}
	maxSpeedNotLocal = maxSpeed.fulSpeed;

	maxSpeed.fulFlags = 1;
	if (dwRet = GetMaxSpeedBetweenDevices(NULL, m_pName, &maxSpeed))
	{
		DllTrace(DLL_TRACE_ERROR,"InitCamera: Error %08x on GetMaxSpeedBetweenDevices (Local)\n",dwRet);
		DllTrace(DLL_TRACE_EXIT,"EXIT InitCamera (%d)\n",CAM_ERROR);
		return CAM_ERROR;
	}
	maxSpeedLocal = maxSpeed.fulSpeed;

	// select the smaller of the two
	m_maxSpeed = (maxSpeedLocal < maxSpeedNotLocal ? maxSpeedLocal : maxSpeedNotLocal);

	// get the vendor and model names from the driver
	// the return codes aren't checked here because they are
	// simple data transfers from the device extension to our
	// buffers
	GetModelName(m_pName,m_nameModel,256);
	GetVendorName(m_pName,m_nameVendor,256);
	GetUniqueID(m_pName,&m_UniqueID);

	// determine video formats/modes/rates
	// private functions return bools and do their own respective tracing
	if(!InquireVideoFormats())
	{
		DllTrace(DLL_TRACE_ERROR,"InitCamera: Error on InquireVideoFormats\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT InitCamera (%d)\n",CAM_ERROR);
		return CAM_ERROR;
	}

	if(!InquireVideoModes())
	{
		DllTrace(DLL_TRACE_ERROR,"InitCamera: Error on InquireVideoModes\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT InitCamera (%d)\n",CAM_ERROR);
		return CAM_ERROR;
	}

	if(!InquireVideoRates())
	{
		DllTrace(DLL_TRACE_ERROR,"InitCamera: Error on InquireVideoRates\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT InitCamera (%d)\n",CAM_ERROR);
		return CAM_ERROR;
	}

	m_cameraInitialized = true;

	DllTrace(DLL_TRACE_EXIT,"EXIT InitCamera (%d)\n",CAM_SUCCESS);
	return CAM_SUCCESS;
}

/*
 * ReadQuadlet
 *
 * Public
 *
 * Essentially a wrapper for ReadRegisterUL
 *
 * Arguments:
 *  - address: the offset into the camera register space to read from
 *      Addresses leading with "f" as in 0xf0000344 will be treated as absolute addresses.
 *      Those without a leading "f" will be treated as offsets into the 
 *        CSR space, so 0x00000344 will be read at CSR + 0x344, usually 0xf0f00344
 *
 *  - pData: the place to put the data read in from the register.  The data will be in
 *      machine order, so the most significant bit would be 0x80000000
 *
 * Returns:
 *  - CAM_SUCCESS
 *  - CAM_ERROR: something bad happened down in the bowels of the OS, use GetLastError() to find out.
 *  - CAM_ERROR_NOT_INITIALIZED: no camera has been selected
 *
 * Comments:
 *  ReadQuadlet catches ERROR_SEM_TIMEOUT, which means the camera was too busy to process the request.
 *  It will retry the request for the initial value of nretries times, by default this is 4, but
 *  it may become a registry variable.
 */

int C1394Camera::ReadQuadlet(unsigned long address, unsigned long *pData)
{
	unsigned long data = 0;
	int nretries = 4;
	DWORD dwRet;

	DllTrace(DLL_TRACE_ENTER,"ENTER ReadQuadlet (%08x,%08x)\n",address,pData);

	if(!m_pName)
	{
		DllTrace(DLL_TRACE_ERROR,"ReadQuadlet: No Camera has been selected\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT ReadQuadlet (%d)\n",CAM_ERROR_NOT_INITIALIZED);
		return CAM_ERROR_NOT_INITIALIZED;
	}

	if(!pData)
	{
		DllTrace(DLL_TRACE_ERROR,"ReadQuadlet: You gave me a NULL pointer you fool!\n");
		SetLastError(ERROR_INVALID_PARAMETER);
		DllTrace(DLL_TRACE_EXIT,"EXIT ReadQuadlet (%d)\n",CAM_ERROR);
		return CAM_ERROR;
	}

	// we're gonna try this nretries times, looking for
	// ERROR_SEM_TIMEOUT, which maps to STATUS_IO_TIMEOUT
	// meaning that the camera can't keep up.
	while((dwRet = ReadRegisterUL(m_pName,address,pData)) != 0 && nretries > 0)
	{
		if(dwRet != ERROR_SEM_TIMEOUT)
			// some other error, break out
			break;
		// Sleep for 10 ms
		Sleep(10);
		nretries--;
		DllTrace(DLL_TRACE_WARNING,"ReadQuadlet: Warning: Timeout on ReadRegister@0x%08x.  Retries Remaining: %d\n",
			address,nretries);
	}

	if(dwRet != 0)
	{
		DllTrace(DLL_TRACE_ERROR,"ReadQuadlet: Unrecoverable error %08x on ReadQuadlet\n",dwRet);
		// dwRet was gotten in ReadQuadlet by a call to GetLastError, so it's still there
		DllTrace(DLL_TRACE_EXIT,"EXIT ReadQuadlet (%d)\n",CAM_ERROR);
		return CAM_ERROR;
	}

	DllTrace(DLL_TRACE_EXIT,"EXIT ReadQuadlet (%d)\n",CAM_SUCCESS);
	return CAM_SUCCESS;
}


/*
 * WriteQuadlet
 *
 * Public
 *
 * Essentially a wrapper for WriteRegisterUL
 *
 * Arguments:
 *  - address: the offset into the camera register space to write to
 *      Addresses leading with "f" as in 0xf0000344 will be treated as absolute addresses.
 *      Those without a leading "f" will be treated as offsets into the 
 *        CSR space, so 0x00000344 will be written at CSR + 0x344, usually 0xf0f00344
 *
 *  - data: the data write to the register
 *
 * Returns:
 *  - CAM_SUCCESS
 *  - CAM_ERROR: something bad happened down in the bowels of the OS, use GetLastError() to find out.
 *  - CAM_ERROR_NOT_INITIALIZED: no camera has been selected
 *
 * Comments:
 *  WriteQuadlet catches ERROR_SEM_TIMEOUT, which means the camera was too busy to process the request.
 *  It will retry the request for the initial value of nretries times, by default this is 4, but
 *  it may become a registry variable.
 */

int C1394Camera::WriteQuadlet(unsigned long address, unsigned long data)
{
	int nretries = 4;
	DWORD dwRet;

	DllTrace(DLL_TRACE_ENTER,"ENTER WriteQuadlet (%08x,%08x)\n",address,data);

	if(!m_pName)
	{
		DllTrace(DLL_TRACE_ERROR,"WriteQuadlet: No Camera has been selected\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT WriteQuadlet (%d)\n",CAM_ERROR_NOT_INITIALIZED);
		return CAM_ERROR_NOT_INITIALIZED;
	}

	// we're gonna try this nretries times, looking for
	// ERROR_SEM_TIMEOUT, which maps to STATUS_IO_TIMEOUT
	// meaning that the camera can't keep up.
	while((dwRet = WriteRegisterUL(m_pName,address,data)) != 0 && nretries > 0)
	{
		if(dwRet != ERROR_SEM_TIMEOUT)
			// some other error, break out
			break;
		// Sleep for 10 ms
		Sleep(10);
		nretries--;
		DllTrace(DLL_TRACE_WARNING,"WriteQuadlet: Warning: Timeout on WriteRegister@0x%08x.  Retries Remaining: %d\n",
			address,nretries);
	}

	if(dwRet != 0)
	{
		DllTrace(DLL_TRACE_ERROR,"WriteQuadlet: Unrecoverable error %08x on WriteRegisterUL\n",dwRet);
		// dwRet was gotten in WriteRegisterUL by a call to GetLastError, so it's still there
		DllTrace(DLL_TRACE_EXIT,"EXIT WriteQuadlet (%d)\n",CAM_ERROR);
		return CAM_ERROR;
	}

	DllTrace(DLL_TRACE_EXIT,"EXIT WriteQuadlet (%d)\n",CAM_SUCCESS);
	return CAM_SUCCESS;
}



/*
 * GetMaxSpeed
 *
 * Public
 *
 * one-line accessor for m_maxSpeed
 * Note, the "*100" returns the rate in Mbps
 */

int C1394Camera::GetMaxSpeed()
{
	return m_maxSpeed * 100;
}


/*
 * GetVersion
 *
 * Public
 * 
 * One-line accessor for m_spec.ulVersion
 */

unsigned long C1394Camera::GetVersion()
{
	return m_spec.ulVersion;
}


/*
 * GetNumberCameras
 *
 * Public
 * 
 * Returns the stored result of GetDevideList if there is one to be had,
 * Otherwise returns the result of a direct call to GetDeviceList, which
 * the number of currently active 1394cmdr device objects.
 */

int C1394Camera::GetNumberCameras()
{
	if(m_linkChecked)
		return m_DeviceData.numDevices;
	else
		return GetDeviceList(&m_DeviceData);
}


/*
 * GetNode
 *
 * Public
 * 
 * One-line accessor for m_node, in case you forget (or are just lazy)
 */

int C1394Camera::GetNode()
{
	return m_node;
}



/****************************************************/
/*                                                  */
/*           PRIVATE MEMBER FUNCTIONS               */
/*                                                  */
/****************************************************/


/*
 * InitResources
 *
 * Private
 *
 * Allocates the Isochronous resources necessary to start an isochronous
 * streaming operation.
 *
 * Called by:
 *   StartImageCapture
 *   StartImageAcquisition
 */

BOOL C1394Camera::InitResources()
{
	ISOCH_ALLOCATE_BANDWIDTH AllocBandwidth;
	ISOCH_ALLOCATE_CHANNEL AllocChannel;
	ISOCH_ALLOCATE_RESOURCES AllocResources;
	ULONG ulRet,ulData;

	DllTrace(DLL_TRACE_ENTER,"ENTER InitResources\n");

	// Tripping either of these would indicate some sort of broken invariant
	// As they are also checked by StartImage*, which are the only functions
	// that call this.

	// however, we will check them anyway

	if (!m_pName)
	{
		DllTrace(DLL_TRACE_ERROR,"InitResources: Error: No camera selected\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT InitResources (FALSE)\n");
		return FALSE;
	}

	if(m_hDeviceAcquisition || m_hDeviceCapture)
	{
		DllTrace(DLL_TRACE_ERROR,"InitResources: Error: Camera is busy, stop image acquisition first\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT InitResources (FALSE)\n");
		return FALSE;
	}

	m_hBandwidth = m_hResource = NULL;
	m_lChannel = -1;

	// what the hell were these doing here?
	// the only mutators for the arguments to
	// these functions are the funcations themselves
	//SetVideoFormat(m_videoFormat);
	//SetVideoMode(m_videoMode);
	//SetVideoFrameRate(m_videoFrameRate);

//	AllocateBandwidth();
	ZeroMemory(&AllocBandwidth, sizeof(ISOCH_ALLOCATE_BANDWIDTH));
	AllocBandwidth.nMaxBytesPerFrameRequested = m_maxBytes;
	AllocBandwidth.fulSpeed = m_maxSpeed;
	ulRet = IsochAllocateBandwidth(NULL, m_pName, &AllocBandwidth);
	if(ulRet)
	{
		DllTrace(DLL_TRACE_ERROR,"InitResources: Error %08x on IsochAllocateBandwidth\n",ulRet);
		goto exit_InitResources;
	}
	DllTrace(DLL_TRACE_VERBOSE,"InitResources: BytesPerFrameAvailable: %d\n",
		AllocBandwidth.BytesPerFrameAvailable);
	DllTrace(DLL_TRACE_VERBOSE,"InitResources: SpeedSelected: %d\n",
		AllocBandwidth.SpeedSelected);

	m_hBandwidth = AllocBandwidth.hBandwidth;

//	AllocateChannel();
	ZeroMemory(&AllocChannel,sizeof(ISOCH_ALLOCATE_CHANNEL));
	AllocChannel.nRequestedChannel = m_node;

	ulRet = IsochAllocateChannel(NULL, m_pName, &AllocChannel);
	if(ulRet)
	{
		DllTrace(DLL_TRACE_ERROR,"InitResources: Error %08x on IsochAllocateChannel\n",ulRet);
		goto exit_InitResources;
	}
	DllTrace(DLL_TRACE_VERBOSE,"InitResources: Channel: %d\n",
		AllocChannel.Channel);
	DllTrace(DLL_TRACE_VERBOSE,"InitResources: ChannelsAvailable: %d\n",
		AllocChannel.ChannelsAvailable);

	m_lChannel = AllocChannel.Channel;

//	AllocateResources();
	ZeroMemory(&AllocResources,sizeof(ISOCH_ALLOCATE_RESOURCES));
	AllocResources.fulSpeed = m_maxSpeed;
	// BUGCHECK
	// OCHI compliant controllers are required to have RESOURCE_STRIP..., but maybe we should check anyway
	AllocResources.fulFlags = RESOURCE_USED_IN_LISTENING|RESOURCE_STRIP_ADDITIONAL_QUADLETS; // Listen Mode
	AllocResources.nChannel = m_lChannel;
	AllocResources.nMaxBytesPerFrame = m_maxBytes;
	AllocResources.nNumberOfBuffers = 10;
	AllocResources.nMaxBufferSize = m_maxBufferSize;
	AllocResources.nQuadletsToStrip = 1;
	ulRet = IsochAllocateResources(NULL, m_pName, &AllocResources);
	if(ulRet)
	{
		DllTrace(DLL_TRACE_ERROR,"InitResources: Error %08x on IsochAllocateResources\n",ulRet);
		goto exit_InitResources;
	}
	DllTrace(DLL_TRACE_VERBOSE,"InitResources: hResource: %08x\n",
		AllocResources.hResource);

	m_hResource = AllocResources.hResource;

	//SetChannelSpeed();
	ulData = (ULONG) m_lChannel;
	ulData <<= 4;
	ulData += (m_maxSpeed/2);
	ulData <<= 24;
	ulRet = WriteQuadlet(0x60c, ulData);
	if(ulRet)
	{
		DllTrace(DLL_TRACE_ERROR,"InitResources: Error %08x on WriteQuadlet(0x060C)\n",ulRet);
		goto exit_InitResources;
	}

	DllTrace(DLL_TRACE_EXIT,"EXIT InitResources (TRUE)\n");

	return TRUE;

exit_InitResources:

	// this is the common exit point for failed resource allocation
	// free them in reverse order

	if(m_hResource)
	{
		IsochFreeResources(NULL,m_pName,m_hResource);
		m_hResource = NULL;
	}

	if(m_lChannel != -1)

	{
		IsochFreeChannel(NULL,m_pName,m_lChannel);
		m_lChannel = -1;
	}

	if(m_hBandwidth)
	{
		IsochFreeBandwidth(NULL,m_pName,m_hBandwidth);
		m_hBandwidth = NULL;
	}

	DllTrace(DLL_TRACE_EXIT,"EXIT InitResources (FALSE)\n");
	return FALSE;
}


/*
 * FreeResources
 *
 * Private
 *
 * Frees the isochronous bandwidth, channel, and resource handle
 * that are allocated by InitResources
 *
 * Called by:
 *   StopImageCapture
 *   StopImageAcquisition
 */

BOOL C1394Camera::FreeResources()
{
	ULONG ulRet;
	BOOL bRet = TRUE;
	if (ulRet = IsochFreeResources(NULL, m_pName, m_hResource))
	{
		DllTrace(DLL_TRACE_ERROR,"FreeResources: Error %08x on IsochFreeResources\n",ulRet);
		return FALSE;
	}
	if (ulRet = IsochFreeChannel(NULL, m_pName, m_lChannel))
	{
		DllTrace(DLL_TRACE_ERROR,"FreeResources: Error %08x on IsochFreeChannel\n",ulRet);
		return FALSE;
	}
	if (IsochFreeBandwidth(NULL, m_pName, m_hBandwidth))
	{
		DllTrace(DLL_TRACE_ERROR,"FreeResources: Error %08x on IsochFreeBandwidth\n",ulRet);
		return FALSE;
	}
	return TRUE;
}



/*
 * OneShot
 *
 * Private
 *
 * Writes to the ONE_SHOT register, telling the camera to transmit one frame
 *
 * Returns TRUE/FALSE as they would typically reflect good/bad
 *
 * Comments:
 *   This function is a wrapper for an otherwise simple procedure.  Maybe it should
 *   be inlined and just return whatever the WriteRegister returns
 *
 *   This function is currently not used by anything at all
 *
 *   Also, MultiShot() should be implemented
 */

BOOL C1394Camera::OneShot()
{
	ULONG ulRet;
	BOOL retval = TRUE;

	DllTrace(DLL_TRACE_ENTER,"ENTER OneShot\n");
	if((ulRet = WriteQuadlet(0x61c,0x80000000)))
	{
		DllTrace(DLL_TRACE_ERROR,"OneShot: error %08x on WriteQuadlet(0x61c)\n",ulRet);
		retval = FALSE;
	}

	DllTrace(DLL_TRACE_EXIT,"EXIT OneShot (%d)\n",retval);
	return retval;
}

// these video stream functions should probably leave
// they are more one-liners that would pop up error boxes

void C1394Camera::StartVideoStream()
{
	DllTrace(DLL_TRACE_ENTER,"ENTER StartVideoStream\n");
	if(WriteQuadlet(0x614,0x80000000) < 0)
		DllTrace(DLL_TRACE_ERROR,"Error Starting Video Stream");
	DllTrace(DLL_TRACE_EXIT,"EXIT StartVideoStream\n");
}


void C1394Camera::StopVideoStream()
{
	DllTrace(DLL_TRACE_ENTER,"ENTER StopVideoStream\n");
	if(WriteQuadlet(0x614,0) < 0)
		DllTrace(DLL_TRACE_ERROR,"Error Stopping Video Stream");
	DllTrace(DLL_TRACE_EXIT,"EXIT StopVideoStream\n");
}


/**************************************************/
/*                                                */
/*           DELETED MEMBER FUNCTIONS             */
/*                                                */
/*  These Functions have become unnecessary for   */
/*  one reason or another.  They will eventually  */
/*  be nuked altogether.                          */
/**************************************************/

// deprecated, this function should not be necessary any more
/*
void C1394Camera::ResetLink(bool root)
{
	ULONG flags;
	//GetDeviceName();
	if (root)
		flags = 2;
	else 
		flags = 0;
	if (BusReset(NULL, m_pName, flags))
		ERRORBOX("Problem with Bus Reset");
}
*/
