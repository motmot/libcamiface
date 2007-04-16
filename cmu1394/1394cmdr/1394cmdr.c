/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    1394diag.c

Abstract:

Author:

    Peter Binder (pbinder) 4/13/98

Revision History:
Date     Who       What
-------- --------- ------------------------------------------------------------
4/13/98  pbinder   taken from original 1394diag...
--*/

#define _1394DIAG_C
#include "pch.h"
#undef _1394DIAG_C

#if DBG

unsigned char t1394DiagDebugLevel = TL_WARNING;
unsigned char t1394DiagTrapLevel = FALSE;

#endif

NTSTATUS
t1394Cmdr_GetConfigInfo(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    ENTER("DriverEntry");

    DriverObject->MajorFunction[IRP_MJ_CREATE]          = t1394Diag_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = t1394Diag_Close;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = t1394Diag_Pnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = t1394Diag_Power;
//    DriverObject->MajorFunction[IRP_MJ_READ]            = t1394Diag_AsyncRead;
//    DriverObject->MajorFunction[IRP_MJ_WRITE]           = t1394Diag_Write;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = t1394Diag_IoControl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = t1394Diag_IoControl;
    DriverObject->DriverExtension->AddDevice            = t1394Diag_PnpAddDevice;

//    DriverObject->DriverUnload                          = OhciDiag_Unload;

    EXIT("DriverEntry", ntStatus);
    return(ntStatus);
} // DriverEntry

NTSTATUS
t1394Diag_Create(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION)(DeviceObject->DeviceExtension);
	char *t;

    ENTER("t1394Diag_Create");

	if(deviceExtension->CSR_offset == 0xffffffff)
	{
		DbgPrint("1394CMDR: oooh, my first handle... lemme figure out the particulars\n");
		DbgPrint("1394CMDR: my version is 6.02.00.0000");
		if(STATUS_SUCCESS != t1394Cmdr_GetConfigInfo(DeviceObject,Irp))
		{
			DbgPrint("1394CMDR: error getting configuration information");
		} else {
			DbgPrint("1394CMDR: my offset is: 0x%08x\n",deviceExtension->CSR_offset);
			DbgPrint("1394CMDR: my specification ID is 0x%06x\n",deviceExtension->unit_spec_ID);
			if(deviceExtension->unit_spec_ID != 0x00A02D)
				DbgPrint("1394CMDR: WARNING! this does not appear to conform with the 1394 DCS\n");
			switch(deviceExtension->unit_sw_version)
			{
			case 0x000100:
				t = "1.04";
				break;
			case 0x000101:
				t = "1.20";
				break;
			case 0x000102:
				t = "1.30";
				break;
			default:
				t = "unknown";
			}
			DbgPrint("1394CMDR: my software version is 0x%06x (%s)\n",
					deviceExtension->unit_sw_version, t);

			DbgPrint("1394CMDR: my name is: %s (len = %d)\n",
				      &(deviceExtension->pModelLeaf->TL_Data),
					  deviceExtension->ModelNameLength);

			DbgPrint("1394CMDR: my vendor is: %s (len = %d)\n",
				      &(deviceExtension->pVendorLeaf->TL_Data),
					  deviceExtension->VendorNameLength);

			DbgPrint("1394CMDR: config rom at 0x%08x\n",
				deviceExtension->pConfigRom);

			DbgPrint("1394CMDR:    signature = %08x (should be 31333934)\n",
				bswap(deviceExtension->pConfigRom->CR_Signiture));

			DbgPrint("1394CMDR:    node uniqueID = %08x%08x\n",
				deviceExtension->pConfigRom->CR_Node_UniqueID[0],
				deviceExtension->pConfigRom->CR_Node_UniqueID[1]);

		}
	}

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    EXIT("t1394Diag_Create", ntStatus);
    return(ntStatus);
} // t1394Diag_Create

NTSTATUS
t1394Diag_Close(
    IN PDEVICE_OBJECT   DriverObject,
    IN PIRP             Irp
    )
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    ENTER("t1394Diag_Close");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    EXIT("t1394Diag_Close", ntStatus);
    return(ntStatus);
} // t1394Diag_Close

