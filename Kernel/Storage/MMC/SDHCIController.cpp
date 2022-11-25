/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/OwnPtr.h>
#include <AK/RefPtr.h>
#include <AK/Types.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Sections.h>
#include <Kernel/Storage/MMC/SDHCIController.h>

namespace Kernel {

UNMAP_AFTER_INIT NonnullRefPtr<SDHCIController> SDHCIController::must_initialize(PCI::DeviceIdentifier const& device_identifier)
{
    auto host_controller_registers_mapping = MUST(Memory::map_typed_writable<SDHCHostRegisters volatile>(PhysicalAddress(PCI::get_BAR0(device_identifier.address()) & 0xfffffff0)));
    auto controller = adopt_ref_if_nonnull(new (nothrow) SDHCIController(device_identifier, move(host_controller_registers_mapping))).release_nonnull();
    controller->initialize_hba();
    return controller;
}

bool SDHCIController::reset()
{
    TODO();
}

bool SDHCIController::shutdown()
{
    TODO();
}

size_t SDHCIController::devices_count() const
{
    size_t count = 0;
    for (u32 index = 0; index < 4; index++) {
        if (!device(index).is_null())
            count++;
    }
    return count;
}

void SDHCIController::start_request(MMCDevice const&, AsyncBlockDeviceRequest&)
{
    VERIFY_NOT_REACHED();
}

void SDHCIController::complete_current_request(AsyncDeviceRequest::RequestResult)
{
    VERIFY_NOT_REACHED();
}

UNMAP_AFTER_INIT SDHCIController::SDHCIController(PCI::DeviceIdentifier const& device_identifier, Memory::TypedMapping<SDHCHostRegisters volatile> host_controller_registers_mapping)
    : StorageController()
    , PCI::Device(device_identifier.address())
    , m_host_controller_registers_mapping(move(host_controller_registers_mapping))
    , m_interrupt_line(device_identifier.interrupt_line())
{
    PCI::enable_io_space(device_identifier.address());
    PCI::enable_memory_space(device_identifier.address());
}

UNMAP_AFTER_INIT SDHCIController::~SDHCIController()
{
}

UNMAP_AFTER_INIT void SDHCIController::initialize_hba()
{
    dbgln("SDHCI controller @ {}: host registers @ {}", pci_address(), PhysicalAddress(PCI::get_BAR0(pci_address()) & 0xfffffff0));
    dbgln("SDHCI controller @ {}: interrupt line was set to {}", pci_address(), m_interrupt_line.value());
}

RefPtr<StorageDevice> SDHCIController::device(u32) const
{
    TODO();
}
}
