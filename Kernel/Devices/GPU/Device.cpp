/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/API/Ioctl.h>
#include <Kernel/FileSystem/SysFS/Subsystems/DeviceIdentifiers/CharacterDevicesDirectory.h>
#include <Kernel/FileSystem/SysFS/Subsystems/Devices/GPU/DisplayConnector/DeviceDirectory.h>
#include <Kernel/FileSystem/SysFS/Subsystems/Devices/GPU/DisplayConnector/Directory.h>
#include <Kernel/GPU/DisplayConnector.h>
#include <Kernel/GPU/Management.h>
#include <Kernel/Memory/MemoryManager.h>

namespace Kernel {

GPUDevice::GPUDevice()
    : CharacterDevice(226, GPUManagement::the().allocate_minor_device_number())
{
}

ErrorOr<NonnullLockRefPtr<Memory::VMObject>> GPUDevice::vmobject_for_mmap(Process&, Memory::VirtualRange const&, u64& offset, bool)
{
    VERIFY(m_shared_framebuffer_vmobject);
    if (offset != 0)
        return Error::from_errno(ENOTSUP);

    return *m_shared_framebuffer_vmobject;
}

ErrorOr<size_t> GPUDevice::read(OpenFileDescription&, u64, UserOrKernelBuffer&, size_t)
{
    return Error::from_errno(ENOTSUP);
}

ErrorOr<size_t> GPUDevice::write(OpenFileDescription&, u64, UserOrKernelBuffer const&, size_t)
{
    return Error::from_errno(ENOTSUP);
}

void DisplayConnector::will_be_destroyed()
{
    GPUManagement::the().detach_display_connector({}, *this);

    // NOTE: We check if m_symlink_sysfs_component is not null, because if we failed
    // at some point in DisplayConnector::after_inserting(), then that method will tear down
    // the object internal members safely, so we don't want to do it again here.
    if (m_symlink_sysfs_component) {
        before_will_be_destroyed_remove_symlink_from_device_identifier_directory();
        m_symlink_sysfs_component.clear();
    }

    // NOTE: We check if m_sysfs_device_directory is not null, because if we failed
    // at some point in DisplayConnector::after_inserting(), then that method will tear down
    // the object internal members safely, so we don't want to do it again here.
    if (m_sysfs_device_directory) {
        SysFSDisplayConnectorsDirectory::the().unplug({}, *m_sysfs_device_directory);
        m_sysfs_device_directory.clear();
    }

    before_will_be_destroyed_remove_from_device_management();
}

ErrorOr<void> DisplayConnector::allocate_framebuffer_resources(size_t rounded_size)
{
    VERIFY((rounded_size % PAGE_SIZE) == 0);
    if (!m_framebuffer_at_arbitrary_physical_range) {
        VERIFY(m_framebuffer_address.value().page_base() == m_framebuffer_address.value());
        m_shared_framebuffer_vmobject = TRY(Memory::SharedFramebufferVMObject::try_create_for_physical_range(m_framebuffer_address.value(), rounded_size));
        m_framebuffer_region = TRY(MM.allocate_kernel_region(m_framebuffer_address.value().page_base(), rounded_size, "Framebuffer"sv, Memory::Region::Access::ReadWrite));
    } else {
        m_shared_framebuffer_vmobject = TRY(Memory::SharedFramebufferVMObject::try_create_at_arbitrary_physical_range(rounded_size));
        m_framebuffer_region = TRY(MM.allocate_kernel_region_with_vmobject(m_shared_framebuffer_vmobject->real_writes_framebuffer_vmobject(), rounded_size, "Framebuffer"sv, Memory::Region::Access::ReadWrite));
    }

    m_framebuffer_data = m_framebuffer_region->vaddr().as_ptr();
    m_fake_writes_framebuffer_region = TRY(MM.allocate_kernel_region_with_vmobject(m_shared_framebuffer_vmobject->fake_writes_framebuffer_vmobject(), rounded_size, "Fake Writes Framebuffer"sv, Memory::Region::Access::ReadWrite));
    return {};
}

ErrorOr<void> DisplayConnector::after_inserting()
{
    after_inserting_add_to_device_management();
    ArmedScopeGuard clean_from_device_management([&] {
        before_will_be_destroyed_remove_from_device_management();
    });

    auto sysfs_display_connector_device_directory = DisplayConnectorSysFSDirectory::create(SysFSDisplayConnectorsDirectory::the(), *this);
    m_sysfs_device_directory = sysfs_display_connector_device_directory;
    SysFSDisplayConnectorsDirectory::the().plug({}, *sysfs_display_connector_device_directory);
    ArmedScopeGuard clean_from_sysfs_display_connector_device_directory([&] {
        SysFSDisplayConnectorsDirectory::the().unplug({}, *m_sysfs_device_directory);
        m_sysfs_device_directory.clear();
    });

    VERIFY(!m_symlink_sysfs_component);
    auto sys_fs_component = TRY(SysFSSymbolicLinkDeviceComponent::try_create(SysFSCharacterDevicesDirectory::the(), *this, *m_sysfs_device_directory));
    m_symlink_sysfs_component = sys_fs_component;
    after_inserting_add_symlink_to_device_identifier_directory();
    ArmedScopeGuard clean_symlink_to_device_identifier_directory([&] {
        VERIFY(m_symlink_sysfs_component);
        before_will_be_destroyed_remove_symlink_from_device_identifier_directory();
        m_symlink_sysfs_component.clear();
    });

    if (auto result_or_error = Memory::page_round_up(m_framebuffer_resource_size); result_or_error.is_error()) {
        // NOTE: The amount of framebuffer resource being specified is erroneous, then default to 16 MiB.
        TRY(allocate_framebuffer_resources(16 * MiB));
        m_framebuffer_resource_size = 16 * MiB;
    } else {
        if (auto allocation_result = allocate_framebuffer_resources(result_or_error.release_value()); allocation_result.is_error()) {
            // NOTE: The amount of framebuffer resource being specified is too big, use 16 MiB just to get going.
            TRY(allocate_framebuffer_resources(16 * MiB));
            m_framebuffer_resource_size = 16 * MiB;
        }
    }

    clean_from_device_management.disarm();
    clean_from_sysfs_display_connector_device_directory.disarm();
    clean_symlink_to_device_identifier_directory.disarm();

    GPUManagement::the().attach_new_display_connector({}, *this);
    if (m_enable_write_combine_optimization) {
        [[maybe_unused]] auto result = m_framebuffer_region->set_write_combine(true);
    }
    return {};
}

bool DisplayConnector::console_mode() const
{
    VERIFY(m_control_lock.is_locked());
    return m_console_mode;
}

void DisplayConnector::set_display_mode(Badge<GPUManagement>, DisplayMode mode)
{
    SpinlockLocker locker(m_control_lock);

    {
        SpinlockLocker locker(m_modeset_lock);
        [[maybe_unused]] auto result = set_y_offset(0);
    }

    m_console_mode = mode == DisplayMode::Console ? true : false;
    if (m_console_mode) {
        VERIFY(m_framebuffer_region->size() == m_fake_writes_framebuffer_region->size());
        memcpy(m_fake_writes_framebuffer_region->vaddr().as_ptr(), m_framebuffer_region->vaddr().as_ptr(), m_framebuffer_region->size());
        m_shared_framebuffer_vmobject->switch_to_fake_sink_framebuffer_writes({});
        enable_console();
    } else {
        disable_console();
        m_shared_framebuffer_vmobject->switch_to_real_framebuffer_writes({});
        VERIFY(m_framebuffer_region->size() == m_fake_writes_framebuffer_region->size());
        memcpy(m_framebuffer_region->vaddr().as_ptr(), m_fake_writes_framebuffer_region->vaddr().as_ptr(), m_framebuffer_region->size());
    }
}

DisplayConnector::ModeSetting DisplayConnector::current_mode_setting() const
{
    SpinlockLocker locker(m_modeset_lock);
    return m_current_mode_setting;
}

ErrorOr<ByteBuffer> DisplayConnector::get_edid() const
{
    if (!m_edid_valid)
        return Error::from_errno(ENODEV);
    return ByteBuffer::copy(m_edid_bytes, sizeof(m_edid_bytes));
}

struct GPUIOCtlChecker {
    unsigned ioctl_number;
    StringView name;
    bool requires_ownership { false };
};

static constexpr GPUIOCtlChecker s_checkers[] = {
    { GPU_IOCTL_GET_PROPERTIES, "GPU_IOCTL_GET_PROPERTIES"sv, false },
    { GPU_IOCTL_FLUSH_HEAD_BUFFERS, "GPU_IOCTL_FLUSH_HEAD_BUFFERS"sv, true },
    { GPU_IOCTL_FLUSH_HEAD, "GPU_IOCTL_FLUSH_HEAD"sv, true },
    { GPU_IOCTL_SET_HEAD_MODE_SETTING, "GPU_IOCTL_SET_HEAD_MODE_SETTING"sv, true },
    { GPU_IOCTL_GET_HEAD_MODE_SETTING, "GPU_IOCTL_GET_HEAD_MODE_SETTING"sv, false },
    { GPU_IOCTL_GET_SAFE_HEAD_MODE_SETTING, "GPU_IOCTL_GET_SAFE_HEAD_MODE_SETTING"sv, true },
    { GPU_IOCTL_SET_RESPONSIBLE, "GPU_IOCTL_SET_RESPONSIBLE"sv, false },
    { GPU_IOCTL_UNSET_RESPONSIBLE, "GPU_IOCTL_UNSET_RESPONSIBLE"sv, true },
};

static StringView ioctl_to_stringview(unsigned request)
{
    for (auto& checker : s_checkers) {
        if (checker.ioctl_number == request)
            return checker.name;
    }
    return "unknown"sv;
}

ErrorOr<bool> DisplayConnector::ioctl_requires_ownership(unsigned request) const
{
    for (auto& checker : s_checkers) {
        if (checker.ioctl_number == request)
            return checker.requires_ownership;
    }
    // Note: In case of unknown ioctl, return EINVAL.
    return Error::from_errno(EINVAL);
}

ErrorOr<void> DisplayConnector::ioctl(OpenFileDescription&, unsigned request, Userspace<void*> arg)
{
    TRY(Process::current().require_promise(Pledge::video));

    // Note: We only allow to set responsibility on a DisplayConnector,
    // get the current ModeSetting or the hardware framebuffer properties without the
    // need of having an established responsibility on a DisplayConnector.
    auto needs_ownership = TRY(ioctl_requires_ownership(request));
    if (needs_ownership) {
        SpinlockLocker locker(m_responsible_process_lock);
        auto process = m_responsible_process.strong_ref();
        if (!process || process.ptr() != &Process::current()) {
            dbgln("DisplayConnector::ioctl: {} requires ownership over the device", ioctl_to_stringview(request));
            return Error::from_errno(EPERM);
        }
    }

    switch (request) {
    case GPU_IOCTL_SET_RESPONSIBLE: {
        SpinlockLocker locker(m_responsible_process_lock);
        auto process = m_responsible_process.strong_ref();
        // Note: If there's already a process being responsible, just return an error.
        // We could technically return 0 if the requesting process was already
        // set to be responsible for this DisplayConnector, but it services
        // no good purpose and should be considered a bug if this happens anyway.
        if (process)
            return Error::from_errno(EPERM);
        m_responsible_process = Process::current();
        return {};
    }
    case GPU_IOCTL_UNSET_RESPONSIBLE: {
        SpinlockLocker locker(m_responsible_process_lock);
        auto process = m_responsible_process.strong_ref();
        if (!process)
            return Error::from_errno(ESRCH);
        if (process.ptr() != &Process::current())
            return Error::from_errno(EPERM);
        m_responsible_process.clear();
        return {};
    }
    case GPU_IOCTL_GET_PROPERTIES: {
        auto user_properties = static_ptr_cast<GPUDeviceProperties*>(arg);
        GPUDeviceProperties properties = device_properties();
        return copy_to_user(user_properties, &properties);
    }
    case GPU_IOCTL_GET_HEAD_MODE_SETTING: {
        auto user_head_mode_setting = static_ptr_cast<GPUConnectorModeSetting*>(arg);
        GPUConnectorModeSetting head_mode_setting {};
        TRY(copy_from_user(&head_mode_setting, user_head_mode_setting));
        if (head_mode_setting.connector_index < 0)
            return Error::from_errno(EINVAL);
        return m_display_connectors.with([](auto& display_connectors) {
            if (display_connectors.size() < static_cast<size_t>(head_mode_setting.connector_index))
                return Error::from_errno(EINVAL);
            auto& connector = display_connectors[static_cast<size_t>(head_mode_setting.connector_index)];
            head_mode_setting.horizontal_stride = m_current_mode_setting.horizontal_stride;
            head_mode_setting.pixel_clock_in_khz = m_current_mode_setting.pixel_clock_in_khz;
            head_mode_setting.horizontal_active = m_current_mode_setting.horizontal_active;
            head_mode_setting.horizontal_front_porch_pixels = m_current_mode_setting.horizontal_front_porch_pixels;
            head_mode_setting.horizontal_sync_time_pixels = m_current_mode_setting.horizontal_sync_time_pixels;
            head_mode_setting.horizontal_blank_pixels = m_current_mode_setting.horizontal_blank_pixels;
            head_mode_setting.vertical_active = m_current_mode_setting.vertical_active;
            head_mode_setting.vertical_front_porch_lines = m_current_mode_setting.vertical_front_porch_lines;
            head_mode_setting.vertical_sync_time_lines = m_current_mode_setting.vertical_sync_time_lines;
            head_mode_setting.vertical_blank_lines = m_current_mode_setting.vertical_blank_lines;
            head_mode_setting.horizontal_offset = m_current_mode_setting.horizontal_offset;
            head_mode_setting.vertical_offset = m_current_mode_setting.vertical_offset;
            return copy_to_user(user_head_mode_setting, &head_mode_setting);
        });
    }
    case GPU_IOCTL_SET_HEAD_MODE_SETTING: {
        auto user_mode_setting = static_ptr_cast<GPUConnectorModeSetting const*>(arg);
        auto head_mode_setting = TRY(copy_typed_from_user(user_mode_setting));
        if (head_mode_setting.connector_index < 0)
            return Error::from_errno(EINVAL);
        if (head_mode_setting.horizontal_stride < 0)
            return Error::from_errno(EINVAL);
        if (head_mode_setting.pixel_clock_in_khz < 0)
            return Error::from_errno(EINVAL);
        if (head_mode_setting.horizontal_active < 0)
            return Error::from_errno(EINVAL);
        if (head_mode_setting.horizontal_front_porch_pixels < 0)
            return Error::from_errno(EINVAL);
        if (head_mode_setting.horizontal_sync_time_pixels < 0)
            return Error::from_errno(EINVAL);
        if (head_mode_setting.horizontal_blank_pixels < 0)
            return Error::from_errno(EINVAL);
        if (head_mode_setting.vertical_active < 0)
            return Error::from_errno(EINVAL);
        if (head_mode_setting.vertical_front_porch_lines < 0)
            return Error::from_errno(EINVAL);
        if (head_mode_setting.vertical_sync_time_lines < 0)
            return Error::from_errno(EINVAL);
        if (head_mode_setting.vertical_blank_lines < 0)
            return Error::from_errno(EINVAL);
        if (head_mode_setting.horizontal_offset < 0)
            return Error::from_errno(EINVAL);
        if (head_mode_setting.vertical_offset < 0)
            return Error::from_errno(EINVAL);

        ModeSetting requested_mode_setting;
        requested_mode_setting.horizontal_stride = 0;
        requested_mode_setting.pixel_clock_in_khz = head_mode_setting.pixel_clock_in_khz;
        requested_mode_setting.horizontal_active = head_mode_setting.horizontal_active;
        requested_mode_setting.horizontal_front_porch_pixels = head_mode_setting.horizontal_front_porch_pixels;
        requested_mode_setting.horizontal_sync_time_pixels = head_mode_setting.horizontal_sync_time_pixels;
        requested_mode_setting.horizontal_blank_pixels = head_mode_setting.horizontal_blank_pixels;
        requested_mode_setting.vertical_active = head_mode_setting.vertical_active;
        requested_mode_setting.vertical_front_porch_lines = head_mode_setting.vertical_front_porch_lines;
        requested_mode_setting.vertical_sync_time_lines = head_mode_setting.vertical_sync_time_lines;
        requested_mode_setting.vertical_blank_lines = head_mode_setting.vertical_blank_lines;
        requested_mode_setting.horizontal_offset = head_mode_setting.horizontal_offset;
        requested_mode_setting.vertical_offset = head_mode_setting.vertical_offset;

        TRY(change_connector_modesetting(static_cast<size_t>(head_mode_setting.connector_index), requested_mode_setting));
        return {};
    }

    case GPU_IOCTL_GET_SAFE_HEAD_MODE_SETTING: {
        SpinlockLocker control_locker(m_control_lock);
        TRY(set_safe_mode_setting());
        return {};
    }
    case GPU_IOCTL_FLUSH_HEAD_BUFFERS: {
        if (console_mode())
            return {};
        if (!partial_flush_support())
            return Error::from_errno(ENOTSUP);
        MutexLocker locker(m_flushing_lock);
        auto user_flush_rects = static_ptr_cast<FBFlushRects const*>(arg);
        auto flush_rects = TRY(copy_typed_from_user(user_flush_rects));
        if (Checked<unsigned>::multiplication_would_overflow(flush_rects.count, sizeof(FBRect)))
            return Error::from_errno(EFAULT);
        if (flush_rects.count > 0) {
            for (unsigned i = 0; i < flush_rects.count; i++) {
                FBRect user_dirty_rect;
                TRY(copy_from_user(&user_dirty_rect, &flush_rects.rects[i]));
                {
                    SpinlockLocker control_locker(m_control_lock);
                    if (console_mode()) {
                        return {};
                    }
                    TRY(flush_rectangle(flush_rects.buffer_index, user_dirty_rect));
                }
            }
        }
        return {};
    };
    case GPU_IOCTL_FLUSH_HEAD: {
        // FIXME: We silently ignore the request if we are in console mode.
        // WindowServer is not ready yet to handle errors such as EBUSY currently.
        MutexLocker locker(m_flushing_lock);
        SpinlockLocker control_locker(m_control_lock);
        if (console_mode()) {
            return {};
        }

        if (!flush_support())
            return Error::from_errno(ENOTSUP);

        TRY(flush_first_surface());
        return {};
    }
    }
    // Note: We already verify that the IOCTL is supported and not unknown in
    // the call to the ioctl_requires_ownership method, so if we reached this
    // section of the code, this is bug.
    VERIFY_NOT_REACHED();
}

}
