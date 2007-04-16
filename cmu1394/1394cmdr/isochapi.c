/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    isochapi.c

Abstract


Author:

    Peter Binder (pbinder) 7/26/97

Revision History:
Date     Who       What
-------- --------- ------------------------------------------------------------
7/26/97  pbinder   birth
4/14/98  pbinder   taken from 1394diag
--*/

#include "pch.h"

NTSTATUS
t1394_IsochAllocateBandwidth(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            nMaxBytesPerFrameRequested,
    IN ULONG            fulSpeed,
    OUT PHANDLE         phBandwidth,
    OUT PULONG          pBytesPerFrameAvailable,
    OUT PULONG          pSpeedSelected
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
	PCAMERA_STATE		cameraState = &(deviceExtension->CameraState);
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("t1394_IsochAllocateBandwidth");

    TRACE(TL_TRACE, ("nMaxBytesPerFrameRequested = 0x%x\n", nMaxBytesPerFrameRequested));
    TRACE(TL_TRACE, ("fulSpeed = 0x%x\n", fulSpeed));

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochAllocateBandwidth;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochAllocateBandwidth;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_ALLOCATE_BANDWIDTH;
    pIrb->Flags = 0;
    pIrb->u.IsochAllocateBandwidth.nMaxBytesPerFrameRequested = nMaxBytesPerFrameRequested;
    pIrb->u.IsochAllocateBandwidth.fulSpeed = fulSpeed;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (NT_SUCCESS(ntStatus)) {

        *phBandwidth = pIrb->u.IsochAllocateBandwidth.hBandwidth;
        *pBytesPerFrameAvailable = pIrb->u.IsochAllocateBandwidth.BytesPerFrameAvailable;
        *pSpeedSelected = pIrb->u.IsochAllocateBandwidth.SpeedSelected;

		cameraState->hIsochBandwidth = *phBandwidth;
		cameraState->IsochBytesPerFrameAvailable = *pBytesPerFrameAvailable;
		cameraState->IsochSpeedSelected = *pSpeedSelected;

        TRACE(TL_TRACE, ("hBandwidth = 0x%x\n", *phBandwidth));
        TRACE(TL_TRACE, ("BytesPerFrameAvailable = 0x%x\n", *pBytesPerFrameAvailable));

        // lets see if we got the speed we wanted
        if (fulSpeed != pIrb->u.IsochAllocateBandwidth.SpeedSelected) {

            TRACE(TL_TRACE, ("Different bandwidth speed selected.\n"));
        }

        TRACE(TL_TRACE, ("SpeedSelected = 0x%x\n", *pSpeedSelected));
    }
    else {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }

    ExFreePool(pIrb);

Exit_IsochAllocateBandwidth:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    EXIT("t1394_IsochAllocateBandwidth", ntStatus);
    return(ntStatus);
} // t1394_IsochAllocateBandwidth

NTSTATUS
t1394_IsochAllocateChannel(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            nRequestedChannel,
    OUT PULONG          pChannel,
    OUT PLARGE_INTEGER  pChannelsAvailable
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
	PCAMERA_STATE cameraState = &(deviceExtension->CameraState);
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("t1394_IsochAllocateChannel");

    TRACE(TL_TRACE, ("nRequestedChannel = 0x%x\n", nRequestedChannel));

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochAllocateChannel;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochAllocateChannel;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_ALLOCATE_CHANNEL;
    pIrb->Flags = 0;
    pIrb->u.IsochAllocateChannel.nRequestedChannel = nRequestedChannel;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (NT_SUCCESS(ntStatus)) {

        *pChannel = pIrb->u.IsochAllocateChannel.Channel;
        *pChannelsAvailable = pIrb->u.IsochAllocateChannel.ChannelsAvailable;

		cameraState->IsochChannel = *pChannel;
		cameraState->IsochChannelsAvailable = *pChannelsAvailable;

        TRACE(TL_TRACE, ("Channel = 0x%x\n", *pChannel));
        TRACE(TL_TRACE, ("ChannelsAvailable.LowPart = 0x%x\n", pChannelsAvailable->LowPart));
        TRACE(TL_TRACE, ("ChannelsAvailable.HighPart = 0x%x\n", pChannelsAvailable->HighPart));
    }
    else {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }

    ExFreePool(pIrb);

Exit_IsochAllocateChannel:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    EXIT("t1394_IsochAllocateChannel", ntStatus);
    return(ntStatus);
} // t1394_IsochAllocateChannel

NTSTATUS
t1394_IsochAllocateResources(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            fulSpeed,
    IN ULONG            fulFlags,
    IN ULONG            nChannel,
    IN ULONG            nMaxBytesPerFrame,
    IN ULONG            nNumberOfBuffers,
    IN ULONG            nMaxBufferSize,
    IN ULONG            nQuadletsToStrip,
    OUT PHANDLE         phResource
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
	PCAMERA_STATE cameraState = &(deviceExtension->CameraState);
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("t1394_IsochAllocateResources");

    TRACE(TL_TRACE, ("fulSpeed = 0x%x\n", fulSpeed));
    TRACE(TL_TRACE, ("fulFlags = 0x%x\n", fulFlags));
    TRACE(TL_TRACE, ("nChannel = 0x%x\n", nChannel));
    TRACE(TL_TRACE, ("nMaxBytesPerFrame = 0x%x\n", nMaxBytesPerFrame));
    TRACE(TL_TRACE, ("nNumberOfBuffers = 0x%x\n", nNumberOfBuffers));
    TRACE(TL_TRACE, ("nMaxBufferSize = 0x%x\n", nMaxBufferSize));
    TRACE(TL_TRACE, ("nQuadletsToStrip = 0x%x\n", nQuadletsToStrip));

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochAllocateResources;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochAllocateResources;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_ALLOCATE_RESOURCES;
    pIrb->Flags = 0;
    pIrb->u.IsochAllocateResources.fulSpeed = fulSpeed;
    pIrb->u.IsochAllocateResources.fulFlags = fulFlags;
    pIrb->u.IsochAllocateResources.nChannel = nChannel;
    pIrb->u.IsochAllocateResources.nMaxBytesPerFrame = nMaxBytesPerFrame;
    pIrb->u.IsochAllocateResources.nNumberOfBuffers = nNumberOfBuffers;
    pIrb->u.IsochAllocateResources.nMaxBufferSize = nMaxBufferSize;
    pIrb->u.IsochAllocateResources.nQuadletsToStrip = nQuadletsToStrip;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (NT_SUCCESS(ntStatus)) {

        PISOCH_RESOURCE_DATA    IsochResourceData;
        KIRQL                   Irql;

        *phResource = pIrb->u.IsochAllocateResources.hResource;
		cameraState->hIsochResource = *phResource;

        TRACE(TL_TRACE, ("hResource = 0x%x\n", *phResource));

        // need to add to our list...
        IsochResourceData = ExAllocatePool(NonPagedPool, sizeof(ISOCH_RESOURCE_DATA));

        if (IsochResourceData) {

            IsochResourceData->hResource = pIrb->u.IsochAllocateResources.hResource;

            KeAcquireSpinLock(&deviceExtension->IsochResourceSpinLock, &Irql);
            InsertHeadList(&deviceExtension->IsochResourceData, &IsochResourceData->IsochResourceList);
            KeReleaseSpinLock(&deviceExtension->IsochResourceSpinLock, Irql);
        }
        else {

            TRACE(TL_WARNING, ("Failed to allocate IsochResourceData!\n"));
        }

    }
    else {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }

    ExFreePool(pIrb);

Exit_IsochAllocateResources:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;

    EXIT("t1394_IsochAllocateResources", ntStatus);
    return(ntStatus);
} // t1394_IsochAllocateResources

