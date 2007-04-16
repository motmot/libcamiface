///////////////////////////////////////////////////////////////////////////////
//
// 1394CamAcq.cpp
//
// source for C1394Camera::
//  - StartImageAcquisition
//  - AcquireImage
//  - StopImageAcquisition
//
// Based on the following functions from isochapi.c originally by Iwan Ulrich:
//
//		IsochStartImageAcquisition
//		IsochAcquireImage
//		IsochStopImageAcquisition
//
//	Copyright 1/2000
// 
//	Iwan Ulrich
//	Robotics Institute
//	Carnegie Mellon University
//	Pittsburgh, PA
//
//  Copyright 3/2002 - 3/2004
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
#include <winerror.h>
#include "1394Camera.h"
#include "pch.h"

/* The Model
 * 
 * - A handful of image buffers are maintained as a circularly-linked list.
 *
 * - The buffer pointed to by m_pCurrentBuffer is the "Current Buffer" and
 *     is guaranteed to not be attached to the isochronous buffer queue.
 *
 * - The buffer pointed to by m_pLastBuffer is the last buffer in the list
 *     to have undergone the IOTCL_ATTACH_BUFFER prodecure.  Under most 
 *     circumstances, the buffer immediately following m_pLastBuffer will
 *     be m_pCurrentBuffer
 *
 * - As each buffer is "acquired", that buffer becomes the "Current Buffer"
 *     all buffers between m_pLastBuffer and the new m_pCurrentBuffer are 
 *     reattached to the isochronous buffer queue
 */

/*
 * StartImageAcquisitionEx
 *
 * Arguments:
 *   nBuffers: The number of buffers to allocate.  Minimum of 1
 *   FrameTimeout: Unimplemented, will eventually allow configuration of timeout
 *   Flags: Reserved, set to zero
 *
 * Return Values:
 *   0 : success
 *   nonzero : some error ;)
 *
 * Reads Member Vars:
 *   m_cameraInitialized
 *   m_videoFlags
 *   m_videoFormat
 *   m_videoMode
 *   m_videoFrameRate
 *   m_hDeviceAcquisition
 *   m_hDeviceCapture
 *   m_pFirstBuffer
 *   m_pCurrentBuffer
 *   m_pLastBuffer
 *
 * Mutates Member Vars:
 *   m_hDeviceAcquisition
 *   m_pFirstBuffer
 *   m_pCurrentBuffer
 *   m_pLastBuffer
 *   
 * Comments:
 *   Allocates System Memory, Allocates Isochronous Channel, Bandwidth, and "Resources"
 */

