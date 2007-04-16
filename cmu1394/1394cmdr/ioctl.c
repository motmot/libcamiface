/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: 

    ioctl.c

Abstract


Author:

    Peter Binder (pbinder) 4/13/98

Revision History:
Date     Who       What
-------- --------- ------------------------------------------------------------
4/13/98  pbinder   taken from 1394diag/ohcidiag
--*/

#include "pch.h"

NTSTATUS
t1394Diag_IoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION      IrpSp;
    PDEVICE_EXTENSION       deviceExtension;
    PVOID                   ioBuffer;
    ULONG                   inputBufferLength;
    ULONG                   outputBufferLength;
    ULONG                   ioControlCode;
    
    ENTER("t1394Diag_IoControl");

    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    // Get a pointer to the device extension
    deviceExtension = DeviceObject->DeviceExtension;

    // Get the pointer to the input/output buffer and it's length
    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    // make sure our device isn't in shutdown mode...
    if (deviceExtension->bShutdown) {

        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        ntStatus = STATUS_NO_SUCH_DEVICE;
        return(ntStatus);
    }

    TRACE(TL_TRACE, ("Irp = 0x%x\n", Irp));

    switch (IrpSp->MajorFunction) {

        case IRP_MJ_DEVICE_CONTROL:
            TRACE(TL_TRACE, ("t1394Diag_IoControl: IRP_MJ_DEVICE_CONTROL\n"));

            ioControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

            switch (ioControlCode) {
/*
                case IOCTL_ALLOCATE_ADDRESS_RANGE:
                    {
                        PALLOCATE_ADDRESS_RANGE     AllocateAddressRange;

                        TRACE(TL_TRACE, ("IOCTL_ALLOCATE_ADDRESS_RANGE\n"));

                        if ((inputBufferLength < sizeof(ALLOCATE_ADDRESS_RANGE)) ||
                            (outputBufferLength < sizeof(ALLOCATE_ADDRESS_RANGE))) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            AllocateAddressRange = (PALLOCATE_ADDRESS_RANGE)ioBuffer;

                            ntStatus = t1394_AllocateAddressRange( DeviceObject,
                                                                       Irp,
                                                                       AllocateAddressRange->fulAllocateFlags,
                                                                       AllocateAddressRange->fulFlags,
                                                                       AllocateAddressRange->nLength,
                                                                       AllocateAddressRange->MaxSegmentSize,
                                                                       AllocateAddressRange->fulAccessType,
                                                                       AllocateAddressRange->fulNotificationOptions,
                                                                       &AllocateAddressRange->Required1394Offset,
                                                                       &AllocateAddressRange->hAddressRange,
                                                                       (PULONG)&AllocateAddressRange->Data
                                                                       );

                            if (NT_SUCCESS(ntStatus))
                                Irp->IoStatus.Information = outputBufferLength;
                        }
                    }
                    break; // IOCTL_ALLOCATE_ADDRESS_RANGE

                case IOCTL_FREE_ADDRESS_RANGE:
                    TRACE(TL_TRACE, ("IOCTL_FREE_ADDRESS_RANGE\n"));

                    if (inputBufferLength < sizeof(HANDLE)) {

                        ntStatus = STATUS_BUFFER_TOO_SMALL;
                    }
                    else {

                        ntStatus = t1394_FreeAddressRange( DeviceObject,
                                                               Irp,
                                                               *(PHANDLE)ioBuffer
                                                               );
                    }
                    break; // IOCTL_FREE_ADDRESS_RANGE

                case IOCTL_ASYNC_READ:
                    {
                        PASYNC_READ     AsyncRead;

                        TRACE(TL_TRACE, ("IOCTL_ASYNC_READ\n"));

                        if (inputBufferLength < sizeof(ASYNC_READ)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            AsyncRead = (PASYNC_READ)ioBuffer;

                            if ((outputBufferLength < sizeof(ASYNC_READ)) || 
                                (outputBufferLength-sizeof(ASYNC_READ) < AsyncRead->nNumberOfBytesToRead)) {

                                ntStatus = STATUS_BUFFER_TOO_SMALL;
                            }
                            else {

                                ntStatus = t1394_AsyncRead( DeviceObject,
                                                                Irp,
                                                                AsyncRead->bRawMode,
                                                                AsyncRead->bGetGeneration,
                                                                AsyncRead->DestinationAddress,
                                                                AsyncRead->nNumberOfBytesToRead,
                                                                AsyncRead->nBlockSize,
                                                                AsyncRead->fulFlags,
                                                                AsyncRead->ulGeneration,
                                                                (PULONG)&AsyncRead->Data
                                                                );

                                if (NT_SUCCESS(ntStatus))
                                    Irp->IoStatus.Information = outputBufferLength;
                            }
                        }
                    }
                    break; // IOCTL_ASYNC_READ

                case IOCTL_ASYNC_WRITE:
                    {
                        PASYNC_WRITE    AsyncWrite;

                        TRACE(TL_TRACE, ("IOCTL_ASYNC_WRITE\n"));

                        if (inputBufferLength < sizeof(ASYNC_WRITE)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            AsyncWrite = (PASYNC_WRITE)ioBuffer;

                            if (inputBufferLength-sizeof(ASYNC_WRITE) < AsyncWrite->nNumberOfBytesToWrite) {

                                ntStatus = STATUS_BUFFER_TOO_SMALL;
                            }
                            else {

                                ntStatus = t1394_AsyncWrite( DeviceObject,
                                                                 Irp,
                                                                 AsyncWrite->bRawMode,
                                                                 AsyncWrite->bGetGeneration,
                                                                 AsyncWrite->DestinationAddress,
                                                                 AsyncWrite->nNumberOfBytesToWrite,
                                                                 AsyncWrite->nBlockSize,
                                                                 AsyncWrite->fulFlags,
                                                                 AsyncWrite->ulGeneration,
                                                                 (PULONG)&AsyncWrite->Data
                                                                 );
                            }
                        }
                    }
                    break; // IOCTL_ASYNC_WRITE

                case IOCTL_ASYNC_LOCK:
                    {
                        PASYNC_LOCK     AsyncLock;

                        TRACE(TL_TRACE, ("IOCTL_ASYNC_LOCK\n"));

                        if ((inputBufferLength < sizeof(ASYNC_LOCK)) ||
                            (outputBufferLength < sizeof(ASYNC_LOCK))) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            AsyncLock = (PASYNC_LOCK)ioBuffer;

                            ntStatus = t1394_AsyncLock( DeviceObject,
                                                            Irp,
                                                            AsyncLock->bRawMode,
                                                            AsyncLock->bGetGeneration,
                                                            AsyncLock->DestinationAddress,
                                                            AsyncLock->nNumberOfArgBytes,
                                                            AsyncLock->nNumberOfDataBytes,
                                                            AsyncLock->fulTransactionType,
                                                            AsyncLock->fulFlags,
                                                            AsyncLock->Arguments,
                                                            AsyncLock->DataValues,
                                                            AsyncLock->ulGeneration,
                                                            (PVOID)&AsyncLock->Buffer
                                                            );

                            if (NT_SUCCESS(ntStatus))
                                Irp->IoStatus.Information = outputBufferLength;
                        }
                    }
                    break; // IOCTL_ASYNC_LOCK

                case IOCTL_ASYNC_STREAM:
                    {
                        PASYNC_STREAM   AsyncStream;

                        TRACE(TL_TRACE, ("IOCTL_ASYNC_STREAM\n"));

                        if (inputBufferLength < sizeof(ASYNC_STREAM)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            AsyncStream = (PASYNC_STREAM)ioBuffer;

                            if (outputBufferLength < sizeof(ASYNC_STREAM)+AsyncStream->nNumberOfBytesToStream) {

                                ntStatus = STATUS_BUFFER_TOO_SMALL;
                            }
                            else {

                                ntStatus = t1394_AsyncStream( DeviceObject,
                                                                  Irp,
                                                                  AsyncStream->nNumberOfBytesToStream,
                                                                  AsyncStream->fulFlags,
                                                                  AsyncStream->ulTag,
                                                                  AsyncStream->nChannel,
                                                                  AsyncStream->ulSynch,
                                                                  (UCHAR)AsyncStream->nSpeed,
                                                                  (PULONG)&AsyncStream->Data
                                                                  );

                                if (NT_SUCCESS(ntStatus))
                                    Irp->IoStatus.Information = outputBufferLength;
                            }
                        }
                    }
                    break; // IOCTL_ASYNC_STREAM
*/
                case IOCTL_ISOCH_ALLOCATE_BANDWIDTH:
                    {
                        PISOCH_ALLOCATE_BANDWIDTH   IsochAllocateBandwidth;

                        TRACE(TL_TRACE, ("IOCTL_ISOCH_ALLOCATE_BANDWIDTH\n"));

                        if ((inputBufferLength < sizeof(ISOCH_ALLOCATE_BANDWIDTH)) ||
                            (outputBufferLength < sizeof(ISOCH_ALLOCATE_BANDWIDTH))) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            IsochAllocateBandwidth = (PISOCH_ALLOCATE_BANDWIDTH)ioBuffer;

                            ntStatus = t1394_IsochAllocateBandwidth( DeviceObject,
                                                                         Irp,
                                                                         IsochAllocateBandwidth->nMaxBytesPerFrameRequested,
                                                                         IsochAllocateBandwidth->fulSpeed,
                                                                         &IsochAllocateBandwidth->hBandwidth,
                                                                         &IsochAllocateBandwidth->BytesPerFrameAvailable,
                                                                         &IsochAllocateBandwidth->SpeedSelected
                                                                         );

                            if (NT_SUCCESS(ntStatus))
                                Irp->IoStatus.Information = outputBufferLength;
                        }
                    }
                    break; // IOCTL_ISOCH_ALLOCATE_BANDWIDTH

                case IOCTL_ISOCH_ALLOCATE_CHANNEL:
                    {
                        PISOCH_ALLOCATE_CHANNEL     IsochAllocateChannel;

                        TRACE(TL_TRACE, ("IOCTL_ISOCH_ALLOCATE_CHANNEL\n"));

                        if ((inputBufferLength < sizeof(ISOCH_ALLOCATE_CHANNEL)) ||
                            (outputBufferLength < sizeof(ISOCH_ALLOCATE_CHANNEL))) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            IsochAllocateChannel = (PISOCH_ALLOCATE_CHANNEL)ioBuffer;

                            ntStatus = t1394_IsochAllocateChannel( DeviceObject,
                                                                       Irp,
                                                                       IsochAllocateChannel->nRequestedChannel,
                                                                       &IsochAllocateChannel->Channel,
                                                                       &IsochAllocateChannel->ChannelsAvailable
                                                                       );

                        if (NT_SUCCESS(ntStatus))
                            Irp->IoStatus.Information = outputBufferLength;
                        }
                    }
                    break; // IOCTL_ISOCH_ALLOCATE_CHANNEL

                case IOCTL_ISOCH_ALLOCATE_RESOURCES:
                    {
                        PISOCH_ALLOCATE_RESOURCES   IsochAllocateResources;

                        TRACE(TL_TRACE, ("IOCTL_ISOCH_ALLOCATE_RESOURCES\n"));

                        if ((inputBufferLength < sizeof(ISOCH_ALLOCATE_RESOURCES)) ||
                            (outputBufferLength < sizeof(ISOCH_ALLOCATE_RESOURCES))) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            IsochAllocateResources = (PISOCH_ALLOCATE_RESOURCES)ioBuffer;

                            ntStatus = t1394_IsochAllocateResources( DeviceObject,
                                                                         Irp,
                                                                         IsochAllocateResources->fulSpeed,
                                                                         IsochAllocateResources->fulFlags,
                                                                         IsochAllocateResources->nChannel,
                                                                         IsochAllocateResources->nMaxBytesPerFrame,
                                                                         IsochAllocateResources->nNumberOfBuffers,
                                                                         IsochAllocateResources->nMaxBufferSize,
                                                                         IsochAllocateResources->nQuadletsToStrip,
                                                                         &IsochAllocateResources->hResource
                                                                         );

                            if (NT_SUCCESS(ntStatus))
                                Irp->IoStatus.Information = outputBufferLength;
                        }
                    }
                    break; // IOCTL_ISOCH_ALLOCATE_RESOURCES