NTSTATUS
t1394_IsochAttachBuffers(
    IN PDEVICE_OBJECT               DeviceObject,
    IN PIRP                         Irp,
    IN ULONG                        outputBufferLength,
    IN HANDLE                       hResource,
    IN ULONG                        nNumberOfDescriptors,
    OUT PISOCH_DESCRIPTOR           pIsochDescriptor,
    IN OUT PRING3_ISOCH_DESCRIPTOR  R3_IsochDescriptor
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION           deviceExtension = DeviceObject->DeviceExtension;
    PIRB                        pIrb;
    ULONG                       i;
    PISOCH_DETACH_DATA          pIsochDetachData;
    PRING3_ISOCH_DESCRIPTOR     pR3TempDescriptor;
    KIRQL                       Irql;
    PIO_STACK_LOCATION          NextIrpStack;
    LARGE_INTEGER               deltaTime;

    PIRP                newIrp;
    CCHAR               StackSize;

    ENTER("t1394_IsochAttachBuffers");

    TRACE(TL_TRACE, ("outputBufferLength = 0x%x\n", outputBufferLength));
    TRACE(TL_TRACE, ("hResource = 0x%x\n", hResource));
    TRACE(TL_TRACE, ("nNumberOfDescriptors = 0x%x\n", nNumberOfDescriptors));
    TRACE(TL_TRACE, ("R3_IsochDescriptor = 0x%x\n", R3_IsochDescriptor));

    //
    // Just assume that we are being passed a Irp that was created in UserMode
    // and allocate one here, because we need to keep an Irp around for
    // the callback routine.  Must submit this asynchronously so use IoAllocateIrp
    //
    StackSize = deviceExtension->StackDeviceObject->StackSize + 1;
    newIrp = IoAllocateIrp (StackSize, FALSE);

    if (!newIrp) {

        TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
        TRAP;
        
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochAttachBuffers;            
    }

    //
    // allocate the irb
    //
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochAttachBuffers;
    } // if

    //
    // allocate our isoch descriptors
    //
    pIsochDescriptor = ExAllocatePool(NonPagedPool, sizeof(ISOCH_DESCRIPTOR)*nNumberOfDescriptors);

    if (!pIsochDescriptor) {

        TRACE(TL_ERROR, ("Failed to allocate pIsochDescriptor!\n"));
        TRAP;

        if (pIrb)
            ExFreePool(pIrb);

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochAttachBuffers;
    }

    KeAcquireSpinLock(&deviceExtension->IsochSpinLock, &Irql);

    pR3TempDescriptor = R3_IsochDescriptor;

    for (i=0;i < nNumberOfDescriptors; i++) {

        TRACE(TL_TRACE, ("pR3TempDescriptor = 0x%x\n", pR3TempDescriptor));
        TRACE(TL_TRACE, ("pR3TempDescriptor->fulFlags = 0x%x\n", pR3TempDescriptor->fulFlags));
        TRACE(TL_TRACE, ("pR3TempDescriptor->ulLength = 0x%x\n", pR3TempDescriptor->ulLength));
        TRACE(TL_TRACE, ("pR3TempDescriptor->nMaxBytesPerFrame = 0x%x\n", pR3TempDescriptor->nMaxBytesPerFrame));
        TRACE(TL_TRACE, ("pR3TempDescriptor->ulSynch = 0x%x\n", pR3TempDescriptor->ulSynch));
        TRACE(TL_TRACE, ("pR3TempDescriptor->ulTag = 0x%x\n", pR3TempDescriptor->ulTag));
        TRACE(TL_TRACE, ("pR3TempDescriptor->CycleTime.CL_CycleOffset = 0x%x\n", pR3TempDescriptor->CycleTime.CL_CycleOffset));
        TRACE(TL_TRACE, ("pR3TempDescriptor->CycleTime.CL_CycleCount = 0x%x\n", pR3TempDescriptor->CycleTime.CL_CycleCount));
        TRACE(TL_TRACE, ("pR3TempDescriptor->CycleTime.CL_SecondCount = 0x%x\n", pR3TempDescriptor->CycleTime.CL_SecondCount));
        TRACE(TL_TRACE, ("pR3TempDescriptor->Data = 0x%x\n", &pR3TempDescriptor->Data));

        TRACE(TL_TRACE, ("pIsochDescriptor[%x] = 0x%x\n", i, &pIsochDescriptor[i]));

        // if we have a ulLength, create the Mdl. if not, Mdl=NULL
        if (pR3TempDescriptor->ulLength) {

            pIsochDescriptor[i].Mdl = IoAllocateMdl (pR3TempDescriptor->Data,
                                                     pR3TempDescriptor->ulLength,
                                                     FALSE,
                                                     FALSE,
                                                     NULL);

            MmBuildMdlForNonPagedPool(pIsochDescriptor[i].Mdl);
            pIsochDescriptor[i].ulLength = MmGetMdlByteCount(pIsochDescriptor[i].Mdl);
        }
        else {

            pIsochDescriptor[i].Mdl = NULL;
            pIsochDescriptor[i].ulLength = pR3TempDescriptor->ulLength;
        }

        pIsochDescriptor[i].fulFlags = pR3TempDescriptor->fulFlags;
        pIsochDescriptor[i].ulLength = MmGetMdlByteCount(pIsochDescriptor[i].Mdl);
        pIsochDescriptor[i].nMaxBytesPerFrame = pR3TempDescriptor->nMaxBytesPerFrame;
        pIsochDescriptor[i].ulSynch = pR3TempDescriptor->ulSynch;
        pIsochDescriptor[i].ulTag = pR3TempDescriptor->ulTag;
        pIsochDescriptor[i].CycleTime = pR3TempDescriptor->CycleTime;

/*		DbgPrint("1394CMDR: pIsochDescriptor = 0x%08x, CT = %d:%d:%d",
			pIsochDescriptor,
			pIsochDescriptor->CycleTime.CL_CycleOffset,
			pIsochDescriptor->CycleTime.CL_CycleCount,
			pIsochDescriptor->CycleTime.CL_SecondCount);
*/

        if (pR3TempDescriptor->bUseCallback) {

            //
            // i'm hoping this is the last descriptor. they should have only set this in the
            // last descriptor, since elsewhere it's not supported.
            //
            if (i != nNumberOfDescriptors-1) {

                TRACE(TL_TRACE, ("Callback on descriptor prior to last!\n"));

                // setting callback to NULL
                pIsochDescriptor[i].Callback = NULL;
            }
            else {

                // need to save hResource, numDescriptors and Irp to use when detaching.
                // this needs to be done before we submit the irp, since the isoch callback
                // can be called before the submitirpsynch call completes.
                pIsochDetachData = ExAllocatePool(NonPagedPool, sizeof(ISOCH_DETACH_DATA));

                if (!pIsochDetachData) {

                    TRACE(TL_ERROR, ("Failed to allocate pIsochDetachData!\n"));
                    TRAP;

                    if (pIsochDescriptor) {
                        ExFreePool(pIsochDescriptor);
                    }

                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    KeReleaseSpinLock(&deviceExtension->IsochSpinLock, Irql);
                    
                    goto Exit_IsochAttachBuffers;
                }

                KeInitializeSpinLock (&pIsochDetachData->IsochTagSpinLock);
                pIsochDetachData->Tag = ISOCH_DETACH_TAG;
                pIsochDetachData->AttachIrb = pIrb;

                InsertHeadList(&deviceExtension->IsochDetachData, &pIsochDetachData->IsochDetachList);

                KeInitializeTimer(&pIsochDetachData->Timer);
                KeInitializeDpc(&pIsochDetachData->TimerDpc, t1394_IsochTimeout, pIsochDetachData);

#define REQUEST_BUSY_RETRY_VALUE        (ULONG)(-100 * 100 * 100 * 100) //10 secs in units of 100nsecs

                deltaTime.LowPart = REQUEST_BUSY_RETRY_VALUE;
                deltaTime.HighPart = -1;
                KeSetTimer(&pIsochDetachData->Timer, deltaTime, &pIsochDetachData->TimerDpc);

                pIsochDetachData->outputBufferLength = outputBufferLength;
                pIsochDetachData->DeviceExtension = deviceExtension;
                pIsochDetachData->hResource = hResource;
                pIsochDetachData->numIsochDescriptors = nNumberOfDescriptors;
                pIsochDetachData->IsochDescriptor = pIsochDescriptor;
                pIsochDetachData->Irp = Irp;
                pIsochDetachData->newIrp = newIrp;
                pIsochDetachData->bDetach = pR3TempDescriptor->bAutoDetach;

                pIsochDescriptor[i].Callback = t1394_IsochCallback;

                pIsochDescriptor[i].Context1 = deviceExtension;
                pIsochDescriptor[i].Context2 = pIsochDetachData;

                TRACE(TL_TRACE, ("IsochAttachBuffers: pIsochDetachData = 0x%x\n", pIsochDetachData));
                TRACE(TL_TRACE, ("IsochAttachBuffers: pIsochDetachData->Irp = 0x%x\n", pIsochDetachData->Irp));
            }
        }
        else {

            pIsochDescriptor[i].Callback = NULL;
        }

        TRACE(TL_TRACE, ("pIsochDescriptor[%x].fulFlags = 0x%x\n", i, pIsochDescriptor[i].fulFlags));
        TRACE(TL_TRACE, ("pIsochDescriptor[%x].ulLength = 0x%x\n", i, pIsochDescriptor[i].ulLength));
        TRACE(TL_TRACE, ("pIsochDescriptor[%x].nMaxBytesPerFrame = 0x%x\n", i, pIsochDescriptor[i].nMaxBytesPerFrame));
        TRACE(TL_TRACE, ("pIsochDescriptor[%x].ulSynch = 0x%x\n", i, pIsochDescriptor[i].ulSynch));
        TRACE(TL_TRACE, ("pIsochDescriptor[%x].ulTag = 0x%x\n", i, pIsochDescriptor[i].ulTag));
        TRACE(TL_TRACE, ("pIsochDescriptor[%x].CycleTime.CL_CycleOffset = 0x%x\n", i, pIsochDescriptor[i].CycleTime.CL_CycleOffset));
        TRACE(TL_TRACE, ("pIsochDescriptor[%x].CycleTime.CL_CycleCount = 0x%x\n", i, pIsochDescriptor[i].CycleTime.CL_CycleCount));
        TRACE(TL_TRACE, ("pIsochDescriptor[%x].CycleTime.CL_SecondCount = 0x%x\n", i, pIsochDescriptor[i].CycleTime.CL_SecondCount));

        pR3TempDescriptor =
           (PRING3_ISOCH_DESCRIPTOR)((ULONG_PTR)pR3TempDescriptor +
                                     pIsochDescriptor[i].ulLength +
                                     sizeof(RING3_ISOCH_DESCRIPTOR));
    } // for

    KeReleaseSpinLock(&deviceExtension->IsochSpinLock, Irql);

    // lets make sure the device is still around
    // if it isn't, we free the irb and return, our pnp
    // cleanup will take care of everything else
    if (deviceExtension->bShutdown) {

        TRACE(TL_TRACE, ("Shutdown!\n"));
        ntStatus = STATUS_NO_SUCH_DEVICE;
        goto Exit_IsochAttachBuffers;
    }

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_ATTACH_BUFFERS;
    pIrb->Flags = 0;
    pIrb->u.IsochAttachBuffers.hResource = hResource;
    pIrb->u.IsochAttachBuffers.nNumberOfDescriptors = nNumberOfDescriptors;
    pIrb->u.IsochAttachBuffers.pIsochDescriptor = pIsochDescriptor;

    // mark our original irp pending
    IoMarkIrpPending(Irp);
    
    //
    // Submit the newIrp directly to the driver below us
    //
    NextIrpStack = IoGetNextIrpStackLocation(newIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pIrb; 

    IoSetCompletionRoutine( newIrp,
                            t1394_IsochAttachCompletionRoutine,
                            pIsochDetachData,
                            TRUE,
                            TRUE,
                            TRUE
                            );

    IoCallDriver(deviceExtension->StackDeviceObject, newIrp);
    ntStatus = STATUS_PENDING;
    
Exit_IsochAttachBuffers:

    EXIT("t1394_IsochAttachBuffers", ntStatus);
    return(ntStatus);
} // t1394_IsochAttachBuffers

