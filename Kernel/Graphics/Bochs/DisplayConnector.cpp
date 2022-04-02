/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/x86/IO.h>
#include <Kernel/Debug.h>
#include <Kernel/Devices/DeviceManagement.h>
#include <Kernel/Graphics/Bochs/Definitions.h>
#include <Kernel/Graphics/Bochs/DisplayConnector.h>
#include <Kernel/Graphics/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Graphics/GraphicsManagement.h>

namespace Kernel {

NonnullRefPtr<BochsDisplayConnector> BochsDisplayConnector::must_create(PhysicalAddress framebuffer_address)
{
    auto device_or_error = DeviceManagement::try_create_device<BochsDisplayConnector>(framebuffer_address);
    VERIFY(!device_or_error.is_error());
    auto connector = device_or_error.release_value();
    MUST(connector->create_attached_framebuffer_console());
    return connector;
}

ErrorOr<void> BochsDisplayConnector::create_attached_framebuffer_console()
{
    auto rounded_size = TRY(Memory::page_round_up(1024 * sizeof(u32) * 768 * 2));
    m_framebuffer_region = TRY(MM.allocate_kernel_region(m_framebuffer_address.page_base(), rounded_size, "Framebuffer"sv, Memory::Region::Access::ReadWrite));
    [[maybe_unused]] auto result = m_framebuffer_region->set_write_combine(true);
    m_framebuffer_data = m_framebuffer_region->vaddr().offset(m_framebuffer_address.offset_in_page()).as_ptr();
    // We assume safe resolution is 1024x768x32
    m_framebuffer_console = Graphics::ContiguousFramebufferConsole::initialize(m_framebuffer_address, 1024, 768, 1024 * sizeof(u32));
    GraphicsManagement::the().set_console(*m_framebuffer_console);
    return {};
}

BochsDisplayConnector::BochsDisplayConnector(PhysicalAddress framebuffer_address)
    : m_framebuffer_address(framebuffer_address)
{
}

static void set_register_with_io(u16 index, u16 data)
{
    IO::out16(VBE_DISPI_IOPORT_INDEX, index);
    IO::out16(VBE_DISPI_IOPORT_DATA, data);
}

static u16 get_register_with_io(u16 index)
{
    IO::out16(VBE_DISPI_IOPORT_INDEX, index);
    return IO::in16(VBE_DISPI_IOPORT_DATA);
}

ErrorOr<ByteBuffer> BochsDisplayConnector::get_edid() const
{
    return Error::from_errno(ENOTIMPL);
}

BochsDisplayConnector::IndexID BochsDisplayConnector::index_id() const
{
    return get_register_with_io(0);
}

ErrorOr<void> BochsDisplayConnector::set_safe_resolution()
{
    DisplayConnector::Resolution safe_resolution { 1024, 768, 32, 1024 * sizeof(u32), {} };
    return set_resolution(safe_resolution);
}

ErrorOr<size_t> BochsDisplayConnector::write_to_first_surface(u64 offset, UserOrKernelBuffer const& buffer, size_t length)
{
    VERIFY(m_control_lock.is_locked());
    if (offset + length > m_framebuffer_region->size())
        return Error::from_errno(EOVERFLOW);
    TRY(buffer.read(m_framebuffer_data + offset, 0, length));
    return length;
}


ErrorOr<void> BochsDisplayConnector::flush_first_surface()
{
    return Error::from_errno(ENOTSUP);
}

ErrorOr<void> BochsDisplayConnector::set_resolution(Resolution const& resolution)
{
    MutexLocker locker(m_modeset_lock);
    size_t width = resolution.width;
    size_t height = resolution.height;
    size_t bpp = resolution.bpp;
    if (bpp != 32) {
        dbgln_if(BXVGA_DEBUG, "BochsDisplayConnector - no support for non-32bpp resolutions");
        return Error::from_errno(ENOTSUP);
    }

    dbgln_if(BXVGA_DEBUG, "BochsDisplayConnector resolution registers set to - {}x{}", width, height);

    set_register_with_io(to_underlying(BochsDISPIRegisters::ENABLE), 0);
    set_register_with_io(to_underlying(BochsDISPIRegisters::XRES), (u16)width);
    set_register_with_io(to_underlying(BochsDISPIRegisters::YRES), (u16)height);
    set_register_with_io(to_underlying(BochsDISPIRegisters::VIRT_WIDTH), (u16)width);
    set_register_with_io(to_underlying(BochsDISPIRegisters::VIRT_HEIGHT), (u16)height * 2);
    set_register_with_io(to_underlying(BochsDISPIRegisters::BPP), 32);
    set_register_with_io(to_underlying(BochsDISPIRegisters::ENABLE), to_underlying(BochsFramebufferSettings::Enabled) | to_underlying(BochsFramebufferSettings::LinearFramebuffer));
    set_register_with_io(to_underlying(BochsDISPIRegisters::BANK), 0);
    if ((u16)width != get_register_with_io(to_underlying(BochsDISPIRegisters::XRES)) || (u16)height != get_register_with_io(to_underlying(BochsDISPIRegisters::YRES))) {
        return Error::from_errno(ENOTIMPL);
    }
    return {};
}

ErrorOr<DisplayConnector::Resolution> BochsDisplayConnector::get_resolution()
{
    MutexLocker locker(m_modeset_lock);
    auto width = get_register_with_io(to_underlying(BochsDISPIRegisters::XRES));
    return Resolution { width, get_register_with_io(to_underlying(BochsDISPIRegisters::YRES)), 32, width * sizeof(u32), {} };
}

ErrorOr<void> BochsDisplayConnector::set_y_offset(size_t y_offset)
{
    MutexLocker locker(m_modeset_lock);
    set_register_with_io(to_underlying(BochsDISPIRegisters::Y_OFFSET), (u16)y_offset);
    return {};
}

void BochsDisplayConnector::enable_console()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_framebuffer_console);
    m_framebuffer_console->enable();
}

void BochsDisplayConnector::disable_console()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_framebuffer_console);
    m_framebuffer_console->disable();
}

ErrorOr<void> BochsDisplayConnector::unblank()
{
    return Error::from_errno(ENOTIMPL);
}

}
