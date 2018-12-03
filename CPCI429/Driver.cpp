/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, CPCI429EvtDeviceAdd)
#pragma alloc_text (PAGE, CPCI429EvtDriverContextCleanup)
#endif

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = CPCI429EvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&config,
                           CPCI429EvtDeviceAdd
                           );

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             WDF_NO_HANDLE
                             );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

NTSTATUS
CPCI429EvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
	WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	WDFDEVICE device;
	PDEVICE_CONTEXT deviceContext;

	WDFQUEUE queue;
	WDF_IO_QUEUE_CONFIG queueConfig;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    //WdfDeviceIoDirect��ʽ
	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);
	//When the I/O manager sends a request for buffered I/O, the IRP contains an internal copy of the caller's buffer
    //rather than the caller's buffer itself. The I/O manager copies data from the caller's buffer to the internal buffer
	//during a write request or from the internal buffer to the caller's buffer when the driver completes a read
	//request.
	//The WDF driver receives a WDF request object, which in turn contains an embedded WDF memory object.
	//The memory object contains the address of the buffer on which the driver should operate.

	//��I/O������һ������I/O��������IRP���������߻��������ڲ������������ǻ���������
	//��д�����ڼ䣬I/O�����������ݴӵ����ߵĻ��������Ƶ��ڲ�������������������������ɶ�����ʱ���ڲ����������Ƶ������ߵĻ�����
	//WDF�����������һ��WDF������󣬸ö����ְ���һ��Ƕ��ʽWDF�ڴ����
	//�ڴ���������������Ӧ�������ϲ����Ļ������ĵ�ַ��

    //status = CPCI429CreateDevice(DeviceInit);

	//��ʼ�����弴�ú͵�Դ�����������ýṹ
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    //TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

	//���弴�û�������
	pnpPowerCallbacks.EvtDevicePrepareHardware = CPCI429EvtDevicePrepareHardware;
	pnpPowerCallbacks.EvtDeviceReleaseHardware = CPCI429EvtDeviceReleaseHardware;
	pnpPowerCallbacks.EvtDeviceD0Entry = CPCI429EvtDeviceD0Entry;
	pnpPowerCallbacks.EvtDeviceD0Exit = CPCI429EvtDeviceD0Exit;

	//ע�ἴ�弴�ú͵�Դ��������
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

	deviceAttributes.SynchronizationScope = WdfSynchronizationScopeDevice;

	status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
	if (!NT_SUCCESS(status)) {
		DbgPrint("[%s:%d]: CREATEFAILED", __FUNCDNAME__, __LINE__); 
		return status;
	}
	deviceContext = DeviceGetContext(device);
	//��default��ʼ��default���У�����һ����ʼ����default����
	WDF_IO_QUEUE_CONFIG_INIT(
		&queueConfig,
		WdfIoQueueDispatchSequential
	);

	queueConfig.EvtIoInternalDeviceControl = CPCI429EvtIoDeviceControl;

	status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
	if (!NT_SUCCESS(status)) {
		DbgPrint("[%s:%d]: IOCREATEFAILED", __FUNCDNAME__, __LINE__);
		return status;
	}
	//����Ĭ�϶���
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
		&queueConfig,
		WdfIoQueueDispatchParallel
	);
	
	queueConfig.EvtIoDeviceControl = CPCI429EvtIoDeviceControl;

	status = WdfIoQueueCreate(
		device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&queue
	);
	if (!NT_SUCCESS(status)) {
		DbgPrint("[%s:%d]: QUEUECREATEFAILED", __FUNCDNAME__, __LINE__);
		return status;
	}

#if 0
	//���ڷ�Ĭ�϶��У�����ָ��Ҫ�ַ���I/O��������
	status = WdfDeviceConfigureRequestDispatching(
		device,
		queue,
		WdfRequestTypeDeviceControl
		);
	if (!NT_SUCCESS(status)) {
		DbgPrint("[%s:%d:%d:%x]: REQUESTLOSE", __FUNCDNAME__, __LINE__, status, status);
		return status;
	}
#endif

	status = WdfDeviceCreateDeviceInterface(
		device,
		(LPGUID)&GUID_DEVINTERFACE_CPCI429,
		NULL //Referencestring ���ʴ�
	);
	if (!NT_SUCCESS(status)) {
		DbgPrint("[%s:%d]: CREATEINTERFACEFAILED", __FUNCDNAME__, __LINE__);
		return status;
	}

    return status;
}

VOID
CPCI429EvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
    )
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE();

    //TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Stop WPP Tracing
    //
    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
}