NTSTATUS
t1394_IsochDetachBuffers(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN HANDLE               hResource,
    IN ULONG                nNumberOfDescriptors,
    IN PISOCH_DESCRIPTOR    IsochDescriptor
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;
    ULONG               i;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("");

    TRACE(TL_TRACE, ("hResource = 0x%x\n", hResource));
    TRACE(TL_TRACE, ("nNumberOfDescriptors = 0x%x\n", nNumberOfDescriptors));
    TRACE(TL_TRACE, ("IsochDescriptor = 0x%x\n", IsochDescriptor));

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochDetachBuffers;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochDetachBuffers;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_DETACH_BUFFERS;
    pIrb->Flags = 0;
    pIrb->u.IsochDetachBuffers.hResource = hResource;
    pIrb->u.IsochDetachBuffers.nNumberOfDescriptors = nNumberOfDescriptors;
    pIrb->u.IsochDetachBuffers.pIsochDescriptor = IsochDescriptor;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }

    if (pIrb)
        ExFreePool(pIrb);

/*    for (i=0; i<nNumberOfDescriptors; i++)
        ExFreePool(IsochDescriptor[i].Mdl);
  */      
    ExFreePool(IsochDescriptor);

Exit_IsochDetachBuffers:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    EXIT("t1394_IsochDetachBuffers", ntStatus);
    return(ntStatus);
} // t1394_IsochDetachBuffers

