/*
 * Copyright (c) 2023, Kirill Nikolaev <cyril7@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Bus/PCI/Device.h>
#include <Kernel/Bus/VirtIO/Device.h>
#include <Kernel/Devices/Storage/StorageController.h>

namespace Kernel {

class VirtIOBlockDevice;

class VirtIOBlockController final : public StorageController
    , VirtIO::Device {
public:
    static ErrorOr<NonnullRefPtr<VirtIOBlockController>> try_create(PCI::DeviceIdentifier const& device_identifier);

    static bool is_handled(PCI::DeviceIdentifier const& device_identifier);

    // ^StorageController
    virtual LockRefPtr<StorageDevice> device(u32 index) const override;
    virtual size_t devices_count() const override { return m_attached_device ? 1 : 0; }

    void start_request(AsyncBlockDeviceRequest&);

private:
    ErrorOr<void> maybe_start_request(AsyncBlockDeviceRequest&);
    void respond();

    ErrorOr<void> attach_block_device();

    VirtIOBlockController(NonnullOwnPtr<VirtIO::TransportEntity> transport);

    // ^VirtIO::Device
    virtual ErrorOr<void> initialize_virtio_resources() override;
    virtual void handle_queue_update(u16 queue_index) override;
    ErrorOr<void> handle_device_config_change() override;

    // ^StorageController
    virtual void complete_current_request(AsyncDeviceRequest::RequestResult) override;

    OwnPtr<Memory::Region> m_header_buf;
    OwnPtr<Memory::Region> m_data_buf;

    SpinlockProtected<RefPtr<AsyncBlockDeviceRequest>, LockRank::None> m_current_request {};
    RefPtr<VirtIOBlockDevice> m_attached_device;
};

}