void
t1394Diag_CancelIrp(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    KIRQL               Irql;
    PBUS_RESET_IRP      BusResetIrp;
    PDEVICE_EXTENSION   deviceExtension;

    ENTER("t1394Diag_CancelIrp");

    deviceExtension = DeviceObject->DeviceExtension;

    KeAcquireSpinLock(&deviceExtension->ResetSpinLock, &Irql);

    BusResetIrp = (PBUS_RESET_IRP) deviceExtension->BusResetIrps.Flink;

    TRACE(TL_TRACE, ("Irp = 0x%x\n", Irp));

    while (BusResetIrp) {

        TRACE(TL_TRACE, ("Cancelling BusResetIrp->Irp = 0x%x\n", BusResetIrp->Irp));

        if (BusResetIrp->Irp == Irp) {

            RemoveEntryList(&BusResetIrp->BusResetIrpList);
            ExFreePool(BusResetIrp);
            break;
        }
        else if (BusResetIrp->BusResetIrpList.Flink == &deviceExtension->BusResetIrps) {
            break;
        }
        else
            BusResetIrp = (PBUS_RESET_IRP)BusResetIrp->BusResetIrpList.Flink;
    }

    KeReleaseSpinLock(&deviceExtension->ResetSpinLock, Irql);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    EXIT("t1394Diag_CancelIrp", STATUS_SUCCESS);
} // t1394Diag_CancelIrp

/////////////////////////////////////////////////////

/*
 * GetState
 *
 * The bottom half of the state stub... as yet unused
 */

NTSTATUS
t1394Cmdr_GetState(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
	OUT PLONG			pState
    )
{

	PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
	*pState = deviceExtension->Status;
	return STATUS_SUCCESS;
}

/*
 * ResetState
 *
 * Clears out any Isoch Channel, Bandwidth, or Resources
 * Detaches and frees any DMA buffers
 * Tells the Camera to Stop streaming data
 */
	

NTSTATUS
t1394Cmdr_ResetState(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
	ULONG CSR_offset;
	PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
	PCAMERA_STATE cameraState = &(deviceExtension->CameraState);
	NTSTATUS ntStatus;
	UCHAR bytes[4];

	CSR_offset = deviceExtension->CSR_offset;

	if(CSR_offset == 0xffffffff)
	{
		DbgPrint("1394CMDR: no CSR_offset available... what the hell's going on?\n");
		return STATUS_UNSUCCESSFUL;
	}

	/* tell the camera to stop streaming */

	RtlZeroMemory(bytes,4);
	ntStatus = t1394Cmdr_WriteRegister(DeviceObject,Irp,deviceExtension->CSR_offset + 0x614,bytes);
    if (!NT_SUCCESS(ntStatus))
	{
        TRACE(TL_ERROR, ("1394CMDR: Write Error = 0x%x\n", ntStatus));
		goto Exit_ResetState;
	}

	// send some isoch stop

	if(cameraState->hIsochResource != NULL)
	{
		ntStatus = t1394_IsochStop(DeviceObject, Irp, cameraState->hIsochResource, 0);
		if (!NT_SUCCESS(ntStatus))
		{
			TRACE(TL_ERROR, ("Error on IsochStop = 0x%x\n", ntStatus));
			goto Exit_ResetState;
		}
	}

	// deallocate any attached buffers

	while (TRUE) {
		KIRQL               Irql;

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

			// need to save the ntStatus of the attach
			// we'll clean up in the same spot for success's and timeout's
			IsochDetachData->AttachStatus = STATUS_SUCCESS;

			// detach no matter what...
			IsochDetachData->bDetach = TRUE;

			t1394_IsochCleanup(IsochDetachData);
		}
		else {

			KeReleaseSpinLock(&deviceExtension->IsochSpinLock, Irql);
			break;
		}
	}

	
	if(cameraState->hIsochBandwidth != NULL)
	{
		DbgPrint("1394CMDR:\t0x%08x : bandwidth",cameraState->hIsochBandwidth);
		ntStatus = t1394_IsochFreeBandwidth(DeviceObject,Irp,cameraState->hIsochBandwidth);
	    if (!NT_SUCCESS(ntStatus))
		{
	        TRACE(TL_ERROR, ("IsochFreeBandwidth Failed = 0x%x\n", ntStatus));
			goto Exit_ResetState;
		}
	}

	if(cameraState->hIsochResource != NULL)
	{
		DbgPrint("1394CMDR:\t0x%08x : Resource",cameraState->hIsochResource);
		ntStatus = t1394_IsochFreeResources(DeviceObject,Irp,cameraState->hIsochResource);
	    if (!NT_SUCCESS(ntStatus))
		{
	        TRACE(TL_ERROR, ("IsochFreeResources Failed = 0x%x\n", ntStatus));
			goto Exit_ResetState;
		}
	}

	if(cameraState->IsochChannel != -1)
	{
		DbgPrint("1394CMDR:\t  %8d  : Channel",cameraState->IsochChannel);
		ntStatus = t1394_IsochFreeChannel(DeviceObject,Irp,cameraState->IsochChannel);
	    if (!NT_SUCCESS(ntStatus))
		{
	        TRACE(TL_ERROR, ("IsochFreeChannel Failed = 0x%x\n", ntStatus));
			goto Exit_ResetState;
		}
	}

