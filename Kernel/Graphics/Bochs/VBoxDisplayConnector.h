/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Graphics/Bochs/DisplayConnector.h>

namespace Kernel {

class VBoxDisplayConnector final
    : public BochsDisplayConnector {
    friend class DeviceManagement;

public:
    static NonnullRefPtr<VBoxDisplayConnector> must_create(PhysicalAddress framebuffer_address);

    virtual IndexID index_id() const override;

private:
    virtual bool double_framebuffering_capable() const override { return false; }
    virtual ErrorOr<ByteBuffer> get_edid() const override;
    virtual ErrorOr<void> set_mode_setting(ModeSetting const&) override;
    virtual ErrorOr<void> set_y_offset(size_t y) override;
    virtual ErrorOr<void> unblank() override;

    explicit VBoxDisplayConnector(PhysicalAddress framebuffer_address);
};
}
