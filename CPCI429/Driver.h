/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/
#define INIITGUID

#pragma warning(disable:4200)
#pragma warning(disable:4201)
#pragma warning(disable:4214)

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>

#include "Public.h"
#include "device.h"
#include "queue.h"
#include "trace.h"

EXTERN_C_START

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD CPCI429EvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP CPCI429EvtDriverContextCleanup;

EXTERN_C_END
