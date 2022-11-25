/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/StringView.h>
#include <Kernel/Devices/DeviceManagement.h>
#include <Kernel/Sections.h>
#include <Kernel/Storage/MMC/MMCDevice.h>
#include <Kernel/Storage/MMC/SDHCIController.h>
#include <Kernel/Storage/StorageManagement.h>

namespace Kernel {

NonnullRefPtr<MMCDevice> MMCDevice::must_create(SDHCIController const& controller, size_t device_index, u16 logical_sector_size, u64 max_addressable_block)
{
    auto minor_device_number = StorageManagement::generate_storage_minor_number();

    auto device_name = MUST(KString::formatted("hd{:c}", 'a' + minor_device_number.value()));

    auto disk_device_or_error = DeviceManagement::try_create_device<MMCDevice>(controller, device_index, minor_device_number, logical_sector_size, max_addressable_block, move(device_name));
    // FIXME: Find a way to propagate errors
    VERIFY(!disk_device_or_error.is_error());
    return disk_device_or_error.release_value();
}

MMCDevice::MMCDevice(SDHCIController const& controller, size_t device_index, MinorNumber minor_number, u16 logical_sector_size, u64 max_addressable_block, NonnullOwnPtr<KString> early_storage_name)
    : StorageDevice(StorageManagement::storage_type_major_number(), minor_number, logical_sector_size, max_addressable_block, move(early_storage_name))
    , m_controller(controller)
    , m_device_index(device_index)
{
}

void MMCDevice::start_request(AsyncBlockDeviceRequest& request)
{
    auto controller = m_controller.strong_ref();
    VERIFY(controller);
    controller->start_request(*this, request);
}

StringView MMCDevice::class_name() const
{
    return "MMCDevice";
}

}