int C1394Camera::StartImageAcquisitionEx(int nBuffers, int FrameTimeout, int Flags)
{
	PACQUISITION_BUFFER				pAcqBuffer;
	BOOL							b;	
	DWORD							dwRet, dwBytesRet;
	ISOCH_LISTEN					isochListen;
	OVERLAPPED						ListenOverlapped;						
	int							i;
	int return_value = 0;

	DllTrace(DLL_TRACE_ENTER,"ENTER StartImageAcquisition (nBuffers = %d)\n",nBuffers);

	if(nBuffers < 1)
	{
		DllTrace(DLL_TRACE_ERROR,"AcquireImage: Invalid number of buffers: %d\n",nBuffers);
		return CAM_ERROR_PARAM_OUT_OF_RANGE;
	}

	if(!m_cameraInitialized)
	{
		DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: Call InitCamera() first.\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT StartImageAcquisition (%d)",CAM_ERROR_NOT_INITIALIZED);
		return CAM_ERROR_NOT_INITIALIZED;
	}

	if(m_videoFormat != 7)
	{
		if(!(m_videoFlags[m_videoFormat][m_videoMode][m_videoFrameRate]))
		{
			DllTrace(DLL_TRACE_ERROR,
					"StartImageAcquisition: Current Video Settings (%d,%d,%d) are not Supported\n",
					m_videoFormat,
					m_videoMode,
					m_videoFrameRate);
			DllTrace(DLL_TRACE_EXIT,"EXIT StartImageAcquisition (%d)\n",CAM_ERROR_INVALID_VIDEO_SETTINGS);
			return CAM_ERROR_INVALID_VIDEO_SETTINGS;
		}
	} else {
		if(!m_bxAvailableFormats[7])
		{
			DllTrace(DLL_TRACE_ERROR,
					"StartImageAcquisition: Current Video Settings (%d,%d,%d) are not Supported\n",
					m_videoFormat,
					m_videoMode,
					m_videoFrameRate);
			DllTrace(DLL_TRACE_EXIT,"EXIT StartImageAcquisition (%d)\n",CAM_ERROR_INVALID_VIDEO_SETTINGS);
			return CAM_ERROR_INVALID_VIDEO_SETTINGS;
		}
		// Fishy to have to do this, but I think it's necessary
		UpdateParameters();
	}

	if(m_hDeviceAcquisition != NULL)
	{
		DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: The Camera is already acquiring images, call StopImageAcquisition first\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT StartImageAcquisition (%d)\n",CAM_ERROR_BUSY);
		return CAM_ERROR_BUSY;
	}

	if(m_hDeviceCapture != NULL)
	{
		DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: The Camera is currently capturing images, call StopImageCapture first\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT StartImageAcquisition (%d)\n",CAM_ERROR_BUSY);
		return CAM_ERROR_BUSY;
	}

	if(!InitResources())
	{
		DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: InitResources Failed\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT StartImageAcquisition (%d)\n",CAM_ERROR_INSUFFICIENT_RESOURCES);
		return CAM_ERROR_INSUFFICIENT_RESOURCES;
	}


	///////////////////////////////////////////
	// allocate and set up the frame buffers //
	///////////////////////////////////////////

	DllTrace(DLL_TRACE_CHECK,"StartImageAcquisition: Initializing Buffers...\n");

	for(i=0; i<nBuffers; i++)
	{
		// allocate the buffer header (stores data about a buffer)
		pAcqBuffer = (PACQUISITION_BUFFER) GlobalAlloc(LPTR,sizeof(ACQUISITION_BUFFER));
		if(!pAcqBuffer)
		{
			DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: Error Allocating AcqBuffer %d\n",i);
			return_value = ERROR_OUTOFMEMORY;
			goto Exit_1394StartImageAcquisition;
		}

		// add it to our list of buffers
		if(i == 0)
		{
			m_pFirstBuffer = m_pLastBuffer = m_pCurrentBuffer = pAcqBuffer;
		} else {
			m_pLastBuffer->pNextBuffer = pAcqBuffer;
			m_pLastBuffer = pAcqBuffer;
		}

		// allocate the actual frame buffer
		// the buffer passed to ATTACH_BUFFER must be aligned on a page boundary
		// thus, allocate an extra 4K and generate a pointer to the first page boundary
		pAcqBuffer->pDataBuf = (unsigned char *)GlobalAlloc(LPTR,m_maxBufferSize + 4096);
		if(!pAcqBuffer->pDataBuf)
		{
			DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: Error Allocating Data Buffer %d\n",i);
			return_value = ERROR_OUTOFMEMORY;
			goto Exit_1394StartImageAcquisition;
		}

		// point pFrameStart at the first page boundary
		pAcqBuffer->pFrameStart = pAcqBuffer->pDataBuf + (4096 - (((int)(pAcqBuffer->pDataBuf)) & 0xfff));

		// set the index (mostly for debugging purposes)
		pAcqBuffer->index = i;

		// give the overlapped structure an event
		pAcqBuffer->overLapped.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
		if(pAcqBuffer->overLapped.hEvent == INVALID_HANDLE_VALUE)
		{
			DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: Error Creating Overlapped Event %d\n",i);
			return_value = GetLastError();
			goto Exit_1394StartImageAcquisition;
		}

		// allocate the AttachBuffers struct
		pAcqBuffer->pIsochAttachBuffers = (PISOCH_ATTACH_BUFFERS) GlobalAlloc(LPTR,sizeof(ISOCH_ATTACH_BUFFERS));
		if(!pAcqBuffer->pIsochAttachBuffers)
		{
			DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: Error Allocating Isoch Attach Buffers %d\n",i);
			return_value = ERROR_OUTOFMEMORY;
			goto Exit_1394StartImageAcquisition;
		}

		// fill in the AttachBuffers struct
		pAcqBuffer->pIsochAttachBuffers->hResource = m_hResource;
		pAcqBuffer->pIsochAttachBuffers->nNumberOfDescriptors = 1;
		pAcqBuffer->pIsochAttachBuffers->ulBufferSize = m_maxBufferSize;
		pAcqBuffer->pIsochAttachBuffers->R3_IsochDescriptor[0].fulFlags = DESCRIPTOR_SYNCH_ON_SY;
		pAcqBuffer->pIsochAttachBuffers->R3_IsochDescriptor[0].ulLength = m_maxBufferSize;
		pAcqBuffer->pIsochAttachBuffers->R3_IsochDescriptor[0].nMaxBytesPerFrame = m_maxBytes;
		pAcqBuffer->pIsochAttachBuffers->R3_IsochDescriptor[0].ulSynch = 1;
		pAcqBuffer->pIsochAttachBuffers->R3_IsochDescriptor[0].ulTag = 0;
		pAcqBuffer->pIsochAttachBuffers->R3_IsochDescriptor[0].CycleTime.CL_CycleOffset = 0;
		pAcqBuffer->pIsochAttachBuffers->R3_IsochDescriptor[0].CycleTime.CL_CycleCount = 0;
		pAcqBuffer->pIsochAttachBuffers->R3_IsochDescriptor[0].CycleTime.CL_SecondCount = 0;
		pAcqBuffer->pIsochAttachBuffers->R3_IsochDescriptor[0].bUseCallback = TRUE;
		pAcqBuffer->pIsochAttachBuffers->R3_IsochDescriptor[0].bAutoDetach = TRUE;

		DllTrace(DLL_TRACE_VERBOSE,"StartImageAcquisition: Allocated buffer %d\n",i);
	}

	// all done making buffers
	// after here, we reference the buffers *exclusively* in thier linked-list form
	// point our last buffer around to the first
	m_pLastBuffer->pNextBuffer = m_pFirstBuffer;
	m_pCurrentBuffer = m_pLastBuffer;

	// open our long term device handle
	m_hDeviceAcquisition = OpenDevice(m_pName, TRUE);

	if(m_hDeviceAcquisition == INVALID_HANDLE_VALUE)
	{
		DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: error opening device (%s)\n",m_pName);
		m_hDeviceAcquisition = NULL;
		return_value = GetLastError();
		goto Exit_1394StartImageAcquisition;
	}
	
	// attach all but last buffer, set that buffer to be the "Current Buffer" to maintain
	// out invariants

	while(m_pLastBuffer->pNextBuffer != m_pCurrentBuffer)
	{
		m_pLastBuffer = m_pLastBuffer->pNextBuffer;

		DllTrace(DLL_TRACE_VERBOSE,"StartImageAcquisition: Attaching buffer: index:%d, size:%d, R3qpp:%d, R3length: %d, FrameStart:%08x\n",
			m_pLastBuffer->index,
			m_pLastBuffer->pIsochAttachBuffers->ulBufferSize,
			m_pLastBuffer->pIsochAttachBuffers->R3_IsochDescriptor->nMaxBytesPerFrame,
			m_pLastBuffer->pIsochAttachBuffers->R3_IsochDescriptor->ulLength,
			m_pLastBuffer->pFrameStart);

		b = DeviceIoControl(
				m_hDeviceAcquisition,
				IOCTL_ATTACH_BUFFER,
				m_pLastBuffer->pIsochAttachBuffers,
				sizeof(ISOCH_ATTACH_BUFFERS),
				m_pLastBuffer->pFrameStart,
				m_pLastBuffer->pIsochAttachBuffers->ulBufferSize,
				&dwBytesRet,
				&(m_pLastBuffer->overLapped)
				);
	
		dwRet = GetLastError();
		if ((dwRet != ERROR_IO_PENDING) && (dwRet != ERROR_SUCCESS)) 
		{
			// bad things have happened
			// what exactly needs done here isn't very well defined, so let's do nothing
			// for now
			DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: Error %08x while Attaching Buffers\n",dwRet);
			return -1;
		}
	}

	// listen							
	if(nBuffers > 1)
	{
		ListenOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
							
		isochListen.hResource = m_hResource;
		isochListen.fulFlags = 0;
		isochListen.StartTime.CL_SecondCount = 0;
		isochListen.StartTime.CL_CycleCount = 0;
		isochListen.StartTime.CL_CycleOffset = 0;
							
		b = DeviceIoControl( m_hDeviceAcquisition,
							IOCTL_ISOCH_LISTEN,
							&isochListen,
							sizeof(ISOCH_LISTEN),
							NULL,
							0,
							&dwBytesRet,
							&ListenOverlapped
							);

		if(!b)
		{
			dwRet = GetLastError();
			if(dwRet != ERROR_IO_PENDING)
			{
				DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: Error %08x on IOCTL_ISOCH_LISTEN\n");
				return -1;
			} else {
				GetOverlappedResult(m_hDeviceAcquisition,&ListenOverlapped,&dwBytesRet,TRUE);
			}
		}
					
		StartVideoStream();

		CloseHandle(ListenOverlapped.hEvent);
	} else {
		m_pLastBuffer = NULL;
	}

	DllTrace(DLL_TRACE_EXIT,"EXIT StartImageAcquisition (0)\n");
	return 0;

/////////////////////////////////////
// Exit_1394StartImageAcquisition: //
/////////////////////////////////////

Exit_1394StartImageAcquisition:

    // we go here if something breaks and we need to clean up anything we've allocated thus far
	DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: Starting Error Cleanup\n");
	m_pCurrentBuffer = pAcqBuffer = m_pFirstBuffer;
	
	while(m_pCurrentBuffer)
	{
		DllTrace(DLL_TRACE_VERBOSE,"StartImageAcquisition: Removing buffer %d\n",m_pFirstBuffer->index);

		if(pAcqBuffer->overLapped.hEvent)
			CloseHandle(pAcqBuffer->overLapped.hEvent);
		if(pAcqBuffer->pDataBuf)
			GlobalFree(pAcqBuffer->pDataBuf);
		if(pAcqBuffer->pIsochAttachBuffers)
			GlobalFree(pAcqBuffer->pIsochAttachBuffers);

		// advance CurrentBuffer
		m_pCurrentBuffer = pAcqBuffer->pNextBuffer;
		GlobalFree(pAcqBuffer);
		pAcqBuffer = m_pCurrentBuffer;

		// check for when we loop around
		if(m_pCurrentBuffer == m_pFirstBuffer)
			m_pCurrentBuffer = NULL;
	}

	m_pCurrentBuffer = m_pFirstBuffer = m_pLastBuffer = NULL;

	StopVideoStream();
	FreeResources();

	DllTrace(DLL_TRACE_EXIT,"EXIT StartImageAcquisition (%d)\n",return_value);
	return return_value;
}

