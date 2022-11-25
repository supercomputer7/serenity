/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Locking/Mutex.h>
#include <Kernel/Storage/StorageDevice.h>

namespace Kernel {

class SDHCIController;
class MMCDevice final : public StorageDevice {
    friend class DeviceManagement;

public:
    static NonnullRefPtr<MMCDevice> must_create(SDHCIController const&, size_t device_index, u16 logical_sector_size, u64 max_addressable_block);
    virtual ~MMCDevice() override = default;

    // ^BlockDevice
    virtual void start_request(AsyncBlockDeviceRequest&) override;

    // ^StorageDevice
    virtual CommandSet command_set() const override { return CommandSet::MMC; }

private:
    // ^DiskDevice
    virtual StringView class_name() const override;

    MMCDevice(const SDHCIController&, size_t device_index, MinorNumber, u16, u64, NonnullOwnPtr<KString>);

    WeakPtr<SDHCIController> m_controller;
    const size_t m_device_index { 0 };
};

}
