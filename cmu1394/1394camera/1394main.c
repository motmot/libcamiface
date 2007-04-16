/*++

Copyright (c) 1998	Microsoft Corporation

Module Name:

    1394main.c

Abstract


Authors:

    Peter Binder (pbinder) 4/22/98
	Christopher Baker (cbaker) 10/01

Revision History:
Date     Who       What
-------- --------- ------------------------------------------------------------
4/22/98  pbinder   moved from 1394api.c
10/11/01 cbaker    stripped out the automagic stuff that was causing problems, modified GetDeviceList
10/20/01 cbaker    added [Get/Reset\CmdrState to assist in recovery from program crashes
10/24/01 cbaker    added ReadRegister/WriteRegister Get[Model/Vendor]Name
10/29/01 cbaker    added WriteRegisterUL
--*/

#define _1394MAIN_C
#define INITGUID
#include "pch.h"
#undef _1394MAIN_C

/*
 * DllMain
 *
 * Dll Entry Function
 *
 * Under 1394diag, this function did a bunch of things with shared memory and process management
 *
 * Now, it does nothing interesting at all
 */

HINSTANCE g_hInstDLL;

BOOL
WINAPI
DllMain(
    HINSTANCE   hInstance,
    ULONG       ulReason,
    LPVOID      pvReserved
    )
{
	HKEY myKey = NULL;
	DWORD dwDisp,dwRet,dwTraceLevel = DLL_TRACE_WARNING,dwType,dwSize = sizeof(DWORD);

	g_hInstDLL = hInstance;

	// now that we don't start the extra process nor muck around with shared
	// memory and device change signals and bus resets, DllMain is really simple

	
	if(ulReason == DLL_PROCESS_ATTACH)
	{
		// load the tracelevel from the registry
		DllTrace(DLL_TRACE_ALWAYS,"DllMain: DLL_PROCESS_ATTACH to 1394Camera.dll version 6.03.00.0000\n");
		dwRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE,"Software\\CMU\\1394Camera",0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&myKey,&dwDisp);
		if(dwRet != ERROR_SUCCESS)
		{
			DllTrace(DLL_TRACE_ERROR,"DllMain: Failed to open the key (%d)\n",dwRet);
			myKey = NULL;
		} else {
			if(dwDisp == REG_CREATED_NEW_KEY)
			{
				DllTrace(DLL_TRACE_CHECK,"DllMain: New key was created, setting TraceLevel to DLL_TRACE_WARNING\n");
				dwTraceLevel = DLL_TRACE_WARNING;
				dwRet = RegSetValueEx(myKey,"DllTraceLevel",0,REG_DWORD,(LPBYTE)&dwTraceLevel,dwSize);
				if(dwRet != ERROR_SUCCESS)
					DllTrace(DLL_TRACE_ERROR,"DllMain: error setting TraceLevel registry key (%d)\n",dwRet);
			} else {
				DllTrace(DLL_TRACE_CHECK,"DllMain: Old Key was opened, getting TraceLevel\n");
				dwRet = RegQueryValueEx(myKey,"DllTraceLevel",NULL,&dwType,(LPBYTE)&dwTraceLevel,&dwSize);
				if(dwRet != ERROR_SUCCESS)
				{
					DllTrace(DLL_TRACE_ERROR,"DllMain: error getting TraceLevel registry key (%d)\n",dwRet);
					// default to TRACE_WARNING (that is, errors and warnings only
					dwTraceLevel = DLL_TRACE_WARNING;
				} else {
					DllTrace(DLL_TRACE_CHECK,"DllMain: Read Tracelevel as %d\n",dwTraceLevel);
				}
			}
			RegCloseKey(myKey);
		}
		SetDllTraceLevel(dwTraceLevel);
	}
	return TRUE;
}

/*
 * GetCmdrState
 *
 * Currently just a stub that doesn't do anything noteworthy
 * May eventually be used to report statistics, or more
 */