NTSTATUS
t1394_IsochFreeBandwidth(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN HANDLE           hBandwidth
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
	PCAMERA_STATE cameraState = &(deviceExtension->CameraState);
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("t1394_IsochFreeBandwidth");

    TRACE(TL_TRACE, ("hBandwidth = 0x%x\n", hBandwidth));

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochFreeBandwidth;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochFreeBandwidth;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_FREE_BANDWIDTH;
    pIrb->Flags = 0;
    pIrb->u.IsochFreeBandwidth.hBandwidth = hBandwidth;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }
    
	cameraState->hIsochBandwidth = NULL;
	cameraState->IsochBytesPerFrameAvailable = 0;
	cameraState->IsochSpeedSelected = 0;

    ExFreePool(pIrb);

Exit_IsochFreeBandwidth:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;

    EXIT("t1394_IsochFreeBandwidth", ntStatus);
    return(ntStatus);
} // t1394_IsochFreeBandwidth

NTSTATUS
t1394_IsochFreeChannel(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            nChannel
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
	PCAMERA_STATE cameraState = &(deviceExtension->CameraState);
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("t1394_IsochFreeChannel");

    TRACE(TL_TRACE, ("nChannel = 0x%x\n", nChannel));

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochFreeChannel;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochFreeChannel;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_FREE_CHANNEL;
    pIrb->Flags = 0;
    pIrb->u.IsochFreeChannel.nChannel = nChannel;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }
    
	cameraState->IsochChannel = -1;
	cameraState->IsochChannelsAvailable.QuadPart = 0;

    ExFreePool(pIrb);

Exit_IsochFreeChannel:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    EXIT("t1394_IsochFreeChannel", ntStatus);
    return(ntStatus);
} // t1394_IsochFreeChannel

NTSTATUS
t1394_IsochFreeResources(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN HANDLE           hResource
    )
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;
	PCAMERA_STATE cameraState = &(deviceExtension->CameraState);
    PIRB                    pIrb;
    PISOCH_RESOURCE_DATA    IsochResourceData;
    KIRQL                   Irql;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("t1394_IsochFreeResources");

    TRACE(TL_TRACE, ("hResource = 0x%x\n", hResource));

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochFreeResources;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochFreeResources;
    } // if

    // remove this one from our list...
    KeAcquireSpinLock(&deviceExtension->IsochResourceSpinLock, &Irql);

    IsochResourceData = (PISOCH_RESOURCE_DATA)deviceExtension->IsochResourceData.Flink;

    while (IsochResourceData) {

        TRACE(TL_TRACE, ("Removing hResource = 0x%x\n", hResource));

        if (IsochResourceData->hResource == hResource) {

            RemoveEntryList(&IsochResourceData->IsochResourceList);
            ExFreePool(IsochResourceData);
            break;
        }
        else if (IsochResourceData->IsochResourceList.Flink == &deviceExtension->IsochResourceData) {
            break;
        }
        else
            IsochResourceData = (PISOCH_RESOURCE_DATA)IsochResourceData->IsochResourceList.Flink;
    }

    KeReleaseSpinLock(&deviceExtension->IsochResourceSpinLock, Irql);

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_FREE_RESOURCES;
    pIrb->Flags = 0;
    pIrb->u.IsochFreeResources.hResource = hResource;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }

	cameraState->hIsochResource = NULL;

    ExFreePool(pIrb);

Exit_IsochFreeResources:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    EXIT("t1394_IsochFreeResources", ntStatus);
    return(ntStatus);
} // t1394_IsochFreeResources

NTSTATUS
t1394_IsochListen(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN HANDLE           hResource,
    IN ULONG            fulFlags,
    IN CYCLE_TIME       StartTime
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("t1394_IsochListen");

    TRACE(TL_TRACE, ("hResource = 0x%x\n", hResource));
    TRACE(TL_TRACE, ("fulFlags = 0x%x\n", fulFlags));
    TRACE(TL_TRACE, ("StartTime.CL_CycleOffset = 0x%x\n", StartTime.CL_CycleOffset));
    TRACE(TL_TRACE, ("StartTime.CL_CycleCount = 0x%x\n", StartTime.CL_CycleCount));
    TRACE(TL_TRACE, ("StartTime.CL_SecondCount = 0x%x\n", StartTime.CL_SecondCount));

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochListen;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochListen;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_LISTEN;
    pIrb->Flags = 0;
    pIrb->u.IsochListen.hResource = hResource;
    pIrb->u.IsochListen.fulFlags = fulFlags;
    pIrb->u.IsochListen.StartTime = StartTime;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }
    
    ExFreePool(pIrb);

Exit_IsochListen:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    EXIT("t1394_IsochListen", ntStatus);
    return(ntStatus);
} // t1394_IsochListen

NTSTATUS
t1394_IsochQueryCurrentCycleTime(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    OUT PCYCLE_TIME     pCurrentCycleTime
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("t1394_IsochQueryCurrentCycleTime");

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochQueryCurrentCycleTime;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochQueryCurrentCycleTime;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_QUERY_CYCLE_TIME;
    pIrb->Flags = 0;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (NT_SUCCESS(ntStatus)) {

        *pCurrentCycleTime = pIrb->u.IsochQueryCurrentCycleTime.CycleTime;
        
        TRACE(TL_TRACE, ("CurrentCycleTime.CL_CycleOffset = 0x%x\n", pCurrentCycleTime->CL_CycleOffset));
        TRACE(TL_TRACE, ("CurrentCycleTime.CL_CycleCount = 0x%x\n", pCurrentCycleTime->CL_CycleCount));
        TRACE(TL_TRACE, ("CurrentCycleTime.CL_SecondCount = 0x%x\n", pCurrentCycleTime->CL_SecondCount));
    }
    else {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }

    ExFreePool(pIrb);

Exit_IsochQueryCurrentCycleTime:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    EXIT("t1394_IsochQueryCurrentCycleTime", ntStatus);
    return(ntStatus);
} // t1394_IsochQueryCurrentCycleTime

NTSTATUS
t1394_IsochQueryResources(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            fulSpeed,
    OUT PULONG          pBytesPerFrameAvailable,
    OUT PLARGE_INTEGER  pChannelsAvailable
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("t1394_IsochQueryResources");

    TRACE(TL_TRACE, ("fulSpeed = 0x%x\n", fulSpeed));

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochQueryResources;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochQueryResources;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_QUERY_RESOURCES;
    pIrb->Flags = 0;
    pIrb->u.IsochQueryResources.fulSpeed = fulSpeed;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (NT_SUCCESS(ntStatus)) {

        *pBytesPerFrameAvailable = pIrb->u.IsochQueryResources.BytesPerFrameAvailable;
        *pChannelsAvailable = pIrb->u.IsochQueryResources.ChannelsAvailable;

        TRACE(TL_TRACE, ("BytesPerFrameAvailable = 0x%x\n", *pBytesPerFrameAvailable));
        TRACE(TL_TRACE, ("ChannelsAvailable.LowPart = 0x%x\n", pChannelsAvailable->LowPart));
        TRACE(TL_TRACE, ("ChannelsAvailable.HighPart = 0x%x\n", pChannelsAvailable->HighPart));
    }
    else {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }

    ExFreePool(pIrb);

Exit_IsochQueryResources:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    EXIT("t1394_IsochQueryResources", ntStatus);
    return(ntStatus);
} // t1394_IsochQueryResources

