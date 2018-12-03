 /*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//
#ifndef _USER_H
#define _USER_H

#include <initguid.h>

DEFINE_GUID (GUID_DEVINTERFACE_CPCI429,
    0xdd01f255,0x19ac,0x4e7e,0xae,0x35,0x15,0x6b,0xa0,0x4a,0xc4,0xe6);
// {dd01f255-19ac-4e7e-ae35-156ba04ac4e6}

#define CPCI429_IOCTL_IN_BUFFERED CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)//the least value is 0x800
#define CPCI429_IOCTL_OUT_BUFFERED CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define CPCI429_IOCTL_READ_PADDRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define CPCI429_IOCTL_WRITE_OFFSETADDRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif