/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/x86/IO.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Debug.h>
#include <Kernel/Devices/DeviceManagement.h>
#include <Kernel/Graphics/Console/ContiguousFramebufferConsole.h>
#include <Kernel/Graphics/GraphicsManagement.h>
#include <Kernel/Graphics/Intel/DisplayConnectorGroup.h>
#include <Kernel/Graphics/Intel/NativeDisplayConnector.h>
#include <Kernel/Memory/Region.h>

namespace Kernel {

NonnullRefPtr<IntelNativeDisplayConnector> IntelNativeDisplayConnector::must_create(IntelDisplayConnectorGroup const& parent_connector_group)
{
    auto device_or_error = DeviceManagement::try_create_device<IntelNativeDisplayConnector>(parent_connector_group);
    VERIFY(!device_or_error.is_error());
    auto connector = device_or_error.release_value();
    return connector;
}

ErrorOr<void> IntelNativeDisplayConnector::create_attached_framebuffer_console(Badge<IntelDisplayConnectorGroup>, PhysicalAddress framebuffer_address)
{
    m_framebuffer_console = Graphics::ContiguousFramebufferConsole::initialize(framebuffer_address, m_framebuffer_width, m_framebuffer_height, m_framebuffer_pitch);
    GraphicsManagement::the().set_console(*m_framebuffer_console);
    return {};
}

IntelNativeDisplayConnector::IntelNativeDisplayConnector(IntelDisplayConnectorGroup const& parent_connector_group)
    : VGAGenericDisplayConnector()
    , m_parent_connector_group(parent_connector_group)
{
}

void IntelNativeDisplayConnector::set_edid_bytes(Badge<IntelDisplayConnectorGroup>, EDID::Parser::RawBytes raw_bytes)
{
    memcpy((u8*)m_crt_edid_bytes, (u8*)raw_bytes, sizeof(m_crt_edid_bytes));
    if (auto parsed_edid = EDID::Parser::from_bytes({ m_crt_edid_bytes, sizeof(m_crt_edid_bytes) }); !parsed_edid.is_error()) {
        m_crt_edid = parsed_edid.release_value();
    } else {
        for (size_t x = 0; x < 128; x = x + 16) {
            dmesgln("IntelNativeGraphicsAdapter: Print offending EDID");
            dmesgln("{:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x}",
                m_crt_edid_bytes[x], m_crt_edid_bytes[x + 1], m_crt_edid_bytes[x + 2], m_crt_edid_bytes[x + 3],
                m_crt_edid_bytes[x + 4], m_crt_edid_bytes[x + 5], m_crt_edid_bytes[x + 6], m_crt_edid_bytes[x + 7],
                m_crt_edid_bytes[x + 8], m_crt_edid_bytes[x + 9], m_crt_edid_bytes[x + 10], m_crt_edid_bytes[x + 11],
                m_crt_edid_bytes[x + 12], m_crt_edid_bytes[x + 13], m_crt_edid_bytes[x + 14], m_crt_edid_bytes[x + 15]);
        }
        dmesgln("IntelNativeGraphicsAdapter: Parsing EDID failed: {}", parsed_edid.error());
    }
}

ErrorOr<ByteBuffer> IntelNativeDisplayConnector::get_edid() const
{
    if (m_crt_edid.has_value())
        return ByteBuffer::copy(m_crt_edid_bytes, sizeof(m_crt_edid_bytes));
    return ByteBuffer {};
}

ErrorOr<void> IntelNativeDisplayConnector::set_resolution(DisplayConnector::Resolution const&)
{
    return Error::from_errno(ENOTIMPL);
}

ErrorOr<void> IntelNativeDisplayConnector::set_y_offset(size_t)
{
    return Error::from_errno(ENOTIMPL);
}

ErrorOr<DisplayConnector::Resolution> IntelNativeDisplayConnector::get_resolution()
{
    return m_parent_connector_group->get_resolution({}, *this);
}

ErrorOr<void> IntelNativeDisplayConnector::unblank()
{
    return Error::from_errno(ENOTIMPL);
}

ErrorOr<void> IntelNativeDisplayConnector::set_safe_resolution()
{
    return m_parent_connector_group->set_safe_resolution({}, *this);
}

}