DWORD
CAMAPI
GetCmdrState( PSTR szDeviceName)
{
	DWORD state;
	HANDLE hDevice = OpenDevice(szDeviceName,FALSE);
	if(hDevice != INVALID_HANDLE_VALUE)
	{
		if(DeviceIoControl(hDevice,IOCTL_GET_CMDR_STATE,NULL,0,&state,sizeof(DWORD),NULL,NULL))
		{
			return state;
		}
	}
	return -1;
}

/*
 * SetCmdrTraceLevel
 *
 * This is another stub in case I ever decide to 
 * reorganize the trace output from the driver.
 */


DWORD
CAMAPI
SetCmdrTraceLevel( PSTR szDeviceName, DWORD nlevel)
{
	DWORD level = nlevel;
	HANDLE hDevice = OpenDevice(szDeviceName,FALSE);
	if(hDevice != INVALID_HANDLE_VALUE)
	{
		if(DeviceIoControl(hDevice,IOCTL_GET_CMDR_STATE,&level,sizeof(DWORD),NULL,0,NULL,NULL))
		{
			return 0;
		}
	}
	return -1;
}


/*
 * GetDeviceList
 *
 * Checks the GUID_1394CAMERA interface for all 1394 Digital cameras in the system
 * and returns their device names in DeviceData
 *
 * args:
 *   DeviceData: a pointer to an externally allocated DEVICE_DATA structure;
 *
 * returns:
 *   the number of cameras discovered, or
 *   -1 on error
 */

DWORD
CAMAPI
GetDeviceList(
    PDEVICE_DATA    DeviceData
    )
{

	    HDEVINFO                            hDevInfo;
    ULONG                               i = 0;
    SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    DeviceInterfaceDetailData;
    ULONG                               requiredSize;
    GUID                                t1394CmdrGUID;
    ULONG                               dwError;
	HWND hWnd = NULL;
    BOOL                                bReturn;

    DllTrace(DLL_TRACE_ENTER, "Enter GetDeviceList(0x%08x)\r\n",DeviceData);

	/******************************/
	/* for this to work, you MUST */
	/* define INITGUID before     */
	/* including pch.h            */
	/* otherwise you get an       */
	/* unresolved external symbol */
	/* for t1394CmdrGUID          */
	/******************************/

    t1394CmdrGUID = GUID_1394CMDR;
	memset(DeviceData,0,sizeof(DEVICE_DATA));

    // get a handle to 1394 test class using guid.
    hDevInfo = SetupDiGetClassDevs( &t1394CmdrGUID,
                                    NULL,
                                    NULL,
                                    (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE)
                                    );

    if (!hDevInfo) {

        dwError = GetLastError();
        DllTrace(DLL_TRACE_ERROR, "GetDeviceList: SetupDiGetClassDevs failed = 0x%x\r\n", dwError);
		return -1;

    }

	// here we're going to loop and find all of the
    // 1394 camera interfaces available.
    while (TRUE) {

		DllTrace(DLL_TRACE_VERBOSE,"GetDeviceList: Enumerating Device %d\r\n",i);

        deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &t1394CmdrGUID, i, &deviceInterfaceData))
		{
            dwError = GetLastError();
			if(dwError != ERROR_NO_MORE_ITEMS)
			{
				DllTrace(DLL_TRACE_ERROR,"GetDeviceList: SetupDiEnumDeviceInterfaces failed -> 0x%x\r\n", dwError);
				return -1;
			} else {
				DllTrace(DLL_TRACE_CHECK,"GetDeviceList: No More Items\r\n");
				break;
			}
        }

		// figure out the size...
		bReturn = SetupDiGetDeviceInterfaceDetail( hDevInfo,
												   &deviceInterfaceData,
												   NULL,
												   0,
												   &requiredSize,
												   NULL
												   );

		if (!bReturn) {
			dwError = GetLastError();
			if(dwError != ERROR_INSUFFICIENT_BUFFER)
			{
				DllTrace(DLL_TRACE_ERROR, "GetDeviceList: SetupDiGetDeviceInterfaceDetail Failed (1) -> 0x%x\r\n", dwError);
				SetupDiDestroyDeviceInfoList(hDevInfo);
				return -1;
			}
		} else {
			DllTrace(DLL_TRACE_ERROR,"GetDeviceList: I really shouldn't be here (%s:%d)\r\n",__FILE__,__LINE__);
			SetupDiDestroyDeviceInfoList(hDevInfo);
			return -1;
		}

		DeviceInterfaceDetailData = LocalAlloc(LPTR, requiredSize);
		if(DeviceInterfaceDetailData == NULL)
		{
			DllTrace(DLL_TRACE_ERROR,"GetDeviceList: LocalAlloc Failed\r\n");
			SetupDiDestroyDeviceInfoList(hDevInfo);
			return -1;
		}

		memset(DeviceInterfaceDetailData,0,requiredSize);
		DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		bReturn = SetupDiGetDeviceInterfaceDetail( hDevInfo,
												   &deviceInterfaceData,
												   DeviceInterfaceDetailData,
												   requiredSize,
												   NULL,
												   NULL
												   );

		if (!bReturn) 
		{
			dwError = GetLastError();

			DllTrace(DLL_TRACE_ERROR, "GetDeviceList: SetupDiGetDeviceInterfaceDetail Failed (2) -> 0x%x\r\n", dwError);

			SetupDiDestroyDeviceInfoList(hDevInfo);
			LocalFree(DeviceInterfaceDetailData);

			return -1;

		} else {
			// we have a device, let's put it in the list.
			DeviceData->numDevices++;
			strcpy(DeviceData->deviceList[i].DeviceName, DeviceInterfaceDetailData->DevicePath);
			LocalFree(DeviceInterfaceDetailData);
		}

        i++;
    } // while

	SetupDiDestroyDeviceInfoList(hDevInfo);

    DllTrace(DLL_TRACE_EXIT,"Exit GetDeviceList --> %d\r\n",i);
	return i;
}