/*
 * StartImageAcquisition
 *
 * Wraps StartImageAcquisitionEx for backwards compatiblility
 */
int C1394Camera::StartImageAcquisition()
{
	return StartImageAcquisitionEx(6,10,0);
}

/*
 * AcquireImage
 *
 * Simply Calls AcquireImageEx(TRUE,NULL)
 */

int C1394Camera::AcquireImage()
{
	return AcquireImageEx(TRUE,NULL);
}

/*
 * AcquireImageEx
 *
 * Arguments:
 *   DropStaleFrames - whether to skip stale frames in the list
 *   lpnDroppedFrames - where to put the  number of dropped frames
 *
 * Return Values:
 *   0 : success
 *   nonzero : some error ;)
 *
 * Reads Member Vars:
 *   m_hDeviceAcquisition
 *   m_pFirstBuffer
 *   m_pCurrentBuffer
 *   m_pLastBuffer
 *
 * Mutates Member Vars:
 *   m_pCurrentBuffer
 *   m_pLastBuffer
 *   m_pData
 *
 * Comments:
 *   Blocks until at least the next buffer in the isochronous queue is ready.
 *
 *   if(DropStaleFrames)
 *     Skips forward to the most recent buffer that is ready, providing the most
 *     recent image data to the user program.
 *     if(lpnDroppedFrames)
 *       returns the number of dropped frames in lpnDroppedFrames
 *
 *   Reattaches all buffers that have fallen off up to, but not including the
 *     buffer described by m_pCurrentBuffer, thus maintaining all invaraints
 *   
 */


