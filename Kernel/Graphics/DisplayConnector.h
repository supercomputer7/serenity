/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <Kernel/Devices/CharacterDevice.h>

namespace Kernel {

class GraphicsManagement;
class DisplayConnector : public CharacterDevice {
    friend class GraphicsManagement;
    friend class DeviceManagement;

public:
    struct Resolution {
        size_t width;
        size_t height;
        size_t bpp;
        size_t pitch;
        Optional<size_t> refresh_rate;
    };

public:
    enum class DisplayMode {
        Graphical,
        Console,
    };

public:
    virtual ~DisplayConnector() = default;

    virtual bool modesetting_capable() const = 0;
    virtual bool double_framebuffering_capable() const = 0;
    virtual bool flush_support() const = 0;
    // Note: This can indicate to userland if the underlying hardware requires
    // a defined refresh rate being supplied when modesetting the screen resolution.
    // Paravirtualized hardware don't need such setting and can safely ignore this.
    virtual bool refresh_rate_support() const = 0;

    bool console_mode() const;
    virtual ErrorOr<ByteBuffer> get_edid() const = 0;
    virtual ErrorOr<void> set_resolution(Resolution const&) = 0;
    virtual ErrorOr<void> set_safe_resolution() = 0;
    virtual ErrorOr<Resolution> get_resolution() = 0;
    virtual ErrorOr<void> set_y_offset(size_t y) = 0;
    virtual ErrorOr<void> unblank() = 0;

    void set_display_mode(Badge<GraphicsManagement>, DisplayMode);

    // ^File
    virtual bool can_read(const OpenFileDescription&, u64) const final override { return true; }
    virtual bool can_write(const OpenFileDescription&, u64) const final override { return true; }
    virtual ErrorOr<size_t> read(OpenFileDescription&, u64, UserOrKernelBuffer&, size_t) override final;
    virtual ErrorOr<size_t> write(OpenFileDescription&, u64, const UserOrKernelBuffer&, size_t) override final;
    virtual ErrorOr<Memory::Region*> mmap(Process&, OpenFileDescription&, Memory::VirtualRange const&, u64, int, bool) override final;
    virtual ErrorOr<void> ioctl(OpenFileDescription&, unsigned request, Userspace<void*> arg) override final;
    virtual StringView class_name() const override final { return "DisplayConnector"sv; }

protected:
    DisplayConnector();
    virtual ErrorOr<size_t> write_to_first_surface(u64 offset, UserOrKernelBuffer const&, size_t length) = 0;
    virtual ErrorOr<void> flush_first_surface() = 0;

    mutable Mutex m_control_lock;

    bool m_console_mode { false };

private:
    virtual void will_be_destroyed() override;
    virtual void after_inserting() override;

    IntrusiveListNode<DisplayConnector, RefPtr<DisplayConnector>> m_list_node;
};
}
