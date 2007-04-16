/*++

Copyright (c) 1999  Microsoft Corporation

Module Name: 

    1394common.h

Abstract

    The common header file for 1394camera.dll and 1394cmdr.sys
	Contains Control Codes and Associated Structures

Dependents:
    1394camera.dll: included from 1394camapi.h, do *not* include directly
	1394cmdr.sys: include after 1394.h

Author:
    Kashif Hasan (khasan) 3/18/01

Revision History:
Date     Who       What
-------- --------- ------------------------------------------------------------
7/13/99  pbinder   birth (parts taken from 1394diag.h)
3/18/01  khasan    removed 1394vdev/1394diag internal information
8/18/02  cbaker    put the structs back in so this can be the only shared header

--*/


#ifndef _1394_COMMON_H_
#define _1394_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

// {F390415A-2EAF-4fd4-ACCC-3D17D38F2898}
DEFINE_GUID(GUID_1394CMDR, 0xf390415a, 0x2eaf, 0x4fd4, 0xac, 0xcc, 0x3d, 0x17, 0xd3, 0x8f, 0x28, 0x98);
#define GUID_1394CMDR_STR                   "F390415A-2EAF-4fd4-ACCC-3D17D38F2898"

//
// these guys are meant to be called from a ring 3 app
// call through the port device object
//
#define IOCTL_1394_TOGGLE_ENUM_TEST_ON          CTL_CODE( \
                                                FILE_DEVICE_UNKNOWN, \
                                                0x88, \
                                                METHOD_BUFFERED, \
                                                FILE_ANY_ACCESS \
                                                )

#define IOCTL_1394_TOGGLE_ENUM_TEST_OFF         CTL_CODE( \
                                                FILE_DEVICE_UNKNOWN, \
                                                0x89, \
                                                METHOD_BUFFERED, \
                                                FILE_ANY_ACCESS \
                                                )

//
// IOCTL info, needs to be visible for application
//
#define DIAG1394_IOCTL_INDEX                            0x0800
#define CMDR1394_IOCTL_INDEX DIAG1394_IOCTL_INDEX

#define IOCTL_ALLOCATE_ADDRESS_RANGE                    CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 0,       \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_FREE_ADDRESS_RANGE                        CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 1,       \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ASYNC_READ                                CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 2,       \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ASYNC_WRITE                               CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 3,       \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ASYNC_LOCK                                CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 4,       \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_ALLOCATE_BANDWIDTH                  CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 5,       \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_ALLOCATE_CHANNEL                    CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 6,       \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_ALLOCATE_RESOURCES                  CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 7,       \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_ATTACH_BUFFERS                      CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 8,       \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_DETACH_BUFFERS                      CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 9,       \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_FREE_BANDWIDTH                      CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 10,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_FREE_CHANNEL                        CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 11,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_FREE_RESOURCES                      CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 12,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_LISTEN                              CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 13,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_QUERY_CURRENT_CYCLE_TIME            CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 14,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_QUERY_RESOURCES                     CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 15,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_SET_CHANNEL_BANDWIDTH               CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 16,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_STOP                                CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 17,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ISOCH_TALK                                CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 18,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_GET_LOCAL_HOST_INFORMATION                CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 19,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_GET_1394_ADDRESS_FROM_DEVICE_OBJECT       CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 20,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_CONTROL                                   CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 21,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_GET_MAX_SPEED_BETWEEN_DEVICES             CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 22,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_SET_DEVICE_XMIT_PROPERTIES                CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 23,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_GET_CONFIGURATION_INFORMATION             CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 24,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_BUS_RESET                                 CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 25,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_GET_GENERATION_COUNT                      CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 26,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_SEND_PHY_CONFIGURATION_PACKET             CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 27,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_BUS_RESET_NOTIFICATION                    CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 28,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ASYNC_STREAM                              CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 29,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_SET_LOCAL_HOST_INFORMATION                CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 30,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_SET_ADDRESS_DATA                          CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 40,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_GET_ADDRESS_DATA                          CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 41,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_BUS_RESET_NOTIFY                          CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 50,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)
                                                        