/*
                case IOCTL_ISOCH_ATTACH_BUFFERS:
                    {
                        PISOCH_ATTACH_BUFFERS       IsochAttachBuffers;

                        TRACE(TL_TRACE, ("IOCTL_ISOCH_ATTACH_BUFFERS\n"));

                        if (inputBufferLength < (sizeof(ISOCH_ATTACH_BUFFERS)+sizeof(RING3_ISOCH_DESCRIPTOR))) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            IsochAttachBuffers = (PISOCH_ATTACH_BUFFERS)ioBuffer;

                            if (outputBufferLength < (sizeof(ISOCH_ATTACH_BUFFERS) + 
                                (sizeof(RING3_ISOCH_DESCRIPTOR)*IsochAttachBuffers->nNumberOfDescriptors))) {

                                ntStatus = STATUS_BUFFER_TOO_SMALL;
                            }
                            else {

                                ntStatus = t1394_IsochAttachBuffers( DeviceObject,
                                                                         Irp,
                                                                         outputBufferLength,
                                                                         IsochAttachBuffers->hResource,
                                                                         IsochAttachBuffers->nNumberOfDescriptors,
                                                                         (PISOCH_DESCRIPTOR)&IsochAttachBuffers->pIsochDescriptor,
                                                                         (PRING3_ISOCH_DESCRIPTOR)&IsochAttachBuffers->R3_IsochDescriptor
                                                                         );

                                if (ntStatus == STATUS_PENDING)
                                    goto t1394Diag_IoControlExit;
                            }
                        }
                    }
                    break; // IOCTL_ISOCH_ATTACH_BUFFERS

                case IOCTL_ISOCH_DETACH_BUFFERS:
                    {
                        PISOCH_DETACH_BUFFERS       IsochDetachBuffers;

                        TRACE(TL_TRACE, ("IOCTL_ISOCH_DETACH_BUFFERS\n"));

                        if (inputBufferLength < sizeof(ISOCH_DETACH_BUFFERS)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            IsochDetachBuffers = (PISOCH_DETACH_BUFFERS)ioBuffer;

                            ntStatus = t1394_IsochDetachBuffers( DeviceObject,
                                                                     Irp,
                                                                     IsochDetachBuffers->hResource,
                                                                     IsochDetachBuffers->nNumberOfDescriptors,
                                                                     (PISOCH_DESCRIPTOR)&IsochDetachBuffers->pIsochDescriptor
                                                                     );
                        }
                    }

                    break; // IOCTL_ISOCH_DETACH_BUFFERS
*/
                case IOCTL_ISOCH_FREE_BANDWIDTH:
                    {
                        TRACE(TL_TRACE, ("IOCTL_ISOCH_FREE_BANDWIDTH\n"));

                        if (inputBufferLength < sizeof(HANDLE)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            ntStatus = t1394_IsochFreeBandwidth( DeviceObject,
                                                                     Irp,
                                                                     *(PHANDLE)ioBuffer
                                                                     );
                        }
                    }
                    break; // IOCTL_ISOCH_FREE_BANDWIDTH
  
                case IOCTL_ISOCH_FREE_CHANNEL:
                    {
                        TRACE(TL_TRACE, ("IOCTL_ISOCH_FREE_CHANNEL\n"));

                        if (inputBufferLength < sizeof(ULONG)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            ntStatus = t1394_IsochFreeChannel( DeviceObject,
                                                                   Irp,
                                                                   *(PULONG)ioBuffer
                                                                   );
                        }
                    }
                    break; // IOCTL_ISOCH_FREE_CHANNEL
  
                case IOCTL_ISOCH_FREE_RESOURCES:
                    {
                        TRACE(TL_TRACE, ("IOCTL_ISOCH_FREE_RESOURCES\n"));

                        if (inputBufferLength < sizeof(HANDLE)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            ntStatus = t1394_IsochFreeResources( DeviceObject,
                                                                     Irp,
                                                                     *(PHANDLE)ioBuffer
                                                                     );
                        }
                    }
                    break; // IOCTL_ISOCH_FREE_RESOURCES

                case IOCTL_ISOCH_LISTEN:
                    {
                        PISOCH_LISTEN       IsochListen;

                        TRACE(TL_TRACE, ("IOCTL_ISOCH_LISTEN\n"));

                        if (inputBufferLength < sizeof(ISOCH_LISTEN)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            IsochListen = (PISOCH_LISTEN)ioBuffer;

                            ntStatus = t1394_IsochListen( DeviceObject,
                                                              Irp,
                                                              IsochListen->hResource,
                                                              IsochListen->fulFlags,
                                                              IsochListen->StartTime
                                                              );
                        }
                    }
                    break; // IOCTL_ISOCH_LISTEN

                case IOCTL_ISOCH_QUERY_CURRENT_CYCLE_TIME:
                    {
                        TRACE(TL_TRACE, ("IOCTL_ISOCH_QUERY_CURRENT_CYCLE_TIME\n"));

                        if (outputBufferLength < sizeof(CYCLE_TIME)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            ntStatus = t1394_IsochQueryCurrentCycleTime( DeviceObject,
                                                                             Irp,
                                                                             (PCYCLE_TIME)ioBuffer
                                                                             );

                            if (NT_SUCCESS(ntStatus))
                                Irp->IoStatus.Information = outputBufferLength;
                        }
                    }
                    break; // IOCTL_ISOCH_QUERY_CURRENT_CYCLE_TIME

                case IOCTL_ISOCH_QUERY_RESOURCES:
                    {
                        PISOCH_QUERY_RESOURCES      IsochQueryResources;

                        TRACE(TL_TRACE, ("IOCTL_ISOCH_QUERY_RESOURCES\n"));

                        if ((inputBufferLength < sizeof(ISOCH_QUERY_RESOURCES)) ||
                            (outputBufferLength < sizeof(ISOCH_QUERY_RESOURCES))) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            IsochQueryResources = (PISOCH_QUERY_RESOURCES)ioBuffer;

                            ntStatus = t1394_IsochQueryResources( DeviceObject,
                                                                      Irp,
                                                                      IsochQueryResources->fulSpeed,
                                                                      &IsochQueryResources->BytesPerFrameAvailable,
                                                                      &IsochQueryResources->ChannelsAvailable
                                                                      );

                            if (NT_SUCCESS(ntStatus))
                                Irp->IoStatus.Information = outputBufferLength;
                        }
                    }
                    break; // IOCTL_ISOCH_QUERY_RESOURCES

                case IOCTL_ISOCH_SET_CHANNEL_BANDWIDTH:
                    {
                        PISOCH_SET_CHANNEL_BANDWIDTH    IsochSetChannelBandwidth;

                        TRACE(TL_TRACE, ("IOCTL_ISOCH_SET_CHANNEL_BANDWIDTH\n"));

                        if (inputBufferLength < sizeof(ISOCH_SET_CHANNEL_BANDWIDTH)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            IsochSetChannelBandwidth = (PISOCH_SET_CHANNEL_BANDWIDTH)ioBuffer;

                            ntStatus = t1394_IsochSetChannelBandwidth( DeviceObject,
                                                                           Irp,
                                                                           IsochSetChannelBandwidth->hBandwidth,
                                                                           IsochSetChannelBandwidth->nMaxBytesPerFrame
                                                                           );
                        }
                    }
                    break; // IOCTL_ISOCH_SET_CHANNEL_BANDWIDTH

                case IOCTL_ISOCH_STOP:
                    {
                        PISOCH_STOP     IsochStop;

                        TRACE(TL_TRACE, ("IOCTL_ISOCH_STOP\n"));

                        if (inputBufferLength < sizeof(ISOCH_STOP)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            IsochStop = (PISOCH_STOP)ioBuffer;

                            ntStatus = t1394_IsochStop( DeviceObject,
                                                            Irp,
                                                            IsochStop->hResource,
                                                            IsochStop->fulFlags
                                                            );
                        }
                    }
                    break; // IOCTL_ISOCH_STOP

                case IOCTL_ISOCH_TALK:
                    {
                        PISOCH_TALK     IsochTalk;

                        TRACE(TL_TRACE, ("IOCTL_ISOCH_TALK\n"));

                        if (inputBufferLength < sizeof(ISOCH_TALK)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            IsochTalk = (PISOCH_TALK)ioBuffer;

                            ntStatus = t1394_IsochTalk( DeviceObject,
                                                            Irp,
                                                            IsochTalk->hResource,
                                                            IsochTalk->fulFlags,
                                                            IsochTalk->StartTime
                                                            );
                        }
                    }
                    break; // IOCTL_ISOCH_TALK

                case IOCTL_GET_LOCAL_HOST_INFORMATION:
                    {
                        PGET_LOCAL_HOST_INFORMATION     GetLocalHostInformation;

                        TRACE(TL_TRACE, ("IOCTL_GET_LOCAL_HOST_INFORMATION\n"));

                        if ((inputBufferLength < sizeof(GET_LOCAL_HOST_INFORMATION)) ||
                            (outputBufferLength < sizeof(GET_LOCAL_HOST_INFORMATION))) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            GetLocalHostInformation = (PGET_LOCAL_HOST_INFORMATION)ioBuffer;

                            ntStatus = t1394_GetLocalHostInformation( DeviceObject,
                                                                          Irp,
                                                                          GetLocalHostInformation->nLevel,
                                                                          &GetLocalHostInformation->Status,
                                                                          (PVOID)GetLocalHostInformation->Information
                                                                          );

                            if (NT_SUCCESS(ntStatus))
                                Irp->IoStatus.Information = outputBufferLength;
                        }
                    }
                    break; // IOCTL_GET_LOCAL_HOST_INFORMATION

                case IOCTL_GET_1394_ADDRESS_FROM_DEVICE_OBJECT:
                    {
                        PGET_1394_ADDRESS   Get1394Address;

                        TRACE(TL_TRACE, ("IOCTL_GET_1394_ADDRESS_FROM_DEVICE_OBJECT\n"));

                        if ((inputBufferLength < sizeof(GET_1394_ADDRESS)) ||
                            (outputBufferLength < sizeof(GET_1394_ADDRESS))) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            Get1394Address = (PGET_1394_ADDRESS)ioBuffer;

                            ntStatus = t1394_Get1394AddressFromDeviceObject( DeviceObject,
                                                                                 Irp,
                                                                                 Get1394Address->fulFlags,
                                                                                 &Get1394Address->NodeAddress
                                                                                 );

                            if (NT_SUCCESS(ntStatus))
                                Irp->IoStatus.Information = outputBufferLength;
                        }
                    }
                    break; // IOCTL_GET_1394_ADDRESS_FROM_DEVICE_OBJECT

                case IOCTL_CONTROL:
                    TRACE(TL_TRACE, ("IOCTL_CONTROL\n"));

                    ntStatus = t1394_Control( DeviceObject,
                                                  Irp
                                                  );

                    break; // IOCTL_CONTROL

                case IOCTL_GET_MAX_SPEED_BETWEEN_DEVICES:
                    {
                        PGET_MAX_SPEED_BETWEEN_DEVICES  MaxSpeedBetweenDevices;

                        TRACE(TL_TRACE, ("IOCTL_GET_MAX_SPEED_BETWEEN_DEVICES\n"));

                        if ((inputBufferLength < sizeof(GET_MAX_SPEED_BETWEEN_DEVICES)) ||
                            (outputBufferLength < sizeof(GET_MAX_SPEED_BETWEEN_DEVICES))) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            MaxSpeedBetweenDevices = (PGET_MAX_SPEED_BETWEEN_DEVICES)ioBuffer;

                            ntStatus = t1394_GetMaxSpeedBetweenDevices( DeviceObject,
                                                                            Irp,
                                                                            MaxSpeedBetweenDevices->fulFlags,
                                                                            MaxSpeedBetweenDevices->ulNumberOfDestinations,
                                                                            (PVOID)&MaxSpeedBetweenDevices->hDestinationDeviceObjects[64],
                                                                            &MaxSpeedBetweenDevices->fulSpeed
                                                                            );

                            if (NT_SUCCESS(ntStatus))
                                Irp->IoStatus.Information = outputBufferLength;
                        }
                    }                    
                    break; // IOCTL_GET_MAX_SPEED_BETWEEN_DEVICES

                case IOCTL_SET_DEVICE_XMIT_PROPERTIES:
                    {
                        PDEVICE_XMIT_PROPERTIES     DeviceXmitProperties;

                        TRACE(TL_TRACE, ("IOCTL_SET_DEVICE_XMIT_PROPERTIES\n"));

                        if (inputBufferLength < sizeof(DEVICE_XMIT_PROPERTIES)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            DeviceXmitProperties = (PDEVICE_XMIT_PROPERTIES)ioBuffer;

                            ntStatus = t1394_SetDeviceXmitProperties( DeviceObject,
                                                                          Irp,
                                                                          DeviceXmitProperties->fulSpeed,
                                                                          DeviceXmitProperties->fulPriority
                                                                          );
                        }
                    }
                    break; // IOCTL_SET_DEVICE_XMIT_PROPERTIES

                case IOCTL_GET_CONFIGURATION_INFORMATION:
                    TRACE(TL_TRACE, ("IOCTL_GET_CONFIGURATION_INFORMATION\n"));

                    ntStatus = t1394_GetConfigurationInformation( DeviceObject,
                                                                      Irp
                                                                      );

                    break; // IOCTL_GET_CONFIGURATION_INFORMATION

                case IOCTL_BUS_RESET:
                    {
                        TRACE(TL_TRACE, ("IOCTL_BUS_RESET\n"));

                        if (inputBufferLength < sizeof(ULONG)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            ntStatus = t1394_BusReset( DeviceObject,
                                                           Irp,
                                                           *((PULONG)ioBuffer)
                                                           );
                        }
                    }
                    break; // IOCTL_BUS_RESET

                case IOCTL_GET_GENERATION_COUNT:
                    {
                        TRACE(TL_TRACE, ("IOCTL_GET_GENERATION_COUNT\n"));

                        if (outputBufferLength < sizeof(ULONG)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            ntStatus = t1394_GetGenerationCount( DeviceObject,
                                                                     Irp,
                                                                     (PULONG)ioBuffer
                                                                     );

                            if (NT_SUCCESS(ntStatus))
                                Irp->IoStatus.Information = outputBufferLength;
                        }
                    }
                    break; // IOCTL_GET_GENERATION_COUNT

                case IOCTL_SEND_PHY_CONFIGURATION_PACKET:
                    {
                        TRACE(TL_TRACE, ("IOCTL_SEND_PHY_CONFIGURATION_PACKET\n"));

                        if (inputBufferLength < sizeof(PHY_CONFIGURATION_PACKET)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            ntStatus = t1394_SendPhyConfigurationPacket( DeviceObject,
                                                                             Irp,
                                                                             *(PPHY_CONFIGURATION_PACKET)ioBuffer
                                                                             );
                        }
                    }
                    break; // IOCTL_SEND_PHY_CONFIGURATION_PACKET

                case IOCTL_BUS_RESET_NOTIFICATION:
                    {
                        TRACE(TL_TRACE, ("IOCTL_BUS_RESET_NOTIFICATION\n"));

                        if (inputBufferLength < sizeof(ULONG)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            ntStatus = t1394_BusResetNotification( DeviceObject,
                                                                       Irp,
                                                                       *((PULONG)ioBuffer)
                                                                       );
                        }
                    }
                    break; // IOCTL_BUS_RESET_NOTIFICATION

                case IOCTL_SET_LOCAL_HOST_INFORMATION:
                    {
                        PSET_LOCAL_HOST_INFORMATION     SetLocalHostInformation;

                        TRACE(TL_TRACE, ("IOCTL_SET_LOCAL_HOST_INFORMATION\n"));

                        if (inputBufferLength < sizeof(SET_LOCAL_HOST_INFORMATION)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            SetLocalHostInformation = (PSET_LOCAL_HOST_INFORMATION)ioBuffer;

                            if (inputBufferLength < (sizeof(SET_LOCAL_HOST_INFORMATION) +
                                                     SetLocalHostInformation->ulBufferSize)) {

                                ntStatus = STATUS_BUFFER_TOO_SMALL;
                            }
                            else {

                                ntStatus = t1394_SetLocalHostProperties( DeviceObject,
                                                                             Irp,
                                                                             SetLocalHostInformation->nLevel,
                                                                             (PVOID)&SetLocalHostInformation->Information
                                                                             );

                                if (NT_SUCCESS(ntStatus))
                                    Irp->IoStatus.Information = outputBufferLength;
                            }
                        }
                    }
                    break; // IOCTL_SET_LOCAL_HOST_INFORMATION
