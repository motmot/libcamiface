//  1394Camera.h: interface for the C1394Camera class.
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

#ifndef __1394CAMERA_H__
#define __1394CAMERA_H__

#ifdef MY1394CAMERA_EXPORTS

// compiling library, reference
// private, potentially modified
// version of the header
#include "1394camapi.h"

#else

// using library, use global version
#include <1394camapi.h>

#endif // MY1394CAMERA_EXPORTS

#include "1394CameraControl.h"
#include "1394CameraControlTrigger.h"
#include "1394CameraControlSize.h"

// error codes

#define CAM_SUCCESS 0
#define CAM_ERROR -1
#define CAM_ERROR_NOT_INITIALIZED 1
#define CAM_ERROR_INVALID_VIDEO_SETTINGS 2
#define CAM_ERROR_BUSY 3
#define CAM_ERROR_INSUFFICIENT_RESOURCES 4
#define CAM_ERROR_PARAM_OUT_OF_RANGE 5
#define CAM_ERROR_FRAME_TIMEOUT 6

// structures used by C1394Camera

/*
 * ACQUISITION_BUFFER
 *
 * Associates all the necessary information for an image acquisition buffer
 * This includes the overlapped structure used with DeviceIoControl, pointers
 * to the buffer and other bookeeping stuff
 */

typedef struct _ACQUISITION_BUFFER {
	OVERLAPPED						overLapped;
    PISOCH_ATTACH_BUFFERS           pIsochAttachBuffers;
	PUCHAR							pDataBuf;
	PUCHAR							pFrameStart;
	int								index;
	struct _ACQUISITION_BUFFER		*pNextBuffer;
} ACQUISITION_BUFFER, *PACQUISITION_BUFFER;

// the C1394Camera class
// member function implementations are in  1394Camera.cpp unless otherwise noted

class CAMAPI C1394Camera  
{
friend class C1394CameraControlSize;  

public:
	// constructor
	C1394Camera();
	// destructor
	~C1394Camera();

	// Selection/Control
	int InitCamera();
	int GetNode();
	int SelectCamera(int node);
	unsigned long GetVersion();
	int GetNumberCameras();
	int GetMaxSpeed();
	int CheckLink();

	// Store/Retrieve Settings from camera EEPROM
	int MemGetNumChannels();
	int MemGetCurrentChannel();
	int MemLoadChannel(int channel);
	int MemSaveChannel(int channel);

	// Store/Retrieve Settings from system Registry
	int RegLoadSettings(const char *pname);
	int RegSaveSettings(const char *pname);

	// Raw register I/O
	int WriteQuadlet(unsigned long address, unsigned long data);
	int ReadQuadlet(unsigned long address, unsigned long *pData);

	// Video format/mode/rate
	int GetVideoFormat();
	int SetVideoFormat(unsigned long format);
	int GetVideoMode();
	int SetVideoMode(unsigned long mode);
	int GetVideoFrameRate();
	int SetVideoFrameRate(unsigned long rate);
	
	// Image Capture (1394CamCap.cpp)
	int StartImageCapture();
	int CaptureImage();
	int StopImageCapture();

	// Image Acquisition (1394CamAcq.cpp)
	int StartImageAcquisition();
	int StartImageAcquisitionEx(int nBuffers, int FrameTimeout, int Flags);
	int AcquireImage();
	int AcquireImageEx(BOOL DropStaleFrames, int *lpnDroppedFrames);
	int StopImageAcquisition();

	// Synchronization Support
	HANDLE GetFrameEvent();

	// Color Format Conversion (1394CamRGB.cpp)

	// convert data to standard: RGB, upper-left corner
	// based on video format/mode
	void getRGB(unsigned char *pBitmap);

	// same as getRGB, except data is returned in the
	// bottom-up, BGR format the MS calls a DIB
	void getDIB(unsigned char *pBitmap);

	// individual RGB converters
	void YtoRGB(unsigned char *pBitmap);
	void Y16toRGB(unsigned char *pBitmap);
	void YUV411toRGB(unsigned char* pBitmap);
	void YUV422toRGB(unsigned char* pBitmap);
	void YUV444toRGB(unsigned char* pBitmap);
	void RGB16toRGB(unsigned char *pBitmap);

	// Control Wrappers (ControlWrappers.cpp)
	void InquireControlRegisters();
	void StatusControlRegisters();
	void SetIris(int value);
	void SetFocus(int value);
	void SetZoom(int value);
	void SetBrightness(int value);
	void SetAutoExposure(int value);
	void SetSharpness(int value);
	void SetWhiteBalance(int u, int v);
	void SetHue(int value);
	void SetSaturation(int value);
	void SetGamma(int value);
	void SetShutter(int value);
	void SetGain(int value);

	// Control Members
	// Feature_Hi
	C1394CameraControl m_controlBrightness;
	C1394CameraControl m_controlAutoExposure;
	C1394CameraControl m_controlSharpness;
	C1394CameraControl m_controlWhiteBalance;
	C1394CameraControl m_controlHue;
	C1394CameraControl m_controlSaturation;
	C1394CameraControl m_controlGamma;
	C1394CameraControl m_controlShutter;
	C1394CameraControl m_controlGain;
	C1394CameraControl m_controlIris;
	C1394CameraControl m_controlFocus;
	C1394CameraControlTrigger m_controlTrigger;
	// there is a temperature control as well to worry about

