#pragma once

#include <AK/HashMap.h>
#include <AK/Types.h>

namespace Kernel {

class Device;
class DeviceRegistrar {
public:
    static DeviceRegistrar& the();
    RefPtr<Device> get_device(unsigned major, unsigned minor);
    void register_device(Device&);
    void for_each(Function<void(Device&)>);
    void unregister_device(Device&);

private:
    DeviceRegistrar();
    HashMap<unsigned, RefPtr<Device>> m_devices;
};
}