Exit_ResetState:

	if(!NT_SUCCESS(ntStatus))
		DbgPrint("1394CMDR: Error 0x%08x while Resetting Camera State\n");
 
	//ntStatus = STATUS_SUCCESS;

	return ntStatus;
}

/*
 * The tracelevel lower half stub
 */

NTSTATUS
SetCmdrTraceLevel( 
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
	IN LONG			dwLevel
    )
{
	PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
	deviceExtension->TraceLevel = dwLevel;
	return STATUS_SUCCESS;
}

NTSTATUS
t1394Cmdr_ReadRegister( 
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
	IN ULONG			ulOffset,
	OUT PUCHAR			pData
    )
{
	IO_ADDRESS DestinationAddress;
	PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
	NTSTATUS ntStatus = STATUS_SUCCESS;

	if(!pData)
		return STATUS_INSUFFICIENT_RESOURCES;

	pData[0] = pData[1] = pData[2] = pData[3] = 0;
	RtlZeroMemory(&DestinationAddress,sizeof(IO_ADDRESS));
	DestinationAddress.IA_Destination_Offset.Off_High = 0xffff;
	DestinationAddress.IA_Destination_Offset.Off_Low = ulOffset;
	ntStatus = t1394_AsyncRead(	DeviceObject,
								Irp,
								FALSE,  // raw mode
								TRUE,   // get generation
								DestinationAddress,
								4, // nNumberOfBytesToRead
								0, // nBlockSize (0 = default)
								0, // Flags
								0, // Generation (ignored if get generation is true),
								(PULONG) pData);
	if(!NT_SUCCESS(ntStatus))
		DbgPrint("1394CMDR: ReadRegister: error %08x on AsyncRead\n",ntStatus);
	return ntStatus;
}

/*
 * WriteRegister
 *
 * Writes a quadlet from pData into the camera registers at the (absolute) offset ulOffset
 */


NTSTATUS
t1394Cmdr_WriteRegister( 
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
	IN ULONG			ulOffset,
	IN PUCHAR			pData
    )
{
	IO_ADDRESS DestinationAddress;
	PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
	NTSTATUS ntStatus = STATUS_SUCCESS;

	if(!pData)
		return STATUS_INSUFFICIENT_RESOURCES;

	RtlZeroMemory(&DestinationAddress,sizeof(IO_ADDRESS));
	DestinationAddress.IA_Destination_Offset.Off_High = 0xffff;
	DestinationAddress.IA_Destination_Offset.Off_Low = ulOffset;
	ntStatus = t1394_AsyncWrite(DeviceObject,
								Irp,
								FALSE,  // raw mode
								TRUE,   // get generation
								DestinationAddress,
								4, // nNumberOfBytesToWrite
								0, // nBlockSize (0 = default)
								0, // Flags
								0, // Generation (ignored if get generation is true),
								(PULONG)pData);
	if(!NT_SUCCESS(ntStatus))
		DbgPrint("WriteRegister: error %08x on SubmitIrpSynch\n",ntStatus);
	return ntStatus;
}

/*
 * GetConfigInfo
 *
 * queries the camera to figure out CSR offset, vendor name and model name
 */