#define IOCTL_GET_DIAG_VERSION                          CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        DIAG1394_IOCTL_INDEX + 51,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_GET_CMDR_STATE                            CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        CMDR1394_IOCTL_INDEX + 52,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_RESET_CMDR_STATE                          CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        CMDR1394_IOCTL_INDEX + 53,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_SET_CMDR_TRACELEVEL                       CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        CMDR1394_IOCTL_INDEX + 54,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_READ_REGISTER								CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        CMDR1394_IOCTL_INDEX + 55,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_WRITE_REGISTER							CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        CMDR1394_IOCTL_INDEX + 56,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_GET_MODEL_NAME							CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        CMDR1394_IOCTL_INDEX + 57,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_GET_VENDOR_NAME							CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        CMDR1394_IOCTL_INDEX + 58,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_GET_CAMERA_SPECIFICATION					CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        CMDR1394_IOCTL_INDEX + 59,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_GET_CAMERA_UNIQUE_ID						CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        CMDR1394_IOCTL_INDEX + 60,      \
                                                        METHOD_BUFFERED,                \
                                                        FILE_ANY_ACCESS)

#define IOCTL_ATTACH_BUFFER								CTL_CODE( FILE_DEVICE_UNKNOWN,  \
                                                        CMDR1394_IOCTL_INDEX + 61,      \
                                                        METHOD_IN_DIRECT,                \
                                                        FILE_ANY_ACCESS)

// now for the structures that go with the control codes, to make this a truly "common" header file
//
// struct used to pass in with IOCTL_ALLOCATE_ADDRESS_RANGE
//
typedef struct _ALLOCATE_ADDRESS_RANGE {
    ULONG           fulAllocateFlags;
    ULONG           fulFlags;
    ULONG           nLength;
    ULONG           MaxSegmentSize;
    ULONG           fulAccessType;
    ULONG           fulNotificationOptions;
    ADDRESS_OFFSET  Required1394Offset;
    HANDLE          hAddressRange;
    UCHAR           Data[1];
} ALLOCATE_ADDRESS_RANGE, *PALLOCATE_ADDRESS_RANGE;

//
// struct used to pass in with IOCTL_ASYNC_READ
//
typedef struct _ASYNC_READ {
    ULONG           bRawMode;
    ULONG           bGetGeneration;
    IO_ADDRESS      DestinationAddress;
    ULONG           nNumberOfBytesToRead;
    ULONG           nBlockSize;
    ULONG           fulFlags;
    ULONG           ulGeneration;
    UCHAR           Data[4];
} ASYNC_READ, *PASYNC_READ;

//
// struct used to pass in with IOCTL_SET_ADDRESS_DATA
//
typedef struct _SET_ADDRESS_DATA {
    HANDLE          hAddressRange;
    ULONG           nLength;
    ULONG           ulOffset;
    UCHAR           Data[1];
} SET_ADDRESS_DATA, *PSET_ADDRESS_DATA,GET_ADDRESS_DATA, *PGET_ADDRESS_DATA;

//
// struct used to pass in with IOCTL_ASYNC_WRITE
//
typedef struct _ASYNC_WRITE {
    ULONG           bRawMode;
    ULONG           bGetGeneration;
    IO_ADDRESS      DestinationAddress;
    ULONG           nNumberOfBytesToWrite;
    ULONG           nBlockSize;
    ULONG           fulFlags;
    ULONG           ulGeneration;
    UCHAR           Data[4];
} ASYNC_WRITE, *PASYNC_WRITE;

//
// struct used to pass in with IOCTL_ASYNC_LOCK
//
typedef struct _ASYNC_LOCK {
    ULONG           bRawMode;
    ULONG           bGetGeneration;
    IO_ADDRESS      DestinationAddress;
    ULONG           nNumberOfArgBytes;
    ULONG           nNumberOfDataBytes;
    ULONG           fulTransactionType;
    ULONG           fulFlags;
    ULONG           Arguments[2];
    ULONG           DataValues[2];
    ULONG           ulGeneration;
    ULONG           Buffer[2];
} ASYNC_LOCK, *PASYNC_LOCK;

