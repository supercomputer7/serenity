/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Debug.h>
#include <Kernel/Devices/DeviceManagement.h>
#include <Kernel/Graphics/Bochs/Definitions.h>
#include <Kernel/Graphics/Bochs/DisplayConnector.h>
#include <Kernel/Graphics/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Graphics/GraphicsManagement.h>

namespace Kernel {

/*

UNMAP_AFTER_INIT void BochsGraphicsAdapter::initialize_framebuffer_devices()
{
    // FIXME: Find a better way to determine default resolution...
    m_framebuffer_device = FramebufferDevice::create(*this, PhysicalAddress(PCI::get_BAR0(pci_address()) & 0xfffffff0), 1024, 768, 1024 * sizeof(u32));
    // While write-combine helps greatly on actual hardware, it greatly reduces performance in QEMU
    m_framebuffer_device->enable_write_combine(false);
    // FIXME: Would be nice to be able to return a ErrorOr<void> here.
    VERIFY(!m_framebuffer_device->try_to_initialize().is_error());
}

*/

NonnullRefPtr<BochsDisplayConnector> BochsDisplayConnector::must_create(PhysicalAddress framebuffer_address, NonnullOwnPtr<Memory::Region> registers_region, size_t registers_region_offset)
{
    auto device_or_error = DeviceManagement::try_create_device<BochsDisplayConnector>(move(registers_region), registers_region_offset);
    VERIFY(!device_or_error.is_error());
    auto connector = device_or_error.release_value();
    MUST(connector->create_attached_framebuffer_console(framebuffer_address));
    return connector;
}

ErrorOr<void> BochsDisplayConnector::create_attached_framebuffer_console(PhysicalAddress framebuffer_address)
{
    // We assume safe resolution is 1024x768x32
    m_framebuffer_console = Graphics::ContiguousFramebufferConsole::initialize(framebuffer_address, 1024, 768, 1024 * sizeof(u32));
    GraphicsManagement::the().set_console(*m_framebuffer_console);
    return {};
}

BochsDisplayConnector::BochsDisplayConnector()
    : DisplayConnector()
{
}

BochsDisplayConnector::BochsDisplayConnector(NonnullOwnPtr<Memory::Region> registers_region, size_t registers_region_offset)
    : DisplayConnector()
{
    m_registers.region = move(registers_region);
    m_registers.offset = registers_region_offset;
}

BochsDisplayConnector::IndexID BochsDisplayConnector::index_id() const
{
    return m_registers->bochs_regs.index_id;
}

void BochsDisplayConnector::set_framebuffer_to_big_endian_format()
{
    MutexLocker locker(m_modeset_lock);
    dbgln_if(BXVGA_DEBUG, "BochsDisplayConnector set_framebuffer_to_big_endian_format");
    full_memory_barrier();
    if (m_registers->extension_regs.region_size == 0xFFFFFFFF || m_registers->extension_regs.region_size == 0)
        return;
    full_memory_barrier();
    m_registers->extension_regs.framebuffer_byteorder = BOCHS_DISPLAY_BIG_ENDIAN;
    full_memory_barrier();
}

void BochsDisplayConnector::set_framebuffer_to_little_endian_format()
{
    MutexLocker locker(m_modeset_lock);
    dbgln_if(BXVGA_DEBUG, "BochsDisplayConnector set_framebuffer_to_little_endian_format");
    full_memory_barrier();
    if (m_registers->extension_regs.region_size == 0xFFFFFFFF || m_registers->extension_regs.region_size == 0)
        return;
    full_memory_barrier();
    m_registers->extension_regs.framebuffer_byteorder = BOCHS_DISPLAY_LITTLE_ENDIAN;
    full_memory_barrier();
}

ErrorOr<void> BochsDisplayConnector::set_safe_resolution()
{
    DisplayConnector::Resolution safe_resolution { 1024, 768, 32, 1024 * sizeof(u32), {} };
    return set_resolution(safe_resolution);
}

ErrorOr<void> BochsDisplayConnector::unblank()
{
    MutexLocker locker(m_modeset_lock);
    full_memory_barrier();
    m_registers->vga_ioports[0] = 0x20;
    full_memory_barrier();
    return {};
}

ErrorOr<size_t> BochsDisplayConnector::write_to_first_surface(u64, UserOrKernelBuffer const&, size_t)
{
    TODO();
}

ErrorOr<void> BochsDisplayConnector::flush_first_surface()
{
    TODO();
}

ErrorOr<void> BochsDisplayConnector::set_y_offset(size_t y_offset)
{
    MutexLocker locker(m_modeset_lock);
    m_registers->bochs_regs.y_offset = y_offset;
    return {};
}

ErrorOr<DisplayConnector::Resolution> BochsDisplayConnector::get_resolution()
{
    MutexLocker locker(m_modeset_lock);
    return Resolution { m_registers->bochs_regs.xres, m_registers->bochs_regs.yres, 32, m_registers->bochs_regs.xres * sizeof(u32), {} };
}

ErrorOr<void> BochsDisplayConnector::set_resolution(Resolution const& resolution)
{
    MutexLocker locker(m_modeset_lock);
    VERIFY(m_framebuffer_console);
    size_t width = resolution.width;
    size_t height = resolution.height;
    size_t bpp = resolution.bpp;
    if (bpp != 32) {
        dbgln_if(BXVGA_DEBUG, "BochsDisplayConnector - no support for non-32bpp resolutions");
        return Error::from_errno(ENOTSUP);
    }

    if (Checked<size_t>::multiplication_would_overflow(width, height, sizeof(u32)))
        return EOVERFLOW;

    dbgln_if(BXVGA_DEBUG, "BochsDisplayConnector resolution registers set to - {}x{}", width, height);
    m_registers->bochs_regs.enable = 0;
    full_memory_barrier();
    m_registers->bochs_regs.xres = width;
    m_registers->bochs_regs.yres = height;
    m_registers->bochs_regs.virt_width = width;
    m_registers->bochs_regs.virt_height = height * 2;
    m_registers->bochs_regs.bpp = 32;
    full_memory_barrier();
    m_registers->bochs_regs.enable = to_underlying(BochsFramebufferSettings::Enabled) | to_underlying(BochsFramebufferSettings::LinearFramebuffer);
    full_memory_barrier();
    m_registers->bochs_regs.bank = 0;
    if (index_id().value() == VBE_DISPI_ID5) {
        set_framebuffer_to_little_endian_format();
    }

    if ((u16)width != m_registers->bochs_regs.xres || (u16)height != m_registers->bochs_regs.yres) {
        return Error::from_errno(ENOTIMPL);
    }
    m_framebuffer_console->set_resolution(width, height, width * sizeof(u32));
    return {};
}

ErrorOr<ByteBuffer> BochsDisplayConnector::get_edid() const
{
    return ByteBuffer::copy(const_cast<u8 const*>(m_registers->edid_data), sizeof(m_registers->edid_data));
}

}
