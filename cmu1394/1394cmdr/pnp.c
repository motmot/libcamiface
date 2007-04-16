/*++

Copyright (c) 1998	Microsoft Corporation

Module Name: 

    pnp.c

Abstract


Author:

    Peter Binder (pbinder) 4/13/98

Revision History:
Date     Who       What
-------- --------- ------------------------------------------------------------
4/13/98  pbinder   taken from 1394diag/ohcidiag
--*/

#define INITGUID
#include "pch.h"

NTSTATUS
t1394Diag_PnpBusReset(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
t1394Diag_PnpAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    )
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT          DeviceObject;
    PDEVICE_EXTENSION       deviceExtension;
	PCAMERA_STATE     cameraState;
    GUID                    t1394GUID;
    PNODE_DEVICE_EXTENSION  pNodeExt;

    ENTER("t1394Diag_PnpAddDevice");

    TRACE(TL_WARNING, ("Adding 1394CMDR.SYS.\n"));

    TRACE(TL_TRACE, ("DriverObject = 0x%x\n", DriverObject));
    TRACE(TL_TRACE, ("PhysicalDeviceObject = 0x%x\n", PhysicalDeviceObject));

    ntStatus = IoCreateDevice( DriverObject,
                               sizeof(DEVICE_EXTENSION),
                               NULL,
                               FILE_DEVICE_UNKNOWN,
                               0,
                               FALSE,
                               &DeviceObject
                               );

    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_ERROR, ("IoCreateDevice Failed!\n"));
        TRAP;
        goto Exit_PnpAddDevice;
    } // if

    TRACE(TL_TRACE, ("DeviceObject = 0x%x\n", DeviceObject));

	DeviceObject->Flags |= DO_DIRECT_IO;

    deviceExtension = DeviceObject->DeviceExtension;
	cameraState = &(deviceExtension->CameraState);
    RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));
	deviceExtension->CSR_offset = 0xffffffff;

    TRACE(TL_TRACE, ("deviceExtension = 0x%x\n", deviceExtension));

    t1394GUID = GUID_1394CMDR;

    ntStatus = IoRegisterDeviceInterface( PhysicalDeviceObject,
                                          &t1394GUID,
                                          NULL,
                                          &deviceExtension->SymbolicLinkName
                                          );

    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_ERROR, ("IoRegisterDeviceInterface Failed!\n"));
        TRAP;

        IoDeleteDevice(DeviceObject);
        ntStatus = STATUS_NO_SUCH_DEVICE;
        goto Exit_PnpAddDevice;
    }

    deviceExtension->StackDeviceObject =
        IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);

    if (!deviceExtension->StackDeviceObject) {

        TRACE(TL_ERROR, ("IoAttachDeviceToDeviceStack Failed!\n"));
        TRAP;

        IoDeleteDevice(DeviceObject);
        ntStatus = STATUS_NO_SUCH_DEVICE;
        goto Exit_PnpAddDevice;
    } // if
	
    TRACE(TL_TRACE, ("StackDeviceObject = 0x%x\n", deviceExtension->StackDeviceObject));

    // get the port device object
    pNodeExt = deviceExtension->StackDeviceObject->DeviceExtension;
    deviceExtension->PortDeviceObject = pNodeExt->PortDeviceObject;

    TRACE(TL_TRACE, ("PortDeviceObject = 0x%x\n", deviceExtension->PortDeviceObject));

    // initialize the spinlock/list to store the bus reset irps...
    KeInitializeSpinLock(&deviceExtension->ResetSpinLock);
    KeInitializeSpinLock(&deviceExtension->CromSpinLock);
    KeInitializeSpinLock(&deviceExtension->AsyncSpinLock);
    KeInitializeSpinLock(&deviceExtension->IsochSpinLock);
    KeInitializeSpinLock(&deviceExtension->IsochResourceSpinLock);
    InitializeListHead(&deviceExtension->BusResetIrps);
    InitializeListHead(&deviceExtension->CromData);
    InitializeListHead(&deviceExtension->AsyncAddressData);
    InitializeListHead(&deviceExtension->IsochDetachData);
    InitializeListHead(&deviceExtension->IsochResourceData);
	KeInitializeEvent(&(deviceExtension->NoBuffersAttachedEvent),NotificationEvent,FALSE);

    DeviceObject->Flags &= DO_POWER_PAGABLE;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	cameraState->IsochChannel = -1;

Exit_PnpAddDevice:

    EXIT("t1394Diag_PnpAddDevice", ntStatus);
    return(ntStatus);
} // t1394Diag_PnpAddDevice

