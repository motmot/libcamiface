/*++

Copyright (c) 1997-1998 Microsoft Corporation

Module Name:

    isochapi.c

Abstract


Author:

    Peter Binder (pbinder) 9-15-97
	Iwan Ulrich  (iu)      1/2000
	Christopher Baker (cbaker)(8/2001)

Revision History:
Date     Who       What
-------- --------- ------------------------------------------------------------
9-15-97  pbinder   birth
4-08-98  pbinder   taken from win1394 
1-xx-00  iu        added several functions to the end (see comment near line 1200)
8-20-01  cbaker    fixed some busy-waiting in IsochAcquireImage that was munching CPU time
11-21-01 cbaker    moved Iwan's functions up into the C1394Camera class to eliminate
                     the need for the global buffers and allow multiple cameras to be accessed
					 within the same application at the same time
08-04-02 cbaker    cut out unused functions (attach/detach buffers and loopback crap

--*/

/*
 * The Original Isoch Attach/Detach Routines are now dangerous to use because of changes
 * to the innards of the driver (ex: MDLs are no longer freed as a part of IsochCleanup)
 * Thus, they have been removed.
 */


#define _ISOCHAPI_C
#include "pch.h"
#undef _ISOCHAPI_C

ULONG
CAMAPI
IsochAllocateBandwidth(
    HWND                        hWnd,
    PSTR                        szDeviceName,
    PISOCH_ALLOCATE_BANDWIDTH   isochAllocateBandwidth
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    TRACE(TL_TRACE, (hWnd, "Enter IsochAllocateBandwidth\r\n"));

    TRACE(TL_TRACE, (hWnd, "nMaxBytesPerFrameRequested = 0x%x\r\n", isochAllocateBandwidth->nMaxBytesPerFrameRequested));
    TRACE(TL_TRACE, (hWnd, "fulSpeed = 0x%x\r\n", isochAllocateBandwidth->fulSpeed));

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_ISOCH_ALLOCATE_BANDWIDTH,
                                 isochAllocateBandwidth,
                                 sizeof(ISOCH_ALLOCATE_BANDWIDTH),
                                 isochAllocateBandwidth,
                                 sizeof(ISOCH_ALLOCATE_BANDWIDTH),
                                 &dwBytesRet,
                                 NULL
                                 );

        if (dwRet) {

            dwRet = ERROR_SUCCESS;

            TRACE(TL_TRACE, (hWnd, "hBandwidth = 0x%x\r\n", isochAllocateBandwidth->hBandwidth));
            TRACE(TL_TRACE, (hWnd, "BytesPerFrameAvailable = 0x%x\r\n", isochAllocateBandwidth->BytesPerFrameAvailable));
            TRACE(TL_TRACE, (hWnd, "Speed Selected = 0x%x\r\n", isochAllocateBandwidth->SpeedSelected));
        }
        else {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }

        // free up resources
        CloseHandle(hDevice);
    }

    TRACE(TL_TRACE, (hWnd, "Exit IsochAllocateBandwidth = %d\r\n", dwRet));
    return(dwRet);
} // IsochAllocateBandwidth

ULONG
CAMAPI
IsochAllocateChannel(
    HWND                        hWnd,
    PSTR                        szDeviceName,
    PISOCH_ALLOCATE_CHANNEL     isochAllocateChannel
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    TRACE(TL_TRACE, (hWnd, "Enter IsochAllocateChannel\r\n"));

    TRACE(TL_TRACE, (hWnd, "nRequestedChannel = 0x%x\r\n", isochAllocateChannel->nRequestedChannel));

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_ISOCH_ALLOCATE_CHANNEL,
                                 isochAllocateChannel,
                                 sizeof(ISOCH_ALLOCATE_CHANNEL),
                                 isochAllocateChannel,
                                 sizeof(ISOCH_ALLOCATE_CHANNEL),
                                 &dwBytesRet,
                                 NULL
                                 );

        if (dwRet) {

            dwRet = ERROR_SUCCESS;

            TRACE(TL_TRACE, (hWnd, "Channel = 0x%x\r\n", isochAllocateChannel->Channel));
            TRACE(TL_TRACE, (hWnd, "ChannelsAvailable.LowPart = 0x%x\r\n", isochAllocateChannel->ChannelsAvailable.LowPart));
            TRACE(TL_TRACE, (hWnd, "ChannelsAvailable.HighPart = 0x%x\r\n", isochAllocateChannel->ChannelsAvailable.HighPart));
        }
        else {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }

        // free up resources
        CloseHandle(hDevice);
    }

    TRACE(TL_TRACE, (hWnd, "Exit IsochAllocateChannel = %d\r\n", dwRet));
    return(dwRet);
} // IsochAllocateChannel

