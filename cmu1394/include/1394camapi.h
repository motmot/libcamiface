//
//  1394camapi.h
//
//  Abstract
//  	Modified from 1394api.h as found in the Windows DDK
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


#ifndef __1394_CAMAPI_H__
#define __1394_CAMAPI_H__

// extern "C" it if necessary (has something to do with calling conventions)

#ifdef __cplusplus
extern "C" {
#endif

// export if compiling, import if using the library

#ifdef MY1394CAMERA_EXPORTS
#define CAMAPI __declspec(dllexport)
#else
#define CAMAPI __declspec(dllimport)
#endif

// Trace Levels for the DLL

#define DLL_TRACE_ALWAYS   -2 // this one is used sparingly for critical debug information
#define DLL_TRACE_NONE    -1
#define DLL_TRACE_ALL    100
#define DLL_TRACE_ERROR    0
#define DLL_TRACE_WARNING  1
#define DLL_TRACE_CHECK    2
#define DLL_TRACE_ENTER    5
#define DLL_TRACE_EXIT     6
#define DLL_TRACE_VERBOSE 10

////////////////////////////////////////////////////////////////////////
// The defines/structs below are taken from 1394.h in the windows DDK //
// These *must* be defined before including 1394common.h              //
// They are redefined here so we don't need 1394.h to use the library //
////////////////////////////////////////////////////////////////////////

//
// Definition of flags for AllocateAddressRange Irb
//
#define BIG_ENDIAN_ADDRESS_RANGE                1

//
// Definition of fulAccessType for AllocateAddressRange
//
#define ACCESS_FLAGS_TYPE_READ                  1
#define ACCESS_FLAGS_TYPE_WRITE                 2
#define ACCESS_FLAGS_TYPE_LOCK                  4

//
// Definition of fulNotificationOptions for AllocateAddressRange
//
#define NOTIFY_FLAGS_NEVER                      0
#define NOTIFY_FLAGS_AFTER_READ                 1
#define NOTIFY_FLAGS_AFTER_WRITE                2
#define NOTIFY_FLAGS_AFTER_LOCK                 4

//
// Definitions of Lock transaction types 
//
#define LOCK_TRANSACTION_MASK_SWAP              1
#define LOCK_TRANSACTION_COMPARE_SWAP           2
#define LOCK_TRANSACTION_FETCH_ADD              3
#define LOCK_TRANSACTION_LITTLE_ADD             4
#define LOCK_TRANSACTION_BOUNDED_ADD            5
#define LOCK_TRANSACTION_WRAP_ADD               6

//
// Definition of fulFlags in Async Read/Write/Lock requests
//

#define ASYNC_FLAGS_NONINCREMENTING             1

//
// flag instucts the port driver to NOT take an int for checking the status
// of this transaction. Always return success...
//

#define ASYNC_FLAGS_NO_STATUS               0x00000002

//
// Definitions of levels of Host controller information
//
#define GET_HOST_UNIQUE_ID                      1
#define GET_HOST_CAPABILITIES                   2
#define GET_POWER_SUPPLIED                      3
#define GET_PHYS_ADDR_ROUTINE                   4
#define GET_HOST_CONFIG_ROM                     5
#define GET_HOST_CSR_CONTENTS                   6

//
// Definitions of capabilities in Host info level 2
//
#define HOST_INFO_PACKET_BASED                  1
#define HOST_INFO_STREAM_BASED                  2
#define HOST_INFO_SUPPORTS_ISOCH_STRIPPING      4
#define HOST_INFO_SUPPORTS_START_ON_CYCLE       8
#define HOST_INFO_SUPPORTS_RETURNING_ISO_HDR    16
#define HOST_INFO_SUPPORTS_ISO_HDR_INSERTION    32

//
// Definitions of Bus Reset flags (used when Bus driver asks Port driver
// to perform a bus reset)
//
#define BUS_RESET_FLAGS_PERFORM_RESET           1
#define BUS_RESET_FLAGS_FORCE_ROOT              2

//
// Definitions of flags for GetMaxSpeedBetweenDevices and 
// Get1394AddressFromDeviceObject
//
#define USE_LOCAL_NODE                          1

//
// Definition of flags for BusResetNotification Irb
//
#define REGISTER_NOTIFICATION_ROUTINE           1
#define DEREGISTER_NOTIFICATION_ROUTINE         2

//
// Definitions of Speed flags used throughout 1394 Bus APIs
//
#define SPEED_FLAGS_100                         0x01
#define SPEED_FLAGS_200                         0x02
#define SPEED_FLAGS_400                         0x04
#define SPEED_FLAGS_800                         0x08
#define SPEED_FLAGS_1600                        0x10
#define SPEED_FLAGS_3200                        0x20

#define SPEED_FLAGS_FASTEST                     0x80000000

//
// Definitions of Isoch Allocate Resources flags
//
#define RESOURCE_USED_IN_LISTENING              1
#define RESOURCE_USED_IN_TALKING                2
#define RESOURCE_BUFFERS_CIRCULAR               4
#define RESOURCE_STRIP_ADDITIONAL_QUADLETS      8
#define RESOURCE_TIME_STAMP_ON_COMPLETION       16
#define RESOURCE_SYNCH_ON_TIME                  32
#define RESOURCE_USE_PACKET_BASED               64

//
// Definitions of Isoch Descriptor flags
//
#define DESCRIPTOR_SYNCH_ON_SY                  0x00000001
#define DESCRIPTOR_SYNCH_ON_TAG                 0x00000002
#define DESCRIPTOR_SYNCH_ON_TIME                0x00000004
#define DESCRIPTOR_USE_SY_TAG_IN_FIRST          0x00000008
#define DESCRIPTOR_TIME_STAMP_ON_COMPLETION     0x00000010
#define DESCRIPTOR_PRIORITY_TIME_DELIVERY       0x00000020
#define DESCRIPTOR_HEADER_SCATTER_GATHER        0x00000040

//
// Definitions of Isoch synchronization flags
//
#define SYNCH_ON_SY                             DESCRIPTOR_SYNCH_ON_SY 
#define SYNCH_ON_TAG                            DESCRIPTOR_SYNCH_ON_TAG
#define SYNCH_ON_TIME                           DESCRIPTOR_SYNCH_ON_TIME

//
// flags for the SetPortProperties request
//
#define SET_LOCAL_HOST_PROPERTIES_NO_CYCLE_STARTS       0x00000001
#define SET_LOCAL_HOST_PROPERTIES_GAP_COUNT             0x00000002
#define SET_LOCAL_HOST_PROPERTIES_MODIFY_CROM           0x00000003

//
// definition of Flags for SET_LOCAL_HOST_PROPERTIES_MODIFY_CROM
//
#define SLHP_FLAG_ADD_CROM_DATA         0x01
#define SLHP_FLAG_REMOVE_CROM_DATA      0x02

//
// Various Interesting 1394 IEEE 1212 locations
//
#define INITIAL_REGISTER_SPACE_HI               0xffff
//#define INITIAL_REGISTER_SPACE_LO               0xf0000000
#define TOPOLOGY_MAP_LOCATION                   0xf0001000
#define SPEED_MAP_LOCATION                      0xf0002000

//
// 1394 Cycle Time format
//
typedef struct _CYCLE_TIME {
    ULONG               CL_CycleOffset:12;      // Bits 0-11
    ULONG               CL_CycleCount:13;       // Bits 12-24
    ULONG               CL_SecondCount:7;       // Bits 25-31
} CYCLE_TIME, *PCYCLE_TIME;

//
// 1394 Node Address format
//
typedef struct _NODE_ADDRESS {
    USHORT              NA_Node_Number:6;       // Bits 10-15
    USHORT              NA_Bus_Number:10;       // Bits 0-9
} NODE_ADDRESS, *PNODE_ADDRESS;

//
// 1394 Address Offset format (48 bit addressing)
//
typedef struct _ADDRESS_OFFSET {
    USHORT              Off_High;
    ULONG               Off_Low;
} ADDRESS_OFFSET, *PADDRESS_OFFSET;

// 
// 1394 I/O Address format
//
typedef struct _IO_ADDRESS {
    NODE_ADDRESS        IA_Destination_ID;
    ADDRESS_OFFSET      IA_Destination_Offset;
} IO_ADDRESS, *PIO_ADDRESS;

//
// 1394 Self ID packet format 
//
typedef struct _SELF_ID {
    ULONG               SID_Phys_ID:6;          // Byte 0 - Bits 0-5
    ULONG               SID_Packet_ID:2;        // Byte 0 - Bits 6-7
    ULONG               SID_Gap_Count:6;        // Byte 1 - Bits 0-5
    ULONG               SID_Link_Active:1;      // Byte 1 - Bit 6
    ULONG               SID_Zero:1;             // Byte 1 - Bit 7
    ULONG               SID_Power_Class:3;      // Byte 2 - Bits 0-2
    ULONG               SID_Contender:1;        // Byte 2 - Bit 3
    ULONG               SID_Delay:2;            // Byte 2 - Bits 4-5
    ULONG               SID_Speed:2;            // Byte 2 - Bits 6-7
    ULONG               SID_More_Packets:1;     // Byte 3 - Bit 0
    ULONG               SID_Initiated_Rst:1;    // Byte 3 - Bit 1
    ULONG               SID_Port3:2;            // Byte 3 - Bits 2-3
    ULONG               SID_Port2:2;            // Byte 3 - Bits 4-5
    ULONG               SID_Port1:2;            // Byte 3 - Bits 6-7
} SELF_ID, *PSELF_ID;

//
// Additional 1394 Self ID packet format (only used when More bit is on)
//
typedef struct _SELF_ID_MORE {
    ULONG               SID_Phys_ID:6;          // Byte 0 - Bits 0-5
    ULONG               SID_Packet_ID:2;        // Byte 0 - Bits 6-7
    ULONG               SID_PortA:2;            // Byte 1 - Bits 0-1
    ULONG               SID_Reserved2:2;        // Byte 1 - Bits 2-3
    ULONG               SID_Sequence:3;         // Byte 1 - Bits 4-6
    ULONG               SID_One:1;              // Byte 1 - Bit 7
    ULONG               SID_PortE:2;            // Byte 2 - Bits 0-1
    ULONG               SID_PortD:2;            // Byte 2 - Bits 2-3
    ULONG               SID_PortC:2;            // Byte 2 - Bits 4-5
    ULONG               SID_PortB:2;            // Byte 2 - Bits 6-7
    ULONG               SID_More_Packets:1;     // Byte 3 - Bit 0
    ULONG               SID_Reserved3:1;        // Byte 3 - Bit 1
    ULONG               SID_PortH:2;            // Byte 3 - Bits 2-3
    ULONG               SID_PortG:2;            // Byte 3 - Bits 4-5
    ULONG               SID_PortF:2;            // Byte 3 - Bits 6-7
} SELF_ID_MORE, *PSELF_ID_MORE;

//
// 1394 Topology Map format
//
typedef struct _TOPOLOGY_MAP {
    USHORT              TOP_Length;             // number of quadlets in map
    USHORT              TOP_CRC;                // 16 bit CRC defined by 1212
    ULONG               TOP_Generation;         // Generation number 
    USHORT              TOP_Node_Count;         // Node count
    USHORT              TOP_Self_ID_Count;      // Number of Self IDs
    SELF_ID             TOP_Self_ID_Array[1];    // Array of Self IDs
} TOPOLOGY_MAP, *PTOPOLOGY_MAP;

//
// 1394 Speed Map format
//
typedef struct _SPEED_MAP {
    USHORT              SPD_Length;             // number of quadlets in map
    USHORT              SPD_CRC;                // 16 bit CRC defined by 1212
    ULONG               SPD_Generation;         // Generation number
    UCHAR               SPD_Speed_Code[4032]; 
} SPEED_MAP, *PSPEED_MAP;

//
// Definitions of the structures that correspond to the Host info levels 
//
typedef struct _GET_LOCAL_HOST_INFO1 {
    LARGE_INTEGER       UniqueId;
} GET_LOCAL_HOST_INFO1, *PGET_LOCAL_HOST_INFO1;

typedef struct _GET_LOCAL_HOST_INFO2 {
    ULONG               HostCapabilities;
    ULONG               MaxAsyncReadRequest;
    ULONG               MaxAsyncWriteRequest;
} GET_LOCAL_HOST_INFO2, *PGET_LOCAL_HOST_INFO2;

typedef struct _GET_LOCAL_HOST_INFO3 {
    ULONG               deciWattsSupplied;
    ULONG               Voltage;                    // x10 -> +3.3 == 33
                                                    // +5.0 == 50,+12.0 == 120
                                                    // etc.
} GET_LOCAL_HOST_INFO3, *PGET_LOCAL_HOST_INFO3;

typedef struct _GET_LOCAL_HOST_INFO4 {
    PVOID               PhysAddrMappingRoutine;
    PVOID               Context;
} GET_LOCAL_HOST_INFO4, *PGET_LOCAL_HOST_INFO4;

//
// the caller can set ConfigRomLength to zero, issue the request, which will
// be failed with STATUS_INVALID_BUFFER_SIZE and the ConfigRomLength will be set
// by the port driver to the proper length. The caller can then re-issue the request
// after it has allocated a buffer for the configrom with the correct length
//
typedef struct _GET_LOCAL_HOST_INFO5 {
    PVOID                   ConfigRom;
    ULONG                   ConfigRomLength;
} GET_LOCAL_HOST_INFO5, *PGET_LOCAL_HOST_INFO5;

typedef struct _GET_LOCAL_HOST_INFO6 {
    ADDRESS_OFFSET          CsrBaseAddress;
    ULONG                   CsrDataLength;
    UCHAR                   CsrDataBuffer[1];
} GET_LOCAL_HOST_INFO6, *PGET_LOCAL_HOST_INFO6;

//
// Definitions of the structures that correspond to the Host info levels 
//
typedef struct _SET_LOCAL_HOST_PROPS2 {
    ULONG       GapCountLowerBound;
} SET_LOCAL_HOST_PROPS2, *PSET_LOCAL_HOST_PROPS2;

typedef struct _SET_LOCAL_HOST_PROPS3 {
    ULONG       fulFlags;
    HANDLE      hCromData;
    ULONG       nLength;
    UCHAR       Buffer[1];
//    PMDL        Mdl;
} SET_LOCAL_HOST_PROPS3, *PSET_LOCAL_HOST_PROPS3;

//
// 1394 Phy Configuration packet format
//
typedef struct _PHY_CONFIGURATION_PACKET {
    ULONG               PCP_Phys_ID:6;          // Byte 0 - Bits 0-5
    ULONG               PCP_Packet_ID:2;        // Byte 0 - Bits 6-7
    ULONG               PCP_Gap_Count:6;        // Byte 1 - Bits 0-5
    ULONG               PCP_Set_Gap_Count:1;    // Byte 1 - Bit 6
    ULONG               PCP_Force_Root:1;       // Byte 1 - Bit 7
    ULONG               PCP_Reserved1:8;        // Byte 2 - Bits 0-7
    ULONG               PCP_Reserved2:8;        // Byte 3 - Bits 0-7
    ULONG               PCP_Inverse;            // Inverse quadlet
} PHY_CONFIGURATION_PACKET, *PPHY_CONFIGURATION_PACKET;

////////////////////////////////////////////////
//                                            //
// End structures and defines from DDK 1394.h //
//                                            //
////////////////////////////////////////////////

// be sure to include the version of the header that lives in the same directory
#include "./1394common.h"

// structures and defines used by the exported C functions:

typedef struct _DEVICE_LIST {
    CHAR            DeviceName[255];
} DEVICE_LIST, *PDEVICE_LIST;

typedef struct _DEVICE_DATA {
    ULONG           numDevices;
    DEVICE_LIST     deviceList[10];
} DEVICE_DATA, *PDEVICE_DATA;

#define bswap(value)    (((ULONG) (value)) << 24 |\
                        (((ULONG) (value)) & 0x0000FF00) << 8 |\
                        (((ULONG) (value)) & 0x00FF0000) >> 8 |\
                        ((ULONG) (value)) >> 24)

