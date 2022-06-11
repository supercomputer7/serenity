/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/x86/IO.h>
#include <Kernel/Debug.h>
#include <Kernel/Devices/DeviceManagement.h>
#include <Kernel/Graphics/Divergent/DisplayConnector.h>
#include <Kernel/Graphics/Divergent/GraphicsAdapter.h>
#include <Kernel/Graphics/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Graphics/GraphicsManagement.h>

namespace Kernel {

NonnullRefPtr<DivergentDisplayConnector> DivergentDisplayConnector::must_create(DivergentGraphicsAdapter const& adapter, ConnectorFuseIndex fuse_index, PhysicalAddress framebuffer_address, size_t framebuffer_resource_size)
{
    auto device_or_error = DeviceManagement::try_create_device<DivergentDisplayConnector>(adapter, fuse_index, framebuffer_address, framebuffer_resource_size);
    VERIFY(!device_or_error.is_error());
    auto connector = device_or_error.release_value();
    MUST(connector->create_attached_framebuffer_console());
    return connector;
}

DivergentDisplayConnector::DivergentDisplayConnector(DivergentGraphicsAdapter const& adapter, ConnectorFuseIndex fuse_index, PhysicalAddress framebuffer_address, size_t framebuffer_resource_size)
    : DisplayConnector(framebuffer_address, framebuffer_resource_size, false)
    , m_parent_adapter(adapter)
    , m_fuse_connector_index(fuse_index)
{
}

ErrorOr<void> DivergentDisplayConnector::create_attached_framebuffer_console()
{
    // We assume safe resolution is 1024x768x32
    m_framebuffer_console = Graphics::ContiguousFramebufferConsole::initialize(m_framebuffer_address.value(), 1024, 768, 1024 * sizeof(u32));
    GraphicsManagement::the().set_console(*m_framebuffer_console);
    return {};
}

void DivergentDisplayConnector::enable_console()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_framebuffer_console);
    m_framebuffer_console->enable();
}

void DivergentDisplayConnector::disable_console()
{
    VERIFY(m_control_lock.is_locked());
    VERIFY(m_framebuffer_console);
    m_framebuffer_console->disable();
}

ErrorOr<void> DivergentDisplayConnector::flush_first_surface()
{
    return Error::from_errno(ENOTSUP);
}

ErrorOr<void> DivergentDisplayConnector::set_safe_mode_setting()
{
    DisplayConnector::ModeSetting safe_mode_set {
        .horizontal_stride = 1024 * sizeof(u32),
        .pixel_clock_in_khz = 0, // Note: There's no pixel clock in paravirtualized hardware
        .horizontal_active = 1024,
        .horizontal_front_porch_pixels = 0, // Note: There's no horizontal_front_porch_pixels in paravirtualized hardware
        .horizontal_sync_time_pixels = 0,   // Note: There's no horizontal_sync_time_pixels in paravirtualized hardware
        .horizontal_blank_pixels = 0,       // Note: There's no horizontal_blank_pixels in paravirtualized hardware
        .vertical_active = 768,
        .vertical_front_porch_lines = 0, // Note: There's no vertical_front_porch_lines in paravirtualized hardware
        .vertical_sync_time_lines = 0,   // Note: There's no vertical_sync_time_lines in paravirtualized hardware
        .vertical_blank_lines = 0,       // Note: There's no vertical_blank_lines in paravirtualized hardware
        .horizontal_offset = 0,
        .vertical_offset = 0,
    };
    return set_mode_setting(safe_mode_set);
}

ErrorOr<void> DivergentDisplayConnector::set_mode_setting(ModeSetting const& mode_setting)
{
    SpinlockLocker locker(m_modeset_lock);
    TRY(m_parent_adapter->apply_mode_setting_on_connector({}, *this, mode_setting));
    m_current_mode_setting = mode_setting;
    return {};
}

ErrorOr<void> DivergentDisplayConnector::set_y_offset(size_t)
{
    return Error::from_errno(ENOTIMPL);
}

ErrorOr<void> DivergentDisplayConnector::unblank()
{
    return Error::from_errno(ENOTIMPL);
}

}