//
// struct used to pass in with IOCTL_ASYNC_STREAM
//
typedef struct _ASYNC_STREAM {
    ULONG           nNumberOfBytesToStream;
    ULONG           fulFlags;
    ULONG           ulTag;
    ULONG           nChannel;
    ULONG           ulSynch;
    ULONG           nSpeed;
    UCHAR           Data[1];
} ASYNC_STREAM, *PASYNC_STREAM;

//
// struct used to pass in with IOCTL_ISOCH_ALLOCATE_BANDWIDTH
//
typedef struct _ISOCH_ALLOCATE_BANDWIDTH {
    ULONG           nMaxBytesPerFrameRequested;
    ULONG           fulSpeed;
    HANDLE          hBandwidth;
    ULONG           BytesPerFrameAvailable;
    ULONG           SpeedSelected;
} ISOCH_ALLOCATE_BANDWIDTH, *PISOCH_ALLOCATE_BANDWIDTH;

//
// struct used to pass in with IOCTL_ISOCH_ALLOCATE_CHANNEL
//
typedef struct _ISOCH_ALLOCATE_CHANNEL {
    ULONG           nRequestedChannel;
    ULONG           Channel;
    LARGE_INTEGER   ChannelsAvailable;
} ISOCH_ALLOCATE_CHANNEL, *PISOCH_ALLOCATE_CHANNEL;

//
// struct used to pass in with IOCTL_ISOCH_ALLOCATE_RESOURCES
//
typedef struct _ISOCH_ALLOCATE_RESOURCES {
    ULONG           fulSpeed;
    ULONG           fulFlags;
    ULONG           nChannel;
    ULONG           nMaxBytesPerFrame;
    ULONG           nNumberOfBuffers;
    ULONG           nMaxBufferSize;
    ULONG           nQuadletsToStrip;
    HANDLE          hResource;
} ISOCH_ALLOCATE_RESOURCES, *PISOCH_ALLOCATE_RESOURCES;

//
// struct used to pass in isoch descriptors
//
typedef struct _RING3_ISOCH_DESCRIPTOR {
    ULONG           fulFlags;
    ULONG           ulLength;
    ULONG           nMaxBytesPerFrame;
    ULONG           ulSynch;
    ULONG           ulTag;
    CYCLE_TIME      CycleTime;
    ULONG           bUseCallback;
    ULONG           bAutoDetach;
    UCHAR           Data[1];
} RING3_ISOCH_DESCRIPTOR, *PRING3_ISOCH_DESCRIPTOR;

//
// struct used to pass in with IOCTL_ISOCH_ATTACH_BUFFERS
// also used by IOCTL_ATTACH_BUFFER (the new one)
//
typedef struct _ISOCH_ATTACH_BUFFERS {
    HANDLE                  hResource;
    ULONG                   nNumberOfDescriptors;
    ULONG                   ulBufferSize;
    ULONG                   pIsochDescriptor;
    RING3_ISOCH_DESCRIPTOR  R3_IsochDescriptor[1];
} ISOCH_ATTACH_BUFFERS, *PISOCH_ATTACH_BUFFERS;

//
// struct used to pass in with IOCTL_ISOCH_DETACH_BUFFERS
//
typedef struct _ISOCH_DETACH_BUFFERS {
    HANDLE          hResource;
    ULONG           nNumberOfDescriptors;
    ULONG           pIsochDescriptor;
} ISOCH_DETACH_BUFFERS, *PISOCH_DETACH_BUFFERS;

//
// struct used to pass in with IOCTL_ISOCH_LISTEN
//
typedef struct _ISOCH_LISTEN {
    HANDLE          hResource;
    ULONG           fulFlags;
    CYCLE_TIME      StartTime;
} ISOCH_LISTEN, *PISOCH_LISTEN;

//
// struct used to pass in with IOCTL_ISOCH_QUERY_RESOURCES
//
typedef struct _ISOCH_QUERY_RESOURCES {
    ULONG           fulSpeed;
    ULONG           BytesPerFrameAvailable;
    LARGE_INTEGER   ChannelsAvailable;
} ISOCH_QUERY_RESOURCES, *PISOCH_QUERY_RESOURCES;