#define bswapw(value)   (((USHORT) (value)) << 8 |\
                        (((USHORT) (value)) & 0xff00) >> 8)

//
// function prototypes
//

// isochapi.c

ULONG
CAMAPI
IsochAllocateBandwidth(
    HWND                        hWnd, 
    PSTR                        szDeviceName,
    PISOCH_ALLOCATE_BANDWIDTH   isochAllocateBandwidth
    );

ULONG
CAMAPI
IsochAllocateChannel( 
    HWND                        hWnd,
    PSTR                        szDeviceName,
    PISOCH_ALLOCATE_CHANNEL     isochAllocateChannel
    );

ULONG
CAMAPI
IsochAllocateResources( 
    HWND                        hWnd,
    PSTR                        szDeviceName,
    PISOCH_ALLOCATE_RESOURCES   isochAllocateResources
    );

ULONG
CAMAPI
IsochFreeBandwidth(
    HWND    hWnd,
    PSTR    szDeviceName,
    HANDLE  hBandwidth
    );

ULONG
CAMAPI
IsochFreeChannel(
    HWND    hWnd,
    PSTR    szDeviceName,
    ULONG   nChannel
    );

ULONG
CAMAPI
IsochFreeResources(
    HWND    hWnd,
    PSTR    szDeviceName,
    HANDLE  hResource
    );