NTSTATUS
t1394_IsochSetChannelBandwidth(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN HANDLE           hBandwidth,
    IN ULONG            nMaxBytesPerFrame
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("t1394_IsochSetChannelBandwidth");

    TRACE(TL_TRACE, ("hBandwidth = 0x%x\n", hBandwidth));
    TRACE(TL_TRACE, ("nMaxBytesPerFrame = 0x%x\n", nMaxBytesPerFrame));

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochSetChannelBandwidth;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochSetChannelBandwidth;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_SET_CHANNEL_BANDWIDTH;
    pIrb->Flags = 0;
    pIrb->u.IsochSetChannelBandwidth.hBandwidth = hBandwidth;
    pIrb->u.IsochSetChannelBandwidth.nMaxBytesPerFrame = nMaxBytesPerFrame;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
    
    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }
    
    ExFreePool(pIrb);

Exit_IsochSetChannelBandwidth:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    EXIT("t1394_IsochSetChannelBandwidth",  ntStatus);
    return(ntStatus);
} // t1394_IsochSetChannelBandwidth

NTSTATUS
t1394_IsochStop(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN HANDLE           hResource,
    IN ULONG            fulFlags
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("t1394_IsochStop");

    TRACE(TL_TRACE, ("hResource = 0x%x\n", hResource));
    TRACE(TL_TRACE, ("fulFlags = 0x%x\n", fulFlags));

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochStop;            
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochStop;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_STOP;
    pIrb->Flags = 0;
    pIrb->u.IsochStop.hResource = hResource;
    pIrb->u.IsochStop.fulFlags = fulFlags;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
 
    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }
    
    ExFreePool(pIrb);
    
Exit_IsochStop:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    EXIT("t1394_IsochStop", ntStatus);
    return(ntStatus);
} // t1394_IsochStop

NTSTATUS
t1394_IsochTalk(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN HANDLE           hResource,
    IN ULONG            fulFlags,
    CYCLE_TIME          StartTime
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;

    PIRP                newIrp;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus;
    
    ENTER("t1394_IsochTalk");

    TRACE(TL_TRACE, ("hResource = 0x%x\n", hResource));
    TRACE(TL_TRACE, ("fulFlags = 0x%x\n", fulFlags));
    TRACE(TL_TRACE, ("StartTime.CL_CycleOffset = 0x%x\n", StartTime.CL_CycleOffset));
    TRACE(TL_TRACE, ("StartTime.CL_CycleCount = 0x%x\n", StartTime.CL_CycleCount));
    TRACE(TL_TRACE, ("StartTime.CL_SecondCount = 0x%x\n", StartTime.CL_SecondCount));

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, deviceExtension->StackDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {

            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            TRAP;
        
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochTalk;
        }
        allocNewIrp = TRUE;
    }
    
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_IsochTalk;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_TALK;
    pIrb->Flags = 0;
    pIrb->u.IsochTalk.hResource = hResource;
    pIrb->u.IsochTalk.fulFlags = fulFlags;
    pIrb->u.IsochTalk.StartTime = StartTime;

    //
    // If we allocated this irp, submit it asynchronously and wait for its
    // completion event to be signaled.  Otherwise submit it synchronously
    //
    if (allocNewIrp) {

        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (deviceExtension->StackDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) {
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    }
    else {
        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, pIrb);
    }
 
    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
        TRAP;
    }
    
    ExFreePool(pIrb);

Exit_IsochTalk:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;
        
    EXIT("t1394_IsochTalk", ntStatus);
    return(ntStatus);
} // t1394_IsochTalk

void
t1394_IsochCallback(
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN PISOCH_DETACH_DATA   IsochDetachData
    )
{
    KIRQL               Irql;
    KIRQL               TagIrql;

    ENTER("t1394_IsochCallback");
    if (!IsochDetachData)
        goto Exit_IsochCallback;

/*	DbgPrint("1394CMDR: Callback says Isoch Descriptor = 0x%08x, CycleTime = %d:%d:%d",
		IsochDetachData->IsochDescriptor,
		IsochDetachData->IsochDescriptor->CycleTime.CL_CycleOffset,
		IsochDetachData->IsochDescriptor->CycleTime.CL_CycleCount,
		IsochDetachData->IsochDescriptor->CycleTime.CL_SecondCount);
*/

    if (!DeviceExtension->bShutdown) {

        KeAcquireSpinLock(&DeviceExtension->IsochSpinLock, &Irql);
        RemoveEntryList(&IsochDetachData->IsochDetachList);
        KeCancelTimer(&IsochDetachData->Timer);

        TRACE(TL_TRACE, ("IsochCallback: IsochDetachData = 0x%x\n", IsochDetachData));
        TRACE(TL_TRACE, ("IsochCallback: IsochDetachData->Irp = 0x%x\n", IsochDetachData->Irp));
        TRACE(TL_TRACE, ("IsochCallback: IsochDetachData->newIrp = 0x%x\n", IsochDetachData->newIrp));

        KeAcquireSpinLock (&IsochDetachData->IsochTagSpinLock, &TagIrql);
        if (IsochDetachData->Tag != ISOCH_DETACH_TAG) {

            TRACE(TL_ERROR, ("Invalid Detach Tag!\n"));
            //TRAP;

            KeReleaseSpinLock(&IsochDetachData->IsochTagSpinLock, TagIrql);
            KeReleaseSpinLock(&DeviceExtension->IsochSpinLock, Irql);
        }
        else {

            // clear the tag...
            IsochDetachData->Tag = 0;
            
            KeReleaseSpinLock(&IsochDetachData->IsochTagSpinLock, TagIrql);
            KeReleaseSpinLock(&DeviceExtension->IsochSpinLock, Irql);

            // need to save the status of the attach
            // we'll clean up in the same spot for success's and timeout's
            IsochDetachData->AttachStatus = IsochDetachData->Irp->IoStatus.Status;

            t1394_IsochCleanup(IsochDetachData);
        }
    }

Exit_IsochCallback:

    EXIT("t1394_IsochCallback", 0);
} // t1394_IsochCallback