/*
 * GetCameraSpecification
 *
 * Retrieves the unit_spec_ID and unit_sw_version fields
 * of the camera from the device driver
 *
 * Valid values are:
 *
 * ulSpecification:
 *
 * 0x00A02D
 *
 * ulVersion:
 *
 * 0x000100 = 1.04
 * 0x000101 = 1.20
 * 0x000102 = 1.30
 */

DWORD
CAMAPI
GetCameraSpecification( PSTR szDeviceName, PCAMERA_SPECIFICATION pSpec)
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    DllTrace(DLL_TRACE_ENTER,"Enter GetCameraVersion\r\n");

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_GET_CAMERA_SPECIFICATION,
                                 pSpec,
                                 sizeof(CAMERA_SPECIFICATION),
                                 pSpec,
                                 sizeof(CAMERA_SPECIFICATION),
                                 &dwBytesRet,
                                 NULL
                                 );

        if (dwRet) 
		{
            dwRet = ERROR_SUCCESS;
        } else {
            dwRet = GetLastError();
            DllTrace(DLL_TRACE_ERROR, "GetCameraSpecification: IoCtl Error -> 0x%x\r\n", dwRet);
        }

        // free up resources
        CloseHandle(hDevice);
    } else {
        dwRet = GetLastError();
        DllTrace(DLL_TRACE_ERROR, "GetCameraSpecification: Couldn't get Device Handle -> 0x%x\r\n", dwRet);        
    }

    DllTrace(DLL_TRACE_EXIT, "Exit GetCameraSpecification\r\n");
    return(dwRet);
} // GetCameraVersion


/*
 * GetCmdrVersion
 *
 * Used to make sure the DLL and driver are compatible versions
 */