NTSTATUS
t1394Diag_Pnp(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION      IrpSp;
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;

    PDEVICE_CAPABILITIES    DeviceCapabilities;

    ENTER("t1394Diag_Pnp");
	KdPrint("1394cmdr: Enter PnP\n");

    TRACE(TL_TRACE, ("DeviceObject = 0x%x\n", DeviceObject));
    TRACE(TL_TRACE, ("Irp = 0x%x\n", Irp));

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:
            TRACE(TL_TRACE, ("t1394Diag_Pnp: IRP_MN_START_DEVICE\n"));

            // pass down to layer below us first.
            ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);

            if (!NT_SUCCESS(ntStatus)) {

                TRACE(TL_ERROR, ("Error submitting Irp!\n"))
            //        TRAP;
            }
            ntStatus = t1394Diag_PnpStartDevice(DeviceObject, Irp);

            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            TRACE(TL_TRACE, ("t1394Diag_Pnp: IRP_MN_QUERY_STOP_DEVICE\n"));

            // pass down to layer below us...
            ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);
            break;

        case IRP_MN_STOP_DEVICE:
            TRACE(TL_TRACE, ("t1394Diag_Pnp: IRP_MN_STOP_DEVICE\n"));

            ntStatus = t1394Diag_PnpStopDevice(DeviceObject, Irp);

            // i'm not allowed to fail here. if i needed to fail, i should have done it
            // at the query_stop_device request.
            Irp->IoStatus.Status = STATUS_SUCCESS;

            // pass down to layer below us...
            ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:
            TRACE(TL_TRACE, ("t1394Diag_Pnp: IRP_MN_CANCEL_STOP_DEVICE\n"));

            // pass down to layer below us first.
            ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);

            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            TRACE(TL_TRACE, ("t1394Diag_Pnp: IRP_MN_QUERY_REMOVE_DEVICE\n"));

            Irp->IoStatus.Status = STATUS_SUCCESS;

            // pass down to layer below us...
            ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);
            break;

        case IRP_MN_SURPRISE_REMOVAL:
			KdPrint("t1394Diag_Pnp: Surprise Removal!\n");
        case IRP_MN_REMOVE_DEVICE:
			KdPrint("t1394Diag_Pnp: Shutting down and cleaning up for removal\n");
            TRACE(TL_TRACE, ("t1394Diag_Pnp: IRP_MN_REMOVE_DEVICE or IRP_MN_SURPRISE_REMOVAL\n"));

            ntStatus = t1394Diag_PnpRemoveDevice(DeviceObject, Irp);

            // pass down to layer below us...
            ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);

            // delete our device
            IoDetachDevice(deviceExtension->StackDeviceObject);
            IoDeleteDevice(DeviceObject);
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            TRACE(TL_TRACE, ("t1394Diag_Pnp: IRP_MN_CANCEL_REMOVE_DEVICE\n"));

            // pass down to layer below us first.
            ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);

            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        case IRP_MN_BUS_RESET:
            TRACE(TL_TRACE, ("t1394Diag_Pnp: IRP_MN_BUS_RESET\n"));

            ntStatus = t1394Diag_PnpBusReset(DeviceObject, Irp);

            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            TRACE(TL_TRACE, ("t1394Diag_Pnp: IRP_MN_QUERY_CAPABILITIES\n"));

            DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

            DeviceCapabilities->SurpriseRemovalOK = TRUE;

            ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);

            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        default:
            TRACE(TL_TRACE, ("Unsupported Pnp Function = 0x%x\n", IrpSp->MinorFunction));

            // pass down to layer below us...
            ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, Irp, NULL);

            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

    } // switch

    EXIT("t1394Diag_Pnp", ntStatus);
    return(ntStatus);
} // t1394Diag_Pnp

NTSTATUS
t1394Diag_PnpStartDevice(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;

    ENTER("t1394Diag_PnpStartDevice");

    deviceExtension->bShutdown = FALSE;

    ntStatus = t1394_BusResetNotification( DeviceObject,
                                               Irp,
                                               REGISTER_NOTIFICATION_ROUTINE
                                               );

    // update the generation count...
    ntStatus = t1394_GetGenerationCount(DeviceObject, Irp, &deviceExtension->GenerationCount);

    // activate the interface...
    ntStatus = IoSetDeviceInterfaceState(&deviceExtension->SymbolicLinkName, TRUE);

    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_ERROR, ("Failed to activate interface!\n"));
        TRAP;
    }

    EXIT("t1394Diag_PnpStartDevice", ntStatus);
    return(ntStatus);
} // t1394Diag_PnpStartDevice

