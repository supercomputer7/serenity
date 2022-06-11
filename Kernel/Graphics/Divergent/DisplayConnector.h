/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/RefPtr.h>
#include <AK/Try.h>
#include <Kernel/Graphics/DisplayConnector.h>
#include <Kernel/Graphics/Console/GenericFramebufferConsole.h>
#include <Kernel/Locking/Spinlock.h>
#include <Kernel/Memory/TypedMapping.h>

namespace Kernel {

class DivergentGraphicsAdapter;
class DivergentDisplayConnector final
    : public DisplayConnector {
    friend class DivergentGraphicsAdapter;
    friend class DeviceManagement;

public:
    enum class ConnectorFuseIndex {
        Fuse0 = 0,
        Fuse1 = 1,
        Fuse2 = 2,
        Fuse3 = 3,
        Fuse4 = 4,
        Fuse5 = 5,
        Fuse6 = 6,
        Fuse7 = 7,
    };

    static NonnullRefPtr<DivergentDisplayConnector> must_create(DivergentGraphicsAdapter const&, ConnectorFuseIndex, PhysicalAddress framebuffer_address, size_t framebuffer_resource_size);

    ConnectorFuseIndex connector_fuse_index() const { return m_fuse_connector_index; }

private:
    ErrorOr<void> create_attached_framebuffer_console();

    DivergentDisplayConnector(DivergentGraphicsAdapter const&, ConnectorFuseIndex, PhysicalAddress framebuffer_address, size_t framebuffer_resource_size);

    virtual bool mutable_mode_setting_capable() const override final { return true; }
    virtual bool double_framebuffering_capable() const override { return false; }
    virtual ErrorOr<void> set_mode_setting(ModeSetting const&) override;
    virtual ErrorOr<void> set_safe_mode_setting() override final;
    virtual ErrorOr<void> set_y_offset(size_t y) override;
    virtual ErrorOr<void> unblank() override;

    virtual bool partial_flush_support() const override final { return false; }
    virtual bool flush_support() const override final { return false; }
    // Note: Paravirtualized hardware doesn't require a defined refresh rate for modesetting.
    virtual bool refresh_rate_support() const override final { return false; }

    virtual ErrorOr<void> flush_first_surface() override final;

    virtual void enable_console() override final;
    virtual void disable_console() override final;

    RefPtr<Graphics::GenericFramebufferConsole> m_framebuffer_console;
    NonnullRefPtr<DivergentGraphicsAdapter> m_parent_adapter;
    ConnectorFuseIndex const m_fuse_connector_index { ConnectorFuseIndex::Fuse0 };
};
}