int C1394Camera::AcquireImageEx(BOOL DropStaleFrames, int *lpnDroppedFrames)
{
	DWORD							dwRet, dwBytesRet;
	ISOCH_LISTEN					isochListen;
	OVERLAPPED						ListenOverlapped;						
	BOOL							b, ready;
	int	dropped = 0;
	PACQUISITION_BUFFER pBuf;
	int n=0;

	DllTrace(DLL_TRACE_ENTER,"ENTER AcquireImage\n");

	if(!m_hDeviceAcquisition)
	{
		DllTrace(DLL_TRACE_ERROR,"AcquireImage: Not Acquiring Images: Call StartImageAcquisition First");
		DllTrace(DLL_TRACE_EXIT,"EXIT AcquireImage (%d)\n",CAM_ERROR_NOT_INITIALIZED);
		return CAM_ERROR_NOT_INITIALIZED;
	}

	pBuf = m_pCurrentBuffer;
	do
	{	
		// check the next buffer to see what's up
		if(pBuf->pNextBuffer != m_pCurrentBuffer)
		{
			if(!(ready = GetOverlappedResult(m_hDeviceAcquisition, &pBuf->pNextBuffer->overLapped, &dwBytesRet, FALSE)))
			{
				dwRet = GetLastError();
				if(dwRet == ERROR_SEM_TIMEOUT)
				{
					DllTrace(DLL_TRACE_WARNING,"AcquireImage: Frame Timeout: index = %d\n",pBuf->pNextBuffer->index);
					ready = TRUE;
				} else if(dwRet != ERROR_IO_INCOMPLETE) {
					DllTrace(DLL_TRACE_ERROR,"AcquireImage: Error %08x while Getting Overlapped Result (Non-Block)\n",dwRet);
					DllTrace(DLL_TRACE_EXIT,"EXIT AcquireImage (%d)\n",dwRet);
					return dwRet;
				}
			}

			if(ready)
			{
				DllTrace(DLL_TRACE_VERBOSE,"AcquireImage: Frame %d is ready\n",pBuf->pNextBuffer->index);
				n++;
				dwRet = 0;
			}
		} else {
			// we've hit the end of the list, attach this one
			// and block-wait
			ready = TRUE;
		}

		if(ready || pBuf == m_pCurrentBuffer)
		{
			// reattach the current buffer

			// reset the overlapped event
			ResetEvent(pBuf->overLapped.hEvent);

			DllTrace(DLL_TRACE_VERBOSE,"AcquireImage: Attaching buffer: index:%d, size:%d, R3qpp:%d, R3length: %d, FrameStart:%08x\n",
				pBuf->index,
				pBuf->pIsochAttachBuffers->ulBufferSize,
				pBuf->pIsochAttachBuffers->R3_IsochDescriptor->nMaxBytesPerFrame,
				pBuf->pIsochAttachBuffers->R3_IsochDescriptor->ulLength,
				pBuf->pFrameStart);

			b = DeviceIoControl(
					m_hDeviceAcquisition,
					IOCTL_ATTACH_BUFFER,
					pBuf->pIsochAttachBuffers,
					sizeof(ISOCH_ATTACH_BUFFERS),
					pBuf->pFrameStart,
					m_maxBufferSize,
					&dwBytesRet,
					&pBuf->overLapped
					);

			dwRet = GetLastError();
			if ((dwRet != ERROR_IO_PENDING) && (dwRet != ERROR_SUCCESS)) 
			{
				DllTrace(DLL_TRACE_ERROR,"AcquireImage: Error %08x while reattaching Buffer %d\n",dwRet,pBuf->index);
				DllTrace(DLL_TRACE_EXIT,"EXIT AcquireImage (%d)\n",dwRet);
				return dwRet;
			}

			if(m_pLastBuffer == NULL)
			{
				// we still have to listen to the stream, usually b/c there's only one frame
				ListenOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
									
				isochListen.hResource = m_hResource;
				isochListen.fulFlags = 0;
				isochListen.StartTime.CL_SecondCount = 0;
				isochListen.StartTime.CL_CycleCount = 0;
				isochListen.StartTime.CL_CycleOffset = 0;

				b = DeviceIoControl( m_hDeviceAcquisition,
									IOCTL_ISOCH_LISTEN,
									&isochListen,
									sizeof(ISOCH_LISTEN),
									NULL,
									0,
									&dwBytesRet,
									&ListenOverlapped
									);

				if(!b)
				{
					dwRet = GetLastError();
					if(dwRet != ERROR_IO_PENDING)
					{
						DllTrace(DLL_TRACE_ERROR,"StartImageAcquisition: Error %08x on IOCTL_ISOCH_LISTEN\n");
						return -1;
					} else {
						GetOverlappedResult(m_hDeviceAcquisition,&ListenOverlapped,&dwBytesRet,TRUE);
					}
				}
				
				StartVideoStream();
				CloseHandle(ListenOverlapped.hEvent);
			}

			m_pLastBuffer = pBuf;
			pBuf = pBuf->pNextBuffer;
		}
	} while(ready && pBuf != m_pCurrentBuffer && (DropStaleFrames || n == 0));

	// now, we handle the case where either there were no frames ready or they were all stale

	if(n == 0 || (DropStaleFrames && pBuf == m_pCurrentBuffer))
	{
		DllTrace(DLL_TRACE_VERBOSE,"AcquireImage: Block-Waiting on Frame %d\n",pBuf->index);
		// block-wait on the overlapped result from the next buffer
		if(!GetOverlappedResult(m_hDeviceAcquisition, &pBuf->overLapped, &dwBytesRet, TRUE))
		{
			dwRet = GetLastError();
			if(dwRet != ERROR_SEM_TIMEOUT)
			{
				DllTrace(DLL_TRACE_ERROR,"AcquireImage: Error %08x while Getting Overlapped Result (Block)\n",dwRet);
				DllTrace(DLL_TRACE_EXIT,"EXIT AcquireImage (%d)\n",dwRet);
				return dwRet;
			}
			DllTrace(DLL_TRACE_WARNING,"AcquireImage: Frame Timeout: index = %d\n",pBuf->index);
		}
		DllTrace(DLL_TRACE_VERBOSE,"AcquireImage: Frame %d is ready\n",pBuf->index);
		n++;
	}

	m_pCurrentBuffer = pBuf;

	// update the public data pointer
	m_pData = m_pCurrentBuffer->pFrameStart;
	DllTrace(DLL_TRACE_VERBOSE,"AcquireImage: Current Buffer is now %d, data at %08x\n",m_pCurrentBuffer->index, m_pData);
	if(dwRet == ERROR_SEM_TIMEOUT)
		dwRet = CAM_ERROR_FRAME_TIMEOUT;
	else 
		dwRet = 0;
	DllTrace(DLL_TRACE_EXIT,"EXIT AcquireImage (%d)\n",dwRet);

	if(lpnDroppedFrames)
		*lpnDroppedFrames = n-1;

	return dwRet;
}

