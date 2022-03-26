/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/RefPtr.h>
#include <AK/Try.h>
#include <Kernel/Graphics/Console/GenericFramebufferConsole.h>
#include <Kernel/Graphics/Definitions.h>
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
    static NonnullRefPtr<IntelNativeDisplayConnector> must_create(IntelDisplayConnectorGroup const&);

    void set_edid_bytes(Badge<IntelDisplayConnectorGroup>, EDID::Parser::RawBytes);
    ErrorOr<void> create_attached_framebuffer_console(Badge<IntelDisplayConnectorGroup>, PhysicalAddress framebuffer_address);

private:
    // ^DisplayConnector
    // FIXME: Implement modesetting capabilities in runtime from userland...
    virtual bool modesetting_capable() const override { return false; }
    // FIXME: Implement double buffering capabilities in runtime from userland...
    virtual bool double_framebuffering_capable() const override { return false; }
    virtual ErrorOr<ByteBuffer> get_edid() const override;
    virtual ErrorOr<void> set_mode_setting(ModeSetting const&) override;
    virtual ErrorOr<void> set_safe_mode_setting() override;
    virtual ErrorOr<ModeSetting> current_mode_setting() override;
    virtual ErrorOr<void> set_y_offset(size_t y) override;
    virtual ErrorOr<void> unblank() override;

    explicit IntelNativeDisplayConnector(IntelDisplayConnectorGroup const&);

    EDID::Parser::RawBytes m_crt_edid_bytes {};
    Optional<EDID::Parser> m_crt_edid;
    NonnullRefPtr<IntelDisplayConnectorGroup> m_parent_connector_group;
};
}