void
t1394_IsochTimeout(
    IN PKDPC                Dpc,
    IN PISOCH_DETACH_DATA   IsochDetachData,
    IN PVOID                SystemArgument1,
    IN PVOID                SystemArgument2
    )
{
    KIRQL               Irql;
    KIRQL               TagIrql;
    PDEVICE_EXTENSION   DeviceExtension;

    ENTER("t1394_IsochTimeout");

    TRACE(TL_WARNING, ("Isoch Timeout!\n"));

    DeviceExtension = IsochDetachData->DeviceExtension;

    if (DeviceExtension && !DeviceExtension->bShutdown) {

        KeAcquireSpinLock(&DeviceExtension->IsochSpinLock, &Irql);
        RemoveEntryList(&IsochDetachData->IsochDetachList);
        KeCancelTimer(&IsochDetachData->Timer);

        TRACE(TL_TRACE, ("IsochTimeout: IsochDetachData = 0x%x\n", IsochDetachData));
        TRACE(TL_TRACE, ("IsochTimeout: IsochDetachData->Irp = 0x%x\n", IsochDetachData->Irp));
        TRACE(TL_TRACE, ("IsochTimeout: IsochDetachData->newIrp = 0x%x\n", IsochDetachData->newIrp));

        KeAcquireSpinLock (&IsochDetachData->IsochTagSpinLock, &TagIrql);
        if (IsochDetachData->Tag != ISOCH_DETACH_TAG) {

            TRACE(TL_ERROR, ("Invalid Detach Tag!\n"));
            //TRAP;

            KeReleaseSpinLock(&IsochDetachData->IsochTagSpinLock, TagIrql);
            KeReleaseSpinLock(&DeviceExtension->IsochSpinLock, Irql);
        }
        else {

            // clear the tag...
            IsochDetachData->Tag = 0;

            KeReleaseSpinLock(&IsochDetachData->IsochTagSpinLock, TagIrql);
            KeReleaseSpinLock(&DeviceExtension->IsochSpinLock, Irql);

            // need to save the status of the attach
            // we'll clean up in the same spot for success's and timeout's
            IsochDetachData->AttachStatus = STATUS_IO_TIMEOUT;
	
			DbgPrint("1394cmdr: isochtimeout calling isochcleanup (DetachData = 0x%08x\n)",IsochDetachData);

            t1394_IsochCleanup(IsochDetachData);
        }
    }
    else
    {
        // we need to cancel the timer here
        if (IsochDetachData)
        {
            TRACE(TL_TRACE, ("Cancelling Detach Timer\n"));
            KeCancelTimer(&IsochDetachData->Timer);
        }
    }

    EXIT("t1394_IsochTimeout", 0);
} // t1394_IsochTimeout

void
t1394_IsochCleanup(
    IN PISOCH_DETACH_DATA   IsochDetachData
    )
{
    ULONG               i;
    PDEVICE_EXTENSION   DeviceExtension;

    ENTER("t1394_IsochCleanup");

    DeviceExtension = IsochDetachData->DeviceExtension;

    //
    // see if we need to detach this buffer
    //
    if ((!IsochDetachData) || (!DeviceExtension))
    {
        goto Exit_IsochDetachBuffers;
    }

//	DbgPrint("enter isochcleanup, status = %08x\n",IsochDetachData->AttachStatus);
    
    if (IsochDetachData->bDetach) {

        PIRB                pIrb;
        NTSTATUS            ntStatus;
        PIO_STACK_LOCATION  NextIrpStack;

//		DbgPrint("--> need to detach buffer\n");

        // make sure we have a valid stack location to being with
        if (!IoGetCurrentIrpStackLocation(IsochDetachData->newIrp))
        {
            // this request must have already been handled, just exit
            TRACE (TL_TRACE, ("t1394_IsochCleanup: Invalid Irp Stack Location found\n"));
            goto Exit_IsochDetachBuffers;
        }

        pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

        if (!pIrb) {

            TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
            TRACE(TL_WARNING, ("Can't detach buffer!\n"));
            TRAP;

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_IsochDetachBuffers;
        } // if

        // save the irb in our detach data context
        IsochDetachData->DetachIrb = pIrb;

        RtlZeroMemory (pIrb, sizeof (IRB));
        pIrb->FunctionNumber = REQUEST_ISOCH_DETACH_BUFFERS;
        pIrb->Flags = 0;
        pIrb->u.IsochDetachBuffers.hResource = IsochDetachData->hResource;
        pIrb->u.IsochDetachBuffers.nNumberOfDescriptors = IsochDetachData->numIsochDescriptors;
        pIrb->u.IsochDetachBuffers.pIsochDescriptor = IsochDetachData->IsochDescriptor;                                                                 

        NextIrpStack = IoGetNextIrpStackLocation(IsochDetachData->newIrp);        
        NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
        NextIrpStack->Parameters.Others.Argument1 = pIrb;

        IoSetCompletionRoutine( IsochDetachData->newIrp,
                                t1394_IsochDetachCompletionRoutine,
                                IsochDetachData,
                                TRUE,
                                TRUE,
                                TRUE
                                );

/*		DbgPrint("1394CMDR: Cleanup (detach = true, before IOcalldriver) says Isoch Descriptor = 0x%08x, CycleTime = %d:%d:%d",
			IsochDetachData->IsochDescriptor,
			IsochDetachData->IsochDescriptor->CycleTime.CL_CycleOffset,
			IsochDetachData->IsochDescriptor->CycleTime.CL_CycleCount,
			IsochDetachData->IsochDescriptor->CycleTime.CL_SecondCount);
*/
        IoCallDriver(DeviceExtension->StackDeviceObject, IsochDetachData->newIrp);
    }
    else {

		DbgPrint("--> no need to detach buffer\n");
        TRACE(TL_TRACE, ("Complete Irp.\n"));

        if (IsochDetachData->AttachIrb)
            ExFreePool(IsochDetachData->AttachIrb);

        for (i=0; i<IsochDetachData->numIsochDescriptors; i++) {

            if (IsochDetachData->IsochDescriptor[i].Mdl)
			{
				//DbgPrint("Unlocking Pages");
				//MmUnlockPages(IsochDetachData->IsochDescriptor[i].Mdl);
                //IoFreeMdl(IsochDetachData->IsochDescriptor[i].Mdl);
			}
        }

/*		DbgPrint("1394CMDR: Cleanup (just before ExFreePool) says Isoch Descriptor = 0x%08x, CycleTime = %d:%d:%d",
			IsochDetachData->IsochDescriptor,
			IsochDetachData->IsochDescriptor->CycleTime.CL_CycleOffset,
			IsochDetachData->IsochDescriptor->CycleTime.CL_CycleCount,
			IsochDetachData->IsochDescriptor->CycleTime.CL_SecondCount);
*/
        ExFreePool(IsochDetachData->IsochDescriptor);

        IsochDetachData->Irp->IoStatus.Status = IsochDetachData->AttachStatus;

        // only set this if its a success...
        if (NT_SUCCESS(IsochDetachData->AttachStatus))
		{
			DbgPrint("t1394_IsochCleanup: IsochDetachData->AttachStatus SUCCESS");
            IsochDetachData->Irp->IoStatus.Information = IsochDetachData->outputBufferLength;
		} else {
			DbgPrint("t1394_IsochCleanup: IsochDetachData->AttachStatus FAILURE (%d)",IsochDetachData->AttachStatus);
            IsochDetachData->Irp->IoStatus.Information = 0;
		}

        //
        // Complete original Irp and free the one we allocated in
        // IsochAttachBuffers
        //
        IoCompleteRequest(IsochDetachData->Irp, IO_SOUND_INCREMENT);
        IoFreeIrp (IsochDetachData->newIrp);

        // all done with IsochDetachData, lets deallocate it...
        ExFreePool(IsochDetachData);
    }

Exit_IsochDetachBuffers:

    EXIT("t1394_IsochCleanup", 0);
} // t1394_IsochCleanup