NTSTATUS
t1394Cmdr_GetConfigInfo(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRB                pIrb;
    PMDL                pMdl;

    PDEVICE_OBJECT      NextDeviceObject;
    PIRP                newIrp = NULL;
    BOOLEAN             allocNewIrp = FALSE;
    KEVENT              Event;
    IO_STATUS_BLOCK     ioStatus = Irp->IoStatus;
	ULONG foo;
	PULONG ulptr;


    //
    // get the location of the next device object in the stack
    //
    NextDeviceObject = deviceExtension->StackDeviceObject;

    //
    // If this is a UserMode request create a newIrp so that the request
    // will be issued from KernelMode
    //
    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, NextDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {
            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_GetStuff;            
        }
        allocNewIrp = TRUE;
    }

    pIrb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if (!pIrb) {

        TRACE(TL_ERROR, ("Failed to allocate pIrb!\n"));
        TRAP;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_GetStuff;
    } // if

    RtlZeroMemory (pIrb, sizeof (IRB));
    
    //
    // figure out how much configuration space we need by setting lengths to zero.
    //

    pIrb->FunctionNumber = REQUEST_GET_CONFIGURATION_INFO;
    pIrb->Flags = 0;
    pIrb->u.GetConfigurationInformation.UnitDirectoryBufferSize = 0;
    pIrb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize = 0;
    pIrb->u.GetConfigurationInformation.VendorLeafBufferSize = 0;
    pIrb->u.GetConfigurationInformation.ModelLeafBufferSize = 0;

    if (allocNewIrp) 
	{
        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (NextDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) 
		{
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    } else {
        ntStatus = t1394_SubmitIrpSynch(NextDeviceObject, Irp, pIrb);
    }

    if (!NT_SUCCESS(ntStatus))
        goto Exit_GetStuff;
/*
	DbgPrint("GetConfigInfo: UD:%d UDD:%d VL:%d ML:%d\n",
			pIrb->u.GetConfigurationInformation.UnitDirectoryBufferSize,
			pIrb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize,
			pIrb->u.GetConfigurationInformation.VendorLeafBufferSize,
			pIrb->u.GetConfigurationInformation.ModelLeafBufferSize);
*/
	//
    // Now go thru and allocate what we need to so we can get our info.
    //

    deviceExtension->pConfigRom = ExAllocatePool(NonPagedPool, sizeof(CONFIG_ROM));
    if (!deviceExtension->pConfigRom) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_GetStuff;
    }


    deviceExtension->pUnitDirectory = ExAllocatePool(NonPagedPool, pIrb->u.GetConfigurationInformation.UnitDirectoryBufferSize);
    if (!deviceExtension->pUnitDirectory) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_GetStuff;
    }


    if (pIrb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize) {
        deviceExtension->pUnitDependentDirectory = ExAllocatePool(NonPagedPool, pIrb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize);
        if (!deviceExtension->pUnitDependentDirectory) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_GetStuff;
        }
    }


    if (pIrb->u.GetConfigurationInformation.VendorLeafBufferSize) {
        // From NonPaged pool since vendor name can be used in a func with DISPATCH level
        deviceExtension->pVendorLeaf = ExAllocatePool(NonPagedPool, pIrb->u.GetConfigurationInformation.VendorLeafBufferSize);
        if (!deviceExtension->pVendorLeaf) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_GetStuff;
        }
    }

    if (pIrb->u.GetConfigurationInformation.ModelLeafBufferSize) {
        deviceExtension->pModelLeaf = ExAllocatePool(NonPagedPool, pIrb->u.GetConfigurationInformation.ModelLeafBufferSize);
        if (!deviceExtension->pModelLeaf) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_GetStuff;
        }
    }

    //
    // Now resubmit the pIrb with the appropriate pointers inside
    //

	allocNewIrp = FALSE;
	newIrp = NULL;

    if (Irp->RequestorMode == UserMode) {

        newIrp = IoBuildDeviceIoControlRequest (IOCTL_1394_CLASS, NextDeviceObject, 
                            NULL, 0, NULL, 0, TRUE, &Event, &ioStatus);

        if (!newIrp) {
            TRACE(TL_ERROR, ("Failed to allocate newIrp!\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_GetStuff;            
        }
        allocNewIrp = TRUE;
    }

    pIrb->FunctionNumber = REQUEST_GET_CONFIGURATION_INFO;
    pIrb->Flags = 0;
    pIrb->u.GetConfigurationInformation.ConfigRom = deviceExtension->pConfigRom;
    pIrb->u.GetConfigurationInformation.UnitDirectory = deviceExtension->pUnitDirectory;
    pIrb->u.GetConfigurationInformation.UnitDependentDirectory = deviceExtension->pUnitDependentDirectory;
    pIrb->u.GetConfigurationInformation.VendorLeaf = deviceExtension->pVendorLeaf;
    pIrb->u.GetConfigurationInformation.ModelLeaf = deviceExtension->pModelLeaf;

    if (allocNewIrp) 
	{
        KeInitializeEvent (&Event, NotificationEvent, FALSE);
        ntStatus = t1394_SubmitIrpAsync (NextDeviceObject, newIrp, pIrb);

        if (ntStatus == STATUS_PENDING) 
		{
            KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE, NULL); 
            ntStatus = ioStatus.Status;
        }
    } else {
        ntStatus = t1394_SubmitIrpSynch(NextDeviceObject, Irp, pIrb);
    }

    if (!NT_SUCCESS(ntStatus))
        goto Exit_GetStuff;

	// now move all the important info into the device extension

	if(deviceExtension->pVendorLeaf && deviceExtension->pVendorLeaf->TL_Length >= 1)
		deviceExtension->VendorNameLength = (USHORT) strlen(&(deviceExtension->pVendorLeaf->TL_Data));

	if(deviceExtension->pModelLeaf && deviceExtension->pModelLeaf->TL_Length >= 1)
		deviceExtension->ModelNameLength = (USHORT) strlen(&(deviceExtension->pModelLeaf->TL_Data));

	if(pIrb->u.GetConfigurationInformation.UnitDirectoryBufferSize < 16)
	{
		DbgPrint("GetConfigInfo: UnitDirectory size (%d) isn't correct, should be >= 16\n",
				pIrb->u.GetConfigurationInformation.UnitDirectoryBufferSize);
		ntStatus = STATUS_UNSUCCESSFUL;
		goto Exit_GetStuff;
	}

	if(pIrb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize < 16)
	{
		DbgPrint("GetConfigInfo: UnitDependentDirectory size (%d) isn't correct, should be >= 16\n",
				pIrb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize);
		ntStatus = STATUS_UNSUCCESSFUL;
		goto Exit_GetStuff;
	}

	ulptr = (PULONG) (deviceExtension->pUnitDirectory);
	foo = bswap(ulptr[1]) & 0x00ffffff;
	deviceExtension->unit_spec_ID = foo;

	foo = bswap(ulptr[2]) & 0x00ffffff;
	deviceExtension->unit_sw_version = foo;

	ulptr = (PULONG) (deviceExtension->pUnitDependentDirectory);
	foo = 0xf0000000 + 4 * (bswap(ulptr[1]) & 0x00ffffff);
	deviceExtension->CSR_offset = foo;

    return STATUS_SUCCESS;

///////////////////
// Exit_GetStuff //
///////////////////

Exit_GetStuff:

    if (allocNewIrp) 
        Irp->IoStatus = ioStatus;

	if(pIrb)
		ExFreePool(pIrb);

	if(!NT_SUCCESS(ntStatus))
	{
		// clean up our mess
		if(deviceExtension->pConfigRom) {
			ExFreePool(deviceExtension->pConfigRom);
			deviceExtension->pConfigRom = NULL;
		}

		if(deviceExtension->pUnitDirectory) {
			ExFreePool(deviceExtension->pUnitDirectory);
			deviceExtension->pUnitDirectory = NULL;
		}

		if(deviceExtension->pUnitDependentDirectory) {
			ExFreePool(deviceExtension->pUnitDependentDirectory);
			deviceExtension->pUnitDependentDirectory = NULL;
		}

		if(deviceExtension->pVendorLeaf) {
			ExFreePool(deviceExtension->pVendorLeaf);
			deviceExtension->pVendorLeaf = NULL;
		}

		if(deviceExtension->pModelLeaf) {
			ExFreePool(deviceExtension->pModelLeaf);
			deviceExtension->pModelLeaf = NULL;
		}
	}

    return ntStatus;
}


