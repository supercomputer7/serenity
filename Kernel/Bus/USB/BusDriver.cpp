/*
 * Copyright (c) 2024, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/SetOnce.h>
#include <Kernel/Arch/PlatformBusDriver.h>
#include <Kernel/FileSystem/SysFS/Subsystems/Bus/USB/BusDirectory.h>
#include <Kernel/Sections.h>

namespace Kernel {

class USBPlatformBusDriver final : public PlatformBusDriver {
public:
    static void init();

    USBPlatformBusDriver()
        : PlatformDriver("USBPlatformBusDriver"sv)
    {
    }
};

UNMAP_AFTER_INIT void USBPlatformBusDriver::init()
{
    SysFSUSBBusDirectory::initialize();
}

PLATFORM_BUS_DEVICE_DRIVER(USBPlatformBusDriver);

}