NTSTATUS
t1394Diag_PnpStopDevice(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;

    ENTER("t1394Diag_PnpStopDevice");

    deviceExtension->bShutdown = TRUE;

    ntStatus = t1394_BusResetNotification( DeviceObject,
                                               Irp,
                                               DEREGISTER_NOTIFICATION_ROUTINE
                                               );

    EXIT("t1394Diag_PnpStopDevice", ntStatus);
    return(ntStatus);
} // t1394Diag_PnpStopDevice

NTSTATUS
t1394Diag_PnpRemoveDevice(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    KIRQL               Irql;

    ENTER("t1394Diag_PnpRemoveDevice");

    TRACE(TL_WARNING, ("Removing 1394CMDR.SYS.\n"));

    if (!deviceExtension->bShutdown) {

        // haven't stopped yet, lets do so
        ntStatus = t1394Diag_PnpStopDevice(DeviceObject, Irp);
    }

    // deactivate the interface...
    ntStatus = IoSetDeviceInterfaceState(&deviceExtension->SymbolicLinkName, FALSE);

    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_ERROR, ("Failed to deactivate interface!\n"));
        TRAP;
    }

    // lets free up any crom data structs we've allocated...
    KeAcquireSpinLock(&deviceExtension->CromSpinLock, &Irql);

    while (!IsListEmpty(&deviceExtension->CromData)) {

        PCROM_DATA      CromData;

        // get struct off list
        CromData = (PCROM_DATA) RemoveHeadList(&deviceExtension->CromData);

        // need to free up everything associated with this allocate...
        if (CromData->Buffer)
            ExFreePool(CromData->Buffer);

        if (CromData->pMdl)
            ExFreePool(CromData->pMdl);

        if (CromData)
            ExFreePool(CromData);
    }

    KeReleaseSpinLock(&deviceExtension->CromSpinLock, Irql);

    // lets free up any allocated addresses and deallocate all
    // memory associated with them...
    KeAcquireSpinLock(&deviceExtension->AsyncSpinLock, &Irql);

    while (!IsListEmpty(&deviceExtension->AsyncAddressData)) {

        PASYNC_ADDRESS_DATA     AsyncAddressData;

        // get struct off list
        AsyncAddressData = (PASYNC_ADDRESS_DATA) RemoveHeadList(&deviceExtension->AsyncAddressData);

        // need to free up everything associated with this allocate...
        if (AsyncAddressData->pMdl)
            ExFreePool(AsyncAddressData->pMdl);

        if (AsyncAddressData->Buffer)
            ExFreePool(AsyncAddressData->Buffer);

        if (AsyncAddressData->AddressRange)
            ExFreePool(AsyncAddressData->AddressRange);

        if (AsyncAddressData)
            ExFreePool(AsyncAddressData);
    }

    KeReleaseSpinLock(&deviceExtension->AsyncSpinLock, Irql);

    // free up any attached isoch buffers
    while (TRUE) {

        KeAcquireSpinLock(&deviceExtension->IsochSpinLock, &Irql);

        if (!IsListEmpty(&deviceExtension->IsochDetachData)) {

            PISOCH_DETACH_DATA      IsochDetachData;

            IsochDetachData = (PISOCH_DETACH_DATA)RemoveHeadList(&deviceExtension->IsochDetachData);

            TRACE(TL_TRACE, ("Surprise Removal: IsochDetachData = 0x%x\n", IsochDetachData));

            KeCancelTimer(&IsochDetachData->Timer);

            // clear the tag...
            IsochDetachData->Tag = 0;

            KeReleaseSpinLock(&deviceExtension->IsochSpinLock, Irql);

            TRACE(TL_TRACE, ("Surprise Removal: IsochDetachData->Irp = 0x%x\n", IsochDetachData->Irp));

            // need to save the status of the attach
            // we'll clean up in the same spot for success's and timeout's
            IsochDetachData->AttachStatus = STATUS_SUCCESS;

            // detach no matter what...
            IsochDetachData->bDetach = TRUE;

			DbgPrint("Surprise Removal: calling IsochCleanup for 0x%08x\n",IsochDetachData);

            t1394_IsochCleanup(IsochDetachData);
        }
        else {

            KeReleaseSpinLock(&deviceExtension->IsochSpinLock, Irql);
            break;
        }
    }

    // remove any isoch resource data
    while (TRUE) {

        KeAcquireSpinLock(&deviceExtension->IsochResourceSpinLock, &Irql);

        if (!IsListEmpty(&deviceExtension->IsochResourceData)) {

            PISOCH_RESOURCE_DATA    IsochResourceData;

            IsochResourceData = (PISOCH_RESOURCE_DATA)RemoveHeadList(&deviceExtension->IsochResourceData);

            KeReleaseSpinLock(&deviceExtension->IsochResourceSpinLock, Irql);

            TRACE(TL_TRACE, ("Surprise Removal: IsochResourceData = 0x%x\n", IsochResourceData));

            if (IsochResourceData) {

                PIRB                pIrb;
                NTSTATUS            ntStatus;
                PIRP                ResourceIrp;
                CCHAR               StackSize;

                TRACE(TL_TRACE, ("Surprise Removal: Freeing hResource = 0x%x\n", IsochResourceData->hResource));

                StackSize = deviceExtension->StackDeviceObject->StackSize + 1;
                ResourceIrp = IoAllocateIrp(StackSize, FALSE);

                if (ResourceIrp == NULL) {

                    TRACE(TL_ERROR, ("Failed to allocate ResourceIrp!\n"));
                    TRAP;
                }
                else {

                    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

                    if (!pIrb) {

                        IoFreeIrp(ResourceIrp);

                        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
                        TRAP;
                    }
                    else {

                        pIrb->FunctionNumber = REQUEST_ISOCH_FREE_RESOURCES;
                        pIrb->Flags = 0;
                        pIrb->u.IsochFreeResources.hResource = IsochResourceData->hResource;

                        ntStatus = t1394_SubmitIrpSynch(deviceExtension->StackDeviceObject, ResourceIrp, pIrb);

                        if (!NT_SUCCESS(ntStatus)) {

                            TRACE(TL_ERROR, ("SubmitIrpSync failed = 0x%x\n", ntStatus));
                            TRAP;
                        }

                        ExFreePool(pIrb);
                        IoFreeIrp(ResourceIrp);
                    }
                }
            }
        }
        else {

            KeReleaseSpinLock(&deviceExtension->IsochResourceSpinLock, Irql);
            break;
        }
    }

    // get rid of any pending bus reset notify requests
    KeAcquireSpinLock(&deviceExtension->ResetSpinLock, &Irql);

    while (!IsListEmpty(&deviceExtension->BusResetIrps)) {

        PBUS_RESET_IRP  BusResetIrp;
        PDRIVER_CANCEL  prevCancel = NULL;

        // get the irp off of the list
        BusResetIrp = (PBUS_RESET_IRP)RemoveHeadList(&deviceExtension->BusResetIrps);

        TRACE(TL_TRACE, ("BusResetIrp = 0x%x\n", BusResetIrp));

        // make this irp non-cancelable...
        prevCancel = IoSetCancelRoutine(BusResetIrp->Irp, NULL);

        TRACE(TL_TRACE, ("Surprise Removal: BusResetIrp->Irp = 0x%x\n", BusResetIrp->Irp));

        // and complete it...
        BusResetIrp->Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(BusResetIrp->Irp, IO_NO_INCREMENT);

        ExFreePool(BusResetIrp);
    }

    KeReleaseSpinLock(&deviceExtension->ResetSpinLock, Irql);

    // free up the symbolic link
    RtlFreeUnicodeString(&deviceExtension->SymbolicLinkName);

    EXIT("t1394Diag_PnpRemoveDevice", ntStatus);
    return(ntStatus);
} // t1394Diag_PnpRemoveDevice

NTSTATUS
t1394Diag_PnpBusReset(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    KIRQL               Irql;
    PDRIVER_CANCEL      prevCancel = NULL;
    PBUS_RESET_IRP      BusResetIrp = NULL;

    ENTER("t1394Diag_PnpBusReset");

    if (deviceExtension->bShutdown)
        return(ntStatus);

    // update the generation count...
    ntStatus = t1394_GetGenerationCount(DeviceObject, Irp, &deviceExtension->GenerationCount);

    // if we have any bus reset notify irps, then nows the time to complete them
    KeAcquireSpinLock(&deviceExtension->ResetSpinLock, &Irql);

    while (!IsListEmpty(&deviceExtension->BusResetIrps)) {

        // get the irp off of the list
        BusResetIrp = (PBUS_RESET_IRP)RemoveHeadList(&deviceExtension->BusResetIrps);

        TRACE(TL_TRACE, ("BusResetIrp = 0x%x\n", BusResetIrp));

        // make this irp non-cancelable...
        prevCancel = IoSetCancelRoutine(BusResetIrp->Irp, NULL);

        TRACE(TL_TRACE, ("Completing BusResetIrp->Irp = 0x%x\n", BusResetIrp->Irp));

        // and complete it...
        BusResetIrp->Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(BusResetIrp->Irp, IO_NO_INCREMENT);

        ExFreePool(BusResetIrp);
    }

    KeReleaseSpinLock(&deviceExtension->ResetSpinLock, Irql);

    EXIT("t1394Diag_PnpBusReset", ntStatus);
    return(ntStatus);
} // t1394Diag_PnpBusReset