ULONG
CAMAPI
GetCmdrVersion(
    HWND            hWnd,
    PSTR            szDeviceName,
    PVERSION_DATA   Version,
    BOOL            bMatch
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;

    DllTrace(DLL_TRACE_ENTER,"Enter GetCmdrVersion\r\n");

    bMatch = FALSE;
    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_GET_DIAG_VERSION,
                                 Version,
                                 sizeof(VERSION_DATA),
                                 Version,
                                 sizeof(VERSION_DATA),
                                 &dwBytesRet,
                                 NULL
                                 );

        if (dwRet) {

            dwRet = ERROR_SUCCESS;

            if ((Version->ulVersion == DIAGNOSTIC_VERSION) &&
                (Version->ulVersion == DIAGNOSTIC_SUB_VERSION)) {

                bMatch = TRUE;
            }

        } else {
            dwRet = GetLastError();
            DllTrace(DLL_TRACE_ERROR, "GetCmdrVersion: IoCtl Error -> 0x%x\r\n", dwRet);
        }

        // free up resources
        CloseHandle(hDevice);
    } else {
        dwRet = GetLastError();
        DllTrace(TL_ERROR, "GetCmdrVersion: Couldn't get Device Handle -> 0x%x\r\n", dwRet);        
    }

    DllTrace(DLL_TRACE_EXIT, "Exit GetCmdrVersion\r\n");
    return(dwRet);
} // GetCmdrVersion

/*
 * ResetCameraState
 *
 * Causes the device driver to clear out any resourcespreviously allocated
 * and move the camera into a passive state
 *
 * USE WITH CAUTION
 */

void
CAMAPI
ResetCameraState(
    PSTR szDeviceName
	)
{
	HANDLE hDevice;
	DWORD dwRet,bytesRet;
	DllTrace(DLL_TRACE_ENTER,"Enter ResetCameraState (%s)\n",szDeviceName);

	hDevice = OpenDevice(szDeviceName,FALSE);
	if(hDevice != INVALID_HANDLE_VALUE)
	{
		DllTrace(DLL_TRACE_CHECK,"Calling IOCtl\n");
        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_RESET_CMDR_STATE,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 &bytesRet,
                                 NULL
                                 );
		if(!dwRet)
			DllTrace(DLL_TRACE_ERROR,"ResetCameraState: IoCtl Failed: %d\n",dwRet);
		else
			DllTrace(DLL_TRACE_CHECK,"ResetCameraState: IoCtl Succeeded\n");

		CloseHandle(hDevice);
	} else {
		DllTrace(DLL_TRACE_ERROR,"ResetCameraState: Couldn't open handle to %s\n",szDeviceName);
	}

	DllTrace(DLL_TRACE_EXIT,"Exit ResetCameraState\n");
}

//
// misc exported functions
//
/*
 * ReadRegister
 *
 * Reads a quad-word from the camera CSR regiser at ulOffset
 * places the result in "bytes"
 *
 * Note: the offset is not an absolute offset, it is the relative offset
 * from the first CSR register, which is maintained now in the device extension
 */

DWORD
CAMAPI
ReadRegister(
	PSTR szDeviceName,
	ULONG ulOffset,
	PUCHAR bytes
	)
{
	HANDLE hDevice;
	DWORD dwRet,bytesRet;
	REGISTER_IOBUF regbuf;
	DllTrace(DLL_TRACE_ENTER,"Enter ReadRegister (0x%08x)\n",ulOffset);

	regbuf.ulOffset = ulOffset;
	memset(regbuf.data,0,4);
	hDevice = OpenDevice(szDeviceName,FALSE);
	if(hDevice != INVALID_HANDLE_VALUE)
	{
		DllTrace(DLL_TRACE_CHECK,"Calling IOCtl\n");
        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_READ_REGISTER,
                                 &regbuf,
                                 sizeof(REGISTER_IOBUF),
                                 &regbuf,
                                 sizeof(REGISTER_IOBUF),
                                 &bytesRet,
                                 NULL
                                 );
		if(!dwRet)
		{
			dwRet = GetLastError();
		} else {
			DllTrace(DLL_TRACE_CHECK,"ReadRegister: IoCtl Succeeded\n");
			DllTrace(DLL_TRACE_CHECK,"ReadRegister: 0x%08x -> [%02x,%02x,%02x,%02x]\n",
					ulOffset,
					regbuf.data[0],
					regbuf.data[1],
					regbuf.data[2],
					regbuf.data[3]
					);
			dwRet = 0;
		}
		CloseHandle(hDevice);
	} else {
		DllTrace(DLL_TRACE_ERROR,"ReadRegister: Couldn't open handle to %s\n",szDeviceName);
		dwRet = -1;
	}

	memcpy(bytes,regbuf.data,4);
	DllTrace(DLL_TRACE_EXIT,"Exit ReadRegister (%d)\n",dwRet);
	return dwRet;
}

