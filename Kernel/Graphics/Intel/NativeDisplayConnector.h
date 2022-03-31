/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/RefPtr.h>
#include <AK/Try.h>
#include <Kernel/Graphics/Console/GenericFramebufferConsole.h>
#include <Kernel/Graphics/VGA/GenericDisplayConnector.h>
#include <Kernel/Memory/TypedMapping.h>
#include <LibEDID/EDID.h>

namespace Kernel {

class IntelDisplayConnectorGroup;
class IntelNativeDisplayConnector
    : public VGAGenericDisplayConnector {
    friend class IntelDisplayConnectorGroup;
    friend class DeviceManagement;

public:
    enum class Type {
        Invalid,
        Analog,
        DVO,
        LVDS,
        TVOut,
        HDMI,
        DisplayPort,
        EmbeddedDisplayPort,
    };

    enum class ConnectorIndex : size_t {
        PortA = 0,
        PortB = 1,
        PortC = 2,
        PortD = 3,
        PortE = 4,
        PortF = 5,
        PortH = 6,
        PortG = 7,
        PortI = 8,
    };

public:
    static NonnullRefPtr<IntelNativeDisplayConnector> must_create(IntelDisplayConnectorGroup const&, ConnectorIndex connector_index, Type);

    void set_edid_bytes(Badge<IntelDisplayConnectorGroup>, Array<u8, 128> const&);
    ErrorOr<void> create_attached_framebuffer_console(Badge<IntelDisplayConnectorGroup>, PhysicalAddress framebuffer_address);

    ConnectorIndex connector_index() const { return m_connector_index; }

private:
    // ^DisplayConnector
    // FIXME: Implement modesetting capabilities in runtime from userland...
    virtual bool modesetting_capable() const override { return false; }
    // FIXME: Implement double buffering capabilities in runtime from userland...
    virtual bool double_framebuffering_capable() const override { return false; }
    virtual ErrorOr<void> set_mode_setting(ModeSetting const&) override;
    virtual ErrorOr<void> set_safe_mode_setting() override;
    virtual ErrorOr<void> set_y_offset(size_t y) override;
    virtual ErrorOr<void> unblank() override;

    IntelNativeDisplayConnector(IntelDisplayConnectorGroup const&, ConnectorIndex connector_index, Type);
    const Type m_type { Type::Analog };
    const ConnectorIndex m_connector_index { 0 };
    NonnullRefPtr<IntelDisplayConnectorGroup> m_parent_connector_group;
};
}
