/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/HashTable.h>
#include <AK/Singleton.h>
#include <Kernel/Devices/DeviceManagement.h>
#include <Kernel/FileSystem/InodeMetadata.h>
#include <Kernel/Sections.h>

namespace Kernel {

static Singleton<DeviceManagement> s_the;

UNMAP_AFTER_INIT DeviceManagement::DeviceManagement()
{
}
UNMAP_AFTER_INIT void DeviceManagement::initialize()
{
    s_the.ensure_instance();
}

UNMAP_AFTER_INIT void DeviceManagement::attach_null_device(Device const& device)
{
    VERIFY(device.major() == 1);
    VERIFY(device.minor() == 3);
    m_null_device = device;
}

DeviceManagement& DeviceManagement::the()
{
    return *s_the;
}

Device* DeviceManagement::get_device(unsigned major, unsigned minor)
{
    return m_devices.with_exclusive([&](auto& map) -> Device* {
        auto it = map.find(encoded_device(major, minor));
        if (it == map.end())
            return nullptr;
        return it->value;
    });
}

void DeviceManagement::before_removal(Badge<Device>, Device& device)
{
    u32 device_id = encoded_device(device.major(), device.minor());
    m_devices.with_exclusive([&](auto& map) -> void {
        VERIFY(map.contains(device_id));
        map.remove(encoded_device(device.major(), device.minor()));
    });
}

void DeviceManagement::after_inserting(Badge<Device>, Device& device)
{
    u32 device_id = encoded_device(device.major(), device.minor());
    m_devices.with_exclusive([&](auto& map) -> void {
        auto it = map.find(device_id);
        if (it != map.end()) {
            dbgln("Already registered {},{}: {}", device.major(), device.minor(), it->value->class_name());
        }
        VERIFY(!map.contains(device_id));
        auto result = map.set(device_id, &device);
        if (result != AK::HashSetResult::InsertedNewEntry) {
            dbgln("Failed to register {},{}: {}", device.major(), device.minor(), it->value->class_name());
            VERIFY_NOT_REACHED();
        }
    });
}

void DeviceManagement::for_each(Function<void(Device&)> callback)
{
    m_devices.with_exclusive([&](auto& map) -> void {
        for (auto& entry : map)
            callback(*entry.value);
    });
}

Device& DeviceManagement::null_device()
{
    return *m_null_device;
}

Device const& DeviceManagement::null_device() const
{
    return *m_null_device;
}

}
