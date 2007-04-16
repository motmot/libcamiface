/*++

Copyright (c) 1999  Microsoft Corporation

Module Name: 

    util.c

Abstract


Author:

    Peter Binder (pbinder) 7/13/99

Revision History:
Date     Who       What
-------- --------- ------------------------------------------------------------
7/13/99  pbinder   birth (functions taken from 1394diag)
--*/

#include "pch.h"

NTSTATUS
t1394_SubmitIrpAsync(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIRB                 Irb
    )
{
    PIO_STACK_LOCATION  NextIrpStack;
    NTSTATUS            ntStatus = STATUS_SUCCESS;

    ENTER("t1394_SubmitIrpAsync");

    if (Irb) {

        NextIrpStack = IoGetNextIrpStackLocation(Irp);
        NextIrpStack->Parameters.Others.Argument1 = Irb;
    }
    else {

        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    ntStatus = IoCallDriver (DeviceObject, Irp);
    return ntStatus;
}

NTSTATUS
t1394_SubmitIrpSynch(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIRB                 Irb
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    KEVENT              Event;
    PIO_STACK_LOCATION  NextIrpStack;

    ENTER("t1394_SubmitIrpSynch");

    TRACE(TL_TRACE, ("DeviceObject = 0x%x\n", DeviceObject));
    TRACE(TL_TRACE, ("Irp = 0x%x\n", Irp));
    TRACE(TL_TRACE, ("Irb = 0x%x\n", Irb));
 
	if (Irp->RequestorMode == UserMode)
		Irp->RequestorMode = KernelMode;

	if (Irb) {

        NextIrpStack = IoGetNextIrpStackLocation(Irp);
        NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
        NextIrpStack->Parameters.Others.Argument1 = Irb;
    }
    else {

        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    IoSetCompletionRoutine( Irp,
                            t1394_SynchCompletionRoutine,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE
                            );

    ntStatus = IoCallDriver(DeviceObject, Irp);

    if (ntStatus == STATUS_PENDING) {

        TRACE(TL_TRACE, ("t1394_SubmitIrpSynch: Irp is pending...\n"));
        
        KeWaitForSingleObject( &Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL
                               );

    }

    ntStatus = Irp->IoStatus.Status;

    EXIT("t1394_SubmitIrpSynch", ntStatus);
    return(ntStatus);
} // t1394_SubmitIrpSynch

NTSTATUS
t1394_SynchCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PKEVENT          Event
    )
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;

    ENTER("t1394_SynchCompletionRoutine");

    if (Event)
        KeSetEvent(Event, 0, FALSE);
    
    EXIT("t1394_SynchCompletionRoutine", ntStatus);
    return(STATUS_MORE_PROCESSING_REQUIRED);
} // t1394_SynchCompletionRoutine


