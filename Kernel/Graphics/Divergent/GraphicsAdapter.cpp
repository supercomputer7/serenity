/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
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
#include <Kernel/Graphics/Divergent/Definitions.h>
#include <Kernel/Graphics/Divergent/DisplayConnector.h>
#include <Kernel/Graphics/Divergent/GraphicsAdapter.h>
#include <Kernel/Graphics/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Graphics/GraphicsManagement.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/Sections.h>

namespace Kernel {

UNMAP_AFTER_INIT NonnullRefPtr<DivergentGraphicsAdapter> DivergentGraphicsAdapter::initialize(PCI::DeviceIdentifier const& pci_device_identifier)
{
    PCI::HardwareID id = pci_device_identifier.hardware_id();
    VERIFY((id.vendor_id == PCI::VendorID::RedHat && id.device_id == 0x0013));
    auto registers_mapping = MUST(Memory::map_typed_writable<DivergentDisplayProgrammingInterface volatile>(PhysicalAddress(PCI::get_BAR2(pci_device_identifier.address()) & 0xfffffff0)));
    auto adapter = adopt_ref(*new (nothrow) DivergentGraphicsAdapter(pci_device_identifier, move(registers_mapping)));
    MUST(adapter->initialize_adapter(pci_device_identifier));
    return adapter;
}

UNMAP_AFTER_INIT DivergentGraphicsAdapter::DivergentGraphicsAdapter(PCI::DeviceIdentifier const& pci_device_identifier, Memory::TypedMapping<DivergentDisplayProgrammingInterface volatile> registers_mapping)
    : PCI::Device(pci_device_identifier.address())
    , m_registers_mapping(move(registers_mapping))
{
}

ErrorOr<void> DivergentGraphicsAdapter::apply_mode_setting_on_connector(Badge<DivergentDisplayConnector>, DivergentDisplayConnector const& connector, DisplayConnector::ModeSetting const& mode_setting)
{
    VERIFY(connector.m_modeset_lock.is_locked());
    auto connector_fuse_index = static_cast<size_t>(to_underlying(connector.connector_fuse_index()));
    VERIFY(connector_fuse_index< m_display_connectors.size());
    if (mode_setting.horizontal_active > divergent_display_max_width)
        return Error::from_errno(ENOTSUP);
    if (mode_setting.vertical_active > divergent_display_max_height)
        return Error::from_errno(ENOTSUP);
    if (mode_setting.horizontal_stride > (mode_setting.horizontal_active * sizeof(u32) * 4))
        return Error::from_errno(ENOTSUP);
    m_registers_mapping->display_transcoders[connector_fuse_index].gates = 0;
    u32 dimensions = mode_setting.horizontal_active | mode_setting.vertical_active << 16;
    m_registers_mapping->display_transcoders[connector_fuse_index].bpp = 32;
    m_registers_mapping->display_transcoders[connector_fuse_index].dimensions = dimensions;
    m_registers_mapping->display_transcoders[connector_fuse_index].stride = mode_setting.horizontal_stride;
    m_registers_mapping->display_transcoders[connector_fuse_index].gates = 1;
    auto framebuffer_offset = TRY(Memory::page_round_up(connector_fuse_index * divergent_display_max_width * sizeof(u32) * divergent_display_max_height));
    m_registers_mapping->display_transcoders[connector_fuse_index].surface_offset = framebuffer_offset;
    return {};
}

UNMAP_AFTER_INIT ErrorOr<void> DivergentGraphicsAdapter::read_edid_and_set_for_connector(size_t connector_index)
{
    VERIFY(m_display_connectors[connector_index]);
    SpinlockLocker locker(m_ddc_lock);
    Array<u8, 128> edid;
    for (size_t byte_index = 0; byte_index < edid.size(); byte_index++) {
        m_registers_mapping->controller_registers.ddc_byte_index = byte_index;
        u32 ddc_control = 1 | (connector_index << 8) | (1 << 31);
        m_registers_mapping->controller_registers.ddc_control = ddc_control;

        auto wait_for_ddc = [this]() -> ErrorOr<void> {
            size_t retries = 0;
            while (retries < 20) {
                auto status = m_registers_mapping->controller_registers.ddc_status;
                if (!(status & 1))
                    return {};
                IO::delay(1000);
                retries++;
            }
            return Error::from_errno(EBUSY);
        };
        TRY(wait_for_ddc());
        u8 edid_data = m_registers_mapping->controller_registers.ddc_data & 0xff;
        edid[byte_index] = edid_data;
    }
    m_display_connectors[connector_index]->set_edid_bytes(edid);
    return {};
}

UNMAP_AFTER_INIT ErrorOr<void> DivergentGraphicsAdapter::initialize_adapter(PCI::DeviceIdentifier const& pci_device_identifier)
{
    auto bar0_space_size = PCI::get_BAR_space_size(pci_device_identifier.address(), 0);
    for (size_t index =0 ; index < 8; index++) {
        if (!(m_registers_mapping->controller_registers.fuses & (1 << index)))
            continue;
        dbgln("Divergent Display Controller @ {}: Found display at fuse {}", pci_device_identifier.address(), index);
        auto connector_fuse_index = static_cast<DivergentDisplayConnector::ConnectorFuseIndex>(index);
        auto framebuffer_offset = TRY(Memory::page_round_up(index * divergent_display_max_width * sizeof(u32) * divergent_display_max_height));
        m_display_connectors[index] = DivergentDisplayConnector::must_create(*this, connector_fuse_index, PhysicalAddress(PCI::get_BAR0(pci_device_identifier.address()) & 0xfffffff0).offset(framebuffer_offset), bar0_space_size);
        TRY(read_edid_and_set_for_connector(index));
        TRY(m_display_connectors[index]->set_safe_mode_setting());
    }
    return {};
}

}
