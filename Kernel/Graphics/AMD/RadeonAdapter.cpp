/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Atomic.h>
#include <AK/Checked.h>
#include <AK/Try.h>
#include <Kernel/Arch/x86/IO.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Bus/PCI/IDs.h>
#include <Kernel/Debug.h>
#include <Kernel/Graphics/AMD/DisplayConnector.h>
#include <Kernel/Graphics/AMD/RadeonAdapter.h>
#include <Kernel/Graphics/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Graphics/GraphicsManagement.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/Sections.h>

namespace Kernel {

UNMAP_AFTER_INIT NonnullRefPtr<RadeonGraphicsAdapter> RadeonGraphicsAdapter::initialize(PCI::DeviceIdentifier const& pci_device_identifier)
{
    PCI::HardwareID id = pci_device_identifier.hardware_id();
    VERIFY((id.vendor_id == PCI::VendorID::ATI));
    auto adapter = adopt_ref_if_nonnull(new (nothrow) RadeonGraphicsAdapter(pci_device_identifier)).release_nonnull();
    MUST(adapter->initialize_adapter(pci_device_identifier));
    return adapter;
}

UNMAP_AFTER_INIT RadeonGraphicsAdapter::RadeonGraphicsAdapter(PCI::DeviceIdentifier const& pci_device_identifier)
    : PCI::Device(pci_device_identifier.address())
{
    if (pci_device_identifier.class_code().value() == 0x3 && pci_device_identifier.subclass_code().value() == 0x0)
        m_is_vga_capable = true;
}

ErrorOr<Memory::MappedROM> RadeonGraphicsAdapter::map_atombios()
{
    auto pci_option_rom_size = PCI::get_expansion_rom_space_size(pci_address());
    auto pci_option_rom_physical_pointer = PCI::read32(pci_address(), PCI::RegisterOffset::EXPANSION_ROM_POINTER);
    if (pci_option_rom_physical_pointer == 0x0 || pci_option_rom_size == 0x0)
        return Error::from_errno(EIO);
    // Note: Write the original value ORed with 1 to enable mapping into physical memory
    PCI::write32(pci_address(), PCI::RegisterOffset::EXPANSION_ROM_POINTER, pci_option_rom_physical_pointer | 1);

    Memory::MappedROM mapping;
    mapping.size = pci_option_rom_size;
    mapping.paddr = PhysicalAddress(pci_option_rom_physical_pointer);
    auto region_size = TRY(Memory::page_round_up(mapping.size));
    mapping.region = TRY(MM.allocate_kernel_region(mapping.paddr, region_size, {}, Memory::Region::Access::Read));
    return mapping;
}

UNMAP_AFTER_INIT ErrorOr<void> RadeonGraphicsAdapter::initialize_adapter(PCI::DeviceIdentifier const&)
{
    auto atombios_rom = TRY(map_atombios());
    m_atombios_parser = ATOMBIOSParser::must_initialize(move(atombios_rom));
    // auto registers_mapping = TRY(Memory::map_typed_writable<BochsDisplayMMIORegisters volatile>(PhysicalAddress(PCI::get_BAR2(pci_device_identifier.address()) & 0xfffffff0)));
    // VERIFY(registers_mapping.region);
    // m_display_connector = BochsDisplayConnector::must_create(PhysicalAddress(PCI::get_BAR0(pci_device_identifier.address()) & 0xfffffff0), registers_mapping.region.release_nonnull(), registers_mapping.offset);
    // TRY(m_display_connector->set_safe_resolution());

    return {};
}

bool RadeonGraphicsAdapter::vga_compatible() const
{
    return true;
}

}
