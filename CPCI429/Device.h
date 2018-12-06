 /*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"

EXTERN_C_START

#define CPCI429_WRITE_MEMORY_OFFSET 0x00000000
#define CPCI429_READ_MEMORY_OFFSET 0x00000000

#define MAXLEN 1024

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
    ULONG PrivateDeviceData;  // just a placeholder
	PVOID MemBaseAddress;
	ULONG PhysicalAddressRegister;
	PVOID BAR0_VirtualAddress;
	ULONG Counter_i;
	ULONG MemLength;
	ULONG OffsetAddressFromApp;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD CPCI429EvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP CPCI429EvtDriverContextCleanup;

EVT_WDF_DEVICE_D0_ENTRY CPCI429EvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT CPCI429EvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE CPCI429EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE CPCI429EvtDeviceReleaseHardware;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL CPCI429EvtIoDeviceControl;

//
// Function to initialize the device and its callbacks
//
NTSTATUS
CPCI429CreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

NTSTATUS
CPCI429EvtDevicePrepareHardware(
	IN WDFDEVICE Device,
	IN WDFCMRESLIST ResourceList,
	IN WDFCMRESLIST ResourceListTranslated
);

NTSTATUS
CPCI429EvtDeviceReleaseHardware(
	IN WDFDEVICE Device,
	IN WDFCMRESLIST ResourceListTranslated
);

NTSTATUS
CPCI429EvtDeviceD0Entry(
	IN WDFDEVICE Device,
	IN WDF_POWER_DEVICE_STATE PreviousState
);

NTSTATUS
CPCI429EvtDeviceD0Exit(
	IN WDFDEVICE Device,
	IN WDF_POWER_DEVICE_STATE TargetState
);

EXTERN_C_END