/*
                case IOCTL_SET_ADDRESS_DATA:
                    {
                        PSET_ADDRESS_DATA   SetAddressData;

                        TRACE(TL_TRACE, ("IOCTL_SET_ADDRESS_DATA\n"));

                        if (inputBufferLength < sizeof(SET_ADDRESS_DATA)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            SetAddressData = (PSET_ADDRESS_DATA)ioBuffer;

                            if (inputBufferLength < (sizeof(SET_ADDRESS_DATA)+SetAddressData->nLength)) {

                                ntStatus = STATUS_BUFFER_TOO_SMALL;
                            }
                            else {

                                ntStatus = t1394_SetAddressData( DeviceObject,
                                                                     Irp,
                                                                     SetAddressData->hAddressRange,
                                                                     SetAddressData->nLength,
                                                                     SetAddressData->ulOffset,
                                                                     (PVOID)&SetAddressData->Data
                                                                     );
                            }
                        }
                    }
                    break; // IOCTL_SET_ADDRESS_DATA

                case IOCTL_GET_ADDRESS_DATA:
                    {
                        PGET_ADDRESS_DATA   GetAddressData;

                        TRACE(TL_TRACE, ("IOCTL_GET_ADDRESS_DATA\n"));

                        if (inputBufferLength < sizeof(GET_ADDRESS_DATA)) {

                            ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            GetAddressData = (PGET_ADDRESS_DATA)ioBuffer;

                            if (inputBufferLength < (sizeof(GET_ADDRESS_DATA)+GetAddressData->nLength)) {

                                ntStatus = STATUS_BUFFER_TOO_SMALL;
                            }
                            else {

                                ntStatus = t1394_GetAddressData( DeviceObject,
                                                                     Irp,
                                                                     GetAddressData->hAddressRange,
                                                                     GetAddressData->nLength,
                                                                     GetAddressData->ulOffset,
                                                                     (PVOID)&GetAddressData->Data
                                                                     );
                                
                                if (NT_SUCCESS(ntStatus))
                                    Irp->IoStatus.Information = outputBufferLength;
                            }
                        }
                    }
                    break; // IOCTL_GET_ADDRESS_DATA
*/
                case IOCTL_BUS_RESET_NOTIFY: {

                    PBUS_RESET_IRP  BusResetIrp;
                    KIRQL           Irql;
                    
                    TRACE(TL_TRACE, ("IOCTL_BUS_RESET_NOTIFY\n"));

                    BusResetIrp = ExAllocatePool(NonPagedPool, sizeof(BUS_RESET_IRP));

                    if (BusResetIrp) {

                        // mark it pending
                        IoMarkIrpPending(Irp);
                        ntStatus = Irp->IoStatus.Status = STATUS_PENDING;
                        BusResetIrp->Irp = Irp;

                        TRACE(TL_TRACE, ("Adding BusResetIrp->Irp = 0x%x\n", BusResetIrp->Irp));

                        // add the irp to the list...
                        KeAcquireSpinLock(&deviceExtension->ResetSpinLock, &Irql);

                        InsertHeadList(&deviceExtension->BusResetIrps, &BusResetIrp->BusResetIrpList);

                        // set the cancel routine for the irp
                        IoSetCancelRoutine(Irp, t1394Diag_CancelIrp);

                        if (Irp->Cancel && IoSetCancelRoutine(Irp, t1394Diag_CancelIrp)) {

                            RemoveEntryList(&BusResetIrp->BusResetIrpList);
                            ntStatus = STATUS_CANCELLED;
                        }

                        KeReleaseSpinLock(&deviceExtension->ResetSpinLock, Irql);

                        // goto t1394Diag_IoControlExit on success so we don't complete the irp
                        if (ntStatus == STATUS_PENDING)
                            goto t1394Diag_IoControlExit;
                    }
                    else
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                    break; // IOCTL_BUS_RESET_NOTIFY

                case IOCTL_GET_DIAG_VERSION:
                    {
                        PVERSION_DATA   Version;

                        TRACE(TL_TRACE, ("IOCTL_GET_DIAG_VERSION\n"));

                        if ((inputBufferLength < sizeof(VERSION_DATA)) &&
                            (outputBufferLength < sizeof(VERSION_DATA))) {

                                ntStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                        else {

                            Version = (PVERSION_DATA)ioBuffer;

                            Version->ulVersion = DIAGNOSTIC_VERSION;
                            Version->ulSubVersion = DIAGNOSTIC_SUB_VERSION;

                            Irp->IoStatus.Information = outputBufferLength;                           
                        }
                    }
                    break; // IOCTL_GET_DIAG_VERSION

 				case IOCTL_GET_CMDR_STATE:
				{	
					PLONG pState;
					DbgPrint("1394CMDR: IOCTL_GET_CMDR_STATE");
					if(outputBufferLength < sizeof(LONG))
					{
						ntStatus = STATUS_BUFFER_TOO_SMALL;
					} else {
						pState = (PLONG) ioBuffer;
						ntStatus = t1394Cmdr_GetState(DeviceObject,Irp,pState);
						Irp->IoStatus.Information = 4;
					}
				}

				//////////////////////////////
				// these added for 1394cmdr //
				//////////////////////////////

				break; //IOCTL_GET_CMDR_STATE

				case IOCTL_RESET_CMDR_STATE:
					ntStatus = t1394Cmdr_ResetState(DeviceObject,Irp);
					break;

				case IOCTL_SET_CMDR_TRACELEVEL:
					break;
				case IOCTL_READ_REGISTER:
				{
					if(outputBufferLength < sizeof(REGISTER_IOBUF)
						|| inputBufferLength < sizeof(REGISTER_IOBUF))
					{
						ntStatus = STATUS_BUFFER_TOO_SMALL;
					} else {
						// an address with a leading 0xF will be interpreted
						// as an absolute offset in register space

						// otherwise, it will be added to the CSR offset
						PREGISTER_IOBUF regbuf = (PREGISTER_IOBUF)ioBuffer;
						ULONG offset = regbuf->ulOffset;

						if(!(offset & 0xF0000000))
							offset += deviceExtension->CSR_offset;

						ntStatus = t1394Cmdr_ReadRegister(DeviceObject,
														  Irp,
														  offset,
														  regbuf->data
														  );
                        if (NT_SUCCESS(ntStatus))
							Irp->IoStatus.Information = outputBufferLength;
					}
				} break;
				case IOCTL_WRITE_REGISTER:
				{
					if(outputBufferLength < sizeof(REGISTER_IOBUF)
						|| inputBufferLength < sizeof(REGISTER_IOBUF))
					{
						ntStatus = STATUS_BUFFER_TOO_SMALL;
					} else {
						// an address with a leading 0xF will be interpreted
						// as an absolute offset in register space

						// otherwise, it will be added to the CSR offset
						PREGISTER_IOBUF regbuf = (PREGISTER_IOBUF) ioBuffer;
						ULONG offset = regbuf->ulOffset;

						if(!(offset & 0xF0000000))
							offset += deviceExtension->CSR_offset;

						ntStatus = t1394Cmdr_WriteRegister(DeviceObject,
														   Irp,
														   offset,
														   regbuf->data
														   );
					}
				} break;
				case IOCTL_GET_MODEL_NAME:
					if(outputBufferLength < (unsigned long)(deviceExtension->ModelNameLength))
					{
						ntStatus = STATUS_BUFFER_TOO_SMALL;
					} else {
						RtlCopyMemory(ioBuffer,&(deviceExtension->pModelLeaf->TL_Data),deviceExtension->ModelNameLength);
						ntStatus = STATUS_SUCCESS;
						Irp->IoStatus.Information = deviceExtension->ModelNameLength;
					}
					break;
				case IOCTL_GET_VENDOR_NAME:
					if(outputBufferLength < (unsigned long)(deviceExtension->VendorNameLength))
					{
						ntStatus = STATUS_BUFFER_TOO_SMALL;
					} else {
						RtlCopyMemory(ioBuffer,&(deviceExtension->pVendorLeaf->TL_Data),deviceExtension->VendorNameLength);
						ntStatus = STATUS_SUCCESS;
						Irp->IoStatus.Information = deviceExtension->VendorNameLength;
					}
					break;
				case IOCTL_GET_CAMERA_SPECIFICATION:
	
					if(outputBufferLength < sizeof(CAMERA_SPECIFICATION))
					{
						ntStatus = STATUS_BUFFER_TOO_SMALL;
					} else {
						PCAMERA_SPECIFICATION pSpec = (PCAMERA_SPECIFICATION)(ioBuffer);
						pSpec->ulSpecification = deviceExtension->unit_spec_ID;
						pSpec->ulVersion = deviceExtension->unit_sw_version;
						Irp->IoStatus.Information = sizeof(CAMERA_SPECIFICATION);
					}
					break;

				case IOCTL_GET_CAMERA_UNIQUE_ID:
					if(outputBufferLength < sizeof(LARGE_INTEGER))
					{
						ntStatus = STATUS_BUFFER_TOO_SMALL;
					} else {
						PLARGE_INTEGER pID = (PLARGE_INTEGER)(ioBuffer);
						pID->HighPart = deviceExtension->pConfigRom->CR_Node_UniqueID[0];
						pID->LowPart = deviceExtension->pConfigRom->CR_Node_UniqueID[1];
						Irp->IoStatus.Information = sizeof(LARGE_INTEGER);
					}
					break;
				case IOCTL_ATTACH_BUFFER:
					// Input Argument: PRING3_ISOCH_DESCRIPTOR
					// Output Argument: The actual buffer as MDL
					if(inputBufferLength < sizeof(ISOCH_ATTACH_BUFFERS))
					{
						DbgPrint("1394CMDR: IOCTL_ATTACH_BUFFER: inputBufferLength too small (%d/%d)",
							inputBufferLength,sizeof(ISOCH_ATTACH_BUFFERS));
						ntStatus = STATUS_BUFFER_TOO_SMALL;
					} else {
						PISOCH_ATTACH_BUFFERS pAttach = (PISOCH_ATTACH_BUFFERS)(ioBuffer);
						PRING3_ISOCH_DESCRIPTOR pR3 = pAttach->R3_IsochDescriptor;
						if(outputBufferLength < pR3->ulLength)
						{
							DbgPrint("1394CMDR: IOCTL_ATTACH_BUFFER: outputBufferLength too small (%d/%d)",
								outputBufferLength,pR3->ulLength);
							ntStatus = STATUS_BUFFER_TOO_SMALL;
						} else {
							// actually call our function
							ntStatus = t1394_NewAttachBuffer( 
								DeviceObject,
								Irp,
								pAttach->hResource,
								Irp->MdlAddress,
								pR3
								);

                            if (ntStatus == STATUS_PENDING)
                                goto t1394Diag_IoControlExit;
						}
					}
					break;


               default:
                    TRACE(TL_ERROR, ("Invalid ioControlCode = 0x%x\n", ioControlCode));
                    TRAP;

                    ntStatus = STATUS_INVALID_PARAMETER;
                    break; // default

            } // switch

            break; // IRP_MJ_DEVICE_CONTROL

        default:
            TRACE(TL_TRACE, ("Unknown IrpSp->MajorFunction = 0x%x\n", IrpSp->MajorFunction));

            // submit this to the driver below us
            ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, Irp, NULL);
            return (ntStatus);
            break;

    } // switch

    // only complete if the device is there
    if (ntStatus != STATUS_NO_SUCH_DEVICE) {
    
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

t1394Diag_IoControlExit:

    EXIT("t1394Diag_IoControl", ntStatus);
    return(ntStatus);
} // t1394Diag_IoControl