/*
 * ReadRegisterUL
 *
 * Same as ReadRegister, except the data is passed in as a pointer to an 
 * unsigned long, with the first "byte" as the most significant byte.
 */

DWORD
CAMAPI
ReadRegisterUL(
	PSTR szDeviceName,
	ULONG ulOffset,
	PULONG pData
	)
{
	DWORD dwRet;
	unsigned char bytes[4];
	int i;

	dwRet = ReadRegister(szDeviceName,ulOffset,bytes);
	*pData = 0;
	for(i=0; i<4; i++)
	{
		*pData <<= 8;
		*pData += bytes[i];
	}

	return dwRet;
}

/*
 * WriteRegister
 *
 * Writes a quad-word to the camera CSR regiser at ulOffset
 * "bytes" represents the quad-word to write
 *
 * Note: the offset is not an absolute offset, it is the relative offset
 * from the first CSR register, which is maintained now in the device extension
 */

DWORD
CAMAPI
WriteRegister(
	PSTR szDeviceName,
	ULONG ulOffset,
	PUCHAR bytes
	)
{
	HANDLE hDevice;
	DWORD dwRet,bytesRet;
	REGISTER_IOBUF regbuf;

	DllTrace(DLL_TRACE_ENTER,"Enter WriteRegister\n",szDeviceName);

	regbuf.ulOffset = ulOffset;
	memcpy(regbuf.data,bytes,4);

	hDevice = OpenDevice(szDeviceName,FALSE);
	if(hDevice != INVALID_HANDLE_VALUE)
	{
		DllTrace(DLL_TRACE_CHECK,"Calling IOCtl\n");
        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_WRITE_REGISTER,
                                 &regbuf,
                                 sizeof(REGISTER_IOBUF),
                                 &regbuf,
                                 sizeof(REGISTER_IOBUF),
                                 &bytesRet,
                                 NULL
                                 );
		if(!dwRet)
		{
			DllTrace(DLL_TRACE_ERROR,"WriteRegister: IoCtl Failed: %d\n",dwRet);
			dwRet = -1;
		} else {
			DllTrace(DLL_TRACE_CHECK,"WriteRegister: IoCtl Succeeded\n");
			dwRet = 0;
		}
		CloseHandle(hDevice);
	} else {
		DllTrace(DLL_TRACE_ERROR,"WriteRegister: Couldn't open handle to %s\n",szDeviceName);
		dwRet = -1;
	}

	DllTrace(DLL_TRACE_EXIT,"Exit WriteRegister (%d)\n",dwRet);
	return dwRet;
}

/*
 * WriteRegisterUL
 *
 * Same as writeRegister, except the data is passed in as an unsigned long,
 * with the first "byte" as the most significant byte.
 */

DWORD
CAMAPI
WriteRegisterUL(
	PSTR szDeviceName,
	ULONG ulOffset,
	ULONG data
	)
{
	unsigned char bytes[4];
	int i;
	for(i=0; i<4; i++)
		bytes[i] = (unsigned char)((data >> (8 * (3 - i))) & 0xff);

	return WriteRegister(szDeviceName,ulOffset,bytes);
}

