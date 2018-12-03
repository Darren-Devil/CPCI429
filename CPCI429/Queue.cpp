/*++

Module Name:

    queue.c

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "queue.tmh"

#pragma warning(disable:4100)

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, CPCI429QueueInitialize)
#pragma alloc_text (PAGE, CPCI429EvtIoDeviceControl)
#endif

NTSTATUS
CPCI429QueueInitialize(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;

    PAGED_CODE();

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
         &queueConfig,
        WdfIoQueueDispatchParallel
        );

    queueConfig.EvtIoDeviceControl = CPCI429EvtIoDeviceControl;
    queueConfig.EvtIoStop = CPCI429EvtIoStop;

    status = WdfIoQueueCreate(
                 Device,
                 &queueConfig,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &queue
                 );

    if(!NT_SUCCESS(status)) {
        //TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
		DbgPrint("[%s:%d]: WdfIoQueueCreate failed", __FUNCDNAME__, __LINE__);
        return status;
    }

    return status;
}

/*
单一的默认I/O队列和单一的请求处理函数，EvtIoDefault。KMDF将会将设备所有的请求发送到默认I/O队列，
然后它会调用驱动程序的EvtIoDefault来将每一个请求递交给驱动程序。

*单一的默认I/O队列和多个请求处理函数，例如EvtIoRead、EvtIoWrite和EvtIoDeviceControl。KMDF会将设备所有的请求发送到默认I/O队列。
然后会调用驱动程序的EvtIoRead处理函数来递交读请求、调用EvtIoWrite处理函数来递交写请求、调用EvtIoDeviceControl处理函数来递交设备I/O控制请求。
*/

VOID
CPCI429EvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

Return Value:

    VOID

--*/
{
	WDFDEVICE device;
	PDEVICE_CONTEXT pDeviceContext;

	NTSTATUS status;

	PVOID inBuffer;
	PVOID outBuffer;
	ULONG AddressOffset;

	device = WdfIoQueueGetDevice(Queue);
	pDeviceContext = DeviceGetContext(device);

	switch (IoControlCode) {
	//根据CTL_CODE请求码作相应的处理
	case CPCI429_IOCTL_WRITE_OFFSETADDRESS:
		status = WdfRequestRetrieveInputBuffer(
			Request,
			sizeof(ULONG),
			&inBuffer,
			NULL
		);
		pDeviceContext->OffsetAddressFromApp = *(ULONG*)inBuffer;
		WdfRequestCompleteWithInformation(Request, status, sizeof(ULONG));
		if (!NT_SUCCESS(status)) {
			DbgPrint("[%s:%d]: WdfRequest failed", __FUNCDNAME__, __LINE__);
			goto Exit;
		}
		break;

	case CPCI429_IOCTL_IN_BUFFERED:
		status = WdfRequestRetrieveInputBuffer(
			Request,
			sizeof(ULONG),
			&inBuffer,
			NULL
		);
		AddressOffset = CPCI429_WRITE_MEMORY_OFFSET + pDeviceContext->OffsetAddressFromApp;
		*(ULONG*)WDF_PTR_ADD_OFFSET(pDeviceContext->BAR0_VirtualAddress, AddressOffset) = *(ULONG*)inBuffer;
		WdfRequestCompleteWithInformation(
			Request,
			status,
			sizeof(ULONG)
		);
		if (!NT_SUCCESS(status)) {
			DbgPrint("[%s:%d]: WDFREQUESTFAILED", __FUNCDNAME__, __LINE__);
			goto Exit;
		}
		break;

	case CPCI429_IOCTL_OUT_BUFFERED:
		status = WdfRequestRetrieveOutputBuffer(
			Request,
			sizeof(ULONG),
			&outBuffer,
			NULL
		);
		AddressOffset = CPCI429_WRITE_MEMORY_OFFSET + pDeviceContext->OffsetAddressFromApp;
		*(ULONG*)outBuffer = *(ULONG*)WDF_PTR_ADD_OFFSET(pDeviceContext->BAR0_VirtualAddress, AddressOffset);
		WdfRequestCompleteWithInformation(
			Request,
			status,
			sizeof(ULONG)
		);
		if (!NT_SUCCESS(status)) {
			DbgPrint("[%s:%d]: RequestFailed!!!", __FUNCDNAME__, __LINE__);
			goto Exit;
		}
		break;

	case CPCI429_IOCTL_READ_PADDRESS:
		status = WdfRequestRetrieveOutputBuffer(
			Request,
			sizeof(ULONG),
			&outBuffer,
			NULL
		);
		*(ULONG*)outBuffer = pDeviceContext->PhysicalAddressRegister;
		WdfRequestCompleteWithInformation(
			Request,
			status,
			sizeof(ULONG)
		);
		if (!NT_SUCCESS(status)) {
			DbgPrint("[%s:%d]: outBuffer failed", __FUNCDNAME__, __LINE__);
			goto Exit;
		}
		break;

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		WdfRequestCompleteWithInformation(
			Request,
			status,
			0
		);
		break;
	}

    /*TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d", 
                Queue, Request, (int) OutputBufferLength, (int) InputBufferLength, IoControlCode);*/

    WdfRequestComplete(Request, STATUS_SUCCESS);

Exit:
	if (!NT_SUCCESS(status)) {
		WdfRequestCompleteWithInformation(
			Request,
			status,
			0
		);
	}
    return;
}

VOID
CPCI429EvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
/*++

Routine Description:

    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
                  that identify the reason that the callback function is being called
                  and whether the request is cancelable.

Return Value:

    VOID

--*/
{
    /*TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d", 
                Queue, Request, ActionFlags);*/

    //
    // In most cases, the EvtIoStop callback function completes, cancels, or postpones
    // further processing of the I/O request.
    //
    // Typically, the driver uses the following rules:
    //
    // - If the driver owns the I/O request, it calls WdfRequestUnmarkCancelable
    //   (if the request is cancelable) and either calls WdfRequestStopAcknowledge
    //   with a Requeue value of TRUE, or it calls WdfRequestComplete with a
    //   completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
    //
    //   Before it can call these methods safely, the driver must make sure that
    //   its implementation of EvtIoStop has exclusive access to the request.
    //
    //   In order to do that, the driver must synchronize access to the request
    //   to prevent other threads from manipulating the request concurrently.
    //   The synchronization method you choose will depend on your driver's design.
    //
    //   For example, if the request is held in a shared context, the EvtIoStop callback
    //   might acquire an internal driver lock, take the request from the shared context,
    //   and then release the lock. At this point, the EvtIoStop callback owns the request
    //   and can safely complete or requeue the request.
    //
    // - If the driver has forwarded the I/O request to an I/O target, it either calls
    //   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
    //   further processing of the request and calls WdfRequestStopAcknowledge with
    //   a Requeue value of FALSE.
    //
    // A driver might choose to take no action in EvtIoStop for requests that are
    // guaranteed to complete in a small amount of time.
    //
    // In this case, the framework waits until the specified request is complete
    // before moving the device (or system) to a lower power state or removing the device.
    // Potentially, this inaction can prevent a system from entering its hibernation state
    // or another low system power state. In extreme cases, it can cause the system
    // to crash with bugcheck code 9F.
    //

    return;
}