ULONG
CAMAPI
IsochListen(
    HWND            hWnd,
    PSTR            szDeviceName,
    PISOCH_LISTEN   isochListen
    );

ULONG
CAMAPI
IsochQueryCurrentCycleTime(
    HWND            hWnd,
    PSTR            szDeviceName,
    PCYCLE_TIME     CycleTime
    );

ULONG
CAMAPI
IsochQueryResources(
    HWND                        hWnd,
    PSTR                        szDeviceName,
    PISOCH_QUERY_RESOURCES      isochQueryResources
    );

ULONG
CAMAPI
IsochSetChannelBandwidth(
    HWND                            hWnd,
    PSTR                            szDeviceName,
    PISOCH_SET_CHANNEL_BANDWIDTH    isochSetChannelBandwidth
    );

ULONG
CAMAPI
IsochStop(
    HWND            hWnd,
    PSTR            szDeviceName,
    PISOCH_STOP     isochStop
    );

ULONG
CAMAPI
IsochTalk(
    HWND            hWnd,
    PSTR            szDeviceName,
    PISOCH_TALK     isochTalk
    );

// 1394main.c

void
CAMAPI
ResetCameraState(
    PSTR szDeviceName
	);

DWORD
CAMAPI
ReadRegister(
	PSTR szDeviceName,
	ULONG ulOffset,
	PUCHAR bytes
	);

