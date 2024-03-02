/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Atomic.h>
#include <AK/Checked.h>
#include <AK/Try.h>
#include <Kernel/Bus/PCI/IDs.h>
#include <Kernel/Devices/GPU/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Devices/GPU/Management.h>
#include <Kernel/Devices/GPU/VMWare/Definitions.h>
#include <Kernel/Devices/GPU/VMWare/DisplayConnector.h>
#include <Kernel/Devices/GPU/VMWare/GraphicsAdapter.h>
#include <Kernel/Library/IOWindow.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/Sections.h>

namespace Kernel {

ErrorOr<NonnullRefPtr<VMWareGraphicsAdapter>> VMWareGraphicsAdapter::create(PCI::Device& pci_device)
{
    auto registers_io_window = TRY(IOWindow::create_for_pci_device_bar(pci_device, PCI::HeaderType0BaseRegister::BAR0));
    auto adapter = TRY(adopt_nonnull_ref_or_enomem(new (nothrow) VMWareGraphicsAdapter(pci_device, move(registers_io_window))));
    TRY(adapter->initialize_adapter());
    return adapter;
}

VMWareGraphicsAdapter::VMWareGraphicsAdapter(PCI::Device& pci_device, NonnullOwnPtr<IOWindow> registers_io_window)
    : m_pci_device(pci_device)
    , m_registers_io_window(move(registers_io_window))
{
    dbgln("VMWare SVGA @ {}, {}", m_pci_device->device_id().address(), m_registers_io_window);
}

u32 VMWareGraphicsAdapter::read_io_register(VMWareDisplayRegistersOffset register_offset) const
{
    SpinlockLocker locker(m_io_access_lock);
    m_registers_io_window->write32(0, to_underlying(register_offset));
    return m_registers_io_window->read32_unaligned(1);
}
void VMWareGraphicsAdapter::write_io_register(VMWareDisplayRegistersOffset register_offset, u32 value)
{
    SpinlockLocker locker(m_io_access_lock);
    m_registers_io_window->write32(0, to_underlying(register_offset));
    m_registers_io_window->write32_unaligned(1, value);
}

ErrorOr<void> VMWareGraphicsAdapter::negotiate_device_version()
{
    write_io_register(VMWareDisplayRegistersOffset::ID, vmware_svga_version_2_id);
    auto accepted_version = read_io_register(VMWareDisplayRegistersOffset::ID);
    dbgln("VMWare SVGA @ {}: Accepted version {}", m_pci_device->device_id().address(), accepted_version);
    if (read_io_register(VMWareDisplayRegistersOffset::ID) == vmware_svga_version_2_id)
        return {};
    return Error::from_errno(ENOTSUP);
}

ErrorOr<void> VMWareGraphicsAdapter::initialize_fifo_registers()
{
    auto framebuffer_size = read_io_register(VMWareDisplayRegistersOffset::FB_SIZE);
    auto fifo_size = read_io_register(VMWareDisplayRegistersOffset::MEM_SIZE);
    auto fifo_physical_address = PhysicalAddress(m_pci_device->resources()[2].physical_memory_address());

    dbgln("VMWare SVGA @ {}: framebuffer size {} bytes, FIFO size {} bytes @ {}", m_pci_device->device_id().address(), framebuffer_size, fifo_size, fifo_physical_address);
    if (framebuffer_size < 0x100000 || fifo_size < 0x10000) {
        dbgln("VMWare SVGA @ {}: invalid framebuffer or fifo size", m_pci_device->device_id().address());
        return Error::from_errno(ENOTSUP);
    }

    m_fifo_registers = TRY(m_pci_device->map_resource<VMWareDisplayFIFORegisters volatile>(PCI::HeaderType0BaseRegister::BAR2, fifo_size));
    m_fifo_registers->start = 16;
    m_fifo_registers->size = 16 + (10 * 1024);
    m_fifo_registers->next_command = 16;
    m_fifo_registers->stop = 16;
    return {};
}

void VMWareGraphicsAdapter::print_svga_capabilities() const
{
    auto svga_capabilities = read_io_register(VMWareDisplayRegistersOffset::CAPABILITIES);
    dbgln("VMWare SVGA capabilities (raw {:x}):", svga_capabilities);
    if (svga_capabilities & (1 << 1))
        dbgln("\tRect copy");
    if (svga_capabilities & (1 << 5))
        dbgln("\tCursor");
    if (svga_capabilities & (1 << 6))
        dbgln("\tCursor Bypass");
    if (svga_capabilities & (1 << 7))
        dbgln("\tCursor Bypass 2");
    if (svga_capabilities & (1 << 8))
        dbgln("\t8 Bit emulation");
    if (svga_capabilities & (1 << 9))
        dbgln("\tAlpha Cursor");
    if (svga_capabilities & (1 << 14))
        dbgln("\t3D acceleration");
    if (svga_capabilities & (1 << 15))
        dbgln("\tExtended FIFO");
    if (svga_capabilities & (1 << 16))
        dbgln("\tMulti-monitor (legacy)");
    if (svga_capabilities & (1 << 17))
        dbgln("\tPitch lock");
    if (svga_capabilities & (1 << 18))
        dbgln("\tIRQ masking");
    if (svga_capabilities & (1 << 19))
        dbgln("\tDisplay topology");
    if (svga_capabilities & (1 << 20))
        dbgln("\tGMR");
    if (svga_capabilities & (1 << 21))
        dbgln("\tTraces");
    if (svga_capabilities & (1 << 22))
        dbgln("\tGMR2");
    if (svga_capabilities & (1 << 23))
        dbgln("\tScreen object 2");
}

ErrorOr<void> VMWareGraphicsAdapter::modeset_primary_screen_resolution(Badge<VMWareDisplayConnector>, size_t width, size_t height)
{
    auto max_width = read_io_register(VMWareDisplayRegistersOffset::MAX_WIDTH);
    auto max_height = read_io_register(VMWareDisplayRegistersOffset::MAX_HEIGHT);
    if (width > max_width || height > max_height)
        return Error::from_errno(ENOTSUP);
    modeset_primary_screen_resolution(width, height);
    return {};
}

size_t VMWareGraphicsAdapter::primary_screen_width(Badge<VMWareDisplayConnector>) const
{
    SpinlockLocker locker(m_operation_lock);
    return read_io_register(VMWareDisplayRegistersOffset::WIDTH);
}
size_t VMWareGraphicsAdapter::primary_screen_height(Badge<VMWareDisplayConnector>) const
{
    SpinlockLocker locker(m_operation_lock);
    return read_io_register(VMWareDisplayRegistersOffset::HEIGHT);
}
size_t VMWareGraphicsAdapter::primary_screen_pitch(Badge<VMWareDisplayConnector>) const
{
    SpinlockLocker locker(m_operation_lock);
    return read_io_register(VMWareDisplayRegistersOffset::BYTES_PER_LINE);
}

void VMWareGraphicsAdapter::primary_screen_flush(Badge<VMWareDisplayConnector>, size_t current_width, size_t current_height)
{
    SpinlockLocker locker(m_operation_lock);
    m_fifo_registers->start = 16;
    m_fifo_registers->size = 16 + (10 * 1024);
    m_fifo_registers->next_command = 16 + 4 * 5;
    m_fifo_registers->stop = 16;
    m_fifo_registers->commands[0] = 1;
    m_fifo_registers->commands[1] = 0;
    m_fifo_registers->commands[2] = 0;
    m_fifo_registers->commands[3] = current_width;
    m_fifo_registers->commands[4] = current_height;
    write_io_register(VMWareDisplayRegistersOffset::SYNC, 1);
}

void VMWareGraphicsAdapter::modeset_primary_screen_resolution(size_t width, size_t height)
{
    SpinlockLocker locker(m_operation_lock);
    write_io_register(VMWareDisplayRegistersOffset::ENABLE, 0);
    write_io_register(VMWareDisplayRegistersOffset::WIDTH, width);
    write_io_register(VMWareDisplayRegistersOffset::HEIGHT, height);
    write_io_register(VMWareDisplayRegistersOffset::BITS_PER_PIXEL, 32);
    write_io_register(VMWareDisplayRegistersOffset::ENABLE, 1);
    write_io_register(VMWareDisplayRegistersOffset::CONFIG_DONE, 1);
}

ErrorOr<void> VMWareGraphicsAdapter::initialize_adapter()
{
    TRY(negotiate_device_version());
    print_svga_capabilities();
    TRY(initialize_fifo_registers());
    // Note: enable the device by modesetting the primary screen resolution
    modeset_primary_screen_resolution(640, 480);

    auto& resource1 = m_pci_device->resources()[1];
    m_display_connector = VMWareDisplayConnector::must_create(*this, PhysicalAddress(resource1.physical_memory_address()), resource1.length);
    TRY(m_display_connector->set_safe_mode_setting());
    return {};
}

}
