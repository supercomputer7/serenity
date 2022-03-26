/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <Kernel/Devices/CharacterDevice.h>
#include <LibC/sys/ioctl_numbers.h>

namespace Kernel {

class GraphicsManagement;
class DisplayConnector : public CharacterDevice {
    friend class GraphicsManagement;
    friend class DeviceManagement;

public:
    struct ModeSetting {
        size_t horizontal_stride; // Note: This is commonly known as "pitch"
        size_t pixel_clock_in_khz;
        size_t horizontal_active;
        size_t horizontal_sync_start;
        size_t horizontal_sync_end;
        size_t horizontal_total;
        size_t vertical_active;
        size_t vertical_sync_start;
        size_t vertical_sync_end;
        size_t vertical_total;
    };

    enum class DisplayMode {
        Graphical,
        Console,
    };

    enum class Hardware3DAccelerationCommandSet {
        None,
        VirGL,
    };

    static_assert(to_underlying(Hardware3DAccelerationCommandSet::None) == 0);

public:
    virtual ~DisplayConnector() = default;

    virtual bool modesetting_capable() const = 0;
    virtual bool double_framebuffering_capable() const = 0;
    virtual bool flush_support() const = 0;
    virtual bool partial_flush_support() const = 0;
    // Note: By default we assume the hardware doesn't support 3D acceleration.
    // It's up to the specific driver to advertise such capability.
    virtual Hardware3DAccelerationCommandSet hardware_3d_acceleration_commands_set() const;
    // Note: This can indicate to userland if the underlying hardware requires
    // a defined refresh rate being supplied when modesetting the screen resolution.
    // Paravirtualized hardware don't need such setting and can safely ignore this.
    virtual bool refresh_rate_support() const = 0;

    bool console_mode() const;
    virtual ErrorOr<ByteBuffer> get_edid() const = 0;
    virtual ErrorOr<void> set_mode_setting(ModeSetting const&) = 0;
    virtual ErrorOr<void> set_safe_mode_setting() = 0;
    virtual ErrorOr<ModeSetting> current_mode_setting() = 0;
    virtual ErrorOr<void> set_y_offset(size_t y) = 0;
    virtual ErrorOr<void> unblank() = 0;

    void set_display_mode(Badge<GraphicsManagement>, DisplayMode);

protected:
    // ^File
    virtual bool is_seekable() const override { return true; }
    virtual bool can_read(OpenFileDescription const&, u64) const final override { return true; }
    virtual bool can_write(OpenFileDescription const&, u64) const final override { return true; }
    virtual ErrorOr<size_t> read(OpenFileDescription&, u64, UserOrKernelBuffer&, size_t) override final;
    virtual ErrorOr<size_t> write(OpenFileDescription&, u64, UserOrKernelBuffer const&, size_t) override final;
    virtual ErrorOr<Memory::Region*> mmap(Process&, OpenFileDescription&, Memory::VirtualRange const&, u64, int, bool) override;
    virtual ErrorOr<void> ioctl(OpenFileDescription&, unsigned request, Userspace<void*> arg) override;
    virtual StringView class_name() const override final { return "DisplayConnector"sv; }

    DisplayConnector();
    virtual ErrorOr<size_t> write_to_first_surface(u64 offset, UserOrKernelBuffer const&, size_t length) = 0;
    virtual void enable_console() = 0;
    virtual void disable_console() = 0;
    virtual ErrorOr<void> flush_first_surface() = 0;
    virtual ErrorOr<void> flush_rectangle(size_t buffer_index, FBRect const& rect);

    mutable Spinlock m_control_lock;
    mutable Mutex m_flushing_lock;

    bool m_console_mode { false };

    bool m_vertical_offsetted { false };

private:
    virtual void will_be_destroyed() override;
    virtual void after_inserting() override;

    IntrusiveListNode<DisplayConnector, RefPtr<DisplayConnector>> m_list_node;
};
}