/*
 * GetModelName
 *
 * Retrieves (at most buflen bytes) of the model name from the 
 * device driver and places it into buffer
 */

DWORD
CAMAPI
GetModelName(
	PSTR szDeviceName,
	PSTR buffer,
	ULONG buflen
	)
{
	HANDLE hDevice;
	DWORD dwRet,bytesRet;

	DllTrace(DLL_TRACE_ENTER,"Enter GetModelName (%s)\n",szDeviceName);

	hDevice = OpenDevice(szDeviceName,FALSE);
	if(hDevice != INVALID_HANDLE_VALUE)
	{
		DllTrace(DLL_TRACE_CHECK,"Calling IOCtl\n");
        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_GET_MODEL_NAME,
                                 NULL,
                                 0,
                                 buffer,
                                 buflen,
                                 &bytesRet,
                                 NULL
                                 );
		if(!dwRet)
		{
			DllTrace(DLL_TRACE_ERROR,"GetModelName: IoCtl Failed\n");
			dwRet = -1;
		} else {
			DllTrace(DLL_TRACE_CHECK,"GetModelName: IoCtl Succeeded\n");
			dwRet = bytesRet;
		}
		CloseHandle(hDevice);
	} else {
		DllTrace(DLL_TRACE_ERROR,"GetModelName: Couldn't open handle to %s\n",szDeviceName);
		dwRet = -1;
	}

	DllTrace(DLL_TRACE_EXIT,"Exit GetModelName (%d)\n",dwRet);
	return dwRet;
}

/*
 * GetVendorName
 *
 * Retrieves (at most buflen bytes) of the vendor name from the 
 * device driver and places it into buffer
 */

DWORD
CAMAPI
GetVendorName(
	PSTR szDeviceName,
	PSTR buffer,
	ULONG buflen
	)
{
	HANDLE hDevice;
	DWORD dwRet,bytesRet;

	DllTrace(DLL_TRACE_ENTER,"Enter GetVendorName (%s)\n",szDeviceName);

	hDevice = OpenDevice(szDeviceName,FALSE);
	if(hDevice != INVALID_HANDLE_VALUE)
	{
		DllTrace(DLL_TRACE_CHECK,"Calling IOCtl\n");
        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_GET_VENDOR_NAME,
                                 NULL,
                                 0,
                                 buffer,
                                 buflen,
                                 &bytesRet,
                                 NULL
                                 );
		if(!dwRet)
		{
			DllTrace(DLL_TRACE_ERROR,"GetVendorName: IoCtl Failed\n");
			dwRet = -1;
		} else {
			DllTrace(DLL_TRACE_CHECK,"GetVendorName: IoCtl Succeeded\n");
			dwRet = bytesRet;
		}
		CloseHandle(hDevice);
	} else {
		DllTrace(DLL_TRACE_ERROR,"GetVendorName: Couldn't open handle to %s\n",szDeviceName);
		dwRet = -1;
	}

	DllTrace(DLL_TRACE_EXIT,"Exit GetVendorName (%d)\n",dwRet);
	return dwRet;
}

DWORD
CAMAPI
GetUniqueID(
	PSTR szDeviceName,
	PLARGE_INTEGER pliUniqueID
	)
{
	HANDLE hDevice;
	DWORD dwRet,bytesRet;

	DllTrace(DLL_TRACE_ENTER,"Enter GetUniqueID (%s)\n",szDeviceName);

	hDevice = OpenDevice(szDeviceName,FALSE);
	if(hDevice != INVALID_HANDLE_VALUE)
	{
		DllTrace(DLL_TRACE_CHECK,"Calling IOCtl\n");
        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_GET_CAMERA_UNIQUE_ID,
                                 NULL,
                                 0,
                                 pliUniqueID,
                                 sizeof(LARGE_INTEGER),
                                 &bytesRet,
                                 NULL
                                 );
		if(!dwRet)
		{
			DllTrace(DLL_TRACE_ERROR,"GetUniqueID: IoCtl Failed\n");
			dwRet = GetLastError();
		} else {
			DllTrace(DLL_TRACE_CHECK,"GetUniqueID: IoCtl Succeeded\n");
			dwRet = 0;
		}
		CloseHandle(hDevice);
	} else {
		DllTrace(DLL_TRACE_ERROR,"GetUniqueID: Couldn't open handle to %s\n",szDeviceName);
		dwRet = GetLastError();
	}

	DllTrace(DLL_TRACE_EXIT,"Exit GetUniqueID (%d)\n",dwRet);
	return dwRet;
}