/*
 * StopImageAcquisition
 *
 * Arguments:
 *   <none>
 *
 * Return Values:
 *   0 : success
 *   nonzero : some error ;)
 *
 * Reads Member Vars:
 *   m_hDeviceAcquisition
 *   m_pFirstBuffer
 *   m_pCurrentBuffer
 *   m_pLastBuffer
 *
 * Mutates Member Vars:
 *   m_hDeviceAcquisition
 *   m_pFirstBuffer
 *   m_pCurrentBuffer
 *   m_pLastBuffer
 *
 * Comments:
 *   Blocks until all buffers are empty (upper limit is ~10 seconds for timeout)
 *
 *   Frees all resources (memory, events, handles, etc.) allocated by
 *     StartImageAcquisition
 *   
 */

int C1394Camera::StopImageAcquisition()
{
	DWORD							dwBytesRet;
	ISOCH_STOP						isochStop;
	OVERLAPPED						StopOverlapped;
	PACQUISITION_BUFFER				pAcqTemp;
	BOOL							b;
	
	DllTrace(DLL_TRACE_ENTER,"ENTER StopImageAcquisition\n");

	if(!m_hDeviceAcquisition)
	{
		DllTrace(DLL_TRACE_ERROR,"StopImageAcquisition: Not Acquiring Images: Call StartImageAcquisition First\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT StopImageAcquisition (%d)\n",CAM_ERROR_NOT_INITIALIZED);
		return CAM_ERROR_NOT_INITIALIZED;
	}

	// wait for buffers to fill up
	// KLUDGE!!! this should really be: stop streaming, stop listening, detach buffers.
	// This needs to be done right!

	do
	{
		DllTrace(DLL_TRACE_VERBOSE,"StopImageAcquisition: Waiting on buffer %d\n",m_pCurrentBuffer->pNextBuffer->index);
		GetOverlappedResult(m_hDeviceAcquisition, &m_pCurrentBuffer->pNextBuffer->overLapped, &dwBytesRet, TRUE);
		m_pCurrentBuffer = m_pCurrentBuffer->pNextBuffer;
	}
	while (m_pCurrentBuffer != m_pLastBuffer);

	DllTrace(DLL_TRACE_CHECK,"StopImageAcquisition: Done waiting for buffers to clear\n");
					
	// stop listening
	StopOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	isochStop.hResource = m_hResource;
	isochStop.fulFlags = 0;
			
	b = DeviceIoControl(m_hDeviceAcquisition,
				IOCTL_ISOCH_STOP,
				&isochStop,
				sizeof(ISOCH_STOP),
				NULL,
				0,
				&dwBytesRet,
				&StopOverlapped
				);
	
	if(!GetOverlappedResult(m_hDeviceAcquisition,&StopOverlapped,&dwBytesRet,TRUE))
		// some error, but we really can't do anything about it now
		DllTrace(DLL_TRACE_ERROR,"StopImageAcquisition: Error %08x on Overlapped Isoch Stop",GetLastError());
	
	CloseHandle(StopOverlapped.hEvent);
		
	// free up all resources
	CloseHandle(m_hDeviceAcquisition);
	m_hDeviceAcquisition = NULL;

	DllTrace(DLL_TRACE_CHECK,"StopImageAcquisition: Cleaning up Buffers\n");

	m_pLastBuffer = m_pFirstBuffer;
	while(m_pLastBuffer)
	{
		DllTrace(DLL_TRACE_VERBOSE,"StopImageAcquisition: Freeing Buffer %d",m_pLastBuffer->index);

		// free the event
		CloseHandle(m_pLastBuffer->overLapped.hEvent);
		// free the frame buffer
		GlobalFree(m_pLastBuffer->pDataBuf);
		// free the Attach Buffers struct
		GlobalFree(m_pLastBuffer->pIsochAttachBuffers);

		pAcqTemp = m_pLastBuffer->pNextBuffer;

		// free the actual buffer
		GlobalFree(m_pLastBuffer);

		m_pLastBuffer = (pAcqTemp == m_pFirstBuffer ? NULL : pAcqTemp);
	}

	m_pFirstBuffer = m_pLastBuffer = m_pCurrentBuffer = NULL;

	StopVideoStream();
	FreeResources();
	DllTrace(DLL_TRACE_EXIT,"EXIT StopImageAcquisition (0)\n");
	return 0;
}

/*
 * GetFrameEvent
 *
 * Public
 *
 * No Arguments
 *
 * Returns:
 *  - Asynchronous Event Handle for the next pending frame on the list
 *  - INVALID_HANDLE_VALUE if there is no pending frame (not Acquiring images)
 */

 HANDLE C1394Camera::GetFrameEvent()
 {
	 if(m_pCurrentBuffer && m_pCurrentBuffer->pNextBuffer)
		 return m_pCurrentBuffer->pNextBuffer->overLapped.hEvent;
	 else
		 return INVALID_HANDLE_VALUE;
 }

