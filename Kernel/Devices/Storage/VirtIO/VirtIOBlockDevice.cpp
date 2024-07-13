/*
 * Copyright (c) 2023, Kirill Nikolaev <cyril7@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Devices/Storage/VirtIO/VirtIOBlockController.h>
#include <Kernel/Devices/Storage/VirtIO/VirtIOBlockDevice.h>
#include <Kernel/Memory/MemoryManager.h>

namespace Kernel {

static constexpr u64 SECTOR_SIZE = 512;
static constexpr u64 MAX_ADDRESSABLE_BLOCK = 1ull << 32;    // FIXME: Supply effective device size.

VirtIOBlockDevice::VirtIOBlockDevice(
    VirtIOBlockController& controller,
    StorageDevice::LUNAddress lun,
    u32 hardware_relative_controller_id)
    : StorageDevice(lun, hardware_relative_controller_id, SECTOR_SIZE, MAX_ADDRESSABLE_BLOCK)
    , m_controller(controller)
{
}

void VirtIOBlockDevice::start_request(AsyncBlockDeviceRequest& request)
{
    m_controller->start_request(request);
}

}