NTSTATUS
t1394_IsochDetachCompletionRoutine(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PISOCH_DETACH_DATA   IsochDetachData
    )
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    ULONG           i;

    ENTER("t1394_IsochDetachCompletionRoutine");

    if (!IsochDetachData) {

        TRACE(TL_WARNING, ("Invalid IsochDetachData\n"));
        goto Exit_IsochDetachCompletionRoutine;
    }

    if (IsochDetachData->DetachIrb)
        ExFreePool(IsochDetachData->DetachIrb);

    TRACE(TL_TRACE, ("Complete Irp.\n"));

    if (IsochDetachData->AttachIrb)
        ExFreePool(IsochDetachData->AttachIrb);

    for (i=0; i<IsochDetachData->numIsochDescriptors; i++) {

        if (IsochDetachData->IsochDescriptor[i].Mdl)
		{
			//DbgPrint("Unlocking Pages");
			//MmUnlockPages(IsochDetachData->IsochDescriptor[i].Mdl);
            //IoFreeMdl(IsochDetachData->IsochDescriptor[i].Mdl);
		}
    }

/*	DbgPrint("1394CMDR: DetachCompletion (just before ExFreePool) says Isoch Descriptor = 0x%08x, CycleTime = %d:%d:%d",
		IsochDetachData->IsochDescriptor,
		IsochDetachData->IsochDescriptor->CycleTime.CL_CycleOffset,
		IsochDetachData->IsochDescriptor->CycleTime.CL_CycleCount,
		IsochDetachData->IsochDescriptor->CycleTime.CL_SecondCount);
*/
    if (IsochDetachData->IsochDescriptor)
        ExFreePool(IsochDetachData->IsochDescriptor);

    // only set this if its a success...
	// CHECK 03/08/03: maybe this always needs to be outputbufferlength
	// to make METHOD_IN_DIRECT happy
    if (NT_SUCCESS(IsochDetachData->AttachStatus))
    //if (1)
	{
        IsochDetachData->Irp->IoStatus.Information = IsochDetachData->outputBufferLength;
	} else {
        IsochDetachData->Irp->IoStatus.Information = 0;
	}

    IsochDetachData->Irp->IoStatus.Status = IsochDetachData->AttachStatus;

    //
    // Complete original Irp and free the one we allocated in
    // IsochAttachBuffers
    //
/*
	DbgPrint("1394cmdr: isochdetachcompletion: completing IRP %08x with status %08x and info %d\n",
			IsochDetachData->Irp,
			IsochDetachData->Irp->IoStatus.Status,
			IsochDetachData->Irp->IoStatus.Information);
*/
    IoCompleteRequest(IsochDetachData->Irp, IO_NO_INCREMENT);
    IoFreeIrp (IsochDetachData->newIrp);



    // all done with IsochDetachData, lets deallocate it...
    if (IsochDetachData)
        ExFreePool(IsochDetachData);

Exit_IsochDetachCompletionRoutine:

    EXIT("t1394_IsochDetachCompletionRoutine", ntStatus);
    return(STATUS_MORE_PROCESSING_REQUIRED);
} // t1394_IsochDetachCompletionRoutine

NTSTATUS
t1394_IsochAttachCompletionRoutine(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PISOCH_DETACH_DATA   IsochDetachData
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    ULONG               i;
    KIRQL               Irql;
    KIRQL               TagIrql;
    PDEVICE_EXTENSION   DeviceExtension;
	PISOCH_ATTACH_BUFFERS pIsochAttachBuffers;
	PRING3_ISOCH_DESCRIPTOR  pR3IsochDescriptor;
	PISOCH_DESCRIPTOR	pIsochDescriptor;
	PIRB				pIrb;

    ENTER("t1394_IsochAttachCompletionRoutine");

/*	if(IsochDetachData)
		DbgPrint("1394CMDR: AttachCompletion says CycleTime = %d:%d:%d",
			IsochDetachData->IsochDescriptor->CycleTime.CL_CycleOffset,
			IsochDetachData->IsochDescriptor->CycleTime.CL_CycleCount,
			IsochDetachData->IsochDescriptor->CycleTime.CL_SecondCount);
*/

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {

//		DbgPrint("Isoch Attach Failed %08x",Irp->IoStatus.Status);

        TRACE(TL_ERROR, ("Isoch Attach Failed! = 0x%x\n", Irp->IoStatus.Status));
        ntStatus = Irp->IoStatus.Status;
    
        if (!IsochDetachData) {
        
            goto Exit_IsochAttachCompletionRoutine;
        }

        DeviceExtension = IsochDetachData->DeviceExtension;

        if (!DeviceExtension->bShutdown) {

            KeAcquireSpinLock(&DeviceExtension->IsochSpinLock, &Irql);
            RemoveEntryList(&IsochDetachData->IsochDetachList);
            KeCancelTimer(&IsochDetachData->Timer);

            TRACE(TL_TRACE, ("IsochAttachCompletionRoutine: IsochDetachData = 0x%x\n", IsochDetachData));
            TRACE(TL_TRACE, ("IsochAttachCompletionRoutine: IsochDetachData->Irp = 0x%x\n", IsochDetachData->Irp));
            TRACE(TL_TRACE, ("IsochAttachCompletionRoutine: IsochDetachData->newIrp = 0x%x\n", IsochDetachData->newIrp));

            KeAcquireSpinLock (&IsochDetachData->IsochTagSpinLock, &TagIrql);

            if (IsochDetachData->Tag != ISOCH_DETACH_TAG) {

                TRACE(TL_ERROR, ("Invalid Detach Tag!\n"));

                KeReleaseSpinLock(&IsochDetachData->IsochTagSpinLock, TagIrql);
                KeReleaseSpinLock(&DeviceExtension->IsochSpinLock, Irql);
            }
            else {

                // clear the tag...
                IsochDetachData->Tag = 0;

                KeReleaseSpinLock(&IsochDetachData->IsochTagSpinLock, TagIrql);
                KeReleaseSpinLock(&DeviceExtension->IsochSpinLock, Irql);

                TRACE(TL_TRACE, ("Complete Irp.\n"));

                if (IsochDetachData->AttachIrb)
                    ExFreePool(IsochDetachData->AttachIrb);

                for (i=0; i<IsochDetachData->numIsochDescriptors; i++) {

                    if (IsochDetachData->IsochDescriptor[i].Mdl)
					{
						//DbgPrint("Unlocking Pages");
						//MmUnlockPages(IsochDetachData->IsochDescriptor[i].Mdl);
                        //IoFreeMdl(IsochDetachData->IsochDescriptor[i].Mdl);
					}
                }
        
                ExFreePool(IsochDetachData->IsochDescriptor);

                //
                // Complete original Irp and free the one we allocated in
                // IsochAttachBuffers
                //

                IsochDetachData->Irp->IoStatus = Irp->IoStatus;
                IoCompleteRequest(IsochDetachData->Irp, IO_NO_INCREMENT);
                IoFreeIrp (IsochDetachData->newIrp);

                // all done with IsochDetachData, lets deallocate it...
                ExFreePool(IsochDetachData);
            }
        }
    }

Exit_IsochAttachCompletionRoutine:

    EXIT("t1394_IsochAttachCompletionRoutine", ntStatus);
    return(STATUS_MORE_PROCESSING_REQUIRED);
} // t1394_IsochAttachCompletionRoutine

