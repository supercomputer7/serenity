/*
 * Copyright (c) 2023, Kirill Nikolaev <cyril7@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Function.h>
#include <AK/Result.h>
#include <AK/Types.h>
#include <Kernel/Devices/Storage/StorageDevice.h>
#include <Kernel/Locking/Mutex.h>

namespace Kernel {

class VirtIOBlockController;
class VirtIOBlockDevice : public StorageDevice {
public:
    // ^StorageDevice
    virtual CommandSet command_set() const override { return CommandSet::SCSI; }

    // ^BlockDevice
    virtual void start_request(AsyncBlockDeviceRequest&) override;

    VirtIOBlockDevice(VirtIOBlockController&, StorageDevice::LUNAddress lun,
        u32 hardware_relative_controller_id);

private:
    NonnullRefPtr<VirtIOBlockController> const m_controller;
};

}