//
// struct used to pass in with IOCTL_ISOCH_SET_CHANNEL_BANDWIDTH
//
typedef struct _ISOCH_SET_CHANNEL_BANDWIDTH {
    HANDLE          hBandwidth;
    ULONG           nMaxBytesPerFrame;
} ISOCH_SET_CHANNEL_BANDWIDTH, *PISOCH_SET_CHANNEL_BANDWIDTH;

//
// struct used to pass in with IOCTL_ISOCH_STOP
//
typedef struct _ISOCH_STOP {
    HANDLE          hResource;
    ULONG           fulFlags;
} ISOCH_STOP, *PISOCH_STOP;

//
// struct used to pass in with IOCTL_ISOCH_TALK
//
typedef struct _ISOCH_TALK {
    HANDLE          hResource;
    ULONG           fulFlags;
    CYCLE_TIME      StartTime;
} ISOCH_TALK, *PISOCH_TALK;

//
// struct used to pass in with IOCTL_GET_LOCAL_HOST_INFORMATION
//
typedef struct _GET_LOCAL_HOST_INFORMATION {
    ULONG           Status;
    ULONG           nLevel;
    ULONG           ulBufferSize;
    UCHAR           Information[1];
} GET_LOCAL_HOST_INFORMATION, *PGET_LOCAL_HOST_INFORMATION;

//
// struct used to pass in with IOCTL_GET_1394_ADDRESS_FROM_DEVICE_OBJECT
//
typedef struct _GET_1394_ADDRESS {
    ULONG           fulFlags;
    NODE_ADDRESS    NodeAddress;
} GET_1394_ADDRESS, *PGET_1394_ADDRESS;

//
// struct used to pass in with IOCTL_GET_MAX_SPEED_BETWEEN_DEVICES
//
typedef struct _GET_MAX_SPEED_BETWEEN_DEVICES {
    ULONG           fulFlags;
    ULONG           ulNumberOfDestinations;
    ULONG           hDestinationDeviceObjects[64];
    ULONG           fulSpeed;
} GET_MAX_SPEED_BETWEEN_DEVICES, *PGET_MAX_SPEED_BETWEEN_DEVICES;

//
// struct used to pass in with IOCTL_SET_DEVICE_XMIT_PROPERTIES
//
typedef struct _DEVICE_XMIT_PROPERTIES {
    ULONG           fulSpeed;
    ULONG           fulPriority;
} DEVICE_XMIT_PROPERTIES, *PDEVICE_XMIT_PROPERTIES;

//
// struct used to pass in with IOCTL_SET_LOCAL_HOST_INFORMATION
//
typedef struct _SET_LOCAL_HOST_INFORMATION {
    ULONG           nLevel;
    ULONG           ulBufferSize;
    UCHAR           Information[1];
} SET_LOCAL_HOST_INFORMATION, *PSET_LOCAL_HOST_INFORMATION;

//
// define's used to make sure the dll/sys driver are in synch
//
#define DIAGNOSTIC_VERSION                      1
#define DIAGNOSTIC_SUB_VERSION                  1

//
// struct used to pass in with IOCTL_GET_CMDR_VERSION
//
typedef struct _VERSION_DATA {
    ULONG           ulVersion;
    ULONG           ulSubVersion;
} VERSION_DATA, *PVERSION_DATA;

//
// struct for use with reading/writing registers
//

typedef struct _REGISTER_IOBUF
{
	ULONG		ulOffset;
	UCHAR		data[4];
} REGISTER_IOBUF, *PREGISTER_IOBUF;	

//
// struct used to get camera specification information
//

typedef struct _CAMERA_SPECIFICATION
{
	ULONG		ulSpecification;
	ULONG		ulVersion;
} CAMERA_SPECIFICATION, *PCAMERA_SPECIFICATION;


#ifdef __cplusplus
}
#endif

#endif // #ifndef _1394_COMMON_H_


