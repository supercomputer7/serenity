/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/x86/IO.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Graphics/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Graphics/Definitions.h>
#include <Kernel/Graphics/GraphicsManagement.h>
#include <Kernel/Graphics/Intel/NativeGraphicsAdapter.h>
#include <Kernel/PhysicalAddress.h>

namespace Kernel {

static constexpr u16 supported_models[] {
    0x29c2, // Intel G35 Adapter
};

static bool is_supported_model(u16 device_id)
{
    for (auto& id : supported_models) {
        if (id == device_id)
            return true;
    }
    return false;
}

RefPtr<IntelNativeGraphicsAdapter> IntelNativeGraphicsAdapter::initialize(PCI::DeviceIdentifier const& pci_device_identifier)
{
    VERIFY(pci_device_identifier.hardware_id().vendor_id == 0x8086);
    if (!is_supported_model(pci_device_identifier.hardware_id().device_id))
        return {};
    auto adapter = adopt_ref_if_nonnull(new (nothrow) IntelNativeGraphicsAdapter(pci_device_identifier.address(), pci_device_identifier.hardware_id()));
    MUST(adapter->initialize_adapter());
    return adapter;
}

ErrorOr<void> IntelNativeGraphicsAdapter::initialize_adapter()
{
    auto address = pci_address();
    dbgln_if(INTEL_GRAPHICS_DEBUG, "Intel Native Graphics Adapter @ {}", address);
    auto bar0_space_size = PCI::get_BAR_space_size(address, 0);
    auto bar2_space_size = PCI::get_BAR_space_size(address, 2);
    dmesgln("Intel Native Graphics Adapter @ {}, MMIO @ {}, space size is {:d} bytes", address, PhysicalAddress(PCI::get_BAR0(address)), bar0_space_size);
    dmesgln("Intel Native Graphics Adapter @ {}, framebuffer @ {}", address, PhysicalAddress(PCI::get_BAR2(address)));

    using MMIORegion = IntelDisplayConnectorGroup::MMIORegion;
    MMIORegion first_region { MMIORegion::BARAssigned::BAR0, PhysicalAddress(PCI::get_BAR0(address) & 0xfffffff0), bar0_space_size };
    MMIORegion second_region { MMIORegion::BARAssigned::BAR2, PhysicalAddress(PCI::get_BAR2(address) & 0xfffffff0), bar2_space_size };

    PCI::enable_bus_mastering(address);
    PCI::enable_io_space(address);
    PCI::enable_memory_space(address);

    using Generation = IntelDisplayConnectorGroup::Generation;
    switch (m_hardware_id.device_id) {
    case 0x29c2:
        m_connector_group = IntelDisplayConnectorGroup::must_create({}, Generation::Gen4, first_region, second_region);
        return {};
    }

    VERIFY_NOT_REACHED();
}

IntelNativeGraphicsAdapter::IntelNativeGraphicsAdapter(PCI::Address address, PCI::HardwareID hardware_id)
    : GenericGraphicsAdapter()
    , PCI::Device(address)
    , m_hardware_id(hardware_id)
{
}

}