/****************************************************************/
/*																*/
/*                New Attach Buffer                             */
/*																*/
/****************************************************************/


NTSTATUS
t1394_NewAttachBuffer(
    IN PDEVICE_OBJECT               DeviceObject,
    IN PIRP Irp,
	IN HANDLE hResource,
	OUT PMDL pMDL,
	IN OUT PRING3_ISOCH_DESCRIPTOR pR3
	)
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION           deviceExtension = DeviceObject->DeviceExtension;
    PIRB                        pIrb;
    ULONG                       i;
    PISOCH_DETACH_DATA          pIsochDetachData;
    PRING3_ISOCH_DESCRIPTOR     pR3TempDescriptor;
    KIRQL                       Irql;
    PIO_STACK_LOCATION          NextIrpStack;
    LARGE_INTEGER               deltaTime;
	//HANDLE						hResource = deviceExtension->CameraState.hIsochResource;
	PISOCH_DESCRIPTOR           pIsochDescriptor;

    PIRP                newIrp;
    CCHAR               StackSize;

	//DbgPrint("NewAttachbuffer for IRP %08x\n",Irp);

//	DbgPrint("NEW_ATTACH_BUFFER: pMDL = %08x",pMDL);
//	DbgPrint("NEW_ATTACH_BUFFER: pR3 = %08x",pR3);
//	DbgPrint("NEW_ATTACH_BUFFER: pR3->ulLength = %d",pR3->ulLength);


    //
    // Just assume that we are being passed a Irp that was created in UserMode
    // and allocate one here, because we need to keep an Irp around for
    // the callback routine.  Must submit this asynchronously so use IoAllocateIrp
    //
    StackSize = deviceExtension->StackDeviceObject->StackSize + 1;
    newIrp = IoAllocateIrp (StackSize, FALSE);

    if (!newIrp) {

        TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
        TRAP;
        
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_NewAttachBuffer;            
    }

    //
    // allocate the irb
    //
    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_NewAttachBuffer;
    } // if

    //
    // allocate our isoch descriptors
    //
    pIsochDescriptor = ExAllocatePool(NonPagedPool, sizeof(ISOCH_DESCRIPTOR));

    if (!pIsochDescriptor) {

        TRACE(TL_ERROR, ("Failed to allocate pIsochDescriptor!\n"));
        TRAP;

        if (pIrb)
            ExFreePool(pIrb);

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_NewAttachBuffer;
    }

	// now that the overhead is done with, grab the spinlock
    KeAcquireSpinLock(&deviceExtension->IsochSpinLock, &Irql);

    pIsochDescriptor->Mdl = pMDL;
	pIsochDescriptor->ulLength = MmGetMdlByteCount(pIsochDescriptor->Mdl);

	// strap in all our other stuff
	pIsochDescriptor->fulFlags = pR3->fulFlags;
    pIsochDescriptor->nMaxBytesPerFrame = pR3->nMaxBytesPerFrame;
    pIsochDescriptor->ulSynch = pR3->ulSynch;
    pIsochDescriptor->ulTag = pR3->ulTag;
    pIsochDescriptor->CycleTime = pR3->CycleTime;

    if (pR3->bUseCallback) {

		// need to save hResource, numDescriptors and Irp to use when detaching.
        // this needs to be done before we submit the irp, since the isoch callback
        // can be called before the submitirpsynch call completes.
        pIsochDetachData = ExAllocatePool(NonPagedPool, sizeof(ISOCH_DETACH_DATA));

        if (!pIsochDetachData) {

            TRACE(TL_ERROR, ("Failed to allocate pIsochDetachData!\n"));
            TRAP;

            if (pIsochDescriptor) {
                ExFreePool(pIsochDescriptor);
            }

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            KeReleaseSpinLock(&deviceExtension->IsochSpinLock, Irql);
            
            goto Exit_NewAttachBuffer;
        }

        KeInitializeSpinLock (&pIsochDetachData->IsochTagSpinLock);
        pIsochDetachData->Tag = ISOCH_DETACH_TAG;
        pIsochDetachData->AttachIrb = pIrb;

        InsertHeadList(&deviceExtension->IsochDetachData, &pIsochDetachData->IsochDetachList);

        KeInitializeTimer(&pIsochDetachData->Timer);
        KeInitializeDpc(&pIsochDetachData->TimerDpc, t1394_IsochTimeout, pIsochDetachData);

#define REQUEST_BUSY_RETRY_VALUE        (ULONG)(-100 * 100 * 100 * 100) //10 secs in units of 100nsecs

        deltaTime.LowPart = REQUEST_BUSY_RETRY_VALUE;
        deltaTime.HighPart = -1;
        KeSetTimer(&pIsochDetachData->Timer, deltaTime, &pIsochDetachData->TimerDpc);

        pIsochDetachData->outputBufferLength = pR3->ulLength;
        pIsochDetachData->DeviceExtension = deviceExtension;
        pIsochDetachData->hResource = hResource;
        pIsochDetachData->numIsochDescriptors = 1;
        pIsochDetachData->IsochDescriptor = pIsochDescriptor;
        pIsochDetachData->Irp = Irp;
        pIsochDetachData->newIrp = newIrp;
        pIsochDetachData->bDetach = pR3->bAutoDetach;

        pIsochDescriptor->Callback = t1394_IsochCallback;

        pIsochDescriptor->Context1 = deviceExtension;
        pIsochDescriptor->Context2 = pIsochDetachData;

	} else {
        pIsochDescriptor->Callback = NULL;
    }

    KeReleaseSpinLock(&deviceExtension->IsochSpinLock, Irql);

    // lets make sure the device is still around
    // if it isn't, we free the irb and return, our pnp
    // cleanup will take care of everything else
    if (deviceExtension->bShutdown) {

        TRACE(TL_TRACE, ("Shutdown!\n"));
        ntStatus = STATUS_NO_SUCH_DEVICE;
        goto Exit_NewAttachBuffer;
    }

    RtlZeroMemory (pIrb, sizeof (IRB));
    pIrb->FunctionNumber = REQUEST_ISOCH_ATTACH_BUFFERS;
    pIrb->Flags = 0;
    pIrb->u.IsochAttachBuffers.hResource = hResource;
    pIrb->u.IsochAttachBuffers.nNumberOfDescriptors = 1;
    pIrb->u.IsochAttachBuffers.pIsochDescriptor = pIsochDescriptor;

    // mark our original irp pending
    IoMarkIrpPending(Irp);
    
    //
    // Submit the newIrp directly to the driver below us
    //
    NextIrpStack = IoGetNextIrpStackLocation(newIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pIrb; 

    IoSetCompletionRoutine( newIrp,
                            t1394_IsochAttachCompletionRoutine,
                            pIsochDetachData,
                            TRUE,
                            TRUE,
                            TRUE
                            );

    IoCallDriver(deviceExtension->StackDeviceObject, newIrp);
    ntStatus = STATUS_PENDING;
    
Exit_NewAttachBuffer:

    //DbgPrint("Exit t1394_NewAttachBuffer (%d)", ntStatus);
    return(ntStatus);
} // t1394_NewAttachBuffer