HANDLE
CAMAPI
OpenDevice(
    PSTR    szDeviceName,
    BOOL    bOverLapped
    )
{
    HANDLE  hDevice;

    if (bOverLapped) {

        hDevice = CreateFile( szDeviceName,
                              GENERIC_WRITE | GENERIC_READ,
                              FILE_SHARE_WRITE | FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_OVERLAPPED,
                              NULL
                              );
    } else {
    
        hDevice = CreateFile( szDeviceName,
                              GENERIC_WRITE | GENERIC_READ,
                              FILE_SHARE_WRITE | FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL
                              );
    }

	if (hDevice == INVALID_HANDLE_VALUE) {
		DllTrace(DLL_TRACE_ERROR,"OpenDevice: Error getting handle for %s\r\n",szDeviceName);
    }

    return(hDevice);
} // OpenDevice

ULONG
CAMAPI
GetMaxSpeedBetweenDevices(
    HWND                            hWnd,
    PSTR                            szDeviceName,
    PGET_MAX_SPEED_BETWEEN_DEVICES  GetMaxSpeedBetweenDevices
    )
{
    HANDLE      hDevice;
    DWORD       dwRet, dwBytesRet;
    ULONG       i;

    TRACE(TL_TRACE, (hWnd, "Enter GetMaxSpeedBetweenDevices\r\n"));

    TRACE(TL_TRACE, (hWnd, "fulFlags = 0x%x\r\n", GetMaxSpeedBetweenDevices->fulFlags));
    TRACE(TL_TRACE, (hWnd, "ulNumberOfDestinations = 0x%x\r\n", GetMaxSpeedBetweenDevices->ulNumberOfDestinations));

    for (i=0; i<GetMaxSpeedBetweenDevices->ulNumberOfDestinations; i++) {

        TRACE(TL_TRACE, (hWnd, "hDestinationDeviceObjects[%d] = 0x%x\r\n", i, 
            GetMaxSpeedBetweenDevices->hDestinationDeviceObjects[i]));
    }

    hDevice = OpenDevice(szDeviceName, FALSE);

    if (hDevice != INVALID_HANDLE_VALUE) {

        dwRet = DeviceIoControl( hDevice,
                                 IOCTL_GET_MAX_SPEED_BETWEEN_DEVICES,
                                 GetMaxSpeedBetweenDevices,
                                 sizeof(GET_MAX_SPEED_BETWEEN_DEVICES),
                                 GetMaxSpeedBetweenDevices,
                                 sizeof(GET_MAX_SPEED_BETWEEN_DEVICES),
                                 &dwBytesRet,
                                 NULL
                                 );

        if (dwRet) {

            dwRet = ERROR_SUCCESS;

            TRACE(TL_TRACE, (hWnd, "fulSpeed = 0x%x\r\n", GetMaxSpeedBetweenDevices->fulSpeed));
        }
        else {

            dwRet = GetLastError();
            TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));
        }

        // free up resources
        CloseHandle(hDevice);
    }
    else {

        dwRet = GetLastError();
        TRACE(TL_ERROR, (hWnd, "Error = 0x%x\r\n", dwRet));        
    }
    
    TRACE(TL_TRACE, (hWnd, "Exit GetMaxSpeedBetweenDevices\r\n"));
    return(dwRet);
} // GetMaxSpeedBetweenDevices