ULONG
CAMAPI
IsochAllocateResources(
    HWND                        hWnd,
    PSTR                        szDeviceName,
    PISOCH_ALLOCATE_RESOURCES   isochAllocateResources
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;
	
    TRACE(TL_TRACE, (hWnd, "Enter IsochAllocateResources\r\n"));

    TRACE(TL_TRACE, (hWnd, "fulSpeed = 0x%x\r\n", isochAllocateResources->fulSpeed));
    TRACE(TL_TRACE, (hWnd, "fulFlags = 0x%x\r\n", isochAllocateResources->fulFlags));
    TRACE(TL_TRACE, (hWnd, "nChannel = 0x%x\r\n", isochAllocateResources->nChannel));
    TRACE(TL_TRACE, (hWnd, "nMaxBytesPerFrame = 0x%x\r\n", isochAllocateResources->nMaxBytesPerFrame));
    TRACE(TL_TRACE, (hWnd, "nNumberOfBuffers = 0x%x\r\n", isochAllocateResources->nNumberOfBuffers));
    TRACE(TL_TRACE, (hWnd, "nMaxBufferSize = 0x%x\r\n", isochAllocateResources->nMaxBufferSize));
    TRACE(TL_TRACE, (hWnd, "nQuadletsToStrip = 0x%x\r\n", isochAllocateResources->nQuadletsToStrip));

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_ISOCH_ALLOCATE_RESOURCES,
                                 isochAllocateResources,
                                 sizeof(ISOCH_ALLOCATE_RESOURCES),
                                 isochAllocateResources,
                                 sizeof(ISOCH_ALLOCATE_RESOURCES),
                                 &dwBytesRet,
                                 NULL
                                 );

        if (dwRet) {

            dwRet = ERROR_SUCCESS;

            TRACE(TL_TRACE, (hWnd, "hResource = 0x%x\r\n", isochAllocateResources->hResource));
        }
        else {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }

        // free up resources
        CloseHandle(hDevice);
    }

    TRACE(TL_TRACE, (hWnd, "Exit IsochAllocateResources = %d\r\n", dwRet));
    return(dwRet);
} // IsochAllocateResources


ULONG
CAMAPI
IsochFreeBandwidth(
    HWND    hWnd,
    PSTR    szDeviceName,
    HANDLE  hBandwidth
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    TRACE(TL_TRACE, (hWnd, "Enter IsochFreeBandwidth\r\n"));

    TRACE(TL_TRACE, (hWnd, "hBandwidth = 0x%x\r\n", hBandwidth));

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_ISOCH_FREE_BANDWIDTH,
                                 &hBandwidth,
                                 sizeof(HANDLE),
                                 NULL,
                                 0,
                                 &dwBytesRet,
                                 NULL
                                 );

        if (!dwRet) {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }
        else {

            dwRet = ERROR_SUCCESS;
        }

        // free up resources
        CloseHandle(hDevice);
    }

    TRACE(TL_TRACE, (hWnd, "Exit IsochFreeBandwidth = %d\r\n", dwRet));
    return(dwRet);
} // IsochFreeBandwidth

ULONG
CAMAPI
IsochFreeChannel(
    HWND    hWnd,
    PSTR    szDeviceName,
    ULONG   nChannel
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    TRACE(TL_TRACE, (hWnd, "Enter IsochFreeChannel\r\n"));

    TRACE(TL_TRACE, (hWnd, "nChannel = 0x%x\r\n", nChannel));

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_ISOCH_FREE_CHANNEL,
                                 &nChannel,
                                 sizeof(ULONG),
                                 NULL,
                                 0,
                                 &dwBytesRet,
                                 NULL
                                 );

        if (!dwRet) {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }
        else {

            dwRet = ERROR_SUCCESS;
        }

        // free up resources
        CloseHandle(hDevice);
    }

    TRACE(TL_TRACE, (hWnd, "Exit IsochFreeChannel = %d\r\n", dwRet));
    return(dwRet);
} // IsochFreeChannel

