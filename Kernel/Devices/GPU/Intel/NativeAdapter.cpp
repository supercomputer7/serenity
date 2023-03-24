/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Devices/GPU/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Devices/GPU/Definitions.h>
#include <Kernel/Devices/GPU/Intel/NativeAdapter.h>
#include <Kernel/Devices/GPU/Management.h>
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

ErrorOr<bool> IntelNativeGPUAdapter::probe(PCI::DeviceIdentifier const& pci_device_identifier)
{
    return is_supported_model(pci_device_identifier.hardware_id().device_id);
}

ErrorOr<NonnullLockRefPtr<GenericGPUAdapter>> IntelNativeGPUAdapter::create(PCI::DeviceIdentifier const& pci_device_identifier)
{
    auto adapter = TRY(adopt_nonnull_lock_ref_or_enomem(new (nothrow) IntelNativeGPUAdapter(pci_device_identifier)));
    TRY(adapter->initialize_adapter());
    return adapter;
}

ErrorOr<void> IntelNativeGPUAdapter::initialize_adapter()
{
    dbgln_if(INTEL_GPU_DEBUG, "Intel Native GPU Adapter @ {}", device_identifier().address());
    auto bar0_space_size = PCI::get_BAR_space_size(device_identifier(), PCI::HeaderType0BaseRegister::BAR0);
    auto bar2_space_size = PCI::get_BAR_space_size(device_identifier(), PCI::HeaderType0BaseRegister::BAR2);
    dmesgln_pci(*this, "MMIO @ {}, space size is {:x} bytes", PhysicalAddress(PCI::get_BAR0(device_identifier())), bar0_space_size);
    dmesgln_pci(*this, "framebuffer @ {}", PhysicalAddress(PCI::get_BAR2(device_identifier())));

    using MMIORegion = IntelDisplayController::MMIORegion;
    MMIORegion first_region { MMIORegion::BARAssigned::BAR0, PhysicalAddress(PCI::get_BAR0(device_identifier()) & 0xfffffff0), bar0_space_size };
    MMIORegion second_region { MMIORegion::BARAssigned::BAR2, PhysicalAddress(PCI::get_BAR2(device_identifier()) & 0xfffffff0), bar2_space_size };

    PCI::enable_bus_mastering(device_identifier());
    PCI::enable_io_space(device_identifier());
    PCI::enable_memory_space(device_identifier());

    switch (device_identifier().hardware_id().device_id) {
    case 0x29c2:
        m_connector_group = TRY(IntelDisplayController::try_create({}, IntelGPU::Generation::Gen4, first_region, second_region));
        return {};
    default:
        return Error::from_errno(ENODEV);
    }
}

IntelNativeGPUAdapter::IntelNativeGPUAdapter(PCI::DeviceIdentifier const& pci_device_identifier)
    : GenericGPUAdapter()
    , PCI::Device(const_cast<PCI::DeviceIdentifier&>(pci_device_identifier))
{
}

}
