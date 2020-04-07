#include <AK/HashMap.h>
#include <Kernel/Devices/Device.h>
#include <Kernel/Devices/DeviceRegistrar.h>
#include <Kernel/FileSystem/InodeMetadata.h>

namespace Kernel {

static DeviceRegistrar* s_devices_register;

DeviceRegistrar& DeviceRegistrar::the()
{
    if (s_devices_register != nullptr)
        return *s_devices_register;
    s_devices_register = new DeviceRegistrar;
    return *s_devices_register;
}

DeviceRegistrar::DeviceRegistrar()
{
}

void DeviceRegistrar::for_each(Function<void(Device&)> callback)
{
    for (auto& entry : m_devices) {
        ASSERT(!entry.value.is_null());
        callback(*entry.value);
    }
}

RefPtr<Device> DeviceRegistrar::get_device(unsigned major, unsigned minor)
{
    auto it = m_devices.find(encoded_device(major, minor));
    if (it == m_devices.end())
        return nullptr;
    ASSERT(!it->value.is_null());
    return it->value;
}

void DeviceRegistrar::register_device(Device& device)
{
    u32 device_id = encoded_device(device.major(), device.minor());
    auto it = m_devices.find(device_id);
    if (it != m_devices.end()) {
        ASSERT(!it->value.is_null());
        dbg() << "Already registered " << device.major() << "," << device.minor() << ": " << it->value->class_name();
    }
    ASSERT(!m_devices.contains(device_id));
    m_devices.set(device_id, adopt(device));
}
void DeviceRegistrar::unregister_device(Device&)
{
}

}