ULONG
CAMAPI
IsochFreeResources(
    HWND    hWnd,
    PSTR    szDeviceName,
    HANDLE  hResource
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    TRACE(TL_TRACE, (hWnd, "Enter IsochFreeResources\r\n"));

    TRACE(TL_TRACE, (hWnd, "hResource = 0x%x\r\n", hResource));

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_ISOCH_FREE_RESOURCES,
                                 &hResource,
                                 sizeof(HANDLE),
                                 NULL,
                                 0,
                                 &dwBytesRet,
                                 NULL
                                 );

        if (!dwRet) {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }
        else {

            dwRet = ERROR_SUCCESS;
        }

        // free up resources
        CloseHandle(hDevice);
    }

    TRACE(TL_TRACE, (hWnd, "Exit IsochFreeResources = %d\r\n", dwRet));
    return(dwRet);
} // IsochFreeResources

ULONG
CAMAPI
IsochListen(
    HWND            hWnd,
    PSTR            szDeviceName,
    PISOCH_LISTEN   isochListen
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    TRACE(TL_TRACE, (hWnd, "Enter IsochListen\r\n"));

    TRACE(TL_TRACE, (hWnd, "hResource = 0x%x\r\n", isochListen->hResource));
    TRACE(TL_TRACE, (hWnd, "fulFlags = 0x%x\r\n", isochListen->fulFlags));
    TRACE(TL_TRACE, (hWnd, "StartTime.CL_CycleOffset = 0x%x\r\n", isochListen->StartTime.CL_CycleOffset));
    TRACE(TL_TRACE, (hWnd, "StartTime.CL_CycleCount = 0x%x\r\n", isochListen->StartTime.CL_CycleCount));
    TRACE(TL_TRACE, (hWnd, "StartTime.CL_SecondCount = 0x%x\r\n", isochListen->StartTime.CL_SecondCount));

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_ISOCH_LISTEN,
                                 isochListen,
                                 sizeof(ISOCH_LISTEN),
                                 NULL,
                                 0,
                                 &dwBytesRet,
                                 NULL
                                 );

        if (!dwRet) {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }
        else {

            dwRet = ERROR_SUCCESS;
        }

        // free up resources
        CloseHandle(hDevice);
    }

    TRACE(TL_TRACE, (hWnd, "Exit IsochListen = %d\r\n", dwRet));
    return(dwRet);
} // IsochListen

ULONG
CAMAPI
IsochQueryCurrentCycleTime(
    HWND            hWnd,
    PSTR            szDeviceName,
    PCYCLE_TIME     CycleTime
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    TRACE(TL_TRACE, (hWnd, "Enter IsochQueryCurrentCycleTime\r\n"));

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_ISOCH_QUERY_CURRENT_CYCLE_TIME,
                                 NULL,
                                 0,
                                 CycleTime,
                                 sizeof(CYCLE_TIME),
                                 &dwBytesRet,
                                 NULL
                                 );

        if (dwRet) {

            dwRet = ERROR_SUCCESS;

            TRACE(TL_TRACE, (hWnd, "CycleTime.CL_CycleOffset = 0x%x\r\n", CycleTime->CL_CycleOffset));
            TRACE(TL_TRACE, (hWnd, "CycleTime.CL_CycleCount = 0x%x\r\n", CycleTime->CL_CycleCount));
            TRACE(TL_TRACE, (hWnd, "CycleTime.CL_SecondCount = 0x%x\r\n", CycleTime->CL_SecondCount));
        }
        else {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }

        // free up resources
        CloseHandle(hDevice);
    }

    TRACE(TL_TRACE, (hWnd, "Exit IsochQueryCurrentCycleTime = %d\r\n", dwRet));
    return(dwRet);
} // IsochQueryCurrentCycleTime

ULONG
CAMAPI
IsochQueryResources(
    HWND                        hWnd,
    PSTR                        szDeviceName,
    PISOCH_QUERY_RESOURCES      isochQueryResources
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    TRACE(TL_TRACE, (hWnd, "Enter IsochQueryResources\r\n"));

    TRACE(TL_TRACE, (hWnd, "fulSpeed = 0x%x\r\n", isochQueryResources->fulSpeed));

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_ISOCH_QUERY_RESOURCES,
                                 isochQueryResources,
                                 sizeof(ISOCH_QUERY_RESOURCES),
                                 isochQueryResources,
                                 sizeof(ISOCH_QUERY_RESOURCES),
                                 &dwBytesRet,
                                 NULL
                                 );

        if (dwRet) {

            dwRet = ERROR_SUCCESS;

            TRACE(TL_TRACE, (hWnd, "BytesPerFrameAvailable = 0x%x\r\n", isochQueryResources->BytesPerFrameAvailable));
            TRACE(TL_TRACE, (hWnd, "ChannelsAvailable.LowPart = 0x%x\r\n", isochQueryResources->ChannelsAvailable.LowPart));
            TRACE(TL_TRACE, (hWnd, "ChannelsAvailable.HighPart = 0x%x\r\n", isochQueryResources->ChannelsAvailable.HighPart));
        }
        else {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }

        // free up resources
        CloseHandle(hDevice);
    }

    TRACE(TL_TRACE, (hWnd, "Exit IsochQueryResources = %d\r\n", dwRet));
    return(dwRet);
} // IsochQueryResources