	// Feature_Lo
	C1394CameraControl m_controlZoom;
	// there's also pan, tilt, and optical filter controls, but we don't care for now

	// Partial Scan Control class
	C1394CameraControlSize m_controlSize;

	// utility members
	int m_width;
	int m_height;
	bool m_linkChecked;
	bool m_cameraInitialized;
	char m_nameModel[256];
	char m_nameVendor[256];
	LARGE_INTEGER m_UniqueID;
	unsigned char* m_pData;
	bool m_bxAvailableFormats[8]; // [format]
	bool m_bxAvailableModes[8][8]; //[format][mode]
	bool m_videoFlags[8][8][8];		// [format][mode][rate]

private:
	BOOL InitResources();
	BOOL FreeResources();
	BOOL InquireVideoFormats();
	BOOL InquireVideoModes();
	BOOL InquireVideoRates();
	BOOL OneShot();
	void UpdateParameters();
	void StopVideoStream();
	void StartVideoStream();

	// pertaining to video format/mode/rate
	int m_videoFrameRate;
	int m_videoMode;
	int m_videoFormat;
	int m_maxBytes;
	int m_maxBufferSize;
	ULONG m_maxSpeed;

	// which camera are we using
	int m_node;
	char* m_pName;

	// camera data grabbed from the driver
	CAMERA_SPECIFICATION m_spec;
	DEVICE_DATA m_DeviceData;

	// our isochronous resources (to replace the above structs)
	HANDLE m_hBandwidth;
	HANDLE m_hResource;
	LONG m_lChannel;

	// buffer management
	PACQUISITION_BUFFER	m_pFirstBuffer;
	PACQUISITION_BUFFER	m_pLastBuffer;
	PACQUISITION_BUFFER	m_pCurrentBuffer;

	// persistent handles
	HANDLE		m_hDeviceAcquisition;
	HANDLE		m_hDeviceCapture;
	

	// things that used to be part of the class, but aren't anymore
	// they are preserved here partly for posterity, and partly "just in case"

	// resetting the link is no longer necessary, so we do not support it
	//void ResetLink(bool root);

	//void DisableVideoMode(int format, int mode);
	//void DisableVideoFormat(int format);
	// removed private functions
	// these functions were no-argument one-liners that used to pop up message boxes
	// they have been consolidated directly into the functions that called them.
	//void Speed(unsigned char value);
	//void AllocateResources();
	//void AllocateChannel();
	//void AllocateBandwidth();
	//void SetChannelSpeed();
	//void GetDeviceName();
	//void StopListen();
	//void Listen();

	// removed members
	// for the most part, they were from the old buffer management code
	//int m_nBuffers;
	//int m_nDescriptors;
	//ULONG		m_acquisitionBufferSize;
	//ULONG		m_captureBufferSize;
	//OVERLAPPED	m_captureOverlapped;
	//PISOCH_ATTACH_BUFFERS m_pIsochAttachBuffers;
	//ISOCH_GET_IMAGE_PARAMS m_isochGetImageParams;
	//ACQUISITION_BUFFER	m_acquisitionBuffer[6];
	//bool m_allocateMemory;
	//HWND m_hWnd;
	//ISOCH_LISTEN m_listen;
	//ISOCH_STOP m_stop;
	// structs for the various Isoch functions we use
	//ISOCH_ALLOCATE_BANDWIDTH m_bandwidth;
	//ISOCH_ALLOCATE_CHANNEL m_channel;
	//ISOCH_ALLOCATE_RESOURCES m_resource;
};

/*
 * CameraControlDialog
 *
 * This function provides a complete control dialog for 
 * all the standard controls for a 1394 camera.
 *
 * Note: this is a non-modal dialog, which means you 
 *   need to do your own message pumping
 *
 * Arguments:
 *   - hWndParent: the parent window for this instance
 *   - pCamera: pointer to the camera to be controlled
 *       this allows multiple control windows to be open for multiple cameras
 *   - bLoadDefaultView : 
 * 
 * Returns:
 *   HWND to the control window, NULL on error.  GetLastError() should work.
 *
 * Location: ControlDialog.cpp
 *
 */

HWND
CAMAPI
CameraControlDialog(
	HWND hWndParent,
	C1394Camera *pCamera,
	BOOL bLoadDefaultView
	);

extern "C" {

/*
 * CameraDebugDialog
 *
 * This function provides a dialog to dynamically change the trace level of the DLL.
 *
 * Arguments:
 *   - hWndParent: the parent window for this instance
 * 
 * Note: this is a modal dialog, which means it will block until you click "OK"
 *
 * Location: debug.c
 */

void
CAMAPI
CameraDebugDialog(
	HWND hWndParent
	);
}

/*
 * CameraControlSizeDialog
 *
 * This function spawns a modal dialog that is a graphical interface to the
 * C1394CameraControlSize class.
 *
 * Arguments:
 *   - hWndParent: the parent window for this instance
 *   - pCamera: pointer to the camera whose size you wish to control
 * 
 * Notes: 
 * - This is a modal dialog, which means it will block until you click "OK" or "Cancel"
 *
 * - It is not recommended to futz with the partial scan controls while the camera is sending
 * image data.
 *
 * Location: ControlSizeDialog.cpp
 */


long
CAMAPI
CameraControlSizeDialog(HWND hWndParent, 
						C1394Camera *pCamera);

#endif // __1394CAMERA_H__
