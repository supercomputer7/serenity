/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Debug.h>
#include <Kernel/Devices/DeviceManagement.h>
#include <Kernel/Graphics/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Graphics/Console/TextModeConsole.h>
#include <Kernel/Graphics/GraphicsManagement.h>
#include <Kernel/Graphics/VGA/GenericDisplayConnector.h>

namespace Kernel {

NonnullRefPtr<VGAGenericDisplayConnector> VGAGenericDisplayConnector::must_create_with_preset_mode_setting(PhysicalAddress framebuffer_address, size_t framebuffer_width, size_t framebuffer_height, size_t framebuffer_pitch)
{
    auto device_or_error = DeviceManagement::try_create_device<VGAGenericDisplayConnector>(framebuffer_address, framebuffer_width, framebuffer_height, framebuffer_pitch);
    VERIFY(!device_or_error.is_error());
    auto connector = device_or_error.release_value();
    MUST(connector->create_attached_framebuffer_console());
    MUST(connector->initialize_edid_for_generic_monitor());
    return connector;
}

NonnullRefPtr<VGAGenericDisplayConnector> VGAGenericDisplayConnector::must_create()
{
    auto connector = adopt_ref_if_nonnull(new (nothrow) VGAGenericDisplayConnector()).release_nonnull();
    MUST(connector->create_attached_text_console());
    return connector;
}

ErrorOr<void> VGAGenericDisplayConnector::create_attached_text_console()
{
    m_framebuffer_console = Graphics::TextModeConsole::initialize();
    GraphicsManagement::the().set_console(*m_framebuffer_console);
    return {};
}

ErrorOr<void> VGAGenericDisplayConnector::create_attached_framebuffer_console()
{
    VERIFY(m_framebuffer_address.has_value());
    SpinlockLocker locker(m_modeset_lock);
    m_framebuffer_console = Graphics::ContiguousFramebufferConsole::initialize(m_framebuffer_address.value(), m_current_mode_setting.horizontal_active, m_current_mode_setting.vertical_active, m_current_mode_setting.horizontal_stride);
    GraphicsManagement::the().set_console(*m_framebuffer_console);
    return {};
}

VGAGenericDisplayConnector::VGAGenericDisplayConnector(PhysicalAddress framebuffer_address, size_t framebuffer_width, size_t framebuffer_height, size_t framebuffer_pitch)
    : DisplayConnector()
    , m_framebuffer_address(framebuffer_address)
{
    DisplayConnector::ModeSetting current_mode_setting {
        .horizontal_stride = framebuffer_pitch,
        .pixel_clock_in_khz = 0, // Note: There's no pixel clock in paravirtualized hardware
        .horizontal_active = framebuffer_width,
        .horizontal_front_porch_pixels = 0, // Note: There's no horizontal_front_porch_pixels in paravirtualized hardware
        .horizontal_sync_time_pixels = 0,   // Note: There's no horizontal_sync_time_pixels in paravirtualized hardware
        .horizontal_blank_pixels = 0,       // Note: There's no horizontal_blank_pixels in paravirtualized hardware
        .vertical_active = framebuffer_height,
        .vertical_front_porch_lines = 0, // Note: There's no vertical_front_porch_lines in paravirtualized hardware
        .vertical_sync_time_lines = 0,   // Note: There's no vertical_sync_time_lines in paravirtualized hardware
        .vertical_blank_lines = 0,       // Note: There's no vertical_blank_lines in paravirtualized hardware
    };
    m_current_mode_setting = current_mode_setting;
}

VGAGenericDisplayConnector::VGAGenericDisplayConnector()
    : DisplayConnector()
    , m_framebuffer_address({})
{
}

VGAGenericDisplayConnector::VGAGenericDisplayConnector(PhysicalAddress framebuffer_address)
    : DisplayConnector()
    , m_framebuffer_address(framebuffer_address)
{
}

void VGAGenericDisplayConnector::enable_console()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_framebuffer_console);
    m_framebuffer_console->enable();
}
void VGAGenericDisplayConnector::disable_console()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_framebuffer_console);
    m_framebuffer_console->disable();
}

ErrorOr<size_t> VGAGenericDisplayConnector::write_to_first_surface(u64 offset, UserOrKernelBuffer const& buffer, size_t length)
{
    VERIFY(m_control_lock.is_locked());
    if (offset + length > m_framebuffer_region->size())
        return Error::from_errno(EOVERFLOW);
    TRY(buffer.read(m_framebuffer_region->vaddr().offset(offset).as_ptr(), 0, length));
    return length;
}

ErrorOr<void> VGAGenericDisplayConnector::flush_first_surface()
{
    return Error::from_errno(ENOTSUP);
}

}