ULONG
CAMAPI
IsochSetChannelBandwidth(
    HWND                            hWnd,
    PSTR                            szDeviceName,
    PISOCH_SET_CHANNEL_BANDWIDTH    isochSetChannelBandwidth
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    TRACE(TL_TRACE, (hWnd, "Enter IsochSetChannelBandwidth\r\n"));

    TRACE(TL_TRACE, (hWnd, "hBandwidth = 0x%x\r\n", isochSetChannelBandwidth->hBandwidth));
    TRACE(TL_TRACE, (hWnd, "nMaxBytesPerFrame = 0x%x\r\n", isochSetChannelBandwidth->nMaxBytesPerFrame));

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_ISOCH_SET_CHANNEL_BANDWIDTH,
                                 isochSetChannelBandwidth,
                                 sizeof(ISOCH_SET_CHANNEL_BANDWIDTH),
                                 NULL,
                                 0,
                                 &dwBytesRet,
                                 NULL
                                 );

        if (!dwRet) {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }
        else {

            dwRet = ERROR_SUCCESS;
        }

        // free up resources
        CloseHandle(hDevice);
    }

    TRACE(TL_TRACE, (hWnd, "Exit IsochSetChannelBandwidth = %d\r\n", dwRet));
    return(dwRet);
} // IsochSetChannelBandwidth

ULONG
CAMAPI
IsochStop(
    HWND            hWnd,
    PSTR            szDeviceName,
    PISOCH_STOP     isochStop
    )
{

    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    TRACE(TL_TRACE, (hWnd, "Enter IsochStop\r\n"));

    TRACE(TL_TRACE, (hWnd, "hResource = 0x%x\r\n", isochStop->hResource));
    TRACE(TL_TRACE, (hWnd, "fulFlags = 0x%x\r\n", isochStop->fulFlags));

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_ISOCH_STOP,
                                 isochStop,
                                 sizeof(ISOCH_STOP),
                                 NULL,
                                 0,
                                 &dwBytesRet,
                                 NULL
                                 );

        if (!dwRet) {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }
        else {

            dwRet = ERROR_SUCCESS;
        }

        // free up resources
        CloseHandle(hDevice);
    }

    TRACE(TL_TRACE, (hWnd, "Exit IsochStop = %d\r\n", dwRet));
    return(dwRet);
} // IsochStop

ULONG
CAMAPI
IsochTalk(
    HWND            hWnd,
    PSTR            szDeviceName,
    PISOCH_TALK     isochTalk
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    TRACE(TL_TRACE, (hWnd, "Enter IsochTalk\r\n"));

    TRACE(TL_TRACE, (hWnd, "hResource = 0x%x\r\n", isochTalk->hResource));
    TRACE(TL_TRACE, (hWnd, "fulFlags = 0x%x\r\n", isochTalk->fulFlags));
    TRACE(TL_TRACE, (hWnd, "StartTime.CL_CycleOffset = 0x%x\r\n", isochTalk->StartTime.CL_CycleOffset));
    TRACE(TL_TRACE, (hWnd, "StartTime.CL_CycleCount = 0x%x\r\n", isochTalk->StartTime.CL_CycleCount));
    TRACE(TL_TRACE, (hWnd, "StartTime.CL_SecondCount = 0x%x\r\n", isochTalk->StartTime.CL_SecondCount));

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_ISOCH_TALK,
                                 isochTalk,
                                 sizeof(ISOCH_TALK),
                                 NULL,
                                 0,
                                 &dwBytesRet,
                                 NULL
                                 );

        if (!dwRet) {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }
        else {

            dwRet = ERROR_SUCCESS;
        }

        // free up resources
        CloseHandle(hDevice);
    }

    TRACE(TL_TRACE, (hWnd, "Exit IsochTalk = %d\r\n", dwRet));
    return(dwRet);
} // IsochTalk