DWORD
CAMAPI
ReadRegisterUL(
	PSTR szDeviceName,
	ULONG ulOffset,
	PULONG pData
	);

DWORD
CAMAPI
WriteRegister(
	PSTR szDeviceName,
	ULONG ulOffset,
	PUCHAR bytes
	);

DWORD 
CAMAPI
WriteRegisterUL(
	PSTR szDeviceName,
	ULONG ulOffset,
	ULONG data
	);


DWORD
CAMAPI
GetModelName(
	PSTR szDeviceName,
	PSTR buffer,
	ULONG buflen
	);

DWORD
CAMAPI
GetVendorName(
	PSTR szDeviceName,
	PSTR buffer,
	ULONG buflen
	);

DWORD
CAMAPI
GetUniqueID(
	PSTR szDeviceName,
	PLARGE_INTEGER pliUniqueID
	);

DWORD
CAMAPI
GetCameraSpecification( 
	PSTR szDeviceName, 
	PCAMERA_SPECIFICATION pSpec
	);

DWORD
CAMAPI
GetCmdrState(
	PSTR szDeviceName
	);

DWORD
CAMAPI
ResetCmdrState(
	PSTR szDeviceName 
	);

DWORD
CAMAPI
SetCmdrTraceLevel(
	PSTR szDeviceName, 
	DWORD nlevel
	);

DWORD
CAMAPI
GetDeviceList(
    PDEVICE_DATA    DeviceData
    );

ULONG
CAMAPI
GetCmdrVersion(
    HWND            hWnd,
    PSTR            szDeviceName,
    PVERSION_DATA   Version,
    BOOL            bMatch
    );

// getmaxspeed was moved from 1394camapi.c into 1394main.c because it was the only function we use from that file

ULONG
CAMAPI
GetMaxSpeedBetweenDevices(
    HWND                            hWnd,
    PSTR                            szDeviceName,
    PGET_MAX_SPEED_BETWEEN_DEVICES  GetMaxSpeedBetweenDevices
    );

// Opendevice moved out of util.c for similar reasons

HANDLE
CAMAPI
OpenDevice(
    PSTR    szDeviceName,
    BOOL    bOverLapped
    );

// debug.c

void 
CAMAPI
SetDllTraceLevel(int nlevel);


// cap off the extern "C"

#ifdef __cplusplus
}
#endif

#endif /* __1394_CAMAPI_H__ */

